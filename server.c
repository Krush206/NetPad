#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "global.h"

typedef struct {
    int type;
    int code;
    int val;
} Event;

static void map_hat(int, int *, int *);
static void emit(int, Event *);
static int map_axis(uint8_t);
static int map_button(uint8_t);
static int setup_uinput(void);

int main(void)
{
    int ufd;
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5501);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        fprintf(stderr, "Can't bind port.\n");
        return 1;
    }

    printf("Listening on UDP %d...\n", 5501);

    ufd = setup_uinput();

    while(1)
    {
        Packet p;

        if(recv(sock, &p, sizeof p, 0) != sizeof p)
            continue;

        switch (p.type)
        {
            Event e;
        case EVT_BUTTON:
            e.type = EV_KEY;
	    e.code = map_button(p.id);
	    e.val = p.value;
            emit(ufd, &e);
            break;
        case EVT_AXIS:
            e.type = EV_ABS;
	    e.code = map_axis(p.id);
	    e.val = p.value;
            emit(ufd, &e);
            break;
	case EVT_HAT:
        {
            int x, y;

            map_hat(p.value, &x, &y);
	    e.type = EV_ABS;
	    e.code = ABS_HAT0X;
	    e.val = x;
            emit(ufd, &e);
	    e.type = EV_ABS;
	    e.code = ABS_HAT0Y;
	    e.val = y;
            emit(ufd, &e);
            break;
        }
        default:
            continue;
        }
    }

    close(ufd);
    close(sock);
    return 0;
}

/* emit helper */
static void emit(int fd, Event *e)
{
    struct input_event in;

    memset(&in, 0, sizeof in);
    in.type = e->type;
    in.code = e->code;
    in.value = e->val;

    write(fd, &in, sizeof in);
}

/* setup virtual controller */
static int setup_uinput(void)
{
    int fd;
    struct uinput_user_dev dev;
    
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0)
    {
        fprintf(stderr, "Can't open /dev/uinput.\n");
        exit(1);
    }

    /* enable events */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);

    /* buttons */
    ioctl(fd, UI_SET_KEYBIT, BTN_A);
    ioctl(fd, UI_SET_KEYBIT, BTN_B);
    ioctl(fd, UI_SET_KEYBIT, BTN_X);
    ioctl(fd, UI_SET_KEYBIT, BTN_Y);

    /* axes */
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

    /* hat (d-pad) */
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);
    ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);

    memset(&dev, 0, sizeof(dev));
    snprintf(dev.name, UINPUT_MAX_NAME_SIZE, "Net Gamepad");

    /* axis ranges */
    dev.absmin[ABS_X] = -32768;
    dev.absmax[ABS_X] = 32767;
    dev.absmin[ABS_Y] = -32768;
    dev.absmax[ABS_Y] = 32767;

    /* hat (d-pad) control */
    dev.absmin[ABS_HAT0X] = -1;
    dev.absmax[ABS_HAT0X] = 1;
    dev.absmin[ABS_HAT0Y] = -1;
    dev.absmax[ABS_HAT0Y] = 1;

    write(fd, &dev, sizeof(dev));
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

/* map SDL button index → evdev */
static int map_button(uint8_t id)
{
    switch(id)
    {
        case 0: return BTN_A;
        case 1: return BTN_B;
        case 2: return BTN_X;
        case 3: return BTN_Y;
        default: return BTN_A + id; /* fallback */
    }
}

/* map SDL axis → evdev */
static int map_axis(uint8_t id)
{
    switch(id)
    {
        case 0: return ABS_X;
        case 1: return ABS_Y;
        default: return ABS_X + id;
    }
}

/* map hat value → X/Y */
static void map_hat(int val, int *x, int *y)
{
    *x = 0;
    *y = 0;

    if(val & 0x01) *y = -1; // up
    if(val & 0x04) *y =  1; // down
    if(val & 0x02) *x =  1; // right
    if(val & 0x08) *x = -1; // left
}
