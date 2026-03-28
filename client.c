#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <SDL2/SDL.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "global.h"

static int parseEvent(SDL_Event *, Packet *);
static int getNum(char *, size_t);
static int pollEvent(int);

int main(int argc, char *argv[])
{
    uint16_t port;
    int running;
    int i;
    int sock;
    int numJoy;
    int padNum;
    char num[5];
    struct sockaddr_in addr;
    SDL_Joystick *joy;
#ifdef _WIN32
    WSADATA wsa;
#endif

    if(argc != 3)
    {
        (void) fprintf(stderr,
                       "Usage: %s "
                       "<server ip> "
                       "<server port>\n",
                       argv[0]);
        return 1;
    }

    (void) memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    if(parsePort(argv[2]))
    {
        (void) fprintf(stderr, "Invalid port.\n");
        return 1;
    }
    (void) sscanf(argv[2], "%hu", &port);
    if(!port)
    {
        (void) fprintf(stderr, "Port out of range.\n");
        return 1;
    }
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if(addr.sin_addr.s_addr == INADDR_NONE)
    {
        (void) fprintf(stderr, "Invalid IP address.\n");
        return 1;
    }

    if(SDL_Init(SDL_INIT_GAMECONTROLLER) != 0)
    {
        (void) fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    numJoy = SDL_NumJoysticks();
    if(numJoy == 0)
    {
        (void) fprintf(stderr, "No controller found.\n");
        return 1;
    }
    for(i = 0; i < numJoy; i++)
    {
        joy = SDL_JoystickOpen(i);
        (void) printf("%d %s\n", i, SDL_JoystickName(joy));
        SDL_JoystickClose(joy);
    }
    switch(getNum(num, sizeof num / sizeof *num))
    {
    case 1:
        return 1;
    case 2:
	fprintf(stderr, "Must be a number.\n");
	return 1;
    case 3:
	fprintf(stderr, "Too many characters.\n");
	return 1;
    }
    (void) sscanf(num, "%d", &padNum);
    joy = SDL_JoystickOpen(padNum);
    if(joy == NULL)
    {
        (void) fprintf(stderr, "Can't open controller.\n");
        return 1;
    }

#ifdef _WIN32
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        (void) fprintf(stderr, "Error creating socket.\n");
        return 1;
    }
    if(connect(sock, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        (void) fprintf(stderr, "Error establishing connection.\n");
        return 1;
    }

    running = 1;
    while(running)
        running = pollEvent(sock);
    SDL_Quit();

#ifdef _WIN32
    (void) closesocket(sock);
    (void) WSACleanup();
#else
    (void) close(sock);
#endif

    return 0;
}

static int pollEvent(int sock)
{
    int running;
    SDL_Event e;

    running = 1;
    if(SDL_PollEvent(&e))
    {
        Packet p;

        (void) memset(&p, 0, sizeof p);
        running = parseEvent(&e, &p);
        if(p.type > 0)
            (void) send(sock, (const char *) &p, sizeof p, 0);
    }
    return running;
}

static int parseEvent(SDL_Event *e, Packet *p)
{
    int running;

    running = 1;
    switch(e->type)
    {
    case SDL_QUIT:
        running = 0;
        break;
    case SDL_JOYHATMOTION:
        p->type = EVT_HAT;
        p->id = e->jhat.hat;
        p->value = e->jhat.value;
        p->time = e->jhat.timestamp;
        break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        p->type = EVT_BUTTON;
        p->id = e->jbutton.button;
        p->value = e->jbutton.state;
        p->time = e->jbutton.timestamp;
        break;
    case SDL_JOYAXISMOTION:
        p->type = EVT_AXIS;
        p->id = e->jaxis.axis;
        p->value = e->jaxis.value;
        p->time = e->jaxis.timestamp;
        break;
    default:
        p->type = -1;
    }
    return running;
}

static int getNum(char *buf, size_t size)
{
    int c;
    char *sbuf;

    sbuf = buf;
    while((c = getchar()) != '\n')
    {
        if(c == EOF)
            return 1;
        if(!isdigit(c))
            return 2;
        *buf++ = c;
        if(buf - sbuf >= size)
	    return 3;
    }
    *buf = '\0';
    return 0;
}
