/*
 * Kernel.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <commons/config.h>
#include <panicommons/panisocket.h>
#include <commons/collections/queue.h>

int PuertoConsola;
int PuertoCpu;
char* IpMemoria;
int PuertoMemoria;
char* IpFS;
int PuertoFS;
int Quantum;
int QuantumSleep;
char* Algoritmo;
int GradoMultiprog;
char** SemIds;
char** SemInit;
char** SharedVars;
int StackSize;

t_dictionary *semaforos;
t_dictionary *variablesCompartidas;

t_list *lista_cpus_conectadas;

typedef enum {
	FIFO = 0, RR = 1
} modo_planificacion;

typedef struct{
	int socket;
	bool corriendo;
}t_cpu;

typedef struct {
	int valor;
	t_queue* cola;
} t_semaforo;

t_config* cargarConfiguracion(char* archivo);
void cargar_varCompartidas();
void crear_semaforos();


#endif /* KERNEL_H_ */
