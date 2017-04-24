/*
 * pcb.c
 *
 *  Created on: 21/4/2017
 *      Author: utnso
 */
#include "pcb.h"
#include <stdlib.h>

int agregar(char *to,int32_t tamano, void* from){
	memcpy(to,from,tamano);
	return tamano;
}

int32_t tamanio_pcb(t_pcb pcb){

	return	sizeof(pcb.pid)+
			sizeof(pcb.pc);
}

t_pcb_serializado serializar(t_pcb pcb){
	t_pcb_serializado pcb_serializado;
	pcb_serializado.tamanio = tamanio_pcb(pcb);
	pcb_serializado.contenido_pcb = malloc(pcb_serializado.tamanio);
	int offset=0;

	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.pid),&pcb.pid);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.pc),&pcb.pc);

	return pcb_serializado;
}

t_pcb* deserializar(char* pcbs)
{
	t_pcb *pcb = malloc(sizeof(t_pcb));

	int offset = 0;

	pcb->pid = pcbs[offset];
	offset += sizeof(pcb->pid);

	pcb->pc = pcbs[offset];
	offset += sizeof(pcb->pc);

	return pcb;
}
