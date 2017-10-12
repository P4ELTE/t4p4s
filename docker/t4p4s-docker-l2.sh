sudo docker build -f t4p4s-16.docker       -t t4p4s-16                .
sudo docker build -f l2-switch-test.docker -t t4p4s-16-l2-switch-test .
sudo docker run --privileged -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev -t t4p4s-16-l2-switch-test
