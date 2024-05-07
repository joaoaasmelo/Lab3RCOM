#ifndef READNONCANONICAL_H
#define READNONCANONICAL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

typedef enum {
    START, 
    FLAG_RCV, 
    A_RCV, 
    C_RCV, 
    C_INF,
    BCC_OK, 
    DATA,
    ESCAPE, 
    REJ,
    BCC2_OK, 
    STOP
} state_t;

#define BUF_RCV_SIZE 2
#define BUF_DATA_SIZE 1024
#define BUF_SEND_SIZE 5
#define SET_FRAME_SIZE 5
#define SET_FLAG 0x5c
#define A_RX 0x03
#define A_TX 0x01

#define READ_IDX 0

#define ESCAPE_CHAR 0x5d
#define ESCAPE_XOR 0x20

#define MAX_RETRIES 3

//Control packet
#define A_DEFAULT 0x00
#define C_DEFAULT 0x00
#define BCC_DEFAULT 0x00

#define C_SET 0x07
#define C_UA 0x06
#define C_RR0 0x01
#define C_RR1 0x11
#define C_REJ0 0x05
#define C_REJ1 0x15
#define C_I0 0x80
#define C_I1 0xc0
#define C_DISC 0x0a

#endif /* READNONCANONICAL_H */