make
sudo dmesg -C

sudo insmod mp3.ko
sudo mknod /dev/mp_device c 251 0
nice ./workapp 1024 R 50000 &
nice ./workapp 1024 R 10000 &
wait
cat /proc/mp3/status
pkill workapp
sudo ./monitor > profile1.dat
sudo rm /dev/mp_device
sudo rmmod mp3

dmesg 
