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
    A_RCV, C_RCV, 
    BCC_OK, 
    DATA, 
    BCC2_OK, 
    STOP
} state_t;

void establishment_receive(int *fd) {
    const char FLAG = 0x5c;
    const char A = 0x03;
    char C = 0x08;
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
    
    printf("--------------------------------\n");
    return; // Termina a função
}

void establishment_sender(int *fd) {
    const char FLAG = 0x5c;
    const char A = 0x01;
    char C = 0x06;
    const char BCC1 = A ^ C;
    
    char buf[5];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    int res = write(*fd, buf, 5);
    printf("%d bytes written\n", res);
    
    return; // Termina a função
}

void data_transfer_receive(int *fd, int fl) {
    unsigned char FLAG = 0x5c;
    unsigned char A = 0x03;
    unsigned char C = 0x00;
    switch(fl){
        case 0:
            C = 0x80;
            break;
        case 1:
            C = 0xc0;
            break;
    }
    printf("Do bom C: %x\n", C);
    unsigned char BCC1 = A ^ C;
    unsigned char BCC2 = 0x00;
    unsigned char data[1024];
    int i = 0;

    state_t maqstate = START;
    unsigned char buf[1];
    
    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        if (res <= 0) continue;
       
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
                }else maqstate = START;
                break;
            case A_RCV:
                if (buf[0] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                }else maqstate = START;
                break;
            case C_RCV:
                if (buf[0] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[0]);
                } else if (buf[0] == FLAG) {
                    maqstate = FLAG_RCV;
                }else maqstate = START;
                break;
            case BCC_OK:
                if (buf[0] == FLAG) {
                    maqstate = START;
                } else {
                    maqstate = DATA;
                    i = 0;
                    data[i] = buf[0];
                }
                break;
            case DATA:
                if (buf[0] == FLAG) {
                    if (BCC2 == data[i]) {
                        maqstate = STOP;
                        printf("BCC2 = %x\n", data[i]);
                        printf("FLAG = %x\n", buf[0]);
                    } else {
                        maqstate = START;
                    }
                } else {
                    BCC2 ^= data[i];
                    data[i+1] = buf[0];
                    printf("DATA %d = %x\n", i+1, data[i]);
                    i++;
                }
                break;
        }
    }
    printf("Received data!\n");
    printf("--------------------------------\n");
}

void data_transfer_sender(int *fd, int fl){

    const char FLAG = 0x5c;
    const char A = 0x01;
    char C = 0x00;
    switch(fl){
        case 0:
            C = 0x11;
            break;
        case 1:
            C = 0x01;
            break;
    }
    const char BCC1 = A ^ C;

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;
    
    res = write(fd,buf,5);
    printf("%d bytes written\n", res);

}

int main(int argc, char** argv)
{
    int fd, c, res;
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
    
    establishment_receive(&fd);

    establishment_sender(&fd);

    sleep(1);

    //flag = 0 --> S = 0

    data_transfer_receive(&fd, 0);

    data_transfer_sender(&fd, 0);

    //flag = 1 --> S = 1

    sleep(1);

    data_transfer_receive(&fd, 1);

    data_transfer_sender(&fd, 1);

    sleep(1);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
            perror("tcsetattr");
            exit(-1);
        }

    close(fd);
    return 0;
}
