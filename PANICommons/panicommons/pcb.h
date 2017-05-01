/*
 * pcb.h
 *
 *  Created on: 21/4/2017
 *      Author: utnso
 */

#ifndef PANICOMMONS_PCB_H_
#define PANICOMMONS_PCB_H_

#include <stdlib.h>
#include <string.h>

typedef struct {
	int32_t pid;
	int32_t pc;
}__attribute__((__packed__)) t_pcb;

typedef struct{
	char* contenido_pcb;
	int32_t tamanio;
} __attribute__((__packed__)) t_pcb_serializado;

t_pcb_serializado serializar(t_pcb pcb);
t_pcb* deserializar(char* pcbs);

#endif /* PANICOMMONS_PCB_H_ */
