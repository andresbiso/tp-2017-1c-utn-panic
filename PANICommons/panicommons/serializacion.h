/*
 * serializacion.h
 *
 *  Created on: 21/4/2017
 *      Author: utnso
 */

#ifndef PANICOMMONS_SERIALIZACION_H_
#define PANICOMMONS_SERIALIZACION_H_

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
	int32_t pag;
	int32_t offset;
	int32_t size;
} __attribute__((__packed__)) t_posMemoria;

typedef struct {
	char id;
	t_posMemoria posicionVar;
} __attribute__((__packed__)) t_variable;

typedef struct {
	int32_t posicion;
	int32_t cant_argumentos;
	t_posMemoria * argumentos;
	int32_t cant_variables;
	t_variable * variables;
	int32_t pos_retorno;
	t_posMemoria pos_var_retorno;
} __attribute__((__packed__)) registro_indice_stack;

typedef struct{
	int32_t pid;
	int32_t pc;
	int32_t cant_pags_totales;
	t_posMemoria fin_stack;
	u_int32_t cant_instrucciones;
	t_posMemoria* indice_codigo;
	u_int32_t tamano_etiquetas;
	char* indice_etiquetas;
	u_int32_t cant_entradas_indice_stack;
	registro_indice_stack* indice_stack;
}__attribute__((__packed__)) t_pcb;

typedef struct{
	char* contenido_pcb;
	int32_t tamanio;
} __attribute__((__packed__)) t_pcb_serializado;

typedef struct
{
	int32_t idPrograma;
	int32_t pagRequeridas;
} __attribute__((__packed__)) t_pedido_inicializar;

t_pcb_serializado serializar(t_pcb pcb);
t_pcb* deserializar(char* pcbs);
t_pedido_inicializar* deserializar_pedido_inicializar(char *pedido_serializado);
char* serializar_pedido_inicializar(t_pedido_inicializar *pedido);

#endif /* PANICOMMONS_SERIALIZACION_H_ */
