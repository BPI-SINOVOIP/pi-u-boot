
if mmc dev 1; then
    set bootargs 'console=ttyS0,115200 rootfstype=ext4 rootwait root=/dev/mmcblk1p16 rw tz_enable vppta cma=343932928@1509949440';
    fatload mmc 1:b 0x15a00000 kernel.dtb;
    fatload mmc 1:b 0x04a80000 vmlinux;
    booti 0x04a80000 - 0x15a00000;
else
    set bootargs 'console=ttyS0,115200 rootfstype=ext4 rootwait root=/dev/mmcblk0p16 rw tz_enable vppta cma=343932928@1509949440';
    fatload mmc 0:b 0x15a00000 kernel.dtb;
    fatload mmc 0:b 0x04a80000 vmlinux;
    booti 0x04a80000 - 0x15a00000;
fi
