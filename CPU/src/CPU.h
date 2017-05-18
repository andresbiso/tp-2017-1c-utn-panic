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

void waitKernel(int socketKernel,t_dictionary* diccionarioFunciones);
void modificarQuantum(int nuevoQuantum);
void modificarQuantumSleep(int nuevoQuantumSleep);
void nuevoPCB(char* pcb, int socket);
void borrarPCB(t_pcb* package);
void ejecutarPrograma();
void ejecutarInstruccion(t_respuesta_solicitar_bytes* respuesta);
void mostrarMensaje(char* mensaje, int socket);
t_config* cargarConfiguracion(char * nombreArchivo);

#endif /* CPU_H_ */
