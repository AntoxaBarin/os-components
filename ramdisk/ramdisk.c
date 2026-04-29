#include <linux/bio.h>
#include <linux/blk-mq.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Shanygin");
MODULE_DESCRIPTION("Block disk in ram");
MODULE_VERSION("0.1");

#define MY_BLOCK_MAJOR 240
#define MY_BLKDEV_NAME "ramdisk"
#define MY_DISK_NAME "ramdisk"
#define MY_HW_QUEUES 1
#define MY_QUEUE_DEPTH 128
#define MY_MAX_HW_SECTORS 2560

static int num_sectors = 128;
module_param(num_sectors, int, 0644);
MODULE_PARM_DESC(num_sectors, "Disk capacity in 512-byte sectors");

static struct my_block_dev {
  struct blk_mq_tag_set tag_set;
  struct gendisk *gd;
  u8 *data;
  size_t size;
} g_dev;

static int my_block_open(struct gendisk *disk, blk_mode_t mode) { return 0; }

static void my_block_release(struct gendisk *disk) {}

static const struct block_device_operations my_block_ops = {
    .owner = THIS_MODULE,
    .open = my_block_open,
    .release = my_block_release,
};

static blk_status_t my_block_transfer(struct my_block_dev *dev, sector_t sector,
                                   size_t len, void *buffer, enum req_op op) {
  loff_t offset = (loff_t)sector << SECTOR_SHIFT;

  if (offset < 0 || offset + len > dev->size) {
    pr_err("ramdisk: out-of-range I/O at %lld+%zu (size=%zu)\n", offset, len, dev->size);
    return BLK_STS_IOERR;
  }

  if (op == REQ_OP_WRITE) {
    memcpy(dev->data + offset, buffer, len);
  }
  else {
    memcpy(buffer, dev->data + offset, len);
  }
  return BLK_STS_OK;
}

static blk_status_t my_xfer_request(struct my_block_dev *dev,
                                    struct request *req) {
  struct req_iterator iter;
  struct bio_vec bvec;
  enum req_op op = req_op(req);

  rq_for_each_segment(bvec, req, iter) {
    sector_t sector = iter.iter.bi_sector;
    void *kaddr = kmap_local_page(bvec.bv_page);
    blk_status_t st;

    st = my_block_transfer(dev, sector, bvec.bv_len, kaddr + bvec.bv_offset, op);
    kunmap_local(kaddr);

    if (st != BLK_STS_OK)
      return st;
  }

  return BLK_STS_OK;
}

static blk_status_t my_queue_rq(struct blk_mq_hw_ctx *hw_ctx,
                                const struct blk_mq_queue_data *q_data) {
  struct request *rq = q_data->rq;
  struct my_block_dev *dev = hw_ctx->queue->queuedata;
  blk_status_t result;

  blk_mq_start_request(rq);

  if (blk_rq_is_passthrough(rq)) {
    pr_info_ratelimited("ramdisk: skipping passthrough request\n");
    result = BLK_STS_IOERR;
    blk_mq_end_request(rq, result);
    return BLK_STS_OK;  
  }

  pr_info("ramdisk: pos=%llu bytes=%u cur=%u dir=%c\n",
          (unsigned long long)blk_rq_pos(rq), blk_rq_bytes(rq),
          blk_rq_cur_bytes(rq), rq_data_dir(rq) ? 'W' : 'R');

  result = my_xfer_request(dev, rq);
  blk_mq_end_request(rq, result);
  return BLK_STS_OK;
}

static const struct blk_mq_ops my_queue_ops = {
    .queue_rq = my_queue_rq,
};

static int create_block_device(struct my_block_dev *dev) {
  struct queue_limits limits = {
      .logical_block_size = SECTOR_SIZE,
      .physical_block_size = SECTOR_SIZE,
      .max_hw_sectors = MY_MAX_HW_SECTORS,
  };
  struct gendisk *gd;
  int err;

  dev->size = (size_t)num_sectors * SECTOR_SIZE;
  dev->data = vmalloc(dev->size);
  if (!dev->data) {
    pr_err("ramdisk: vmalloc(%zu) failed\n", dev->size);
    return -ENOMEM;
  }

  dev->tag_set.ops = &my_queue_ops;
  dev->tag_set.nr_hw_queues = MY_HW_QUEUES;
  dev->tag_set.queue_depth = MY_QUEUE_DEPTH;
  dev->tag_set.numa_node = NUMA_NO_NODE;
  dev->tag_set.cmd_size = 0;

  err = blk_mq_alloc_tag_set(&dev->tag_set);
  if (err) {
    pr_err("ramdisk: blk_mq_alloc_tag_set failed: %d\n", err);
    goto err_free_data;
  }

  gd = blk_mq_alloc_disk(&dev->tag_set, dev);
  if (IS_ERR(gd)) {
    err = PTR_ERR(gd);
    pr_err("ramdisk: blk_mq_alloc_disk failed: %d\n", err);
    goto err_free_tag_set;
  }

  blk_queue_logical_block_size(gd->queue, limits.logical_block_size);
  blk_queue_physical_block_size(gd->queue, limits.physical_block_size);
  blk_queue_max_hw_sectors(gd->queue, limits.max_hw_sectors);

  gd->major = MY_BLOCK_MAJOR;
  gd->first_minor = 0;
  gd->minors = 1;
  gd->fops = &my_block_ops;
  gd->private_data = dev;
  snprintf(gd->disk_name, DISK_NAME_LEN, MY_DISK_NAME);
  set_capacity(gd, num_sectors);

  dev->gd = gd;

  err = add_disk(gd);
  if (err) {
    pr_err("ramdisk: add_disk failed: %d\n", err);
    goto err_put_disk;
  }

  return 0;

err_put_disk:
  put_disk(gd);
  dev->gd = NULL;

err_free_tag_set:
  blk_mq_free_tag_set(&dev->tag_set);

err_free_data:
  vfree(dev->data);
  dev->data = NULL;

  return err;
}

static void delete_block_device(struct my_block_dev *dev) {
  if (dev->gd) {
    del_gendisk(dev->gd);
    put_disk(dev->gd);
    dev->gd = NULL;
  }
  if (dev->tag_set.tags) {
    blk_mq_free_tag_set(&dev->tag_set);
  }
  vfree(dev->data);
  dev->data = NULL;
}

static int __init my_block_init(void) {
  int err;

  if (num_sectors <= 0) {
    pr_err("ramdisk: invalid num_sectors=%d\n", num_sectors);
    return -EINVAL;
  }

  err = register_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
  if (err < 0) {
    pr_err("ramdisk: register_blkdev failed: %d\n", err);
    return err;
  }

  err = create_block_device(&g_dev);
  if (err) {
    goto err_unregister;
  }

  return 0;

err_unregister:
  unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
  return err;
}

static void __exit my_block_exit(void) {
  delete_block_device(&g_dev);
  unregister_blkdev(MY_BLOCK_MAJOR, MY_BLKDEV_NAME);
}

module_init(my_block_init);
module_exit(my_block_exit);
