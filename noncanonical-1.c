/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[5];

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
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

    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,5);   /* returns after 5 chars have been input */
        buf[res]=0;               /* so we can printf... */

       if (buf[0] != 0x5c){
                   // printf("%x\n", buf[0]);
                    printf("error in FLAG\n");
                    exit(-1);
                } 
        if (buf[1] != 0x03){
                    printf("error in ADDRESS\n");
                    exit(-1);
                }  
        if (buf[2] != 0x08){
                    printf("error in CONTROL\n");
                    exit(-1);
                }  
         
        if (buf[3] != (buf[2]^buf[1])){
                    printf("error in BCC1\n");
                    exit(-1);
                }  
        printf("message received: \n");        
        printf("FLAG = %x\n",buf[0]);
        printf("A = %x\n",buf[1]);
        printf("C = %x\n",buf[2]);
        printf("BCC = %x\n",buf[3]);
        printf("FLAG= %x\n",buf[4]);    
        printf("\n\n");

        char A = 0x01 , C= 0x06 , BCC = A^C; 
        char UA[5];
        UA[0] = buf[0];
        UA[1] = A;
        UA[2] = C;
        UA[3] = BCC;
        UA[4] = buf[4];
        res = write(fd, UA,5);
        printf("bytes written : %d\n",res);
        printf("message sent: \n");        
        printf("FLAG = %x\n",UA[0]);
        printf("A = %x\n",UA[1]);
        printf("C = %x\n",UA[2]);
        printf("BCC = %x\n",UA[3]);
        printf("FLAG= %x\n",UA[4]);
        //printf(":%x:%d\n", buf, res);
        if (buf[0]=='z') STOP=TRUE;
    }


    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */
    
    
    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
