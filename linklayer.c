// Link layer protocol implementation

#include "include/linklayer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
int alarmFlag = FALSE, alarmCounter = 0, timeout = 0, retrans_data_counter = 0, ns = 0, numTries = 1;
int check = TRUE, status_llwrite = 0, status_llread = 0, status_llclose = 0, fd;
int valid = TRUE, nr = 1;
linkLayer connection;
struct termios oldtio, newtio;
LinkLayerState state = START; // Initial state


void stateMachineCheck(LinkLayerState* status, unsigned char byte, int type)
{
    unsigned char save[2];
    if(type == TRANSMITTER)
    {
        switch (*status)
        {
        case START:
            if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;
        
        case FLAG_RCV:
            if(byte == TR || byte == REC) {
                
                save[0] = byte;
                *status = A_RCV;
            }
            else if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;

        case A_RCV:
            if((byte == C_UA) || (byte == C_SET) || (byte == C_DISC) || (byte == C_RR0) || (byte == C_RR1)) { 
                
                save[1] = byte;
                *status = C_RCV;
            }else if( (byte == C_REJ0) || (byte == C_REJ1) ){
                printf("Negative acknowledgement (REJ)\n");
                save[1] = byte;
                *status = REJ;
            }
            else if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else if((byte == C_I0) || (byte == C_I1)) {
                save[1] = byte;
                *status = C_INF;
            }
            else {
                *status = START;
            }
            break;

        case C_RCV:
            if(byte == (save[0] ^ save[1])) {
                *status = BCC1_RCV;
                break;
            }
            else if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;
        case REJ:
            break;
        case BCC1_RCV:
            if(byte == FLAG) {
                *status = STOP;
            }
            else {
                *status = START;
            }
            break;
        
        case STOP:
            break;

        default:
            break;
        }

    }
    else if(type == RECEIVER)
    {
        switch (*status)
        {
        case START:
            if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;
        
        case FLAG_RCV:
            if(byte == TR || byte == REC) { 
                save[0] = byte;
                *status = A_RCV;
            }
            else if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;

        case A_RCV:
            if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else if((byte == C_I0) || (byte == C_I1)) {
                save[1] = byte;
                *status = C_INF;
            }
            else {
                *status = START;
            }
            break;

        case C_INF:
            if(byte == (save[0]^save[1])) { //se  A^C = BBC ?
                *status = BCC1_RCV;
            }
            else if(byte == FLAG) {
                *status = FLAG_RCV;
            }
            else {
                *status = START;
            }
            break;

        case BCC1_RCV:
            if(byte == FLAG) {
                *status = STOP;
            }
            break;
        
        case STOP:
            break;

        default:
            break;
        }

    }

}

int sendSet()
{
    unsigned char buf[BUF_SIZE + 1] = {0};
    buf[0] = FLAG;
    buf[4] = FLAG;
    buf[1] = TR;
    buf[2] = C_SET;
    buf[3] = buf[1] ^ buf[2];
    (void)signal(SIGALRM, alarmHandler);

    while (state != STOP && alarmCounter < (connection.numTries + 1))
    {
        if (!alarmFlag)
        {
            printf("SET sended to establish connection.\n");

            state = START;
            write(fd, buf, 5);
            alarm(connection.timeOut); 
            alarmFlag = TRUE;
        }

        if (read(fd, buf, 1) > 0) {
            stateMachineCheck(&state,buf[0],0);
            if(state == STOP)
            {
                printf("####################################\n");
                printf("UA received, connection established.\n");
                printf("####################################\n");

                break;
            }
        }
    }
    return alarmCounter;
}

int sendUAreadSet()
{
    unsigned char buffer[BUF_SIZE +1] = {0};
    while(TRUE)
    {
        int bytes = read(fd,buffer,1);
        stateMachineCheck(&state, buffer[0], 0);
        if(state == STOP)
        {

            printf("#################################################\n");
            printf("SET received to establish connection, sending UA.\n");
            printf("#################################################\n");
            break;
        }
    }
    buffer[0] = FLAG;
    buffer[1] = A_UA;
    buffer[2] = C_UA;
    buffer[3] = buffer[1] ^ buffer[2];
    buffer[4] = FLAG;
    int bytes = write(fd,buffer,5);
    return 1;
}

void alarmHandler(int signal) {
    printf("Alarm counter timeout number: %d\n", alarmCounter);// Increment the alarm counter to keep track of alarms.
    alarmFlag = FALSE;   // Set the alarm flag to TRUE indicating the alarm has occurred.
    alarmCounter++;
}

int resetAlarm() {
    alarmCounter = 0;
    alarmFlag = FALSE;

}

int llopen(linkLayer connectionParameters)
{
    connection = connectionParameters;
    fd = open(connection.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connection.serialPort);
        exit(-1);
    }


    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connection.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0.1;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    unsigned char byte; 
    timeout = connection.timeOut;
    retrans_data_counter = connection.numTries;
    switch(connection.role) { 
        case TRANSMITTER:{
            sendSet();
            break;
        }
        case RECEIVER:{
            sendUAreadSet();
            break;
        }
        default: 
            return -1; 
            break; 
    }  
return 1;
}

int llwrite(unsigned char *buf, int bufSize)
{
   unsigned char trama_info[2*MAX_PAYLOAD_SIZE];

    trama_info[0] = FLAG;
    trama_info[1] = A_SET;
    trama_info[2] = (ns == 0) ? C_I0 : C_I1;
    trama_info[3] = trama_info[1] ^ trama_info[2]; 

    int trama_info_size = 0; 
    int bcc_2 = 0; 

    for(int j = 0; j < bufSize; j++)
    { 
        bcc_2 ^= buf[j];
    }

    for(int i = 0; i < bufSize; i++) {
        if(buf[i] == FLAG) {
            trama_info[4+trama_info_size] = ESC;
            trama_info[5+trama_info_size] = ESCE;
            trama_info_size+= 2;
        }
        else if(buf[i] == ESC) {
            trama_info[4+trama_info_size] = ESC;
            trama_info[5+trama_info_size] = ESCD;
            trama_info_size+= 2;
        }
        else {
            trama_info[4+trama_info_size] = buf[i];
            trama_info_size++;
        }
    }
    if(bcc_2 == FLAG) {
        trama_info[4+trama_info_size] = ESC;
        trama_info[5+trama_info_size] = ESCE;
        trama_info[6+trama_info_size] = FLAG;
        trama_info_size+= 1;
    }
    else if(bcc_2 == ESC) {
        trama_info[4+trama_info_size] = ESC;
        trama_info[5+trama_info_size] = ESCD;
        trama_info[6+trama_info_size] = FLAG;
        trama_info_size+= 1;
    }
    else {
        trama_info[4+trama_info_size] = bcc_2;
        trama_info[5+trama_info_size] = FLAG;
    }

    trama_info_size += 6;

    unsigned char buf_aux[BUF_SIZE+1] = {0};

    (void)signal(SIGALRM,alarmHandler);    
    while(status_llwrite != STOP && alarmCounter < (connection.numTries + 1)) 
    {
        if(!alarmFlag || status_llwrite == REJ)
        {
            if(status_llwrite == REJ) printf("REJ received, resending packet.\n");
            status_llwrite = START; 
            fprintf(stderr,"Sent Packet: NS=%d\n",ns);
            int byte_written = write(fd,trama_info,trama_info_size);
            alarm(connection.timeOut);
            alarmFlag = TRUE;
        }

        int k = 0; 

        if(read(fd,buf_aux + k,1) > 0)
        {
            fprintf(stderr,"State: %d", status_llwrite);
                
            stateMachineCheck(&status_llwrite, buf_aux[k], 0);
            if(status_llwrite == REJ)
            {
                printf("Buf: %02x Entrei no REJ\n", buf_aux[k]);
                read(fd,buf_aux + k,1);
                k++;
                read(fd,buf_aux + k,1);
                k++;    
            }
            fprintf(stderr,"Buf: %02x \n", buf_aux[k]);
            k++;
            if(status_llwrite == STOP)
            {
                printf("STOP achieved\n");
                ns = (1+ns) % 2; 
                break;
            }
        }    
    }

        status_llwrite = START;
        alarmFlag = FALSE;
        if(alarmCounter >= (connection.numTries + 1))
        {
            alarmCounter = 0; 
            return -1;
        }

    return trama_info_size; 

}

int writeRepPacket() {
    unsigned char out[6];

    // Set the beginning and ending flag bytes.
    out[0] = FLAG;
    out[4] = FLAG;

    // Define the packet type (command).
    out[1] = TR;

    // Determine the REJ type based on the frame number (nr).
    if (nr) 
    {
        out[2] = C_REJ1; // If nr = 1, use C_REJ1.
        
    }        
    else {
        out[2] = C_REJ0; // If nr = 0, use C_REJ0.
    }
    
    // Calculate and set the BCC (Block Check Character) field.
    out[3] = out[1] ^ out[2];

    // Send the constructed REJ packet to the serial port.
    write(fd, out, 5);

    return 5; // The size of the sent REJ packet is always 5 bytes.
}

int llread(unsigned char * packet) {

    int size, bcc_cal, bcc_2;
    unsigned char buf[2 * MAX_PAYLOAD_SIZE];
    unsigned char initialPack[MAX_PAYLOAD_SIZE]; // Buffer for storing the initial packet data.
    unsigned char outbuf[6]; // Buffer for constructing response frame.
    
    while (numTries <= retrans_data_counter) {
        // Reset the variables for each new attempt
        size = 0;
        bcc_cal = 0;

        status_llread = START;
        memset(packet, 0, sizeof(packet)); // buffer with the data for the application layer
        memset(buf, 0, sizeof(buf)); // buffer with the data from the serial port
        memset(initialPack, 0, sizeof(initialPack)); // buffer with the data from the serial port
        memset(outbuf, 0, sizeof(outbuf)); // Initialize the response frame buffer.

        while (1) {
            if (read(fd, buf + size, 1) > 0) {
                if (size + 1 > MAX_PAYLOAD_SIZE * 2) {
                    // If the flag couldn't be found in the end packet, send a REJ packet.
                    printf("The flag couldn't be found in the end of the packet\n");
                    writeRepPacket();
                    return -1;
                }
                stateMachineCheck(&status_llread, buf[size], 1); // Process the received data.
                size++;
                if (status_llread == STOP) {
                    printf("Received complete frame\n");
                    break;
                }
            }
        }
        status_llread = START;

        bcc_2 = 0;
        if (buf[size - 3] == ESC && buf[size - 2] == ESCD) {
            bcc_2 = ESC;
            size--;
        } else if (buf[size - 3] == ESC && buf[size - 2] == ESCE) {
            bcc_2 = FLAG;
            size--;
        } else {
            bcc_2 = buf[size - 2];
        }

        //puts ONLY the data in the initialPack buffer
        for (int l = 0; l < size - 6; l++) {
            initialPack[l] = buf[4 + l]; //data begins at the 5th byte
        }

        // Byte destuffing
        int j = 0;
        for(int i = 0; i < size - 6; i++) {
            if(initialPack[i] == ESC && initialPack[i + 1] == ESCE) {
                packet[j] = FLAG; j++;
                i++;
            } else if(initialPack[i] == ESC && initialPack[i + 1] == ESCD) {
                packet[j] = ESC; j++;
                i++;
            } else {packet[j] = initialPack[i];j++;}
        }

        for (int i = 0; i < j; i++) bcc_cal ^= packet[i];

        outbuf[0] = FLAG;
        outbuf[1] = TR;
        outbuf[4] = FLAG;

        // Check if the BCC is correct
        if(bcc_cal == bcc_2){
            nr = (nr + 1) % 2;
            if(nr)  outbuf[2] = C_RR1;
            else    outbuf[2] = C_RR0;

            outbuf[3] = outbuf[1] ^ outbuf[2];
            if(write(fd, outbuf, 5) < 0) {
                printf("Error sending RR.\n");
                return -1;
            }
            printf("Acknowledgement frame sent.\n");

            return j;// Return the size of the received packet.
            
        }else if(bcc_cal != bcc_2){
            if(nr) outbuf[2] = C_REJ1;
            else   outbuf[2] = C_REJ0;

            printf("Bcc_cal: %02x and Bcc_2: %02x\n", bcc_cal, bcc_2);

            printf("Bad packet detected.\n");
            outbuf[3] = outbuf[1] ^ outbuf[2];
            write(fd, outbuf, 5);

            printf("%dº REJ sent.\n", numTries);
            numTries++;
        }

    }
    // If the loop ends, it means the maximum number of retries was reached without success
    return -1; // Indicate failure
}

sendDiscCommand()
{
    unsigned char buf[BUF_SIZE + 1] = {0};
    buf[0] = FLAG;
    buf[1] = TR;
    buf[2] = C_DISC;
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;
    (void)signal(SIGALRM,alarmHandler);
    while(status_llclose != STOP && alarmCounter < (connection.numTries + 1))
    {
        if(!alarmFlag)
        {
            printf("DISC sended to close connection.\n");
            status_llclose = START; 
            write(fd,buf,5);
            alarm(connection.timeOut);
            alarmFlag = TRUE; 
        }
        if(read(fd,buf,1) > 0 )
        {
            stateMachineCheck(&status_llclose, buf[0], 0);
            if(status_llclose == STOP) 
            {
                printf("Receiver DISC received.\n");
                break;
            }
        }
    }
    return alarmCounter;
}


int llclose(linkLayer connection ,int showStatistics) {
    alarmFlag = FALSE;
    alarmCounter = 0;
    unsigned char buf[BUF_SIZE + 1];

    switch (connection.role) {
        case TRANSMITTER:
            if (sendDiscCommand() == 3) {
                return -1; // Error sending DISC command.
            }

            // Send UA (Unnumbered Acknowledgment).
            buf[0] = FLAG;
            buf[1] = REC;
            buf[2] = C_UA;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;
            write(fd, buf, 5);

            printf("######################################\n");
            printf("UA sended, connection is being closed.\n");
            printf("######################################\n");
            break;

        case RECEIVER:
            // Receive DISC command.
            while (1) {
                int bytes = read(fd, buf2, 1);
                stateMachineCheck(&status_llclose, buf2[0], 0);
                if (status_llclose == STOP) {
                    printf("Transmitter DISC received.\n");
                    break;
                }
            }

            // Send DISC response.
            buf[0] = FLAG;
            buf[1] = REC;
            buf[2] = C_DISC;
            buf[3] = buf[1] ^ buf[2];
            buf[4] = FLAG;
            int bytes = write(fd, buf2, 5);
            printf("DISC sended to close connection.\n");


            // Receive UA response.
            unsigned char buf2[BUF_SIZE + 1];
            status_llclose = START;
            while (1) {
                int bytes = read(fd, buf2, 1);
                stateMachineCheck(&status_llclose, buf2[0], 0);
                if (status_llclose == STOP) {
                    printf("########################################\n");
                    printf("UA received, connection is being closed.\n");
                    printf("########################################\n");
                    break;
                }
            }
            break;

        default:
            break;
    }

    // Restore the original serial port settings.
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    // Close the serial port.
    close(fd);

    return 1; // Successful closure.
}
