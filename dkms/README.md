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
```
