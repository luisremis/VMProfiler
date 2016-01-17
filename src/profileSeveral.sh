make
sudo dmesg -C

a=1
b=1

while [ $a -le 20 ]
	do
	echo $a

	sudo insmod mp3.ko
	sudo mknod /dev/mp_device c 251 0

	while [ $b -le $a ]
	do
	nice ./workapp 200 R 10000 &
	b=`expr $b + 1`
	done

	wait
   	sudo ./monitor > profile3_$a.data
	sudo rm /dev/mp_device
	sudo rmmod mp3

   	a=`expr $a + 1`
   	b=1
done