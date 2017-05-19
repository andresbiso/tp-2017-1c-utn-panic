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
	int32_t exit_code;
}__attribute__((__packed__)) t_pcb;

typedef struct{
	char* contenido_pcb;
	int32_t tamanio;
} __attribute__((__packed__)) t_pcb_serializado;

typedef struct
{
	int32_t pagRequeridas;
	int32_t idPrograma;
} __attribute__((__packed__)) t_pedido_inicializar;

typedef struct
{
	char* mensaje;
	int32_t tamanomensaje;
	int32_t idPrograma;
	int terminoProceso;
} __attribute__((__packed__)) t_aviso_consola;

typedef enum{OK_INICIALIZAR=1,SIN_ESPACIO_INICIALIZAR=-1} codigo_respuesta_inicializar;

typedef struct
{
	int32_t idPrograma;
	codigo_respuesta_inicializar codigoRespuesta;
} __attribute__((__packed__)) t_respuesta_inicializar;


typedef struct
{
	int32_t pid;
	int32_t pagina;
	int32_t offsetPagina;
	int32_t tamanio;
} __attribute__((__packed__)) t_pedido_solicitar_bytes;

typedef enum{OK_SOLICITAR=1,PAGINA_SOL_NOT_FOUND=-1,PAGINA_SOLICITAR_OVERFLOW=-2} codigo_solicitar_bytes;

typedef struct
{
	int32_t pid;
	int32_t pagina;
	codigo_solicitar_bytes codigo;
	int32_t tamanio;
	char* data;
} __attribute__((__packed__)) t_respuesta_solicitar_bytes;

typedef struct
{
	int32_t pid;
	int32_t pagina;
	int32_t offsetPagina;
	int32_t tamanio;
	char* data;
} __attribute__((__packed__)) t_pedido_almacenar_bytes;

typedef enum{OK_ALMACENAR=1,PAGINA_ALM_NOT_FOUND=-1,PAGINA_ALM_OVERFLOW=-2} codigo_almacenar_bytes;

typedef struct
{
	int32_t pid;
	codigo_almacenar_bytes codigo;
} __attribute__((__packed__))t_respuesta_almacenar_bytes;

typedef struct
{
	int32_t pid;
} __attribute__((__packed__)) t_pedido_finalizar_programa;

typedef enum{OK_FINALIZAR=1,ERROR_FINALIZAR=-1} codigo_finalizar_programa;

typedef struct
{
	int32_t pid;
	codigo_finalizar_programa codigo;
} __attribute__((__packed__))t_respuesta_finalizar_programa;


t_pcb_serializado* serializar_pcb(t_pcb* pcb);
t_pcb* deserializar_pcb(char* pcbs);

char* serializar_aviso_consola(t_aviso_consola *pedido);
t_aviso_consola* deserializar_aviso_consola(char *pedido_serializado);

//Memoria
t_pedido_inicializar* deserializar_pedido_inicializar(char *pedido_serializado);
char* serializar_pedido_inicializar(t_pedido_inicializar *pedido);
t_respuesta_inicializar* deserializar_respuesta_inicializar(char *respuesta_serializado);
char* serializar_respuesta_inicializar(t_respuesta_inicializar *respuesta);


t_pedido_solicitar_bytes* deserializar_pedido_solicitar_bytes(char *pedido_serializado);
char* serializar_pedido_solicitar_bytes(t_pedido_solicitar_bytes *pedido);
t_respuesta_solicitar_bytes* deserializar_respuesta_solicitar_bytes(char *respuesta_serializada);
char* serializar_respuesta_solicitar_bytes(t_respuesta_solicitar_bytes *respuesta);

t_pedido_almacenar_bytes* deserializar_pedido_almacenar_bytes(char *pedido_serializado);
char* serializar_pedido_almacenar_bytes(t_pedido_almacenar_bytes *pedido);
t_respuesta_almacenar_bytes* deserializar_respuesta_almacenar_bytes(char *respuesta_serializado);
char* serializar_respuesta_almacenar_bytes(t_respuesta_almacenar_bytes *respuesta);

t_pedido_finalizar_programa* deserializar_pedido_finalizar_programa(char *pedido_serializado);
char* serializar_pedido_finalizar_programa(t_pedido_finalizar_programa *pedido);
t_respuesta_finalizar_programa* deserializar_respuesta_finalizar_programa(char *respuesta_serializado);
char* serializar_respuesta_finalizar_programa(t_respuesta_finalizar_programa *respuesta);

//Memoria

#endif /* PANICOMMONS_SERIALIZACION_H_ */
