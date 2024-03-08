TARGETS = tourboxlinux

CFLAGS = -O2 -Wall -Wextra
DESTDIR = $${HOME}/bin

.PHONY: all clean install uninstall install-service uninstall-service

all: $(TARGETS)

tourboxlinux: tourboxlinux.o

clean:
	-rm -f $(TARGETS) *~ tourboxlinux.o \#*

install: tourboxlinux
	install tourboxlinux -Dt $(DESTDIR)

uninstall:
	-rm $(DESTDIR)/tourboxlinux

install-service: install
	-mkdir -p ~/.config/systemd/user
	cp tourboxlinux.service ~/.config/systemd/user
	sed -i -e "s;@DESTDIR@;$(DESTDIR);" ~/.config/systemd/user/tourboxlinux.service
	systemctl --user daemon-reload
	systemctl --user enable tourboxlinux.service
	systemctl --user start tourboxlinux.service
	systemctl --user daemon-reload

uninstall-service:
	-systemctl --user disable tourboxlinux.service
	-systemctl --user stop tourboxlinux.service
	-rm -f ~/.config/systemd/user/tourboxlinux.service
	-systemctl --user daemon-reload
