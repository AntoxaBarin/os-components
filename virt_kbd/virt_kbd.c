#include <linux/init.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/random.h>
#include <linux/timer.h>

#define DRIVER_NAME "virt_kbd"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Shanygin");
MODULE_DESCRIPTION("Virtual Random Keyboard");
MODULE_VERSION("0.1");

static int interval = 20; // seconds
static bool enabled = true;

module_param(interval, int, 0644);
MODULE_PARM_DESC(interval, "Interval key geenration in seconds (default 20)");

module_param(enabled, bool, 0644);
MODULE_PARM_DESC(enabled, "Enable/disable key generation (default 1)");

static const unsigned int key_pool[] = {
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
    KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
    KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_0,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
};
#define KEY_POOL_SIZE ARRAY_SIZE(key_pool)

static struct input_dev *virt_kbd_dev;
static struct timer_list virt_kbd_timer;

static void virt_kbd_send_key(unsigned int keycode) {
  // key down
  input_report_key(virt_kbd_dev, keycode, 1);
  input_sync(virt_kbd_dev);

  // key up
  input_report_key(virt_kbd_dev, keycode, 0);
  input_sync(virt_kbd_dev);

  pr_info(DRIVER_NAME ": sent key event keycode=%u\n", keycode);
}

static void virt_kbd_timer_cb(struct timer_list *t) {
  if (enabled) {
    unsigned int idx;
    unsigned int keycode;
    u32 rnd;

    get_random_bytes(&rnd, sizeof(rnd));
    idx = rnd % KEY_POOL_SIZE;
    keycode = key_pool[idx];

    pr_info(DRIVER_NAME
            ": timer fired, generating key press (keycode=%u, pool_idx=%u)\n",
            keycode, idx);

    virt_kbd_send_key(keycode);
  } else {
    pr_debug(DRIVER_NAME ": timer fired but generation is disabled\n");
  }

  if (interval < 1) {
    interval = 1;
  }

  mod_timer(&virt_kbd_timer, jiffies + (unsigned long)interval * HZ);
}

static int __init virt_kbd_init(void) {
  int ret;
  unsigned int i;

  pr_info(DRIVER_NAME ": interval=%d s, enabled=%d\n", interval, enabled);

  virt_kbd_dev = input_allocate_device();
  if (!virt_kbd_dev) {
    pr_err(DRIVER_NAME ": failed to allocate input device\n");
    return -ENOMEM;
  }

  virt_kbd_dev->name = "Virtual Random Keyboard";
  virt_kbd_dev->phys = "virt_kbd/input0";
  virt_kbd_dev->id.bustype = BUS_VIRTUAL;
  virt_kbd_dev->id.vendor = 0x0000;
  virt_kbd_dev->id.product = 0x0000;
  virt_kbd_dev->id.version = 0x0100;

  __set_bit(EV_KEY, virt_kbd_dev->evbit); // keyboard event
  __set_bit(EV_SYN, virt_kbd_dev->evbit); // sync

  for (i = 0; i < KEY_POOL_SIZE; i++) {
    __set_bit(key_pool[i], virt_kbd_dev->keybit);
  }

  ret = input_register_device(virt_kbd_dev);
  if (ret) {
    pr_err(DRIVER_NAME ": input_register_device failed (%d)\n", ret);
    input_free_device(virt_kbd_dev);
    return ret;
  }

  pr_info(DRIVER_NAME ": input device registered\n");

  if (interval < 1) {
    pr_warn(DRIVER_NAME ": invalid interval %d, using 1 s\n", interval);
    interval = 1;
  }

  timer_setup(&virt_kbd_timer, virt_kbd_timer_cb, 0);
  mod_timer(&virt_kbd_timer, jiffies + (unsigned long)interval * HZ);

  pr_info(DRIVER_NAME ": timer started, first event in %d s\n", interval);
  return 0;
}

static void __exit virt_kbd_exit(void) {
  pr_info(DRIVER_NAME ": unloading, stopping timer\n");
  del_timer_sync(&virt_kbd_timer);
  input_unregister_device(virt_kbd_dev);
  pr_info(DRIVER_NAME ": unloaded\n");
}

module_init(virt_kbd_init);
module_exit(virt_kbd_exit);
