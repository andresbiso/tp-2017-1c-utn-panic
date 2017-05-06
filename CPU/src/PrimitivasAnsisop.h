#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

#include <stdio.h>
#include <string.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <ctype.h>
#include <panicommons/serializacion.h>

#define PAGESIZE 4

/*typedef struct entrada_salida{
	t_nombre_dispositivo dispositivo;
	int tiempo;
}t_entrada_salida;*/

typedef struct grabar_valor{
	t_nombre_compartida variable;
	void* valorGrabar;
}t_grabar_valor;

typedef struct funciones_ansisop{
	AnSISOP_funciones funciones_comunes;
	AnSISOP_kernel funciones_kernel;
} FuncionesAnsisop;

t_pcb* actual_pcb;

FuncionesAnsisop* inicializar_primitivas();

// AnSISOP_funciones
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero	direccion_variable,	t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void finalizar(void);
void retornar(t_valor_variable retorno);

// AnSISOP_kernel
void waitAnsisop(t_nombre_semaforo identificador_semaforo);
void signalAnsisop(t_nombre_semaforo identificador_semaforo);
t_puntero reservar(t_valor_variable espacio);
void liberar(t_puntero puntero);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo direccion);
void cerrar(t_descriptor_archivo descriptor_archivo);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);

#endif /* PRIMITIVAS_H_ */
