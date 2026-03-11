#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Shanygin");
MODULE_DESCRIPTION("Character device driver dumping all written data into dmesg");
MODULE_VERSION("0.1");

#define DEVICE_NAME "nulldump"
#define CLASS_NAME "nulldump_class"
#define LINE_SIZE 16

static int major;
static struct class *nulldump_class = NULL;
static struct device *nulldump_device = NULL;

static ssize_t nulldump_read(struct file *file, char __user *buf, size_t size,
                             loff_t *ppos) {
  pr_info("nulldump: read of %zu bytes from pid=%d, comm=%s\n", size,
          current->pid, current->comm);
  return 0;
}

static ssize_t nulldump_write(struct file *file, const char __user *data,
                              size_t size, loff_t *ppos) {
  size_t offset = 0;
  unsigned char buf[LINE_SIZE];

  pr_info("nulldump: write of %zu bytes from pid=%d, comm=%s\n", size,
          current->pid, current->comm);

  while (offset < size) {
    size_t chunk = size - offset;
    size_t bytes_this_line = LINE_SIZE;
    if (chunk < LINE_SIZE) {
      bytes_this_line = chunk;
    }
    unsigned long uncopied;

    uncopied = copy_from_user(buf, data + offset, bytes_this_line);
    if (uncopied) {
      return -EFAULT;
    }

    print_hex_dump(KERN_INFO, "nulldump: ", DUMP_PREFIX_OFFSET, LINE_SIZE, 1,
                   buf, bytes_this_line, true);
    offset += bytes_this_line;
  }

  return size;
}

static char *nulldump_devnode(const struct device *dev, umode_t *mode) {
  *mode = S_IRUGO | S_IWUGO; // rw-rw-rw-
  return 0;
}

static struct file_operations nulldump_fops = {
    .owner = THIS_MODULE,
    .read = nulldump_read,
    .write = nulldump_write,
};

static int __init nulldump_start(void) {
  int ret;

  major = register_chrdev(0, DEVICE_NAME, &nulldump_fops);
  if (major < 0) {
    ret = major;
    pr_err("nulldump: failed to register char device: %d\n", ret);
    goto register_err;
  }

  nulldump_class = class_create(CLASS_NAME);
  if (IS_ERR(nulldump_class)) {
    ret = PTR_ERR(nulldump_class);
    pr_err("nulldump: failed to create class: %d\n", ret);
    goto class_err;
  }

  nulldump_class->devnode = nulldump_devnode;
  nulldump_device = device_create(nulldump_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
  if (IS_ERR(nulldump_device)) {
    ret = PTR_ERR(nulldump_device);
    pr_err("nulldump: failed to create char device: %d\n", ret);
    goto device_err;
  }

  pr_info("nulldump: created device at /dev/%s\n", DEVICE_NAME);
  return 0;

device_err:
  class_destroy(nulldump_class);

class_err:
  unregister_chrdev(major, DEVICE_NAME);

register_err:
  return ret;
}

static void __exit nulldump_end(void) {
  device_destroy(nulldump_class, MKDEV(major, 0));
  class_destroy(nulldump_class);
  unregister_chrdev(major, DEVICE_NAME);
  pr_info("nulldump: unregistered device\n");
}

module_init(nulldump_start);
module_exit(nulldump_end);
