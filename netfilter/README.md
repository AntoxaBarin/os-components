# netfilter

```bash
$ sudo insmod netfilter.ko
$ curl -s -o /dev/null https://ya.ru

$ sudo dmesg -T | grep tcp_tracker
[Mon Apr 27 19:58:35 2026] tcp_tracker: loaded (filter_port=0, log_all=0)
[Mon Apr 27 19:58:58 2026] tcp_tracker: OUT 192.168.122.216:51250 -> 5.255.255.242:443  flags:S

$ echo '443' | sudo tee /sys/kernel/tcp_tracker/filter_port 
443
$ timeout 30 curl -v https://ya.ru
* Host ya.ru:443 was resolved.
* IPv6: 2a02:6b8::2:242
* IPv4: 77.88.44.242, 5.255.255.242, 77.88.55.242
*   Trying 77.88.44.242:443...
*   Trying [2a02:6b8::2:242]:443...
* Immediate connect fail for 2a02:6b8::2:242: Network is unreachable

$ sudo dmesg -T | grep tcp_tracker
[Mon Apr 27 19:58:35 2026] tcp_tracker: loaded (filter_port=0, log_all=0)
[Mon Apr 27 19:58:58 2026] tcp_tracker: OUT 192.168.122.216:51250 -> 5.255.255.242:443  flags:S
[Mon Apr 27 19:59:51 2026] tcp_tracker: blocking destination port 443
[Mon Apr 27 20:00:06 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:07 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:08 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:09 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:10 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:11 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:13 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:17 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
[Mon Apr 27 20:00:26 2026] tcp_tracker: BLOCKED 192.168.122.216:53028 -> 77.88.44.242:443  flags:S
```
