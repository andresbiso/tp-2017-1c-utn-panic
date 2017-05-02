/*
 * serializacion.c
 *
 *  Created on: 21/4/2017
 *      Author: utnso
 */
#include "serializacion.h"
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
	int i;
	int offset=0;

	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.pid),&pcb.pid);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.pc),&pcb.pc);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.cant_pags_totales),&pcb.cant_pags_totales);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.fin_stack),&pcb.fin_stack);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.cant_instrucciones),&pcb.cant_instrucciones);

	for(i=0;i<pcb.cant_instrucciones;i++){
		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(t_posMemoria) ,&pcb.indice_codigo[i]);
	}

	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.tamano_etiquetas),&pcb.tamano_etiquetas);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,pcb.tamano_etiquetas,pcb.indice_etiquetas);
	offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.cant_entradas_indice_stack),&pcb.cant_entradas_indice_stack);

	for(i=0;i<pcb.cant_entradas_indice_stack;i++){
		int j;
		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.indice_stack->posicion),&pcb.indice_stack[i].posicion);
		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.indice_stack->cant_argumentos),&pcb.indice_stack[i].cant_argumentos);

			for(j=0;j<pcb.indice_stack[i].cant_argumentos;j++){
				offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(t_posMemoria),&pcb.indice_stack[i].argumentos[j]);
			}

		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.indice_stack->cant_variables),&pcb.indice_stack[i].cant_variables);

			for(j=0;j<pcb.indice_stack[i].cant_variables;j++){
				offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(t_variable),&pcb.indice_stack[i].variables[j]);
			}

		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.indice_stack->pos_retorno),&pcb.indice_stack[i].pos_retorno);
		offset+=agregar(pcb_serializado.contenido_pcb+offset,sizeof(pcb.indice_stack->pos_var_retorno),&pcb.indice_stack[i].pos_var_retorno);
	}

	return pcb_serializado;
}

t_pcb* deserializar(char* pcbs)
{
	int i;
	t_pcb *pcb = malloc(sizeof(t_pcb));

	int offset = 0;

	pcb->pid = pcbs[offset];
	offset += sizeof(pcb->pid);

	pcb->pc = pcbs[offset];
	offset += sizeof(pcb->pc);

	pcb->cant_pags_totales = pcbs[offset];
	offset += sizeof(pcb->cant_pags_totales);

	pcb->fin_stack = *((t_posMemoria*)(pcbs+offset));
	offset += sizeof(pcb->fin_stack);

	pcb->cant_instrucciones = pcbs[offset];
	offset += sizeof(pcb->cant_instrucciones);

	pcb->indice_codigo = malloc(pcb->cant_instrucciones*sizeof(t_posMemoria));
	for(i=0;i < pcb->cant_instrucciones;i++){
		pcb->indice_codigo[i] = *((t_posMemoria*)(pcbs+offset));
		offset += sizeof(t_posMemoria);
	}

	pcb->tamano_etiquetas = *((u_int32_t*)(pcbs +offset));
	offset += sizeof(pcb->tamano_etiquetas);

	pcb->indice_etiquetas = malloc(pcb->tamano_etiquetas*sizeof(char));
	memcpy(pcb->indice_etiquetas,pcbs+offset,pcb->tamano_etiquetas*sizeof(char));
	offset += pcb->tamano_etiquetas;

	pcb->cant_entradas_indice_stack = pcbs[offset];
	offset += sizeof(pcb->cant_entradas_indice_stack);

	pcb->indice_stack = malloc(pcb->cant_entradas_indice_stack * sizeof(registro_indice_stack));
	for(i=0;i<pcb->cant_entradas_indice_stack;i++){
		int j;
		pcb->indice_stack[i].posicion = pcbs[offset];
		offset += sizeof(pcb->indice_stack->posicion);

		pcb->indice_stack[i].cant_argumentos = pcbs[offset];
		offset += sizeof(pcb->indice_stack->cant_argumentos);

		pcb->indice_stack[i].argumentos = malloc(sizeof(t_posMemoria)*pcb->indice_stack[i].cant_argumentos);
		for(j=0;j<pcb->indice_stack[i].cant_argumentos;j++){
			pcb->indice_stack[i].argumentos[j] = *((t_posMemoria*)(pcbs+offset));
			offset +=sizeof(t_posMemoria);
		}

		pcb->indice_stack[i].cant_variables = pcbs[offset];
		offset += sizeof(pcb->indice_stack->cant_variables);

		pcb->indice_stack[i].variables = malloc(sizeof(t_variable) * pcb->indice_stack[i].cant_variables);
		for(j=0;j<pcb->indice_stack[i].cant_variables;j++){
			pcb->indice_stack[i].variables[j] = *((t_variable*)(pcbs+offset));
			offset +=sizeof(t_variable);
		}

		pcb->indice_stack[i].pos_retorno = pcbs[offset];
		offset += sizeof(pcb->indice_stack->pos_retorno);

		pcb->indice_stack[i].pos_var_retorno = *((t_posMemoria*)(pcbs+offset));
		offset += sizeof(pcb->indice_stack->pos_var_retorno);
	}

	return pcb;
}

char* serializar_pedido_inicializar(t_pedido_inicializar *pedido){
	char *respuesta = malloc(sizeof(t_pedido_inicializar));

	int32_t tamanoidprograma = sizeof(pedido->idPrograma);
	int32_t tamanopagsrequeridas = sizeof(pedido->pagRequeridas);

	int offset = 0;
	memcpy(respuesta,&(pedido->pagRequeridas),tamanopagsrequeridas);
	offset += tamanopagsrequeridas;

	memcpy(respuesta+offset,&(pedido->idPrograma),tamanoidprograma);

	return respuesta;
}

t_pedido_inicializar* deserializar_pedido_inicializar(char *pedido_serializado){
	t_pedido_inicializar *respuesta = malloc(sizeof(t_pedido_inicializar));

	int offset = 0;
	memcpy(&respuesta->pagRequeridas,(void*)pedido_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->pagRequeridas);
	memcpy(&respuesta->idPrograma,(void*)pedido_serializado+offset,sizeof(int32_t));

	return respuesta;
}

t_pedido_solicitar_bytes* deserializar_pedido_solicitar_bytes(char *pedido_serializado){
	t_pedido_solicitar_bytes *respuesta = malloc(sizeof(t_pedido_solicitar_bytes));

	int offset = 0;
	memcpy(&respuesta->pid,(void*)pedido_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->pid);
	memcpy(&respuesta->pagina,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->pagina);
	memcpy(&respuesta->tamanio,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->tamanio);
	memcpy(&respuesta->offsetPagina,(void*)pedido_serializado+offset,sizeof(int32_t));

	return respuesta;
}

char* serializar_pedido_solicitar_bytes(t_pedido_solicitar_bytes *pedido){
	char *respuesta = malloc(sizeof(t_pedido_solicitar_bytes));

	int offset = 0;
	memcpy(respuesta,&(pedido->pid),sizeof(pedido->pid));
	offset += sizeof(pedido->pid);
	memcpy(respuesta+offset,&(pedido->pagina),sizeof(pedido->pagina));
	offset += sizeof(pedido->pagina);
	memcpy(respuesta+offset,&(pedido->tamanio),sizeof(pedido->tamanio));
	offset += sizeof(pedido->tamanio);
	memcpy(respuesta+offset,&(pedido->offsetPagina),sizeof(pedido->offsetPagina));

	return respuesta;
}

t_pedido_almacenar_bytes* deserializar_pedido_almacenar_bytes(char *pedido_serializado){
	t_pedido_almacenar_bytes *respuesta = malloc(sizeof(t_pedido_almacenar_bytes));

	int offset = 0;
	memcpy(&respuesta->pid,(void*)pedido_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->pid);
	memcpy(&respuesta->pagina,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->pagina);
	memcpy(&respuesta->tamanio,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->tamanio);
	memcpy(&respuesta->offsetPagina,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->offsetPagina);
	memcpy(&respuesta->data,(void*)pedido_serializado+offset,respuesta->tamanio);

	return respuesta;
}

char* serializar_pedido_almacenar_bytes(t_pedido_almacenar_bytes *pedido){
	char *respuesta = malloc(sizeof(t_pedido_almacenar_bytes));

	int offset = 0;
	memcpy(respuesta,&(pedido->pid),sizeof(pedido->pid));
	offset += sizeof(pedido->pid);
	memcpy(respuesta+offset,&(pedido->pagina),sizeof(pedido->pagina));
	offset += sizeof(pedido->pagina);
	memcpy(respuesta+offset,&(pedido->tamanio),sizeof(pedido->tamanio));
	offset += sizeof(pedido->tamanio);
	memcpy(respuesta+offset,&(pedido->offsetPagina),sizeof(pedido->offsetPagina));
	offset += sizeof(pedido->offsetPagina);
	memcpy(respuesta+offset,&(pedido->data),pedido->tamanio);

	return respuesta;
}

