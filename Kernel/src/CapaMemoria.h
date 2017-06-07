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

t_dictionary *semaforos;
t_dictionary *paginasGlobalesHeap;

void getVariableCompartida(char* data, int socket);
void setVariableCompartida(char* data, int socket);
void wait(char* data,int socket);
void signal(char* data,int socket);


#endif /* SRC_CAPAMEMORIA_H_ */
