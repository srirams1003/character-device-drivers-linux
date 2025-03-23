make clean && make
sudo insmod mychardev.ko
lsmod | grep mychardev
ls /dev | grep mychardev
sudo dmesg

# here is how to read and write to the one of the device drivers
#  echo "hi my chardev 2" > /dev/mychardev-2 # write
#  cat /dev/mychardev-2 # read
