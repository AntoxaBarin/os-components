# Virtual keyboard

```bash
sudo insmod virt_kbd.ko
rmmod virt_kbd

echo 0 > /sys/module/virt_kbd/parameters/enabled  # pause
echo 1 > /sys/module/virt_kbd/parameters/enabled  # enable 
cat /sys/module/virt_kbd/parameters/interval      # get interval
echo 5 > /sys/module/virt_kbd/parameters/interval # change interval
```

Example:

```bash
sudo insmod virt_kbd.ko
cat /proc/bus/input/devices

...
I: Bus=0006 Vendor=0000 Product=0000 Version=0100
N: Name="Virtual Random Keyboard"
P: Phys=virt_kbd/input0
S: Sysfs=/devices/virtual/input/input5
U: Uniq=
H: Handlers=kbd event4 ### 
B: PROP=0
B: EV=3
B: KEY=7f07fc3ff0ffc


sudo evtest /dev/input/event4
Input driver version is 1.0.1
Input device ID: bus 0x6 vendor 0x0 product 0x0 version 0x100
Input device name: "Virtual Random Keyboard"
Supported events:
    Event type 0 (EV_SYN)
    Event type 1 (EV_KEY)
    Event code 2 (KEY_1)
    Event code 3 (KEY_2)
    ...
    Event code 50 (KEY_M)

Properties:
Testing ... (interrupt to exit)
Event: time 1778524016.074053, type 1 (EV_KEY), code 2 (KEY_1), value 1
Event: time 1778524016.074053, -------------- SYN_REPORT ------------
Event: time 1778524016.074077, type 1 (EV_KEY), code 2 (KEY_1), value 0
Event: time 1778524016.074077, -------------- SYN_REPORT ------------
Event: time 1778524036.554062, type 1 (EV_KEY), code 4 (KEY_3), value 1
Event: time 1778524036.554062, -------------- SYN_REPORT ------------
Event: time 1778524036.554083, type 1 (EV_KEY), code 4 (KEY_3), value 0
Event: time 1778524036.554083, -------------- SYN_REPORT ------------
Event: time 1778524057.033957, type 1 (EV_KEY), code 44 (KEY_Z), value 1
Event: time 1778524057.033957, -------------- SYN_REPORT ------------
Event: time 1778524057.033984, type 1 (EV_KEY), code 44 (KEY_Z), value 0
Event: time 1778524057.033984, -------------- SYN_REPORT ------------
Event: time 1778524077.513923, type 1 (EV_KEY), code 44 (KEY_Z), value 1
Event: time 1778524077.513923, -------------- SYN_REPORT ------------
Event: time 1778524077.513940, type 1 (EV_KEY), code 44 (KEY_Z), value 0
Event: time 1778524077.513940, -------------- SYN_REPORT ------------
```
