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
#include <parser/metadata_program.h>

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
	int32_t pagina;
} __attribute__((__packed__))t_pedido_liberar_pagina;

typedef enum{OK_LIBERAR=1,ERROR_LIBERAR=-1} codigo_liberar_pagina;

typedef struct
{
	codigo_liberar_pagina codigo;
} __attribute__((__packed__))t_respuesta_liberar_pagina;

typedef struct
{
	int32_t pid;
	int32_t tamanio;
	char* nombre_variable_compartida;
} __attribute__((__packed__))t_pedido_obtener_variable_compartida;

typedef enum{OK_VARIABLE=1,ERROR_VARIABLE=-1} codigo_variable_compartida;

typedef struct
{
	int32_t valor_variable_compartida;
	codigo_variable_compartida codigo;
} __attribute__((__packed__))t_respuesta_obtener_variable_compartida;

typedef struct
{
	int32_t pid;
	int32_t tamanio;
	char* nombre_variable_compartida;
	int32_t valor_variable_compartida;
} __attribute__((__packed__))t_pedido_asignar_variable_compartida;

typedef enum{OK_ASIGNAR_VARIABLE=1,ERROR_ASIGNAR_VARIABLE=-1} codigo_asignar_variable_compartida;

typedef struct
{
	codigo_asignar_variable_compartida codigo;
} __attribute__((__packed__))t_respuesta_asignar_variable_compartida;

typedef enum {
	FINALIZAR_OK = 0,
	FINALIZAR_SIN_RECURSOS=-1,
	FINALIZAR_ARCHIVO_NO_EXISTE=-2,
	FINALIZAR_LEER_ARCHIVO_SIN_PERMISOS=-3,
	FINALIZAR_ESCRIBIR_ARCHIVO_SIN_PERMISOS=-4,
	FINALIZAR_EXCEPCION_MEMORIA=-5,
	FINALIZAR_DESCONEXION_CONSOLA=-6,
	FINALIZAR_BY_CONSOLE =-7,
	FINALIZAR_PAGE_OVERFLOW=-8,
	FINALIZAR_SIN_MEMORIA=-9,
	FINALIZAR_ERROR_SIN_DEFINICION=-20
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

typedef enum{WAIT_OK=1,WAIT_BLOCKED=-1,WAIT_NOT_EXIST=-2} codigo_respuesta_wait;

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

typedef struct{
	int32_t pid;
	int32_t bytes;
	int32_t paginasTotales;
}t_pedido_reservar;

typedef enum{RESERVAR_OK=0,RESERVAR_OVERFLOW=-1,RESERVAR_SIN_ESPACIO=-2} codigo_respuesta_reservar;

typedef struct{
	int32_t puntero;
	codigo_respuesta_reservar codigo;
}t_respuesta_reservar;

typedef struct{
	int32_t pid;
	int32_t pagina;
	int32_t offset;
}t_pedido_liberar;

typedef enum{LIBERAR_OK=0,LIBERAR_ERROR=-1} codigo_respuesta_liberar;

typedef struct{
	codigo_respuesta_liberar codigo;
}t_respuesta_liberar;

typedef struct{
	int32_t pid;
	int32_t descriptor_archivo;
	int32_t tamanio;
}t_pedido_leer;

typedef enum{LEER_OK=0,LEER_BLOCKED=-1,LEER_NO_EXISTE=-2} codigo_respuesta_leer;

typedef struct{
	char* informacion;
	int32_t tamanio;
	codigo_respuesta_leer codigo;
}t_respuesta_leer;

typedef struct
{
	int32_t pid;
	t_banderas* flags;
	int32_t tamanio;
	char* direccion;
} __attribute__((__packed__))t_pedido_abrir_archivo;

typedef struct
{
	int32_t pid;
	int32_t tamanio;
	char* direccion;
} __attribute__((__packed__))t_pedido_cerrar_archivo;

typedef struct
{
	int32_t tamanio;
	char* direccion;
} __attribute__((__packed__))t_pedido_validar_crear_archivo_fs;

typedef enum{ABRIR_OK = 0, ERROR_ABRIR = 1} codigo_respuesta_abrir;

typedef struct{
	int32_t fd;
	codigo_respuesta_abrir codigo;
}t_respuesta_abrir_archivo;

u_int32_t tamanio_pcb(t_pcb* pcb);
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

t_pedido_liberar_pagina* deserializar_pedido_liberar_pagina(char *pedido_serializado);
char* serializar_pedido_liberar_pagina(t_pedido_liberar_pagina *pedido);
t_respuesta_liberar_pagina* deserializar_respuesta_liberar_pagina(char *respuesta_serializado);
char* serializar_respuesta_liberar_pagina(t_respuesta_liberar_pagina *respuesta);

//Memoria

//Kernel

t_respuesta_obtener_variable_compartida* deserializar_respuesta_obtener_variable_compartida(char* pedido_serializado);
char* serializar_respuesta_obtener_variable_compartida(t_respuesta_obtener_variable_compartida* respuesta);

t_pedido_obtener_variable_compartida* deserializar_pedido_obtener_variable_compartida(char* pedido_serializado);
char* serializar_pedido_obtener_variable_compartida(t_pedido_obtener_variable_compartida* pedido);

t_respuesta_asignar_variable_compartida* deserializar_respuesta_asignar_variable_compartida(char* pedido_serializado);
char* serializar_respuesta_asignar_variable_compartida(t_respuesta_asignar_variable_compartida* respuesta);

t_pedido_asignar_variable_compartida* deserializar_pedido_asignar_variable_compartida(char* pedido_serializado);
char* serializar_pedido_asignar_variable_compartida(t_pedido_asignar_variable_compartida* pedido);


void destruir_pcb (t_pcb *pcbADestruir);

char* serializar_pedido_signal(t_pedido_signal* pedido_deserializado);
t_pedido_signal* deserializar_pedido_signal(char* pedido_serializado);

char* serializar_respuesta_signal(t_respuesta_signal* respuesta_deserializada);
t_respuesta_signal* deserializar_respuesta_signal(char* respuesta_serializada);

char* serializar_pedido_wait(t_pedido_wait* pedido_deserializado);
t_pedido_wait* deserializar_pedido_wait(char* pedido_serializado);

char* serializar_respuesta_wait(t_respuesta_wait* respuesta_deserializada);
t_respuesta_wait* deserializar_respuesta_wait(char* respuesta_serializada);

char* serializar_pedido_reservar(t_pedido_reservar* pedido);
t_pedido_reservar* deserializar_pedido_reservar(char* pedido_serializado);

char* serializar_respuesta_reservar(t_respuesta_reservar* pedido);
t_respuesta_reservar* deserializar_respuesta_reservar(char* pedido_serializado);

char* serializar_pedido_liberar(t_pedido_liberar* pedido);
t_pedido_liberar* deserializar_pedido_liberar(char* pedido_serializado);

char* serializar_respuesta_liberar(t_respuesta_liberar* respuesta);
t_respuesta_liberar* deserializar_respuesta_liberar(char* respuesta_serializada);

char* serializar_pedido_abrir_archivo(t_pedido_abrir_archivo* pedido);
t_pedido_abrir_archivo* deserializar_pedido_abrir_archivo(char* pedido_serializado);

char* serializar_respuesta_abrir_archivo(t_respuesta_abrir_archivo* respuesta);
t_respuesta_abrir_archivo* deserializar_respuesta_abrir_archivo(char* respuesta_serializada);

char* serializar_pedido_leer_archivo(t_pedido_leer* pedido);
t_pedido_leer* deserializar_pedido_leer_archivo(char* pedido_serializado);

char* serializar_respuesta_leer_archivo(t_respuesta_leer* respuesta);
t_respuesta_leer* deserializar_respuesta_leer_archivo(char* respuesta_serializada);

t_pedido_cerrar_archivo* deserializar_pedido_cerrar_archivo(char* pedido_serializado);

char* serializar_pedido_validar_crear_archivo(t_pedido_validar_crear_archivo_fs* pedido);
t_pedido_validar_crear_archivo_fs* deserializar_pedido_validar_crear_archivo(char* pedido_serializado);

//Kernel

//FS

char* serializar_respuesta_validar_archivo(t_respuesta_validar_archivo* rta);
t_respuesta_validar_archivo* deserializar_respuesta_validar_archivo(char* rta);

char* serializar_respuesta_crear_archivo(t_respuesta_crear_archivo* rta);
t_respuesta_crear_archivo* deserializar_respuesta_crear_archivo(char* rta);

//FS

#endif /* PANICOMMONS_SERIALIZACION_H_ */
