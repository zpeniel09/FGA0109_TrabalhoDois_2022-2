#ifndef _UART_
#define _UART_

void init_uart();
void close_uart();

float requestFloat(char cmd[]);
int requestInt(char cmd[]);
int sendInt(char cmd[], int x);
int sendSignal(char cmd[], int x);
int sendFloat(char cmd[], float x);

#endif