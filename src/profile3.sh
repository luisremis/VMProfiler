make
sudo dmesg -C

sudo insmod mp3.ko
sudo mknod /dev/mp_device c 251 0
nice ./workapp 200 R 10000 &
wait
sudo ./monitor > profile3_1.dat
sudo rm /dev/mp_device
sudo rmmod mp3

sudo insmod mp3.ko
sudo mknod /dev/mp_device c 251 0
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
wait
sudo ./monitor > profile3_2.dat
sudo rm /dev/mp_device
sudo rmmod mp3

sudo insmod mp3.ko
sudo mknod /dev/mp_device c 251 0
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
wait
sudo ./monitor > profile3_3.dat
sudo rm /dev/mp_device
sudo rmmod mp3

sudo insmod mp3.ko
sudo mknod /dev/mp_device c 251 0
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
nice ./workapp 200 R 10000 &
wait
sudo ./monitor > profile3_4.dat
sudo rm /dev/mp_device
sudo rmmod mp3

dmesg 
