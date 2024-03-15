TARGETS = tourboxlinux

CFLAGS = -O2 -Wall -Wextra
PREFIX = /usr/local

.PHONY: all clean install uninstall install-service uninstall-service

all: $(TARGETS)

tourboxlinux: tourboxlinux.o

clean:
	-rm -f $(TARGETS) *~ tourboxlinux.o \#*

install: tourboxlinux
	sudo install tourboxlinux -Dt $(PREFIX)/bin
	sudo cp 90-tourbox.rules /etc/udev/rules.d
	sudo udevadm control --reload
	echo 'Please reconnect TourBox'

uninstall:
	-rm $(PREFIX)/bin/tourboxlinux /etc/udev/rules.d/90-tourbox.rules
	sudo udevadm control --reload
