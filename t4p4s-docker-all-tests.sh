
sudo docker build -f docker/t4p4s.docker    -t t4p4s    .
sudo docker build -f docker/t4p4s-all-tests.docker -t t4p4s-all-tests .

sudo docker run --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /dev:/dev \
	-t t4p4s-all-tests \
	$@
