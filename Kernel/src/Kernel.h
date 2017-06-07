/*
 * Kernel.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdbool.h>
#include <stdlib.h>
#include "inotify.h"

#include "CapaMemoria.h"

int PuertoConsola;
int PuertoCpu;
char* IpMemoria;
int PuertoMemoria;
char* IpFS;
int PuertoFS;
int Quantum;
int QuantumSleep;
char* Algoritmo;
char* configFileName;
int GradoMultiprog;
char** SemIds;
char** SemInit;
char** SharedVars;
int StackSize;

typedef enum {
	FIFO = 0, RR = 1
} modo_planificacion;

modo_planificacion Modo;

int ultimoPID;
pthread_mutex_t	mutexPID = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexMemoria = PTHREAD_MUTEX_INITIALIZER;

t_config* cargarConfiguracion(char* archivo);
void cargar_varCompartidas();
void crear_semaforos();
t_pcb* armar_nuevo_pcb(char* codigo);
void inicializar_programa(t_pcb* nuevo_pcb);
void cargar_programa(int32_t socket, int pid);
void relacionar_cpu_programa(t_cpu *cpu, t_consola *programa, t_pcb *pcb);
void elminar_consola_por_socket(int socket);
void elminar_consola_por_pid(int pid);
bool esta_libre(void * unaCpu);
void programa(void* arg);
void enviar_a_cpu();
void cargarCPU(int32_t socket);
void respuesta_inicializar_programa(int socket, int socketMemoria, char* codigo);
bool almacenarBytes(t_pcb* pcb,int socketMemoria,char* codigo);
void finalizarProgramaConsola(char*data,int socket);
void finalizarProceso(void* pidArg);
t_respuesta_finalizar_programa* finalizarProcesoMemoria(int32_t pid);
void eliminar_consola_por_pid(int32_t pid);

#endif /* KERNEL_H_ */
