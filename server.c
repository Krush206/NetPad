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
static int setup_uinput(int *);

int main(void)
{
    int fd;
    int sock;
    struct sockaddr_in addr;

    if(argc != 2)
    {
        fprintf(stderr, "Usage: %s <server port>", argv[0]);
	return 1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    if(parsePort(argv[1]))
    {
        (void) fprintf(stderr, "Invalid port.\n");
        return 1;
    }
    (void) sscanf(argv[1], "%hu", &port);
    if(!port)
    {
        (void) fprintf(stderr, "Port out of range.\n");
        return 1;
    }
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        fprintf(stderr, "Can't bind port.\n");
        return 1;
    }
    if(setup_uinput(&fd))
    {
        fprintf(stderr, "Error setting up uinput.\n");
        return 1;
    }
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
            emit(fd, &e);
            break;
        case EVT_AXIS:
            e.type = EV_ABS;
            e.code = map_axis(p.id);
            e.val = p.value;
            emit(fd, &e);
            break;
        case EVT_HAT:
        {
            int x, y;

            map_hat(p.value, &x, &y);
            e.type = EV_ABS;
            e.code = ABS_HAT0X;
            e.val = x;
            emit(fd, &e);
            e.type = EV_ABS;
            e.code = ABS_HAT0Y;
            e.val = y;
            emit(fd, &e);
            break;
        }
        default:
            continue;
        }
    }

    (void) close(fd);
    (void) close(sock);
    return 0;
}

static void emit(int fd, Event *e)
{
    struct input_event in;

    (void) memset(&in, 0, sizeof in);
    in.type = e->type;
    in.code = e->code;
    in.value = e->val;

    (void) write(fd, &in, sizeof in);
}

static int setup_uinput(int *fd)
{
    struct uinput_user_dev dev;
    
    *fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(*fd < 0)
        return 1;

    (void) ioctl(*fd, UI_SET_EVBIT, EV_KEY);
    (void) ioctl(*fd, UI_SET_EVBIT, EV_ABS);

    (void) ioctl(*fd, UI_SET_KEYBIT, BTN_A);
    (void) ioctl(*fd, UI_SET_KEYBIT, BTN_B);
    (void) ioctl(*fd, UI_SET_KEYBIT, BTN_X);
    (void) ioctl(*fd, UI_SET_KEYBIT, BTN_Y);

    (void) ioctl(*fd, UI_SET_ABSBIT, ABS_X);
    (void) ioctl(*fd, UI_SET_ABSBIT, ABS_Y);

    (void) ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0X);
    (void) ioctl(*fd, UI_SET_ABSBIT, ABS_HAT0Y);

    (void) memset(&dev, 0, sizeof dev);
    (void) snprintf(dev.name, UINPUT_MAX_NAME_SIZE, "Net Gamepad");

    dev.absmin[ABS_X] = -32768;
    dev.absmax[ABS_X] = 32767;
    dev.absmin[ABS_Y] = -32768;
    dev.absmax[ABS_Y] = 32767;

    dev.absmin[ABS_HAT0X] = -1;
    dev.absmax[ABS_HAT0X] = 1;
    dev.absmin[ABS_HAT0Y] = -1;
    dev.absmax[ABS_HAT0Y] = 1;

    (void) write(*fd, &dev, sizeof dev);
    (void) ioctl(*fd, UI_DEV_CREATE);

    return 0;
}

static int map_button(uint8_t id)
{
    switch(id)
    {
    case 0:
        return BTN_A;
    case 1:
        return BTN_B;
    case 2:
        return BTN_X;
    case 3:
        return BTN_Y;
    default:
        return BTN_A + id;
    }
}

static int map_axis(uint8_t id)
{
    switch(id)
    {
    case 0:
        return ABS_X;
    case 1:
        return ABS_Y;
    default:
        return ABS_X + id;
    }
}

static void map_hat(int val, int *x, int *y)
{
    *x = 0;
    *y = 0;

    if(val & 0x01)
        *y = -1;
    if(val & 0x04)
        *y =  1;
    if(val & 0x02)
        *x =  1;
    if(val & 0x08)
        *x = -1;
}
