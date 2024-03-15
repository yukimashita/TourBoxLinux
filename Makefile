TARGETS = tourboxlinux

CFLAGS = -O2 -Wall -Wextra
PREFIX = /usr/local

.PHONY: all clean install uninstall

all: $(TARGETS)

tourboxlinux: tourboxlinux.o

clean:
	-rm -f $(TARGETS) *~ tourboxlinux.o \#*

install: tourboxlinux
	sudo install tourboxlinux -Dt $(PREFIX)/bin
	sed -e 's;@PREFIX@;/usr/local;' <tourboxlinux.service | sudo tee /etc/systemd/system/tourboxlinux.service >/dev/null
	sudo systemctl daemon-reload
	sudo systemctl enable tourboxlinux.service
	sudo systemctl stop tourboxlinux.service
	sudo cp 90-tourboxlinux.rules /etc/udev/rules.d
	sudo udevadm control --reload
	echo 'Please reconnect TourBox'

uninstall:
	-sudo rm /etc/udev/rules.d/90-tourboxlinux.rules
	sudo udevadm control --reload
	sudo systemctl stop tourboxlinux.service
	sudo systemctl disable tourboxlinux.service
	-sudo rm /etc/systemd/system/tourboxlinux.service
	sudo systemctl daemon-reload
	-sudo rm $(PREFIX)/bin/tourboxlinux
