# udev

1. Правило при подключении флешки:

```bash
$ sudo nano /etc/udev/rules.d/99-usb.rules
$ sudo udevadm control --reload-rules
```

Подключаем флешку:

```bash
$ sudo dmesg -T | grep usb

[Вс апр 26 23:24:52 2026] usb 4-3: new SuperSpeed USB device number 2 using xhci_hcd
[Вс апр 26 23:24:54 2026] usb 4-3: New USB device found, idVendor=090c, idProduct=1000, bcdDevice=11.00
[Вс апр 26 23:24:54 2026] usb 4-3: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[Вс апр 26 23:24:54 2026] usb 4-3: Product: USB Flash Disk
[Вс апр 26 23:24:54 2026] usb 4-3: Manufacturer: General
[Вс апр 26 23:24:54 2026] usb 4-3: SerialNumber: NTRP000000015333
[Вс апр 26 23:24:54 2026] usb-storage 4-3:1.0: USB Mass Storage device detected
[Вс апр 26 23:24:54 2026] scsi host0: usb-storage 4-3:1.0
[Вс апр 26 23:24:54 2026] usbcore: registered new interface driver usb-storage
[Вс апр 26 23:24:54 2026] usbcore: registered new interface driver uas
```
Проверяем, что правило отработало:

```bash
$ cat /tmp/usb-test.log

2026-04-26_23:31:33 ADDED device=sda vendor=General model=USB_DISK
2026-04-26_23:31:33 ADDED device=sda1 vendor=General model=USB_DISK
# тут флешка была извлечена
2026-04-26_23:31:34 REMOVED device=sda1 vendor=General model=USB_DISK
2026-04-26_23:31:34 REMOVED device=sda vendor=General model=USB_DISK
```

Правило, устанавливающее правила:

```bash
$ sudo nano /etc/udev/rules.d/98-usb-mode.rules
$ sudo udevadm control --reload-rules

# Всем пользователям из группы docker даем 0660
$ ls -hal /dev/sda*
brw-rw---- 1 root docker 8, 0 апр 26 23:38 /dev/sda
brw-rw---- 1 root docker 8, 1 апр 26 23:38 /dev/sda1
```
