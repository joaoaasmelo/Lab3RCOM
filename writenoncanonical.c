#include "include/readnoncanonical.h"

void establishment_sender(int *fd) {
    const char FLAG = SET_FLAG;
    const char A = A_TX;
    char C = C_SET;
    const char BCC1 = A ^ C;
    
    char buf[SET_FRAME_SIZE];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    int res = write(*fd, buf, 5);
    printf("%d bytes written (SET)\n\n", res);
    
    return; // Termina a função
}

void establishment_receive(int *fd) {
    const char FLAG = SET_FLAG;
    const char A = A_RX;
    char C = C_UA;
    const char BCC1 = A ^ C;
    
    state_t maqstate = START;
    char buf[BUF_RCV_SIZE]; // Tamanho do array buf ajustado para 2, se der erro mudar para 256

    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        buf[res] = '\0';

        switch (maqstate) {
               case START:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG = %x\n", buf[0]);
                }
                break;
            case FLAG_RCV:
                if (buf[READ_IDX] == A) {
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case A_RCV:
                if (buf[READ_IDX] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case C_RCV:
                if (buf[READ_IDX] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case BCC_OK:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = STOP;
                    printf("FLAG = %x\n", buf[0]);
                } else {
                    maqstate = START;
                }
                break;
        }
    }
    
    printf("--------------UA--------------\n\n");
    return; // Termina a função
}

void data_transfer_sender(int *fd, int Ns){

    const char FLAG = SET_FLAG;
    const char A = A_TX;
    char C = C_DEFAULT;
    switch(Ns){
        case 0:
            C = C_I0;
            break;
        case 1:
            C = C_I1;
            break;
    }
    const char BCC1 = A ^ C;
    char BCC2 = BCC_DEFAULT;

    unsigned char DATA[] = {0x01, 0x02, 0x03, 0x04, 0x05};

    char buf[6 + sizeof(DATA)];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;

    int esc_count = 0;//adicionar +n ao tamanho do array casi haja n escapes

    for (int i = 0; i < sizeof(DATA); i++) {
        if(DATA[i] == FLAG || DATA[i] == ESCAPE_CHAR) { //0x5c ou 0x5d -> 0x5d 0x7c
            buf[4+i] = ESCAPE_CHAR;
            buf[5+i] = DATA[i] ^ ESCAPE_XOR;
            esc_count++;
        } else {
            buf[4+i] = DATA[i];
        }
    }
    
    buf[4 + esc_count + sizeof(DATA)] = BCC2;
    buf[5 + esc_count + sizeof(DATA)] = FLAG;

    int res = write(*fd, buf, 6 + sizeof(DATA));
    if(Ns == 0) printf("%d bytes written (I0)\n\n", res);
    else        printf("%d bytes written (I1)\n\n", res);
    return; // Termina a função

}

void data_transfer_receive(int *fd, int Nr) {

    unsigned char FLAG = SET_FLAG;
    unsigned char A = A_RX;
    unsigned char C = C_DEFAULT;
    switch(Nr){
        case 0:
            C = C_RR0;
            break;
        case 1:
            C = C_RR1;
            break;
    }

    const char BCC1 = A ^ C;
    char BCC2 = BCC_DEFAULT;

    state_t maqstate = START;
    char buf[BUF_RCV_SIZE]; // Tamanho do array buf ajustado para 2, se der erro mudar para 256

    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        buf[res] = '\0';

        switch (maqstate) {
               case START:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG = %x\n", buf[0]);
                }
                break;
            case FLAG_RCV:
                if (buf[READ_IDX] == A) {
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case A_RCV:
                if (buf[READ_IDX] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case C_RCV:
                if (buf[READ_IDX] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case BCC_OK:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = STOP;
                    printf("FLAG = %x\n", buf[0]);
                } else {
                    maqstate = START;
                }
                break;
        }
    }
    
    if(Nr == 0) printf("--------------RR0--------------\n\n");
    else printf("--------------RR1--------------\n\n");

    return; // Termina a função
}

void termination_sender(int *fd, int fl) {
    const char FLAG = SET_FLAG;
    const char A = A_TX;
    char C = C_DEFAULT;
    switch(fl){
        case 0:
            C = C_DISC;
            break;
        case 1:
            C = C_UA;
            break;
    }
    const char BCC1 = A ^ C;
    
    char buf[SET_FRAME_SIZE];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    int res = write(*fd, buf, 5);
    if(fl == 0) printf("%d bytes written (DISC)\n\n", res);
    else printf("%d bytes written (UA)\n\n", res);
    
    return; // Termina a função
}

void termination_receive(int *fd, int fl) {
    unsigned char FLAG = SET_FLAG;
    unsigned char A = A_RX;
    unsigned C;
    switch(fl){
        case 0:
            C = C_DISC;
            break;
        case 1:
            C = C_UA;
            break;
    }
    const char BCC1 = A ^ C;
    
    state_t maqstate = START;
    char buf[BUF_RCV_SIZE]; // Tamanho do array buf ajustado para 2, se der erro mudar para 256

    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        

        switch (maqstate) {
               case START:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG = %x\n", buf[0]);
                }
                break;
            case FLAG_RCV:
                if (buf[READ_IDX] == A) {
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case A_RCV:
                if (buf[READ_IDX] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case C_RCV:
                if (buf[READ_IDX] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[0]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            case BCC_OK:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = STOP;
                    printf("FLAG = %x\n", buf[0]);
                } else {
                    maqstate = START;
                }
                break;
        }
    }
    
    if(fl == 0) printf("--------------DISC------------\n\n");
    else printf("--------------UA--------------\n\n");
    
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

    establishment_sender(&fd);

    establishment_receive(&fd);

    printf("--------------------------------\n");
    printf("------Establishment done!-------\n");
    printf("--------------------------------\n");

    int Ns = 0;
    data_transfer_sender(&fd, Ns);

    int Nr = 1;
    data_transfer_receive(&fd, Nr);

    /*Ns = 1;
    data_transfer_sender(&fd, Ns);

    Nr = 0;
    data_transfer_receive(&fd, Nr);*/

    //DISC == 0 e UA == 1
    termination_sender(&fd, 0);

    termination_receive(&fd, 0);

    termination_sender(&fd, 1);

    printf("--------------------------------\n");
    printf("-------Termination done!--------\n");
    printf("--------------------------------\n");

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
