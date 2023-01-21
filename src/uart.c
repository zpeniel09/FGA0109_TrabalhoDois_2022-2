#include <stdio.h>
#include <string.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>   //Used for UART
#include <termios.h> //Used for UART

#include "crc16.h"

int uart0_filestream = -1;

void init_uart(){
    uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY); // Open in non blocking read/write mode
    if (uart0_filestream == -1) {printf("Erro - Não foi possível iniciar a UART.\n");}
    else {printf("UART inicializada!\n");}
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; //<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);
}

void close_uart() {printf("UART finalizada!\n"); close(uart0_filestream);}

int checkCrc(char conteudoRecebido[]){
    short crc = calcula_CRC(conteudoRecebido, 7);
    if ((crc & 0xFF) == conteudoRecebido[7] && ((crc >> 8) & 0xFF) == conteudoRecebido[8])
        return 1;
    return 0;
}

float requestFloat(char protocolo[]){
    unsigned char tx_buffer[20];
    unsigned char *p_tx_buffer;

    p_tx_buffer = &tx_buffer[0];
    *p_tx_buffer++ = protocolo[0];
    *p_tx_buffer++ = protocolo[1];
    *p_tx_buffer++ = protocolo[2];
    *p_tx_buffer++ = protocolo[3];
    *p_tx_buffer++ = protocolo[4];
    *p_tx_buffer++ = protocolo[5];
    *p_tx_buffer++ = protocolo[6];

    short crc = calcula_CRC(&tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
    *p_tx_buffer++ = crc & 0xFF;
    *p_tx_buffer++ = (crc >> 8) & 0xFF;

    if (uart0_filestream != -1){
        int escreve_uart = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
        if (escreve_uart < 0) {printf("UART TX error\n"); return 0;}
    }
    usleep(700000);
    if (uart0_filestream != -1){
        unsigned char rx_buffer[9];
        int rx_length = read(uart0_filestream, rx_buffer, 9);

        short crc = calcula_CRC(rx_buffer, 7);
        if((crc & 0xFF) != rx_buffer[7] && ((crc >> 8) & 0xFF) != rx_buffer[8]) {printf("Erro de CRC\n"); requestFloat(protocolo);}

        if (rx_length < 0) {return 0;}
        else if (rx_length == 0) {return 0;}
        else{
            unsigned char temp[4] = {rx_buffer[3], rx_buffer[4], rx_buffer[5], rx_buffer[6]};
            float temperatura = *((float *)&temp);
            return temperatura;
        }
    }
}

int requestInt(char cmd[]){
    unsigned char tx_buffer[20];
    unsigned char *p_tx_buffer;

    p_tx_buffer = &tx_buffer[0];
    *p_tx_buffer++ = cmd[0];
    *p_tx_buffer++ = cmd[1];
    *p_tx_buffer++ = cmd[2];
    *p_tx_buffer++ = cmd[3];
    *p_tx_buffer++ = cmd[4];
    *p_tx_buffer++ = cmd[5];
    *p_tx_buffer++ = cmd[6];

    short crc = calcula_CRC(&tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));

    *p_tx_buffer++ = crc & 0xFF;
    *p_tx_buffer++ = (crc >> 8) & 0xFF;

    if (uart0_filestream != -1){
        int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
        if (count < 0) {return 0;}
    }
    usleep(700000);

    //----- CHECK FOR ANY RX BYTES -----
    if (uart0_filestream != -1){
        unsigned char rx_buffer[9];
        int rx_length = read(uart0_filestream, &rx_buffer, 9); // Filestream, buffer to store in, number of bytes to read (max)
        int tentativa = 0;

        for (tentativa = 0; tentativa < 5; tentativa++){
            if (checkCrc(rx_buffer)) {break;}
            else {requestInt(cmd);}
        }

        if (tentativa == 5) {printf("Erros ao lidar com CRC"); return -1;}
        if (rx_length <= 0) {return -1;}
        else {int x = rx_buffer[3] + (rx_buffer[4] << 8) + (rx_buffer[5] << 16) + (rx_buffer[6] << 24); return x;}
    }
}

int sendInt(char cmd[], int x){
    unsigned char tx_buffer[50];
    unsigned char *p_tx_buffer;

    p_tx_buffer = &tx_buffer[0];
    *p_tx_buffer++ = cmd[0];
    *p_tx_buffer++ = cmd[1];
    *p_tx_buffer++ = cmd[2];
    *p_tx_buffer++ = cmd[3];
    *p_tx_buffer++ = cmd[4];
    *p_tx_buffer++ = cmd[5];
    *p_tx_buffer++ = cmd[6];

    *p_tx_buffer++ = x & 0xFF;
    *p_tx_buffer++ = (x >> 8) & 0xFF;
    *p_tx_buffer++ = (x >> 16) & 0xFF;
    *p_tx_buffer++ = (x >> 24) & 0xFF;

    short crc = calcula_CRC(&tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));

    *p_tx_buffer++ = crc & 0xFF;
    *p_tx_buffer++ = (crc >> 8) & 0xFF;


    if (uart0_filestream != -1){
        int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
        if (count < 0) {return 0;}
        else {return 1;}
    }
    usleep(700000);
}

int sendSignal(char cmd[]){
    unsigned char tx_buffer[20];
    unsigned char *p_tx_buffer;

    p_tx_buffer = &tx_buffer[0];
    *p_tx_buffer++ = cmd[0];
    *p_tx_buffer++ = cmd[1];
    *p_tx_buffer++ = cmd[2];
    *p_tx_buffer++ = cmd[3];
    *p_tx_buffer++ = cmd[4];
    *p_tx_buffer++ = cmd[5];
    *p_tx_buffer++ = cmd[6];
    *p_tx_buffer++ = cmd[7];

    short crc = calcula_CRC(&tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));

    *p_tx_buffer++ = crc & 0xFF;
    *p_tx_buffer++ = (crc >> 8) & 0xFF;

    if (uart0_filestream != -1){
        int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
        if (count < 0) {return 0;}
    }
    usleep(700000);
    
    //----- CHECK FOR ANY RX BYTES -----
    if (uart0_filestream != -1){
        unsigned char rx_buffer[10];
        int rx_length = read(uart0_filestream, &rx_buffer, 10); // Filestream, buffer to store in, number of bytes to read (max)
        int tentativa = 0;

        for (tentativa = 0; tentativa < 5; tentativa++){
            if (checkCrc(rx_buffer)) {break;}
            else {sendSignal(cmd);}
        }

        if (tentativa == 5) {printf("Erros ao lidar com CRC"); return 0;}
        if (rx_length <= 0) {return 0;}
        else{
            int x = rx_buffer[3] + (rx_buffer[4] << 8) + (rx_buffer[5] << 16) + (rx_buffer[6] << 24);
            if (x==cmd[7])
                return 1;
        }
    }
}

int sendFloat(char cmd[], float f){
    unsigned char tx_buffer[50];
    unsigned char *p_tx_buffer;

    p_tx_buffer = &tx_buffer[0];
    *p_tx_buffer++ = cmd[0];
    *p_tx_buffer++ = cmd[1];
    *p_tx_buffer++ = cmd[2];
    *p_tx_buffer++ = cmd[3];
    *p_tx_buffer++ = cmd[4];
    *p_tx_buffer++ = cmd[5];
    *p_tx_buffer++ = cmd[6];

    *p_tx_buffer++ = *((int *)&f) & 0xFF;
    *p_tx_buffer++ = (*((int *)&f) >> 8) & 0xFF;
    *p_tx_buffer++ = (*((int *)&f) >> 16) & 0xFF;
    *p_tx_buffer++ = (*((int *)&f) >> 24) & 0xFF;

    short crc = calcula_CRC(&tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
    *p_tx_buffer++ = crc & 0xFF;
    *p_tx_buffer++ = (crc >> 8) & 0xFF;

    if (uart0_filestream != -1){
        int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));
        if (count < 0) {return 0;}
        else {return 1;}
    }
    usleep(700000);
}