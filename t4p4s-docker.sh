
if [ $# -eq 0 ]; then
	echo Error: no arguments found
	echo The arguments will be passed on to t4p4s.sh executed in the container
	exit 1
fi

sudo docker build -f docker/t4p4s.docker    -t t4p4s    .
sudo docker build -f docker/t4p4s-sh.docker -t t4p4s-sh .

sudo docker run --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /dev:/dev \
	-t t4p4s-sh \
	$@
