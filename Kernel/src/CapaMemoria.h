/*
 * CapaMemoria.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef SRC_CAPAMEMORIA_H_
#define SRC_CAPAMEMORIA_H_

#include <commons/collections/dictionary.h>
#include "estados.h"

t_dictionary *semaforos;
t_dictionary *variablesCompartidas;

void getVariableCompartida(char* data, int socket);
void wait(char* data,int socket);
void signal(char* data,int socket);
void setVariableCompartida(char* data, int socket);

#endif /* SRC_CAPAMEMORIA_H_ */
