# ShanghaiTech University CS 210 Computer Architecture II Project

## Overview
The original goal is to build an ARM coprocessor in an x86 architecture to speed up the performance on data processing.  
Our goal is to first simulate the result on emulators such as QEMU, then try out on real hardware.

## Simulation

### QEMU set up
I am using Arch Linux, so the installation is simple:
```bash
$ sudo pacman -S qemu qemu-arch-extra libvirt
```
Then download [kernel image](http://releases.linaro.org/openembedded/aarch64/17.01/Image) and [disk image](http://releases.linaro.org/openembedded/aarch64/17.01/vexpress64-openembedded_lamp-armv8-gcc-5.2_20170127-761.img.gz) from Linaro.  
Following the instructions on the [web page](http://releases.linaro.org/openembedded/aarch64/17.01/), start the emulator:
```bash
$ qemu-system-aarch64 -m 1024 -cpu cortex-a57 -nographic -machine virt \
  -kernel Image -append 'root=/dev/vda2 rw rootwait mem=1024M console=ttyAMA0,38400n8' \
  -netdev user,id=user0 -device virtio-net-device,netdev=user0  -device virtio-blk-device,drive=disk \
  -drive if=none,id=disk,file=vexpress64-openembedded_lamp-armv8-gcc-5.2_20170127-761.img
```
You can change increase the memory by modifying `-m` option and kernel parameters.  
Once the VM is up and running, you can access host at 10.0.2.2.

### Network bandwidth test

Download the source code of [iperf3](https://github.com/esnet/iperf/archive/3.6.tar.gz) and compile it:
```bash
$ ./configure
$ make
$ make install
```
You also need to install it on your host. First start server on host:
```bash
$ iperf3 -s
```
Then test the bandwidth:
```bash
$ iperf3 -c 10.0.2.2
```
Our result the bandwidth is around 150 Mbps.

### File transfer
We use GNU/Linux's socket to do the file transferring. Details see `src/server.c` and `src/client.c`.  
The transfer speed is around 110 Mbps.  
