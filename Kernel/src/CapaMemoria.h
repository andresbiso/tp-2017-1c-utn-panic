/*
 * CapaMemoria.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef SRC_CAPAMEMORIA_H_
#define SRC_CAPAMEMORIA_H_

#include "estados.h"

typedef struct {
	int valor;
	t_queue* cola;
} t_semaforo;

typedef struct{
	int32_t size;
	bool isFree;
} __attribute__((__packed__)) t_heap_metadata;

typedef struct{
	int32_t nroPagina;
	int32_t espacioDisponible;//Es el espacio disponible
}t_pagina_heap;

typedef struct {
	int32_t maxPaginas;
	t_list* paginas;
} t_paginas_proceso;

t_dictionary *semaforos;
t_dictionary *paginasGlobalesHeap;
pthread_mutex_t mutexMemoriaHeap;

void getVariableCompartida(char* data, int socket);
void setVariableCompartida(char* data, int socket);
void wait(char* data,int socket);
void signal(char* data,int socket);
void reservar(void* data,int socket);
void liberar(void* data,int socket);
void removePaginaHeap(int32_t pid, int32_t pagina);
void cleanMemoriaHeap(int32_t pid);

#endif /* SRC_CAPAMEMORIA_H_ */
