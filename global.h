#define EVT_HAT 1
#define EVT_BUTTON 2
#define EVT_AXIS 3

typedef struct {
    uint8_t type;
    uint8_t id;
    int16_t value;
    uint32_t time;
} Packet;

extern int parsePort(char *);
