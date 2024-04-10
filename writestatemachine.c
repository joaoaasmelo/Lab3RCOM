/*Non-Canonical Input Processing*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

typedef struct state_machine{
    int state, next_state;
}state_machine_t;


int main(int argc, char** argv)
{
    int fd, c, res, res1;
    struct termios oldtio,newtio;
    char buf[5];
    int i, sum = 0, speed = 0;
    state_machine_t m1;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS5", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
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

    char flag = 0x5c, A = 0x03, SET = 0x08, BCC = A^SET;
    buf[0] = flag;
    buf[1] = A;
    buf[2] = SET;
    buf[3] = BCC;
    buf[4] = flag;

    /*testing*/

    res = write(fd,buf,5);
    printf("%d bytes written\n", res);
    m1.state = 0;

    while (STOP==FALSE) {       /* loop for input */
        res = read(fd, buf, 1);
        buf[res]=0;

        if(m1.state == 0 && buf[0] == flag){ //start
            m1.next_state = 1;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        if(m1.state == 0 && buf[0] != flag){ //start /0
            m1.next_state = 0;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 1 && buf[0] == A){ //FLAG_RCV +1
            m1.next_state = 2;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 1 && buf[0] == flag){ //FLAG_RCV /0
            m1.next_state = 1;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 1 && buf[0] != flag && buf[0] != A){ //FLAG_RCV -1
            m1.next_state = 0;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 2 && buf[0] == SET){ //A_RCV +1
            m1.next_state = 3;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 2 && buf[0] == flag){ //A_RCV /0
            m1.next_state = 1;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 2 && buf[0] != flag && buf[0] != SET){ //A_RCV -1
            m1.next_state = 0;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 3 && buf[0] == BCC){ //C_RCV
            m1.next_state = 4;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 3 && buf[0] == flag){ //C_RCV
            m1.next_state = 1;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 3 && buf[0] != flag && buf[0] != BCC){ //C_RCV
            m1.next_state = 0;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 4 && buf[0] == flag){ //BCC_OK
            m1.next_state = 5;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 4 && buf[0] != flag){ //BCC_OK
            m1.next_state = 0;
            m1.state = m1.next_state;
            printf("%x\n", buf[0]);
            res = read(fd, buf, 1);
        }
        else if(m1.state == 5){ //STOP
            STOP = TRUE;
            printf("%x\n", buf[0]);
        }
    }


    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */

    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    
    close(fd);
    return 0;
}
