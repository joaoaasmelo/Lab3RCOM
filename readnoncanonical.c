#include "include/readnoncanonical.h"


void establishment_receive(int *fd) {
    const char FLAG = SET_FLAG;
    const char A = A_TX;
    char C = C_SET;
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
    
    printf("--------------SET--------------\n\n");
    return; // Termina a função
}

void establishment_sender(int *fd) {
    const char FLAG = SET_FLAG;
    const char A = A_RX;
    char C = C_UA;
    const char BCC1 = A ^ C;
    
    char buf[SET_FRAME_SIZE];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;

    int res = write(*fd, buf, 5);
    printf("%d bytes written (UA)\n\n", res);
    
    return; // Termina a função
}

void data_transfer_receive(int *fd, int Ns) {
    unsigned char FLAG = SET_FLAG;
    unsigned char A = A_TX;
    unsigned char C = C_DEFAULT;
    switch(Ns){
        case 0:
            C = C_I0;
            break;
        case 1:
            C = C_I1;
            break;
    }
    
    unsigned char BCC1 = A ^ C;
    unsigned char BCC2 = BCC_DEFAULT;
    unsigned char data[BUF_DATA_SIZE];
    int i = 0;

    state_t maqstate = START;
    unsigned char buf[BUF_RCV_SIZE];
    
    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        if (res <= 0) continue;
       
        switch (maqstate) {
            case START:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG = %x\n", buf[READ_IDX]);
                }
                break;
            case FLAG_RCV:
                if (buf[READ_IDX] == A) {
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                }else maqstate = START;
                break;
            case A_RCV:
                if (buf[READ_IDX] == C) {
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                }else maqstate = START;
                break;
            case C_RCV:
                if (buf[READ_IDX] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                }else maqstate = START;
                break;
            case BCC_OK:
                if (buf[READ_IDX] == FLAG) {
                    maqstate = START;
                } else {
                    maqstate = DATA;
                    i = 0;
                    data[i] = buf[0];
                }
                break;
            case DATA:
                if (buf[READ_IDX] == FLAG) {
                    if (BCC2 == data[i]) {
                        maqstate = STOP;
                        printf("BCC2 = %x\n", data[i]);
                        printf("FLAG = %x\n", buf[0]);
                    } else {
                        maqstate = START;
                    }
                } else {
                    BCC2 ^= data[i];
                    data[i+1] = buf[READ_IDX];
                    printf("DATA %d = %x\n", i+1, data[i]);
                    i++;
                }
                break;
        }
    }

    if(Ns == 0) printf("--------------I0--------------\n\n");
    else printf("--------------I1--------------\n\n");
    return; // Termina a função
}

void data_transfer_sender(int *fd, int Nr){

    const char FLAG = SET_FLAG;
    const char A = A_RX;
    char C = C_DEFAULT;
    switch(Nr){
        case 0:
            C = C_RR0;
            break;
        case 1:
            C = C_RR1;
            break;
    }
    const char BCC1 = A ^ C;

    char buf[SET_FRAME_SIZE];

    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC1;
    buf[4] = FLAG;
    
    int res = write(*fd,buf,5);
    if(Nr == 0) printf("%d bytes written (RR0)\n\n", res);
    else printf("%d bytes written (RR1)\n\n", res);
    printf("%d bytes written\n", res);

}

void termination_receive(int *fd, int fl) {
    unsigned char FLAG = SET_FLAG;
    unsigned char A = A_TX;
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

void termination_sender(int *fd, int fl) {
    const char FLAG = SET_FLAG;
    const char A = A_RX;
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
    printf("%d bytes written\n", res);
    
    if(fl == 0) printf("%d bytes written (DISC)\n\n", res);
    else printf("%d bytes written (UA)\n\n", res);
    
    return; // Termina a função
}

void receiver(int *fd) {
    unsigned char FLAG = SET_FLAG;
    unsigned char A = A_DEFAULT;
    unsigned char C = C_DEFAULT;
    unsigned char BCC1 = 0;
    unsigned char BCC2 = 0;  // Inicialização do BCC2
    unsigned char data[BUF_DATA_SIZE];
    int i = 0;

    state_t maqstate = START;
    unsigned char buf[BUF_RCV_SIZE];
    
    while (maqstate != STOP) {
        int res = read(*fd, buf, 1);
        if (res <= 0) continue;
       
        switch (maqstate) {
            case START:
                if (buf[READ_IDX] == SET_FLAG) {
                    maqstate = FLAG_RCV;
                    printf("FLAG = %x\n", buf[READ_IDX]);
                }
                break;

            case FLAG_RCV:
                if (buf[READ_IDX] == A_TX || buf[READ_IDX] == A_RX) {
                    A = buf[READ_IDX];
                    maqstate = A_RCV;
                    printf("A = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;

            case A_RCV:
                if (buf[READ_IDX] == C_UA || buf[READ_IDX] == C_SET || buf[READ_IDX] == C_DISC) {
                    C = buf[READ_IDX];
                    BCC1 = A ^ C;
                    maqstate = C_RCV;
                    printf("C = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == I0 || buf[READ_IDX] == I1) {
                    C = buf[READ_IDX];
                    BCC1 = A ^ C;
                    maqstate = C_INF;
                    printf("C = %x\n", buf[READ_IDX]);
                } else if(buf[READ_IDX] == C_REJ0 || buf[READ_IDX] == C_REJ1) {
                    maqstate = REJ;
                    printf("Negative acknowledgement (REJ)\n");
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV; 
                }else {
                    maqstate = START;
                }
                break;

            case C_RCV:
                if (buf[READ_IDX] == BCC1) {
                    maqstate = BCC_OK;
                    printf("BCC1 = %x\n", buf[READ_IDX]);
                } else if (buf[READ_IDX] == FLAG) {
                    maqstate = FLAG_RCV;
                } else {
                    maqstate = START;
                }
                break;
            
            case BCC_OK:
                if(C == C_I0 || C == C_I1) {
                    maqstate = DATA;
                    data[i] = buf[READ_IDX];
                    BCC2 ^= data[i];
                    printf("DATA %d = %x\n", i, data[i]);
                    i++;
                } else if(C == C_DISC || C == C_UA || C == C_SET) {
                    maqstate = STOP;
                    printf("FLAG = %x\n", buf[READ_IDX]);
                } else {
                    maqstate = START;
                }
                break;
            case DATA:
                if (buf[READ_IDX] == FLAG) {
                    if (BCC2 == data[i]) {
                        maqstate = STOP;
                        printf("BCC2 = %x\n", BCC2);
                        printf("FLAG = %x\n", buf[READ_IDX]);
                    } else {
                        maqstate = START;
                    }
                } else if (buf[READ_IDX] == ESCAPE_CHAR) {
                    maqstate = ESCAPE;
                } else {
                    BCC2 ^= data[i];
                    data[i+1] = buf[READ_IDX];
                    printf("DATA %d = %x\n", i+1, data[i]);
                    i++;
                }
                break;
            case ESCAPE:
                data[i] = buf[READ_IDX] ^ ESCAPE_XOR;
                BCC2 ^= data[i];
                printf("ESCAPED DATA %d = %x\n", i, data[i]);
                i++;
                maqstate = DATA;
                break;
            case REJ:
                break;      
        }

        printf("--------------I--------------\n\n");
        return;

    }
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

    //establishment
    
    establishment_receive(&fd);

    establishment_sender(&fd);

    printf("--------------------------------\n");
    printf("------Establishment done!-------\n");
    printf("--------------------------------\n");


    //flag = 0 --> S = 0
    int Ns = 0;
    data_transfer_receive(&fd, Ns);

    int Nr = 1;
    data_transfer_sender(&fd, Nr);

    /*Ns = 1;
    data_transfer_receive(&fd, Ns);

    Nr = 0;
    data_transfer_sender(&fd, Nr);*/

    //DISC == 0 e UA == 1
    termination_receive(&fd, 0);

    termination_sender(&fd, 0);

    termination_receive(&fd, 1);

    printf("--------------------------------\n");
    printf("-------Termination done!--------\n");
    printf("--------------------------------\n");

    sleep(1);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
            perror("tcsetattr");
            exit(-1);
        }

    close(fd);
    return 0;
}
