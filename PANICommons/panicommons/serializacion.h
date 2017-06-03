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
	int32_t tamaniomensaje;
	int32_t idPrograma;
	int32_t terminoProceso;
	int32_t mostrarPorPantalla;
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

typedef struct
{
	int32_t pid;
	int32_t tamanio;
	char* nombre_variable_compartida;
} __attribute__((__packed__))t_pedido_variable_compartida;

typedef enum{OK_VARIABLE=1,ERROR_VARIABLE=-1} codigo_variable_compartida;

typedef struct
{
	int32_t valor_variable_compartida;
	codigo_variable_compartida codigo;
} __attribute__((__packed__))t_respuesta_variable_compartida;

typedef enum {
	FINALIZAR_SIN_RECURSOS=-1, FINALIZAR_EXEPCION_MEMORIA=-5, FINALIZAR_BY_CONSOLE = -7, FINALIZAR_OK = 0
} exit_codes;

typedef struct
{
	char* ruta;
	int32_t offset;
	int32_t tamanio;
} __attribute__((__packed__)) t_pedido_datos_fs;

typedef enum{VALIDAR_OK=1, NO_EXISTE_ARCHIVO=-1} codigo_validar_archivo;

typedef struct
{
	codigo_validar_archivo codigoRta;
}__attribute__((__packed__)) t_respuesta_validar_archivo;

typedef enum{CREAR_OK=1, NO_HAY_BLOQUES=-1, CREAR_ERROR=-2} codigo_crear_archivo;

typedef struct
{
	codigo_crear_archivo codigoRta;
} __attribute__((__packed__)) t_respuesta_crear_archivo;

typedef struct{
	int32_t tamanio;
	char* semId;
	t_pcb* pcb;
}t_pedido_wait;

typedef enum{WAIT_OK=1,WAIT_BLOCK=-1,WAIT_NOT_EXIST=-2} codigo_respuesta_wait;

typedef struct{
	codigo_respuesta_wait respuesta;
}t_respuesta_wait;

typedef struct{
	int32_t tamanio;
	char* semId;
}t_pedido_signal;

typedef enum{SIGNAL_OK=1,SIGNAL_NOT_EXIST=-1} codigo_respuesta_signal;

typedef struct{
	codigo_respuesta_signal respuesta;
}t_respuesta_signal;


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

//Kernel

t_respuesta_variable_compartida* deserializar_respuesta_variable_compartida(char* pedido_serializado);
char* serializar_respuesta_variable_compartida(t_respuesta_variable_compartida* respuesta);

t_pedido_variable_compartida* deserializar_pedido_variable_compartida(char* pedido_serializado);
char* serializar_pedido_variable_compartida(t_pedido_variable_compartida* pedido);

void destruir_pcb (t_pcb *pcbADestruir);

char* serializar_pedido_signal(t_pedido_signal* pedido_deserializado);
t_pedido_signal* deserializar_pedido_signal(char* pedido_serializado);

char* serializar_respuesta_signal(t_respuesta_signal* respuesta_deserializada);
t_respuesta_signal* deserializar_respuesta_signal(char* respuesta_serializada);

char* serializar_pedido_wait(t_pedido_wait* pedido_deserializado);
t_pedido_wait* deserializar_pedido_wait(char* pedido_serializado);

char* serializar_respuesta_wait(t_respuesta_wait* respuesta_deserializada);
t_respuesta_wait* deserializar_respuesta_wait(char* respuesta_serializada);

//Kernel

//FS

char* serializar_respuesta_validar_archivo(t_respuesta_validar_archivo* rta);
t_respuesta_validar_archivo* deserializar_respuesta_validar_archivo(char* rta);

char* serializar_respuesta_crear_archivo(t_respuesta_crear_archivo* rta);
t_respuesta_crear_archivo* deserializar_respuesta_crear_archivo(char* rta);

//FS

#endif /* PANICOMMONS_SERIALIZACION_H_ */
