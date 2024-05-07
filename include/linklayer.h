#ifndef LINKLAYER
#define LINKLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct linkLayer{
    char serialPort[50];
    int role; //defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
} linkLayer;

typedef enum
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC1_RCV,
    STOP,
    C_INF,
    REJ
} LinkLayerState;

//ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1


//SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

//CONNECTION deafault values
#define BAUDRATE_DEFAULT B38400
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//Framing constants
#define FRAME_SIZE 5
#define A_SET 0x03
#define A_UA 0x01
#define FLAG 0x5c
#define ESC 0x5d
#define ESCE 0x7c
#define ESCD 0x7d
#define TR 0x03
#define REC 0x01
#define C_I1 0x80
#define C_I0 0xc0

//Control packet
#define C_FILE_SIZE 0
#define C_FILE_NAME 1
#define C_SET 0x07
#define C_UA 0x06
#define C_RR0 0x01
#define C_RR1 0x11
#define C_REJ0 0x05
#define C_REJ1 0x15
#define C_DISC 0x0a

//Data Packet
#define DATA_PACKET 0x01

//MISC
#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

void stateMachineCheck(LinkLayerState* status, unsigned char byte, int type);

/**
 * Set up the link layer connection.
 *
 * This function initializes the link layer connection, including opening the
 * serial port, configuring its settings, and performing link establishment.
 *
 * @param connectionParameters The link layer parameters to configure the connection.
 *
 * @return The file descriptor of the opened serial port, or -1 in case of error.
 */
int llopen(linkLayer connectionParameters);

/**
 * Handle the alarm signal.
 *
 * This function is called when the alarm signal is received.
 * It sets the `alarmFlag` to `TRUE` and increments the `alarmCounter`.
 *
 * @param signal The signal received, usually `SIGALRM`.
 */
void alarmHandler(int signal);

/**
 * Write data to the serial port using the Link Layer protocol.
 *
 * This function constructs and sends a frame containing the data to be transmitted.
 *
 * @param fd The file descriptor of the serial port.
 * @param buf A pointer to the data buffer to be transmitted.
 * @param bufSize The size of the data buffer.
 *
 * @return The size of the transmitted frame (chars written).
 */
int llwrite(unsigned char* buf, int bufSize);
/**
 * Send a REJ packet when an error is detected in the received data.
 *
 * This function constructs and sends a REJ (REJection) packet to indicate that an error
 * was detected in the received data. The specific packet format and content are determined
 * based on the value of the `nr` variable, indicating the current frame number.
 *
 * @return The size of the sent REJ packet in bytes (always 5).
 */
int writeRepPacket();

/**
 * Read and process frames from the serial port.
 *
 * This function reads frames from the serial port and processes them to construct the
 * complete data packet. It handles byte stuffing, BCC verification, and frame extraction.
 *
 * @param packet A pointer to the buffer where the constructed data packet will be stored.
 * @return The size of the received and processed data packet in bytes or -1 if an error occurs.
 */
int llread(unsigned char* packet);

/**
 * Perform link layer closing procedures.
 *
 * This function handles link layer closing procedures based on the role of the connection. It can either send a
 * DISC (Disconnect) command and wait for a UA (Unnumbered Acknowledgment) response or receive a DISC command and
 * send a UA response. The function also restores the serial port settings and closes the port.
 *
 * @param showStatistics If set to 1, display statistics; otherwise, set to 0.
 * @return 1 on successful closure, -1 on error.
 */
int llclose(linkLayer connectionParameters, int showStatistics);

#endif
