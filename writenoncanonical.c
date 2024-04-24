#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
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

void establishment(int *fd, int fl) {
    const char FLAG = 0x5c;
    const char A = 0x01; //ja ta mudado par o UA
    char C = 0x00;

    switch (fl){
        case 0:
            C = 0x06;
            break;
        case 1:
            C = 0x11;
            break;
        case 2:
            C = 0x01;
            break;
    }

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
                    printf("FLAG = %x\n", buf[0]);
                }
                break;
            case FLAG_RCV:
                if (buf[0] == A) {
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case A_RCV:
                if (buf[0] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case C_RCV:
                if (buf[0] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case BCC_OK:
                if (buf[0] == FLAG) {
                    maqstate = STOP;
                    printf("FLAG = %x\n", buf[0]);
                } else {
                    maqstate = START;
                }
                break;
        }
    }
    printf("STOP\n");
    return; // Termina a função
}

void data_transfer(int *fd) {
    const char FLAG = 0x5c;
    const char A = 0x03;
    const char C = 0x08;
    const char BCC1 = A ^ C;
    char BCC2 = 0x00;
    const char DATA[5] = {0x01, 0x02, 0x03, 0x04, 0x05};

    char buf[11]; // Tamanho do array buf ajustado para 2, se der erro mudar para 256

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;

    for (int i = 0; i < 5; i++) {
        buf[4+i] = DATA[i];
        BCC2 ^= DATA[i];
    }

    buf[9] = BCC2;
    buf[10] = FLAG;

    write(*fd, buf, 11);

    return; // Termina a função
}

int main(int argc, char** argv)
{
    int fd, c, res, fl;
    struct termios oldtio,newtio;
    char buf[5];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS5", argv[1])!=0) &&
          (strcmp("/dev/ttyS10", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS5\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    char FLAG = 0x5c, A = 0x03, C = 0x08, BCC1 = A^C;

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    res = write(fd,buf,5);
    printf("%d bytes written\n", res);

    fl=0;//conexão
    establishment(&fd, fl);

    data_transfer(&fd);

    fl=1;//RR=1 
    establishment(&fd, fl);

    fl=2;//RR=0
    establishment(&fd, fl);

    sleep(1);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
