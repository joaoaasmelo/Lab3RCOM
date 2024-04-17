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
    BCC_OK,
    STOP
} state_t;

void maquina_estados(int *fd) {
    const char FLAG = 0x5c;
    const char A = 0x03;
    const char C = 0x08;
    const char BCC1 = A ^ C;
    
    state_t maqstate = START;
    char buf[2]; // Tamanho do array buf ajustado para 2, se der erro mudar para 256

    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        buf[res] = '\0';

        switch (maqstate) {
            case START:
                if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("START -> FLAG_RCV\n");
                }
                break;
            case FLAG_RCV:
                if (buf[0] == A) {
                    maqstate = A_RCV;
                    printf("FLAG_RCV -> A_RCV\n");
                    printf("FLAG = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG_RCV -> FLAG_RCV\n");
                } else {
                    maqstate = START;
                    printf("FLAG_RCV -> START\n");
                }
                break;
            case A_RCV:
                if (buf[0] == C) {
                    maqstate = C_RCV;
                    printf("A_RCV -> C_RCV\n");
                    printf("A = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("A_RCV -> FLAG_RCV\n");
                } else {
                    maqstate = START;
                    printf("A_RCV -> START\n");
                }
                break;
            case C_RCV:
                if (buf[0] == BCC1) {
                    maqstate = BCC_OK;
                    printf("C_RCV -> BCC_OK\n");
                    printf("C = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("C_RCV -> FLAG_RCV\n");
                } else {
                    maqstate = START;
                    printf("C_RCV -> START\n");
                }
                break;
            case BCC_OK:
                if (buf[0] == FLAG) {
                    maqstate = STOP;
                    printf("BCC_OK -> STOP\n");
                    printf("BCC1 = %x\n", buf[0]);
                } else {
                    maqstate = START;
                    printf("BCC_OK -> START\n");
                }
                break;
        }
    }
    printf("STOP\n");
    return; // Termina a função
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) &&
          (strcmp("/dev/ttyS11", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    
    maquina_estados(&fd);

    char FLAG = 0x5c, A = 0x01, C = 0x06, BCC1 = A^C;

    char buf[5];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    res = write(fd,buf,5);
    printf("%d bytes written\n", res);

   sleep(1);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
