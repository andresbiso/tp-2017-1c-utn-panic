/*
 * CPU.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include "PrimitivasAnsisop.h"

bool desconexion=false;
bool terminoCPU=false;
bool signalSIGUSR1=false;

void waitKernel(int socketKernel,t_dictionary* diccionarioFunciones);
void modificarQuantum(char*data, int socket);
void modificarQuantumSleep(char*data, int socket);
void nuevoPCB(char* pcb, int socket);
void finishCPU(char* data, int socket);
void borrarPCB(t_pcb* package);
void ejecutarPrograma();
void ejecutarInstruccion(t_respuesta_solicitar_bytes* respuesta);
void mostrarMensaje(char* mensaje, int socket);
t_config* cargarConfiguracion(char * nombreArchivo);
void recibirTamanioPagina(int socket);
void signalHandler(int signal);

#endif /* CPU_H_ */
