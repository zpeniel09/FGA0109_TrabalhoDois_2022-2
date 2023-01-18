// Fonte: Renato Sampaio 
// https://gitlab.com/fse_fga/trabalhos-2022_2/trabalho-2-2022-2/-/blob/main/PID/pid.h

#ifndef PID_H_
#define PID_H_

void pid_configura_constantes(double Kp_, double Ki_, double Kd_);
void pid_atualiza_referencia(float referencia_);
double pid_controle(double saida_medida);

#endif /* PID_H_ */