#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

//Framing constants
#define FRAME_SIZE 5
#define A_SET 0x03
#define A_UA 0x01
#define FLAG 0x7E
#define ESC 0x7D
#define ESCE 0x5E
#define ESCD 0x5D
#define TR 0x03
#define REC 0x01
#define IFCTRL_ON 0x40
#define IFCTRL_OFF 0x00

//Control packet
#define C_START 2
#define C_END 3
#define C_FILE_SIZE 0
#define C_FILE_NAME 1
#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81
#define C_DISC 0x0B

//Data Packet
#define DATA_PACKET 0x01

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC1_RCV,
    STOP
} state_t;

#define FALSE 0
#define TRUE 1