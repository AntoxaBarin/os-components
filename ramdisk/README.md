# ramdisk

```bash
$ sudo insmod ./ramdisk.ko num_sectors=262144

$ sudo blockdev --getsize64 /dev/ramdisk
134217728
$ cat /proc/devices | grep ramdisk
240 ramdisk
$ sudo dmesg | tail -5
[ 1688.748725] ramdisk: pos=261632 bytes=57344 cur=4096 dir=R
[ 1688.748785] ramdisk: pos=261752 bytes=65536 cur=4096 dir=R
[ 1688.748848] ramdisk: pos=261896 bytes=61440 cur=4096 dir=R
[ 1688.748913] ramdisk: pos=262024 bytes=28672 cur=4096 dir=R
[ 1688.748959] ramdisk: pos=262088 bytes=20480 cur=4096 dir=R

$ sudo mkdir -p /mnt/ramdisk-test
$ sudo mkfs.ext4 -F /dev/ramdisk
mke2fs 1.47.0 (5-Feb-2023)
Creating filesystem with 32768 4k blocks and 32768 inodes

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (4096 blocks): done
Writing superblocks and filesystem accounting information: done

$ sudo mount -t ext4 /dev/ramdisk /mnt/ramdisk-test
$ mount | grep ramdisk
/dev/ramdisk on /mnt/ramdisk-test type ext4 (rw,relatime)

$ df -h /mnt/ramdisk-test
Filesystem      Size  Used Avail Use% Mounted on
/dev/ramdisk    104M   24K   95M   1% /mnt/ramdisk-test

$ for i in 1 2 3 4 5; do
    sudo dd if=/dev/urandom of=/mnt/ramdisk-test/file_$i.bin \
            bs=4096 count=$((i*256)) status=none
done

$ ls -hal /mnt/ramdisk-test/
total 16M
-rw-r--r-- 1 root root 1.0M Apr 29 11:40 file_1.bin
-rw-r--r-- 1 root root 2.0M Apr 29 11:40 file_2.bin
-rw-r--r-- 1 root root 3.0M Apr 29 11:40 file_3.bin
-rw-r--r-- 1 root root 4.0M Apr 29 11:40 file_4.bin
-rw-r--r-- 1 root root 5.0M Apr 29 11:40 file_5.bin
drwx------ 2 root root  16K Apr 29 11:40 lost+found

$ sudo sha256sum /mnt/ramdisk-test/*.bin | sudo tee /tmp/hashes.before
$ sync
$ sudo umount /mnt/ramdisk-test
$ sudo mount -t ext4 /dev/ramdisk /mnt/ramdisk-test
$ sudo sha256sum /mnt/ramdisk-test/*.bin > /tmp/hashes.after

$ diff /tmp/hashes.before /tmp/hashes.after
$ echo $?
0 # no diff
```
