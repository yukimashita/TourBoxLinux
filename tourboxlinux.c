#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <err.h>

// [uinput module](https://kernel.org/doc/html/v6.2/input/uinput.html)
#include <linux/uinput.h>
#include <linux/input-event-codes.h>


#define USB_PRODUCT_NAME "TourBox NEO"
#define USB_VENDER_ID 0x2e3c
#define USB_PRODUCT_ID 0x5740
#define UNUSED(v) (void)v


static int verbose;
static volatile sig_atomic_t quit;


#define B(name, value) \
    name ## _PUSH = value, \
    name ## _RELEASE = (value | 0x80)

#define H(name, value) \
    name ## _RIGHT = (value | 0x40), \
    name ## _LEFT = value

#define V(name, value) \
    name ## _UP = (value | 0x40), \
    name ## _DOWN = value

enum TourBox_key {
    TBK_NONE = -1,
    B(TBK_SIDE, 0x01),
    V(TBK_SCROLL, 0x09),
    B(TBK_SCROLL, 0x0a),
    B(TBK_TOP, 0x02),
    B(TBK_C1, 0x22),
    B(TBK_C2, 0x23),
    B(TBK_TOUR, 0x2a),
    H(TBK_KNOB, 0x04),
    B(TBK_KNOB, 0x37),
    H(TBK_DIAL, 0x0f),
    B(TBK_DIAL, 0x38),
    B(TBK_UP, 0x10),
    B(TBK_RIGHT, 0x13),
    B(TBK_DOWN, 0x11),
    B(TBK_LEFT, 0x12),
    B(TBK_TALL, 0x00),
    B(TBK_SHORT, 0x03)
};

enum modifier_key {
    modifier_none = 0,
    modifier_control = 1,
    modifier_shift = 2,
    modifier_alt = 4,
    modifier_meta = 8
};


int emit(int fd, int type, int code, int value)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = value;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    return write(fd, &ie, sizeof(ie)) == sizeof(ie)? 0: 1;
}


void syn(int fd)
{
    emit(fd, EV_SYN, SYN_REPORT, 0);
}


void modifier(int fd, enum modifier_key modifier_key, int push)
{
    if (modifier_key & modifier_control)
        emit(fd, EV_KEY, KEY_LEFTCTRL, push);
    if (modifier_key & modifier_shift)
        emit(fd, EV_KEY, KEY_LEFTSHIFT, push);
    if (modifier_key & modifier_alt)
        emit(fd, EV_KEY, KEY_LEFTALT, push);
    if (modifier_key & modifier_meta)
        emit(fd, EV_KEY, KEY_LEFTMETA, push);
}


void key_push(int fd, int code, int value, int modifier_key)
{
    UNUSED(value);

    modifier(fd, modifier_key, 1);
    emit(fd, EV_KEY, code, 1);
    syn(fd);
}


void key_release(int fd, int code, int value, int modifier_key)
{
    UNUSED(value);

    emit(fd, EV_KEY, code, 0);
    modifier(fd, modifier_key, 0);
    syn(fd);
}


void key_input(int fd, int code, int value, int modifier_key)
{
    key_push(fd, code, value, modifier_key);
    key_release(fd, code, value, modifier_key);
}


struct tbk_keymap {
    enum TourBox_key tbk;
    // type, code, value -> struct uinput_event
    // see /usr/include/linux/{uinput,input-event-codes}.h
    //int type; // unused
    int code;
    int value;
    enum modifier_key modifier;
    void (*handler)(int fd, int code, int value, int modifier_key);
};

#define KEYMAP_FOREACH(km) for (km = keymaps; km->tbk != TBK_NONE; km++)

const struct tbk_keymap keymaps[] = {
    // SIDE
    {
        TBK_SIDE_PUSH,
        KEY_LEFTSHIFT,
        1,
        modifier_none,
        key_push
    },
    {
        TBK_SIDE_RELEASE,
        KEY_LEFTSHIFT,
        0,
        modifier_none,
        key_release
    },

    // SCROLL
    {
        TBK_SCROLL_UP,
        KEY_LEFTBRACE,
        1,
        modifier_none,
        key_input
    },
    {
        TBK_SCROLL_DOWN,
        KEY_RIGHTBRACE,
        1,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_SCROLL_PUSH,
        KEY_??,
        0,
        modifier_none,
        key_input
    },
    */

    // TOP
    {
        TBK_TOP_PUSH,
        KEY_BACKSLASH,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_TOP_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */

    // C1, C2
    {
        TBK_C1_PUSH,
        KEY_1,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_C1_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */
    {
        TBK_C2_PUSH,
        KEY_2,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_C2_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */

    // TOUR
    {
        TBK_TOUR_PUSH,
        KEY_SLASH,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_TOUR_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */

    // KNOB
    {
        TBK_KNOB_RIGHT,
        KEY_RIGHT,
        0,
        modifier_alt,
        key_input
    },
    {
        TBK_KNOB_LEFT,
        KEY_LEFT,
        0,
        modifier_alt,
        key_input
    },
    /*
    {
        TBK_KNOB_PUSH,
        KEY_??,
        0,
        modifier_none,
        key_input
    },
    */

    // DIAL
    {
        TBK_DIAL_RIGHT,
        KEY_DOT,
        0,
        modifier_control|modifier_alt,
        key_input
    },
    {
        TBK_DIAL_LEFT,
        KEY_DOT,
        0,
        modifier_control|modifier_alt|modifier_shift,
        key_input
    },
    {
        TBK_DIAL_PUSH,
        KEY_EQUAL,
        0,
        modifier_none,
        key_input
    },

    // D-PAD
    {
        TBK_UP_PUSH,
        KEY_DOT,
        0,
        modifier_control|modifier_alt|modifier_shift,
        key_push
    },
    {
        TBK_UP_RELEASE,
        KEY_DOT,
        0,
        modifier_control|modifier_alt|modifier_shift,
        key_release
    },
    {
        TBK_RIGHT_PUSH,
        KEY_RIGHT,
        0,
        modifier_alt,
        key_push
    },
    {
        TBK_RIGHT_RELEASE,
        KEY_RIGHT,
        0,
        modifier_alt,
        key_release
    },
    {
        TBK_DOWN_PUSH,
        KEY_DOT,
        0,
        modifier_control|modifier_alt,
        key_push
    },
    {
        TBK_DOWN_RELEASE,
        KEY_DOT,
        0,
        modifier_control|modifier_alt,
        key_release
    },
    {
        TBK_LEFT_PUSH,
        KEY_LEFT,
        0,
        modifier_alt,
        key_push
    },
    {
        TBK_LEFT_RELEASE,
        KEY_LEFT,
        0,
        modifier_alt,
        key_release
    },

    // TOLL, SHORT
    {
        TBK_TALL_PUSH,
        KEY_3,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_TALL_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */
    {
        TBK_SHORT_PUSH,
        KEY_4,
        0,
        modifier_none,
        key_input
    },
    /*
    {
        TBK_SHORT_RELEASE,
        KEY_??,
        0,
        modifier_none,
        key_release
    },
    */

    // sentinel
    {
        TBK_NONE,
        -1,
        -1,
        -1,
        NULL
    }
};


void usage()
{
    fprintf(stderr, "tourboxlinux [-D] [-v] [-d serial-device]\n");
    exit(1);
}


void sighandler(int signo)
{
    UNUSED(signo);

    quit = 1;
}


const struct tbk_keymap *keymap_find(enum TourBox_key tbk)
{
    const struct tbk_keymap *km;

    KEYMAP_FOREACH(km) {
        if (km->tbk == tbk)
            return km;
    }

    return NULL;
}


int uinput_setup()
{
    int fd;
    struct uinput_setup usetup;
    const struct tbk_keymap *km;

    if ((fd = open("/dev/uinput", O_WRONLY|O_NONBLOCK)) < 0)
        err(1, "/dev/uinput");

    if (verbose) {
        int v;
        if (ioctl(fd, UI_GET_VERSION, &v) < 0)
            err(1, "UI_GET_VERSION");
        printf("uinput version: %d\n", v);
    }

    KEYMAP_FOREACH(km) {
        if (km->handler == key_input ||
            km->handler == key_push ||
            km->handler == key_release)
        {
            if (ioctl(fd, UI_SET_KEYBIT, km->code) < 0)
                err(1, "UI_SET_KEYBIT");
        } else
            abort();
    }

    // EV_KEY, modifiers
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        err(1, "UI_SET_EVBIT");
    if (ioctl(fd, UI_SET_KEYBIT, KEY_LEFTCTRL) < 0)
        err(1, "UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT) < 0)
        err(1, "UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, KEY_LEFTALT) < 0)
        err(1, "UI_SET_KEYBIT");
    if (ioctl(fd, UI_SET_KEYBIT, KEY_LEFTMETA) < 0)
        err(1, "UI_SET_KEYBIT");

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = USB_VENDER_ID;
    usetup.id.product = USB_PRODUCT_ID;
    strncpy(usetup.name, USB_PRODUCT_NAME, sizeof(usetup.name)-1);
    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0)
        err(1, "UI_DEV_SETUP");
    if (ioctl(fd, UI_DEV_CREATE) < 0)
        err(1, "UI_DEV_CREATE");

    return fd;
}


void uinput_destroy(int fd)
{
    if (ioctl(fd, UI_DEV_DESTROY) < 0)
        err(1, "UI_DEV_DESTROY");
    close(fd);
}

int tourbox_setup(const char *dev)
{
    int fd;
    ssize_t n;
    struct termios oterm, term;
    char buf[256];
    static const uint8_t magic1[] = {
        0x55, 0x00, 0x07, 0x88, 0x94, 0x00, 0x1a, 0xfe
    };
    static const uint8_t magic2[] = {
        0xb5, 0x00, 0x5d, 0x04, 0x08, 0x05, 0x08, 0x06,
        0x08, 0x07, 0x08, 0x08, 0x08, 0x09, 0x08, 0x0b,
        0x08, 0x0c, 0x08, 0x0d, 0x08, 0x0e, 0x08, 0x0f,
        0x08, 0x26, 0x08, 0x27, 0x08, 0x28, 0x08, 0x29,
        0x08, 0x3b, 0x08, 0x3c, 0x08, 0x3d, 0x08, 0x3e,
        0x08, 0x3f, 0x08, 0x40, 0x08, 0x41, 0x08, 0x42,
        0x08, 0x43, 0x08, 0x44, 0x08, 0x45, 0x08, 0x46,
        0x08, 0x47, 0x08, 0x48, 0x08, 0x49, 0x08, 0x4a,
        0x08, 0x4b, 0x08, 0x4c, 0x08, 0x4d, 0x08, 0x4e,
        0x08, 0x4f, 0x08, 0x50, 0x08, 0x51, 0x08, 0x52,
        0x08, 0x53, 0x08, 0x54, 0x08, 0xa8, 0x08, 0xa9,
        0x08, 0xaa, 0x08, 0xab, 0x08, 0xfe
    };

    if ((fd = open(dev, O_RDWR|O_NOCTTY|O_NONBLOCK)) < 0)
        err(1, "open");

    if (tcgetattr(fd, &oterm) < 0)
        err(1, "tcgetattr");
    term = oterm;
    cfmakeraw(&term);
    term.c_cc[VMIN] = 1;
    cfsetspeed(&term, B115200);
    if (tcflush(fd, TCIOFLUSH) < 0)
        err(1, "tcflush");
    if (tcsetattr(fd, TCSAFLUSH, &term) < 0)
        err(1, "tcsetattr");

    if (write(fd, magic1, sizeof(magic1)) != sizeof(magic1))
        err(1, "write");
    usleep(100000);
    n = read(fd, buf, sizeof(buf));
    (void)n;

    if (write(fd, magic2, sizeof(magic2)) != sizeof(magic2))
        err(1, "write");

    return fd;
}


void tourbox_destroy(int fd)
{
    //tcsetattr(fd, TCSANOW, &oterm);
    //tcflush(fd, TCIOFLUSH);
    close(fd);
}


int main(int argc, char *argv[])
{
    int c, fdi, fdo;
    int daemonize;
    unsigned char tbk;
    const char *dev;
    const struct tbk_keymap *map;

    if (signal(SIGINT, sighandler) == SIG_ERR)
        err(1, "signal");
    if (signal(SIGTERM, sighandler) == SIG_ERR)
        err(1, "signal");

    daemonize = 0;
    verbose = 0;
    dev = "/dev/ttyACM0";
    while ((c = getopt(argc, argv, "vd:")) != -1) {
        switch (c) {
          case 'D':
            daemonize = 1;
            break;
          case 'v':
            verbose = 1;
            break;
          case 'd':
            dev = optarg;
            break;
          default:
            usage();
        }
    }

    if (daemonize) {
        if (daemon(0, 0) < 0)
            err(1, "daemon");
    }

    fdi = tourbox_setup(dev);
    fdo = uinput_setup();

    sleep(1);
    quit = 0;
    while (!quit) {
        if (read(fdi, &tbk, 1) != 1) {
            usleep(100000);
            continue;
        }
        if (verbose)
            printf("-> 0x%02x\n", tbk);
        if ((map = keymap_find(tbk)))
            map->handler(fdo, map->code, map->value, map->modifier);
    }

    uinput_destroy(fdo);
    tourbox_destroy(fdi);

    return 0;
}
