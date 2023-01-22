#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <softPwm.h>
#include <time.h>
#include <wiringPi.h>

#include "uart.h"
#include "pid.h"

unsigned char solicitaTemperaturaInterna[7] = {0x01, 0x23, 0xC1, 1, 3, 0, 8};
unsigned char solicitaTemperaturaDeReferencia[7] = {0x01, 0x23, 0xC2, 1, 3, 0, 8};
unsigned char comandosDoUsuario[7] = {0x01, 0x23, 0xC3, 1, 3, 0, 8};

unsigned char enviaSinalDeControleInt[7] = {0x01, 0x16, 0xD1, 1, 3, 0, 8};
unsigned char enviaEstadoDoSistemaLigado[8] = {0x01, 0x16, 0xD3, 1, 3, 0, 8, 1};
unsigned char enviaEstadoDoSistemaDesligado[8] = {0x01, 0x16, 0xD3, 1, 3, 0, 8, 0};
unsigned char enviaEstadoDeFuncionamentoFuncionando[8] = {0x01, 0x16, 0xD5, 1, 3, 0, 8, 1};
unsigned char enviaEstadoDeFuncionamentoParado[8] = {0x01, 0x16, 0xD5, 1, 3, 0, 8, 0};
unsigned char modoDeControleDaTemperaturaDeReferenciaCurvaAtivado[8] = {0x01, 0x16, 0xD4, 1, 3, 0, 8, 1};
unsigned char modoDeControleDaTemperaturaDeReferenciaCurvaDesativado[8] = {0x01, 0x16, 0xD4, 1, 3, 0, 8, 0};
unsigned char enviaSinalDeReferenciaFloat[7] = {0x01, 0x16, 0xD2, 1, 3, 0, 8};

int curva, algoritmoIniciado, sistemaLigado, count;
float TempReferencia = 0.0;

const int init_gpio(){ // Inicializa os pinos de PWM

    const int PWMpinRes = 4; // WiringPI GPIO 23
    const int PWMpinVet = 5; // WiringPI GPIO 24

    if (wiringPiSetup() == -1)  exit(1);
    if (softPwmCreate(PWMpinRes, 0, 100) == 0){ printf("PWM do resistor criado!\n"); }
    if (softPwmCreate(PWMpinVet, 0, 100) == 0){ printf("PWM da ventoinha criado!\n"); }
    return PWMpinRes, PWMpinVet;
}

void init_estado(){
    pid_configura_constantes(30.0, 0.2, 400.0);
    sendSignal(enviaEstadoDoSistemaDesligado, 0);
    sendSignal(enviaEstadoDeFuncionamentoParado, 0);
    sendSignal(modoDeControleDaTemperaturaDeReferenciaCurvaDesativado, 0);
    curva = 0;
    sistemaLigado = 0;
    algoritmoIniciado = 0;
    count = 1;
}

void delay(unsigned milliseconds) // Função de delay em milisegundos
{
    clock_t pause;
    clock_t start;
    pause = milliseconds * (CLOCKS_PER_SEC / 1000);
    start = clock();
    while( (clock() - start) < pause );
}

void pid_activation(double pidOfResistor, const int PWMpinRes, const int PWMpinVet){ // Ativa o PID

    if(pidOfResistor > -40 && pidOfResistor < 0){ pidOfResistor = -40; }
    printf("PID: %f\n", pidOfResistor);
    sendInt(enviaSinalDeControleInt, pidOfResistor);

    if (pidOfResistor < 0){
            softPwmWrite(PWMpinRes, 0);
            delay(0.5);
            softPwmWrite(PWMpinVet, pidOfResistor * -1);
            delay(0.5);
    }

    else if (pidOfResistor > 0){
            softPwmWrite(PWMpinVet, 0);
            delay(0.5);
            softPwmWrite(PWMpinRes, pidOfResistor);
            delay(0.5);
        }   
}

void loop(const int PWMpinRes, const int PWMpinVet){ // Loop principal
    time_t t = time(NULL);   
    struct tm tm = *localtime(&t);
    int usuario = requestInt(comandosDoUsuario);
    printf("Comando do Usuario: %d\n", usuario);

    if      (usuario == 161)      {printf("Ligar Sistema\n"); sendSignal(enviaEstadoDoSistemaLigado, 1); sistemaLigado = 1;}
    else if (usuario == 162) {printf("Desligar Sistema\n"); sendSignal(enviaEstadoDoSistemaDesligado, 0); sistemaLigado = 0;}
    else if (usuario == 163) {printf("Algoritmo Iniciado\n"); sendSignal(enviaEstadoDeFuncionamentoFuncionando, 1); algoritmoIniciado = 1;}
    else if (usuario == 164) {printf("Algoritmo Parado\n"); sendSignal(enviaEstadoDeFuncionamentoParado, 0); algoritmoIniciado = 0;}

    if (sistemaLigado == 0)     {printf("Sistema desligado, por favor ligue o sistema\n"); sleep(2); return;}
    if (algoritmoIniciado == 0) {printf("Algoritmo desligado, por favor inicie o software\n"); sleep(2); return;}
    if(curva == 0){
        TempReferencia = requestFloat(solicitaTemperaturaDeReferencia);
        printf("Temperatura Referencia: %f\n", TempReferencia);
        pid_atualiza_referencia(TempReferencia);
    }
    else if(curva == 1){
        printf("Curva de temperatura de referência ativada\n"); // Curva de temperatura de referência
        if      (count > 0 && count < 30) {TempReferencia = 25.0;}
        else if (count >= 30 && count < 60)  {TempReferencia = 38.0;}
        else if (count >= 60 && count < 120) {TempReferencia = 46.0;}
        else if (count >= 120 && count < 130) {TempReferencia = 57.0;}
        else if (count >= 130 && count < 150) {TempReferencia = 61.0;}
        else if (count >= 150 && count < 180) {TempReferencia = 63.0;}
        else if (count >= 180 && count < 210) {TempReferencia = 54.0;}
        else if (count >= 210 && count < 240) {TempReferencia = 33.0;}
        else if (count >= 240 && count < 300) {TempReferencia = 25.0;}
        else    {curva = 0; count = 0; sendSignal(modoDeControleDaTemperaturaDeReferenciaCurvaDesativado, 0);}

        printf("Nova Temperatura Referencia (Curva): %f         ", TempReferencia);
        printf("Segundos: %d\n", count);
        pid_atualiza_referencia(TempReferencia);
        sendFloat(enviaSinalDeReferenciaFloat, TempReferencia);
        count++;
    }

    float TempInterna = requestFloat(solicitaTemperaturaInterna);
    printf("Temperatura Interna: %f\n", TempInterna);
    double pidOfResistor = pid_controle(TempInterna);

    if (usuario == 165){ // Modo Curva
        if(curva == 0){
            curva = 1;
            sendSignal(modoDeControleDaTemperaturaDeReferenciaCurvaAtivado, 1);
            float newref = 25.0;
            pid_atualiza_referencia(newref);
            sendFloat(enviaSinalDeReferenciaFloat, newref);
        }
        else if(curva == 1) {curva = 0; sendSignal(modoDeControleDaTemperaturaDeReferenciaCurvaDesativado, 0);}
        printf("Modo Curva Ativado/Desativado\n");
    }
    pid_activation(pidOfResistor, PWMpinRes, PWMpinVet);

    FILE *measuredData;
    measuredData = fopen("log.csv", "a+");
    fprintf(measuredData, "%02d/%02d/%d;%02d:%02d:%02d;%0.2lf;%0.2lf;%0.2lf\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, TempInterna, TempReferencia, pidOfResistor);
    fclose(measuredData);
}

int main(){
    printf("-------- Montando a estruturar do programa --------\n");
    const int PWMpinRes = 4; // WiringPI GPIO 23
    const int PWMpinVet = 5; // WiringPI GPIO 24
    if (wiringPiSetup() == -1) exit(1);
    if (softPwmCreate(PWMpinRes, 0, 100) == 0) {printf("PWM do resistor criado!\n");}
    if (softPwmCreate(PWMpinVet, 0, 100) == 0) {printf("PWM da ventoinha criado!\n");}
    init_uart();
    init_estado();
    while(1) {loop(PWMpinRes, PWMpinVet);}
    close_uart();
}