make clean && make
sudo insmod mychardev.ko
lsmod | grep mychardev
ls /dev | grep mychardev
sudo dmesg
