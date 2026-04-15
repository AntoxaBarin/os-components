# DKMS module

```bash
$ sudo mkdir -p /usr/src/membuf-0.1
$ sudo cp membuf.c Makefile dkms.conf /usr/src/membuf-0.1

$ sudo dkms add -m membuf -v 0.1
Creating symlink /var/lib/dkms/membuf/0.1/source -> /usr/src/membuf-0.1

$ sudo dkms build -m membuf -v 0.1
Sign command: /usr/bin/kmodsign
Certificate or key are missing, generating them using update-secureboot-policy...
Adding '/var/lib/shim-signed/mok/MOK.der' to shim:
Signing key: /var/lib/shim-signed/mok/MOK.priv
Public certificate (MOK): /var/lib/shim-signed/mok/MOK.der

Building module:
Cleaning build area...
make -j16 KERNELRELEASE=6.8.0-107-generic -C /lib/modules/6.8.0-107-generic/build M=/var/lib/dkms/membuf/0.1/build modules...
Signing module /var/lib/dkms/membuf/0.1/build/./membuf.ko
Cleaning build area...

$ sudo dkms install -m membuf -v 0.1
membuf.ko.zst:
Running module version sanity check.
 - Original module
   - No original module exists within this kernel
 - Installation
   - Installing to /lib/modules/6.8.0-107-generic/updates/dkms/
depmod...

$ dkms status
membuf/0.1, 6.8.0-107-generic, x86_64: installed
```


# Debug kernel

```bash
$ uname -r
6.8.0-110-generic

$ apt source linux
$ cp /boot/config-$(uname -r) .config
$ scripts/config --enable CONFIG_DEBUG_KMEMLEAK
$ scripts/config --enable CONFIG_DEBUG_KMEMLEAK_DEFAULT_OFF
$ scripts/config --enable CONFIG_KASAN
$ scripts/config --enable CONFIG_KASAN_GENERIC

$ make olddefconfig
$ make -j$(nproc) bindeb-pkg LOCALVERSION=-debug-kmemleak-kasan

$ scripts/config --disable CONFIG_SYSTEM_TRUSTED_KEYS
$ scripts/config --disable CONFIG_SYSTEM_REVOCATION_KEYS
$ scripts/config --set-str CONFIG_SYSTEM_TRUSTED_KEYS ""
$ scripts/config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""
$ make olddefconfig

$ make -j$(nproc) bzImage modules LOCALVERSION=-debug-kmemleak-kasan
$ sudo make modules_install

$ sudo make install
INSTALL /boot
run-parts: executing /etc/kernel/postinst.d/dkms 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
 * dkms: running auto installation service for kernel 6.8.12-debug-kmemleak-kasan
 * dkms: autoinstall for kernel 6.8.12-debug-kmemleak-kasan                                                                          [ OK ] 
run-parts: executing /etc/kernel/postinst.d/initramfs-tools 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
update-initramfs: Generating /boot/initrd.img-6.8.12-debug-kmemleak-kasan
run-parts: executing /etc/kernel/postinst.d/unattended-upgrades 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
run-parts: executing /etc/kernel/postinst.d/update-notifier 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
run-parts: executing /etc/kernel/postinst.d/xx-update-initrd-links 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
I: /boot/initrd.img is now a symlink to initrd.img-6.8.12-debug-kmemleak-kasan
run-parts: executing /etc/kernel/postinst.d/zz-update-grub 6.8.12-debug-kmemleak-kasan /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
Sourcing file `/etc/default/grub'
Generating grub configuration file ...
Found linux image: /boot/vmlinuz-6.8.12-debug-kmemleak-kasan
Found initrd image: /boot/initrd.img-6.8.12-debug-kmemleak-kasan
Found linux image: /boot/vmlinuz-6.8.0-110-generic
Found initrd image: /boot/initrd.img-6.8.0-110-generic
Warning: os-prober will not be executed to detect other bootable partitions.
Systems on them will not be added to the GRUB boot configuration.
Check GRUB_DISABLE_OS_PROBER documentation entry.
Adding boot menu entry for UEFI Firmware Settings ...
done


$ sudo reboot
$ uname -r
6.8.12-debug-kmemleak-kasan

$ sudo cp -r . /usr/src/membuf-0.1

$ sudo dkms add -m membuf -v 0.1
Creating symlink /var/lib/dkms/membuf/0.1/source -> /usr/src/membuf-0.1

$ sudo dkms build -m membuf -v 0.1
ign command: /usr/bin/kmodsign
Binary update-secureboot-policy not found, modules won't be signed

Building module:
Cleaning build area...
make -j8 KERNELRELEASE=6.8.12-debug-kmemleak-kasan -C /lib/modules/6.8.12-debug-kmemleak-kasan/build M=/var/lib/dkms/membuf/0.1/build modules....
Cleaning build area...

$ sudo dkms install -m membuf -v 0.1

membuf.ko:
Running module version sanity check.
 - Original module
   - No original module exists within this kernel
 - Installation
   - Installing to /lib/modules/6.8.12-debug-kmemleak-kasan/updates/dkms/
depmod..........

$ sudo dkms status
membuf/0.1, 6.8.12-debug-kmemleak-kasan, x86_64: installed

$ sudo nano /etc/default/grub
# GRUB_CMDLINE_LINUX_DEFAULT="kmemleak=on"

$ sudo update-grub
$ cat /proc/cmdline
BOOT_IMAGE=/vmlinuz-6.8.12-debug-kmemleak-kasan root=/dev/mapper/ubuntu--vg-ubuntu--lv ro kmemleak=on

$ sudo dmesg | grep kmem
[    0.000000] Linux version 6.8.12-debug-kmemleak-kasan (ivan@ivan) (gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, GNU ld (GNU Binutils for Ubuntu) 2.42) #9 SMP PREEMPT_DYNAMIC Wed Apr 15 00:52:16 UTC 2026 (Ubuntu 6.8.0-110.110-generic 6.8.12)
[    0.000000] Command line: BOOT_IMAGE=/vmlinuz-6.8.12-debug-kmemleak-kasan root=/dev/mapper/ubuntu--vg-ubuntu--lv ro kmemleak=on
[    0.764260] Kernel command line: BOOT_IMAGE=/vmlinuz-6.8.12-debug-kmemleak-kasan root=/dev/mapper/ubuntu--vg-ubuntu--lv ro kmemleak=on
[   12.767923] kmemleak: Kernel memory leak detector initialized (mem pool available: 13534)
[   12.768010] kmemleak: Automatic memory scanning thread started


# Added leak in membuf_open():

[Wed Apr 15 09:27:25 2026] kmemleak: 2 new suspected memory leaks (see /sys/kernel/debug/kmemleak)

$ sudo cat /sys/kernel/debug/kmemleak
unreferenced object 0xffff8881073ede00 (size 256):
  comm "cat", pid 2345, jiffies 4296343664
  hex dump (first 32 bytes):
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  backtrace (crc 0):
    kmemleak_alloc+0x4a/0x90
    kmalloc_trace+0x33e/0x400
    qrtr_proto_init+0x52f/0xff0 [qrtr]
    chrdev_open+0x231/0x6a0
    do_dentry_open+0x60e/0x1370
    vfs_open+0xb0/0xf0
    path_openat+0x279a/0x40b0
    do_filp_open+0x1bd/0x410
    do_sys_openat2+0x14b/0x190
    __x64_sys_openat+0x128/0x220
    x64_sys_call+0x1eb1/0x25a0
    do_syscall_64+0x7f/0x180
    entry_SYSCALL_64_after_hwframe+0x78/0x80
unreferenced object 0xffff8881073ec200 (size 256):
  comm "cat", pid 3338, jiffies 4296551476
  hex dump (first 32 bytes):
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  backtrace (crc 0):
    kmemleak_alloc+0x4a/0x90
    kmalloc_trace+0x33e/0x400
    qrtr_proto_init+0x52f/0xff0 [qrtr]
    chrdev_open+0x231/0x6a0
    do_dentry_open+0x60e/0x1370
    vfs_open+0xb0/0xf0
    path_openat+0x279a/0x40b0
    do_filp_open+0x1bd/0x410
    do_sys_openat2+0x14b/0x190
    __x64_sys_openat+0x128/0x220
    x64_sys_call+0x1eb1/0x25a0
    do_syscall_64+0x7f/0x180
    entry_SYSCALL_64_after_hwframe+0x78/0x80


# KASAN

[Wed Apr 15 10:11:10 2026] ==================================================================
[Wed Apr 15 10:11:10 2026] BUG: KASAN: slab-out-of-bounds in membuf_write+0x21b/0x2c0 [membuf]
[Wed Apr 15 10:11:10 2026] Write of size 1 at addr ffff8881078d7000 by task tee/5277

[Wed Apr 15 10:11:10 2026] CPU: 6 PID: 5277 Comm: tee Tainted: G           OE      6.8.12-debug-kmemleak-kasan #9
[Wed Apr 15 10:11:10 2026] Hardware name: QEMU Ubuntu 24.04 PC (Q35 + ICH9, 2009), BIOS 1.16.3-debian-1.16.3-2 04/01/2014
[Wed Apr 15 10:11:10 2026] Call Trace:
[Wed Apr 15 10:11:10 2026]  <TASK>
[Wed Apr 15 10:11:10 2026]  dump_stack_lvl+0x76/0xa0
[Wed Apr 15 10:11:10 2026]  print_report+0xd2/0x670
[Wed Apr 15 10:11:10 2026]  ? __pfx__raw_spin_lock_irqsave+0x10/0x10
[Wed Apr 15 10:11:10 2026]  ? kasan_complete_mode_report_info+0x26/0x210
[Wed Apr 15 10:11:10 2026]  kasan_report+0xd7/0x120
[Wed Apr 15 10:11:10 2026]  ? membuf_write+0x21b/0x2c0 [membuf]
[Wed Apr 15 10:11:10 2026]  ? membuf_write+0x21b/0x2c0 [membuf]
[Wed Apr 15 10:11:10 2026]  __asan_report_store1_noabort+0x17/0x30
[Wed Apr 15 10:11:10 2026]  membuf_write+0x21b/0x2c0 [membuf]
[Wed Apr 15 10:11:10 2026]  vfs_write+0x234/0x1090
[Wed Apr 15 10:11:10 2026]  ? __pfx_vfs_write+0x10/0x10
[Wed Apr 15 10:11:10 2026]  ? __kasan_check_write+0x14/0x30
[Wed Apr 15 10:11:10 2026]  ? _raw_spin_lock_irqsave+0x96/0x100
[Wed Apr 15 10:11:10 2026]  ? __kasan_check_read+0x11/0x20
[Wed Apr 15 10:11:10 2026]  ? __fget_light+0x5c/0x480
[Wed Apr 15 10:11:10 2026]  ? sched_clock_noinstr+0x9/0x10
[Wed Apr 15 10:11:10 2026]  ? sched_clock+0x10/0x30
[Wed Apr 15 10:11:10 2026]  ksys_write+0x11e/0x250
[Wed Apr 15 10:11:10 2026]  ? __pfx_ksys_write+0x10/0x10
[Wed Apr 15 10:11:10 2026]  ? __kasan_check_write+0x14/0x30
[Wed Apr 15 10:11:10 2026]  __x64_sys_write+0x72/0xc0
[Wed Apr 15 10:11:10 2026]  x64_sys_call+0x7e/0x25a0
[Wed Apr 15 10:11:10 2026]  do_syscall_64+0x7f/0x180
[Wed Apr 15 10:11:10 2026]  ? __pfx_vfs_write+0x10/0x10
[Wed Apr 15 10:11:10 2026]  ? __asan_memset+0x39/0x50
[Wed Apr 15 10:11:10 2026]  ? __rseq_handle_notify_resume+0x1ae/0xc30
[Wed Apr 15 10:11:10 2026]  ? __pfx___rseq_handle_notify_resume+0x10/0x10
[Wed Apr 15 10:11:10 2026]  ? __kasan_check_write+0x14/0x30
[Wed Apr 15 10:11:10 2026]  ? fpregs_restore_userregs+0xea/0x210
[Wed Apr 15 10:11:10 2026]  ? switch_fpu_return+0xe/0x20
[Wed Apr 15 10:11:10 2026]  ? arch_exit_to_user_mode_prepare.isra.0+0x95/0xe0
[Wed Apr 15 10:11:10 2026]  ? syscall_exit_to_user_mode+0x43/0x1e0
[Wed Apr 15 10:11:10 2026]  ? do_syscall_64+0x8c/0x180
[Wed Apr 15 10:11:10 2026]  ? __count_memcg_events+0xe0/0x370
[Wed Apr 15 10:11:10 2026]  ? handle_mm_fault+0x13a/0x890
[Wed Apr 15 10:11:10 2026]  ? __kasan_check_read+0x11/0x20
[Wed Apr 15 10:11:10 2026]  ? fpregs_assert_state_consistent+0x22/0xb0
[Wed Apr 15 10:11:10 2026]  ? arch_exit_to_user_mode_prepare.isra.0+0x1a/0xe0
[Wed Apr 15 10:11:10 2026]  ? irqentry_exit_to_user_mode+0x38/0x1e0
[Wed Apr 15 10:11:10 2026]  ? irqentry_exit+0x43/0x50
[Wed Apr 15 10:11:10 2026]  ? clear_bhb_loop+0x30/0x80
[Wed Apr 15 10:11:10 2026]  ? clear_bhb_loop+0x30/0x80
[Wed Apr 15 10:11:10 2026]  ? clear_bhb_loop+0x30/0x80
[Wed Apr 15 10:11:10 2026]  entry_SYSCALL_64_after_hwframe+0x78/0x80
[Wed Apr 15 10:11:10 2026] RIP: 0033:0x7a149351c5a4
[Wed Apr 15 10:11:10 2026] Code: c7 00 16 00 00 00 b8 ff ff ff ff c3 66 2e 0f 1f 84 00 00 00 00 00 f3 0f 1e fa 80 3d a5 ea 0e 00 00 74 13 b8 01 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 54 c3 0f 1f 00 55 48 89 e5 48 83 ec 20 48 89
[Wed Apr 15 10:11:10 2026] RSP: 002b:00007ffffc864188 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[Wed Apr 15 10:11:10 2026] RAX: ffffffffffffffda RBX: 0000000000000006 RCX: 00007a149351c5a4
[Wed Apr 15 10:11:10 2026] RDX: 0000000000000006 RSI: 00007ffffc8642e0 RDI: 0000000000000003
[Wed Apr 15 10:11:10 2026] RBP: 00007ffffc8641b0 R08: 0000000000000006 R09: 0000000000000001
[Wed Apr 15 10:11:10 2026] R10: 00000000000001b6 R11: 0000000000000202 R12: 0000000000000006
[Wed Apr 15 10:11:10 2026] R13: 00007ffffc8642e0 R14: 00005832f86662c0 R15: 0000000000000006
[Wed Apr 15 10:11:10 2026]  </TASK>

[Wed Apr 15 10:11:10 2026] Allocated by task 5264:
[Wed Apr 15 10:11:10 2026]  kasan_save_stack+0x39/0x70
[Wed Apr 15 10:11:10 2026]  kasan_save_track+0x14/0x40
[Wed Apr 15 10:11:10 2026]  kasan_save_alloc_info+0x37/0x60
[Wed Apr 15 10:11:10 2026]  __kasan_kmalloc+0xc3/0xd0
[Wed Apr 15 10:11:10 2026]  __kmalloc+0x23e/0x570
[Wed Apr 15 10:11:10 2026]  create_membuf_device+0xdd/0x390 [membuf]
[Wed Apr 15 10:11:10 2026]  membuf_init+0xae/0xff0 [membuf]
[Wed Apr 15 10:11:10 2026]  do_one_initcall+0xae/0x400
[Wed Apr 15 10:11:10 2026]  do_init_module+0x2a7/0x800
[Wed Apr 15 10:11:10 2026]  load_module+0x7c7a/0x85f0
[Wed Apr 15 10:11:10 2026]  init_module_from_file+0xf6/0x180
[Wed Apr 15 10:11:10 2026]  idempotent_init_module+0x273/0x8c0
[Wed Apr 15 10:11:10 2026]  __x64_sys_finit_module+0xc0/0x140
[Wed Apr 15 10:11:10 2026]  x64_sys_call+0x2019/0x25a0
[Wed Apr 15 10:11:10 2026]  do_syscall_64+0x7f/0x180
[Wed Apr 15 10:11:10 2026]  entry_SYSCALL_64_after_hwframe+0x78/0x80

[Wed Apr 15 10:11:10 2026] The buggy address belongs to the object at ffff8881078d6000
                            which belongs to the cache kmalloc-4k of size 4096
[Wed Apr 15 10:11:10 2026] The buggy address is located 0 bytes to the right of
                            allocated 4096-byte region [ffff8881078d6000, ffff8881078d7000)

[Wed Apr 15 10:11:10 2026] The buggy address belongs to the physical page:
[Wed Apr 15 10:11:10 2026] page:00000000bf03036e refcount:1 mapcount:0 mapping:0000000000000000 index:0x0 pfn:0x1078d0
[Wed Apr 15 10:11:10 2026] head:00000000bf03036e order:3 entire_mapcount:0 nr_pages_mapped:0 pincount:0
[Wed Apr 15 10:11:10 2026] flags: 0x17ffffc0000840(slab|head|node=0|zone=2|lastcpupid=0x1fffff)
[Wed Apr 15 10:11:10 2026] page_type: 0xffffffff()
[Wed Apr 15 10:11:10 2026] raw: 0017ffffc0000840 ffff888100043040 dead000000000122 0000000000000000
[Wed Apr 15 10:11:10 2026] raw: 0000000000000000 0000000080040004 00000001ffffffff 0000000000000000
[Wed Apr 15 10:11:10 2026] page dumped because: kasan: bad access detected

[Wed Apr 15 10:11:10 2026] Memory state around the buggy address:
[Wed Apr 15 10:11:10 2026]  ffff8881078d6f00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
[Wed Apr 15 10:11:10 2026]  ffff8881078d6f80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
[Wed Apr 15 10:11:10 2026] >ffff8881078d7000: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[Wed Apr 15 10:11:10 2026]                    ^
[Wed Apr 15 10:11:10 2026]  ffff8881078d7080: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[Wed Apr 15 10:11:10 2026]  ffff8881078d7100: fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc fc
[Wed Apr 15 10:11:10 2026] ==================================================================
[Wed Apr 15 10:11:10 2026] Disabling lock debugging due to kernel taint

```
