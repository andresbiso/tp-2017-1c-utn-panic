/*
 * CPU.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include "PrimitivasAnsisop.h"

int puertoKernel;
char* ipKernel;
int puertoMemoria;
char* ipMemoria;
int quantum;
int quantumSleep;
t_log* log;

int socketKernel;
int socketMemoria;

#endif /* CPU_H_ */
