sudo docker build -f docker/t4p4s.docker       -t t4p4s       .
sudo docker build -f docker/t4p4s-l2fwd.docker -t t4p4s-l2fwd .

sudo docker run --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /dev:/dev \
	-t t4p4s-l2fwd
