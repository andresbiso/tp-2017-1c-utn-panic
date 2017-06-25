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

int32_t tamanio_indice_stack(t_pcb* pcb){
	int i;
	int32_t contador = 0;
	for(i=0; i < (pcb->cant_entradas_indice_stack); i++)
		contador += sizeof(pcb->indice_stack->posicion) +

					sizeof(pcb->indice_stack->cant_argumentos) +
					sizeof(t_posMemoria) * pcb->indice_stack[i].cant_argumentos+

					sizeof(pcb->indice_stack->cant_variables) +
					sizeof(t_variable) * pcb->indice_stack[i].cant_variables+

					sizeof(pcb->indice_stack->pos_retorno)+
					sizeof(pcb->indice_stack->pos_var_retorno);

	return contador;
}

u_int32_t tamanio_pcb(t_pcb* pcb){
	return
			sizeof(pcb->pid)+
			sizeof(pcb->pc)+
			sizeof(pcb->cant_pags_totales)+
			sizeof(pcb->fin_stack)+

			sizeof(pcb->cant_instrucciones)+
			sizeof(t_posMemoria) * pcb->cant_instrucciones +

			sizeof(pcb->tamano_etiquetas)+
			pcb->tamano_etiquetas+

			sizeof(pcb->cant_entradas_indice_stack)+
			tamanio_indice_stack(pcb)+
			sizeof(pcb->exit_code);
}

t_pcb_serializado* serializar_pcb(t_pcb* pcb){
	t_pcb_serializado* pcb_serializado = malloc(sizeof(t_pcb_serializado));
	pcb_serializado->tamanio = tamanio_pcb(pcb);
	pcb_serializado->contenido_pcb = malloc(pcb_serializado->tamanio);

	int i;
	int offset=0;

	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->pid),&pcb->pid);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->pc),&pcb->pc);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->cant_pags_totales),&pcb->cant_pags_totales);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->fin_stack),&pcb->fin_stack);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->cant_instrucciones),&pcb->cant_instrucciones);

	for(i=0;i<pcb->cant_instrucciones;i++){
		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(t_posMemoria) ,&pcb->indice_codigo[i]);
	}

	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->tamano_etiquetas),&pcb->tamano_etiquetas);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,pcb->tamano_etiquetas,pcb->indice_etiquetas);
	offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->cant_entradas_indice_stack),&pcb->cant_entradas_indice_stack);

	for(i=0;i<pcb->cant_entradas_indice_stack;i++){
		int j;
		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->indice_stack->posicion),&pcb->indice_stack[i].posicion);
		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->indice_stack->cant_argumentos),&pcb->indice_stack[i].cant_argumentos);

			for(j=0;j<pcb->indice_stack[i].cant_argumentos;j++){
				offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(t_posMemoria),&pcb->indice_stack[i].argumentos[j]);
			}

		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->indice_stack->cant_variables),&pcb->indice_stack[i].cant_variables);

			for(j=0;j<pcb->indice_stack[i].cant_variables;j++){
				offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(t_variable),&pcb->indice_stack[i].variables[j]);
			}

		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->indice_stack->pos_retorno),&pcb->indice_stack[i].pos_retorno);
		offset+=agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->indice_stack->pos_var_retorno),&pcb->indice_stack[i].pos_var_retorno);
	}

	agregar(pcb_serializado->contenido_pcb+offset,sizeof(pcb->exit_code),&pcb->exit_code);

	return pcb_serializado;
}

t_pcb* deserializar_pcb(char* pcbs)
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

	pcb->exit_code = pcbs[offset];

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

char* serializar_aviso_consola(t_aviso_consola *pedido){
	char *respuesta = malloc((sizeof(int32_t)*4)+pedido->tamaniomensaje);

	int offset = 0;
	memcpy(respuesta,&(pedido->terminoProceso),sizeof(int32_t));
	offset += sizeof(pedido->terminoProceso);
	memcpy(respuesta+offset,&(pedido->mostrarPorPantalla),sizeof(int32_t));
	offset += sizeof(pedido->mostrarPorPantalla);
	memcpy(respuesta+offset,&(pedido->idPrograma),sizeof(int32_t));
	offset += sizeof(pedido->idPrograma);
	memcpy(respuesta+offset,&(pedido->tamaniomensaje),sizeof(int32_t));
	offset += sizeof(pedido->tamaniomensaje);
	memcpy(respuesta+offset,pedido->mensaje,pedido->tamaniomensaje);

	return respuesta;
}

t_aviso_consola* deserializar_aviso_consola(char *pedido_serializado){
	t_aviso_consola *respuesta = malloc(sizeof(t_aviso_consola));

	int offset = 0;

	memcpy(&respuesta->terminoProceso,(void*)pedido_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->terminoProceso);
	memcpy(&respuesta->mostrarPorPantalla,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->mostrarPorPantalla);
	memcpy(&respuesta->idPrograma,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset += sizeof(respuesta->idPrograma);
	memcpy(&(respuesta->tamaniomensaje),(void*)pedido_serializado+offset,sizeof(respuesta->tamaniomensaje));
	offset += sizeof(respuesta->tamaniomensaje);
	respuesta->mensaje=malloc(respuesta->tamaniomensaje+1);
	memcpy(respuesta->mensaje,(void*)pedido_serializado+offset,respuesta->tamaniomensaje);
	respuesta->mensaje[respuesta->tamaniomensaje]='\0';

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

t_respuesta_inicializar* deserializar_respuesta_inicializar(char *respuesta_serializado){
	t_respuesta_inicializar*respuesta = malloc(sizeof(t_respuesta_inicializar));

	int offset = 0;
	memcpy(&respuesta->idPrograma,(void*)respuesta_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->idPrograma);
	memcpy(&respuesta->codigoRespuesta,(void*)respuesta_serializado+offset,sizeof(int32_t));

	return respuesta;

}

char* serializar_respuesta_inicializar(t_respuesta_inicializar *respuesta){
	char *buffer = malloc(sizeof(t_respuesta_inicializar));

	int offset = 0;
	memcpy(buffer,&(respuesta->idPrograma),sizeof(respuesta->idPrograma));
	offset += sizeof(respuesta->idPrograma);
	memcpy(buffer+offset,&(respuesta->codigoRespuesta),sizeof(respuesta->codigoRespuesta));

	return buffer;
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
	respuesta->data=malloc(respuesta->tamanio);
	memcpy(respuesta->data,(void*)pedido_serializado+offset,respuesta->tamanio);

	return respuesta;
}

char* serializar_pedido_almacenar_bytes(t_pedido_almacenar_bytes *pedido){
	char *respuesta = malloc(sizeof(int32_t)*4+pedido->tamanio);

	int offset = 0;
	memcpy(respuesta,&(pedido->pid),sizeof(pedido->pid));
	offset += sizeof(pedido->pid);
	memcpy(respuesta+offset,&(pedido->pagina),sizeof(pedido->pagina));
	offset += sizeof(pedido->pagina);
	memcpy(respuesta+offset,&(pedido->tamanio),sizeof(pedido->tamanio));
	offset += sizeof(pedido->tamanio);
	memcpy(respuesta+offset,&(pedido->offsetPagina),sizeof(pedido->offsetPagina));
	offset += sizeof(pedido->offsetPagina);
	memcpy(respuesta+offset,pedido->data,pedido->tamanio);

	return respuesta;
}

t_respuesta_almacenar_bytes* deserializar_respuesta_almacenar_bytes(char *respuesta_serializado){
	t_respuesta_almacenar_bytes*respuesta = malloc(sizeof(t_respuesta_almacenar_bytes));

	int offset = 0;
	memcpy(&respuesta->pid,(void*)respuesta_serializado,sizeof(int32_t));
	offset += sizeof(respuesta->pid);
	memcpy(&respuesta->codigo,(void*)respuesta_serializado+offset,sizeof(int32_t));

	return respuesta;

}

char* serializar_respuesta_almacenar_bytes(t_respuesta_almacenar_bytes *respuesta){
	char *buffer = malloc(sizeof(t_respuesta_almacenar_bytes));

	int offset = 0;
	memcpy(buffer,&(respuesta->pid),sizeof(respuesta->pid));
	offset += sizeof(respuesta->pid);
	memcpy(buffer+offset,&(respuesta->codigo),sizeof(respuesta->codigo));

	return buffer;
}


t_respuesta_solicitar_bytes* deserializar_respuesta_solicitar_bytes(char *respuesta_serializada){
	t_respuesta_solicitar_bytes *respuesta = malloc(sizeof(t_respuesta_solicitar_bytes));

	int offset = 0;
	memcpy(&respuesta->pid,(void*)respuesta_serializada,sizeof(int32_t));
	offset += sizeof(respuesta->pid);
	memcpy(&respuesta->codigo,(void*)respuesta_serializada+offset,sizeof(respuesta->codigo));
	offset += sizeof(respuesta->codigo);
	memcpy(&respuesta->pagina,(void*)respuesta_serializada+offset,sizeof(int32_t));
	offset += sizeof(respuesta->pagina);
	memcpy(&respuesta->tamanio,(void*)respuesta_serializada+offset,sizeof(int32_t));
	offset += sizeof(respuesta->tamanio);
	respuesta->data=malloc(respuesta->tamanio);
	memcpy(respuesta->data,(void*)respuesta_serializada+offset,respuesta->tamanio);

	return respuesta;
}

char* serializar_respuesta_solicitar_bytes(t_respuesta_solicitar_bytes *respuesta){
	char *buffer = malloc(sizeof(int32_t)*3+sizeof(codigo_solicitar_bytes)+respuesta->tamanio);

	int offset = 0;
	memcpy(buffer,&(respuesta->pid),sizeof(respuesta->pid));
	offset += sizeof(respuesta->pid);
	memcpy(buffer+offset,&(respuesta->codigo),sizeof(respuesta->codigo));
	offset += sizeof(respuesta->codigo);
	memcpy(buffer+offset,&(respuesta->pagina),sizeof(respuesta->pagina));
	offset += sizeof(respuesta->pagina);
	memcpy(buffer+offset,&(respuesta->tamanio),sizeof(respuesta->tamanio));
	offset += sizeof(respuesta->tamanio);
	memcpy(buffer+offset,respuesta->data,respuesta->tamanio);

	return buffer;
}

t_pedido_finalizar_programa* deserializar_pedido_finalizar_programa(char *pedido_serializado){
	t_pedido_finalizar_programa* pedido = malloc(sizeof(t_pedido_finalizar_programa));

	memcpy(&pedido->pid,(void*)pedido_serializado,sizeof(int32_t));

	return pedido;
}

char* serializar_pedido_finalizar_programa(t_pedido_finalizar_programa *pedido){
	char *buffer = malloc(sizeof(t_pedido_finalizar_programa));

	memcpy(buffer,&(pedido->pid),sizeof(pedido->pid));

	return buffer;
}

t_respuesta_finalizar_programa* deserializar_respuesta_finalizar_programa(char *respuesta_serializado){
	t_respuesta_finalizar_programa* respuesta = malloc(sizeof(t_respuesta_finalizar_programa));

	int offset=0;
	memcpy(&respuesta->pid,(void*)respuesta_serializado,sizeof(int32_t));
	offset+=sizeof(respuesta->pid);
	memcpy(&respuesta->codigo,(void*)respuesta_serializado+offset,sizeof(int32_t));

	return respuesta;
}

char* serializar_respuesta_finalizar_programa(t_respuesta_finalizar_programa *pedido){
	char *buffer = malloc(sizeof(t_respuesta_finalizar_programa));

	int offset=0;
	memcpy(buffer,&(pedido->pid),sizeof(pedido->pid));
	offset+=sizeof(pedido->pid);
	memcpy(buffer+offset,&(pedido->codigo),sizeof(pedido->codigo));

	return buffer;
}

t_respuesta_obtener_variable_compartida* deserializar_respuesta_obtener_variable_compartida(char* pedido_serializado) {
	t_respuesta_obtener_variable_compartida* pedido = malloc(sizeof(t_respuesta_obtener_variable_compartida));

	int offset=0;
	memcpy(&pedido->valor_variable_compartida,(void*)pedido_serializado,sizeof(int32_t));
	offset+=sizeof(pedido->valor_variable_compartida);
	memcpy(&pedido->codigo,(void*)pedido_serializado+offset,sizeof(codigo_variable_compartida));

	return pedido;
}

char* serializar_respuesta_obtener_variable_compartida(t_respuesta_obtener_variable_compartida* respuesta){
	char* respuesta_serializada = malloc(sizeof(t_respuesta_obtener_variable_compartida));

	int offset=0;
	memcpy(respuesta_serializada,&respuesta->valor_variable_compartida,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(respuesta_serializada+offset,&respuesta->codigo,sizeof(codigo_variable_compartida));

	return respuesta_serializada;
}

t_pedido_obtener_variable_compartida* deserializar_pedido_obtener_variable_compartida(char* pedido_serializado){
	t_pedido_obtener_variable_compartida* pedido = malloc(sizeof(t_pedido_obtener_variable_compartida));

	int offset=0;
	memcpy(&pedido->pid,(void*)pedido_serializado,sizeof(int32_t));
	offset+=sizeof(pedido->pid);
	memcpy(&pedido->tamanio,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(pedido->tamanio);
	pedido->nombre_variable_compartida= malloc(pedido->tamanio+2);//uno para el \0 otro para el !
	pedido->nombre_variable_compartida[0]='!';
	pedido->nombre_variable_compartida[pedido->tamanio+1]='\0';
	memcpy(pedido->nombre_variable_compartida+1,(void*)pedido_serializado+offset,pedido->tamanio);//El +1 por el !

	return pedido;

}

char* serializar_pedido_obtener_variable_compartida(t_pedido_obtener_variable_compartida* pedido) {
	char *buffer = malloc(sizeof(int32_t)*2+pedido->tamanio);

	int offset=0;
	memcpy(buffer,&(pedido->pid),sizeof(pedido->pid));
	offset+=sizeof(pedido->pid);
	memcpy(buffer+offset,&(pedido->tamanio),sizeof(pedido->tamanio));
	offset+=sizeof(pedido->tamanio);
	memcpy(buffer+offset,pedido->nombre_variable_compartida,pedido->tamanio);

	return buffer;
}

t_respuesta_asignar_variable_compartida* deserializar_respuesta_asignar_variable_compartida(char* pedido_serializado) {
	t_respuesta_asignar_variable_compartida* respuesta = malloc(sizeof(t_respuesta_asignar_variable_compartida));

	memcpy(&respuesta->codigo,(void*)pedido_serializado,sizeof(codigo_asignar_variable_compartida));

	return respuesta;
}

char* serializar_respuesta_asignar_variable_compartida(t_respuesta_asignar_variable_compartida* respuesta) {
	char* respuesta_serializada = malloc(sizeof(t_respuesta_asignar_variable_compartida));

	memcpy(respuesta_serializada,&respuesta->codigo,sizeof(codigo_asignar_variable_compartida));

	return respuesta_serializada;
}

t_pedido_asignar_variable_compartida* deserializar_pedido_asignar_variable_compartida(char* pedido_serializado) {
	t_pedido_asignar_variable_compartida* pedido = malloc(sizeof(t_pedido_asignar_variable_compartida));

	int offset=0;
	memcpy(&pedido->pid,(void*)pedido_serializado,sizeof(int32_t));
	offset+=sizeof(pedido->pid);
	memcpy(&pedido->tamanio,(void*)pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(pedido->tamanio);
	pedido->nombre_variable_compartida= malloc(pedido->tamanio+2);//uno para el \0 otro para el !
	pedido->nombre_variable_compartida[0]='!';
	pedido->nombre_variable_compartida[pedido->tamanio+1]='\0';
	memcpy(pedido->nombre_variable_compartida+1,(void*)pedido_serializado+offset,pedido->tamanio);//El +1 por el !
	offset+=pedido->tamanio;
	memcpy(&pedido->valor_variable_compartida,(void*)pedido_serializado+offset,sizeof(int32_t));

	return pedido;
}

char* serializar_pedido_asignar_variable_compartida(t_pedido_asignar_variable_compartida* pedido) {
	char* buffer = malloc(sizeof(int32_t)*3+pedido->tamanio);

	int offset=0;
	memcpy(buffer,&(pedido->pid),sizeof(pedido->pid));
	offset+=sizeof(pedido->pid);
	memcpy(buffer+offset,&(pedido->tamanio),sizeof(pedido->tamanio));
	offset+=sizeof(pedido->tamanio);
	memcpy(buffer+offset,pedido->nombre_variable_compartida,pedido->tamanio);
	offset+=pedido->tamanio;
	memcpy(buffer+offset,&(pedido->valor_variable_compartida),sizeof(int32_t));

	return buffer;
}


void destruir_pcb (t_pcb *pcbADestruir){
	int i;

	if(pcbADestruir==NULL)
		return;

	free(pcbADestruir->indice_codigo); //hago este free por que se asigno la memoria con un unico malloc
	free(pcbADestruir->indice_etiquetas);
	for(i=0; i <(pcbADestruir->cant_entradas_indice_stack) ; i++){
		if(pcbADestruir->indice_stack[i].cant_argumentos)
			free(pcbADestruir->indice_stack[i].argumentos);
		if(pcbADestruir->indice_stack[i].cant_variables)
			free(pcbADestruir->indice_stack[i].variables);
	}
	if(pcbADestruir->cant_entradas_indice_stack>0)
		free(pcbADestruir->indice_stack);

	free(pcbADestruir);
	pcbADestruir = NULL;
}

char* serializar_respuesta_validar_archivo(t_respuesta_validar_archivo* rta)
{
	char* buffer = malloc(sizeof(t_respuesta_validar_archivo));

	memcpy(buffer, &(rta->codigoRta), sizeof(rta->codigoRta));

	return buffer;
}

t_respuesta_validar_archivo* deserializar_respuesta_validar_archivo(char* rta)
{
	t_respuesta_validar_archivo* respuesta = malloc(sizeof(t_respuesta_validar_archivo));

	memcpy(&respuesta->codigoRta,(void*)rta,sizeof(codigo_validar_archivo));

	return respuesta;
}

char* serializar_respuesta_crear_archivo(t_respuesta_crear_archivo* rta)
{
	char *buffer = malloc(sizeof(t_respuesta_crear_archivo));

	memcpy(buffer,&(rta->codigoRta),sizeof(rta->codigoRta));

	return buffer;
}

t_respuesta_crear_archivo* deserializar_respuesta_crear_archivo(char* rta)
{
	t_respuesta_crear_archivo* respuesta = malloc(sizeof(t_respuesta_crear_archivo));

	memcpy(&respuesta->codigoRta,(void*)rta,sizeof(codigo_crear_archivo));

	return respuesta;
}

char* serializar_pedido_signal(t_pedido_signal* pedido_deserializado){
	char *buffer = malloc((sizeof(int32_t)*2)+pedido_deserializado->tamanio);
	int offset =0;

	memcpy(buffer,&(pedido_deserializado->pid),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&(pedido_deserializado->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,pedido_deserializado->semId,pedido_deserializado->tamanio);

	return buffer;
}

t_pedido_signal* deserializar_pedido_signal(char* pedido_serializado){
	t_pedido_signal* pedido = malloc(sizeof(t_pedido_signal));
	int offset =0;

	memcpy(&(pedido->pid),pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&(pedido->tamanio),pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->semId=malloc(pedido->tamanio+1);
	pedido->semId[pedido->tamanio]='\0';
	memcpy(pedido->semId,pedido_serializado+offset,pedido->tamanio);

	return pedido;
}

char* serializar_respuesta_signal(t_respuesta_signal* respuesta_deserializada){
	char* buffer = malloc(sizeof(t_respuesta_signal));
	memcpy(buffer,&(respuesta_deserializada->respuesta),sizeof(int32_t));

	return buffer;
}

t_respuesta_signal* deserializar_respuesta_signal(char* respuesta_serializada){
	t_respuesta_signal* respuesta = malloc(sizeof(t_respuesta_signal));
	memcpy(&(respuesta->respuesta),respuesta_serializada,sizeof(int32_t));
	return respuesta;
}

char* serializar_pedido_wait(t_pedido_wait* pedido_deserializado){
	char* buffer = malloc(sizeof(int32_t)+pedido_deserializado->tamanio+tamanio_pcb(pedido_deserializado->pcb));
	int offset=0;

	memcpy(buffer,&(pedido_deserializado->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,pedido_deserializado->semId,pedido_deserializado->tamanio);
	offset+=pedido_deserializado->tamanio;
	t_pcb_serializado* pcb = serializar_pcb(pedido_deserializado->pcb);
	memcpy(buffer+offset,pcb->contenido_pcb,pcb->tamanio);

	free(pcb->contenido_pcb);
	free(pcb);
	return buffer;
}

t_pedido_wait* deserializar_pedido_wait(char* pedido_serializado){
	t_pedido_wait* pedido = malloc(sizeof(t_pedido_wait));
	int offset=0;

	memcpy(&(pedido->tamanio),pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->semId=malloc(pedido->tamanio+1);
	pedido->semId[pedido->tamanio]='\0';
	memcpy(pedido->semId,pedido_serializado+offset,pedido->tamanio);
	offset+=pedido->tamanio;
	pedido->pcb = deserializar_pcb(pedido_serializado+offset);

	return pedido;
}

char* serializar_respuesta_wait(t_respuesta_wait* respuesta_deserializada){
	char* buffer = malloc(sizeof(t_respuesta_wait));
	memcpy(buffer,&(respuesta_deserializada->respuesta),sizeof(int32_t));

	return buffer;
}

t_respuesta_wait* deserializar_respuesta_wait(char* respuesta_serializada){
	t_respuesta_wait* respuesta = malloc(sizeof(t_respuesta_wait));
	memcpy(&(respuesta->respuesta),respuesta_serializada,sizeof(int32_t));
	return respuesta;
}

char* serializar_pedido_reservar(t_pedido_reservar* pedido){
	char* buffer = malloc(sizeof(t_pedido_reservar));

	int offset=0;
	memcpy(buffer,&pedido->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->paginasTotales,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->bytes,sizeof(int32_t));

	return buffer;
}

t_pedido_reservar* deserializar_pedido_reservar(char* pedido_serializado){
	t_pedido_reservar* pedido = malloc(sizeof(t_pedido_reservar));

	int offset=0;
	memcpy(&pedido->pid,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->paginasTotales,pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->bytes,pedido_serializado+offset,sizeof(int32_t));

	return pedido;
}

char* serializar_respuesta_reservar(t_respuesta_reservar* respuesta){
	char* buffer = malloc(sizeof(t_respuesta_reservar));

	int offset=0;
	memcpy(buffer,&respuesta->codigo,sizeof(codigo_respuesta_reservar));
	offset+=sizeof(codigo_respuesta_reservar);
	memcpy(buffer+offset,&respuesta->puntero,sizeof(int32_t));

	return buffer;
}

t_respuesta_reservar* deserializar_respuesta_reservar(char* respuesta_serializada){
	t_respuesta_reservar* respuesta = malloc(sizeof(t_respuesta_reservar));

	int offset=0;
	memcpy(&respuesta->codigo,respuesta_serializada,sizeof(codigo_respuesta_reservar));
	offset+=sizeof(codigo_respuesta_reservar);
	memcpy(&respuesta->puntero,respuesta_serializada+offset,sizeof(int32_t));

	return respuesta;
}

char* serializar_pedido_liberar(t_pedido_liberar* pedido){
	char* buffer = malloc(sizeof(t_pedido_liberar));

	int offset=0;

	memcpy(buffer,&pedido->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->pagina,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->offset,sizeof(int32_t));

	return buffer;
}

t_pedido_liberar* deserializar_pedido_liberar(char* pedido_serializado){

	t_pedido_liberar* pedido = malloc(sizeof(t_pedido_liberar));

	int offset=0;

	memcpy(&pedido->pid,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->pagina,pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->offset,pedido_serializado+offset,sizeof(int32_t));

	return pedido;
}

char* serializar_respuesta_liberar(t_respuesta_liberar* respuesta){
	char* buffer = malloc(sizeof(t_respuesta_liberar));

	memcpy(buffer,&respuesta->codigo,sizeof(codigo_respuesta_liberar));

	return buffer;
}

t_respuesta_liberar* deserializar_respuesta_liberar(char* respuesta_serializada){
	t_respuesta_liberar* respuesta = malloc(sizeof(t_respuesta_liberar));

	memcpy(&respuesta->codigo,respuesta_serializada,sizeof(codigo_respuesta_liberar));

	return respuesta;
}


t_pedido_liberar_pagina* deserializar_pedido_liberar_pagina(char *pedido_serializado){
	t_pedido_liberar_pagina* pedido = malloc(sizeof(t_pedido_liberar_pagina));

	int offset=0;
	memcpy(&pedido->pagina,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->pid,pedido_serializado+offset,sizeof(int32_t));

	return pedido;
}

char* serializar_pedido_liberar_pagina(t_pedido_liberar_pagina *pedido){
	char* buffer = malloc(sizeof(t_pedido_liberar_pagina));

	int offset=0;
	memcpy(buffer,&pedido->pagina,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->pid,sizeof(int32_t));

	return buffer;
}

char* serializar_respuesta_borrar_archivo(t_respuesta_borrar_archivo* rta)
{
	char *buffer = malloc(sizeof(t_respuesta_borrar_archivo));

	memcpy(buffer,&(rta->codigoRta),sizeof(rta->codigoRta));


	return buffer;
}

t_respuesta_liberar_pagina* deserializar_respuesta_liberar_pagina(char *respuesta_serializado){
	t_respuesta_liberar_pagina* respuesta = malloc(sizeof(t_respuesta_liberar_pagina));

	memcpy(&respuesta->codigo,respuesta_serializado,sizeof(codigo_respuesta_reservar));

	return respuesta;
}

char* serializar_respuesta_liberar_pagina(t_respuesta_liberar_pagina *respuesta){
	char* buffer = malloc(sizeof(t_respuesta_liberar_pagina));

	memcpy(buffer,&respuesta->codigo,sizeof(codigo_respuesta_liberar));

	return buffer;
}

char* serializar_pedido_validar_crear_borrar_archivo(t_pedido_validar_crear_borrar_archivo_fs* pedido){
	char *buffer = malloc(sizeof(int32_t)+pedido->tamanio);
	int offset =0;

	memcpy(buffer,&(pedido->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,pedido->direccion,pedido->tamanio);

	return buffer;
}

t_pedido_validar_crear_borrar_archivo_fs* deserializar_pedido_validar_crear_borrar_archivo(char* pedido_serializado){
	t_pedido_validar_crear_borrar_archivo_fs* pedido = malloc(sizeof(t_pedido_validar_crear_borrar_archivo_fs));
	int offset =0;

	memcpy(&(pedido->tamanio),pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->direccion=malloc(pedido->tamanio+1);
	pedido->direccion[pedido->tamanio]='\0';
	memcpy(pedido->direccion,pedido_serializado+offset,pedido->tamanio);

	return pedido;
}

t_pedido_abrir_archivo* deserializar_pedido_abrir_archivo(char* pedido_serializado){
	t_pedido_abrir_archivo* pedido = malloc(sizeof(t_pedido_abrir_archivo));
	int offset = 0;

	memcpy(&pedido->pid,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->flags=malloc(sizeof(t_banderas));
	memcpy(&pedido->flags->creacion,pedido_serializado+offset,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(&pedido->flags->escritura,pedido_serializado+offset,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(&pedido->flags->lectura,pedido_serializado+offset,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(&pedido->tamanio,pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->direccion=malloc(pedido->tamanio+1);
	pedido->direccion[pedido->tamanio]='\0';
	memcpy(pedido->direccion,pedido_serializado+offset,pedido->tamanio);

	return pedido;
}

char* serializar_pedido_abrir_archivo(t_pedido_abrir_archivo *pedido){
	char* buffer = malloc(sizeof(int32_t)*2+sizeof(t_banderas)+pedido->tamanio);
	int offset = 0;

	memcpy(buffer,&pedido->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->flags->creacion,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(buffer+offset,&pedido->flags->escritura,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(buffer+offset,&pedido->flags->lectura,sizeof(bool));
	offset+=sizeof(bool);
	memcpy(buffer+offset,&(pedido->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,pedido->direccion,pedido->tamanio);

	return buffer;
}

t_respuesta_borrar_archivo* deserializar_respuesta_borrar_archivo(char* rta)
{
	t_respuesta_borrar_archivo* respuesta = malloc(sizeof(t_respuesta_borrar_archivo));

	memcpy(&respuesta->codigoRta,(void*)rta,sizeof(codigo_borrar_archivo));

	return respuesta;
}

char* serializar_respuesta_abrir_archivo(t_respuesta_abrir_archivo* respuesta){
	char* buffer = malloc(sizeof(t_respuesta_abrir_archivo));

	int offset=0;
	memcpy(buffer,&respuesta->fd,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&respuesta->codigo,sizeof(codigo_respuesta_abrir));

	return buffer;
}

t_respuesta_abrir_archivo* deserializar_respuesta_abrir_archivo(char* rta){
	t_respuesta_abrir_archivo* respuesta = malloc(sizeof(t_respuesta_abrir_archivo));
	int offset = 0;

	memcpy(&respuesta->fd,rta,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&respuesta->codigo,rta+offset,sizeof(codigo_respuesta_abrir));

	return respuesta;
}

char* serializar_respuesta_pedido_lectura(t_respuesta_pedido_lectura* rta)
{
	char *buffer = malloc(sizeof(int32_t)+rta->tamanio+sizeof(codigo_respuesta_pedido_lectura));

	int offset = 0;
	memcpy(buffer,&(rta->tamanio),sizeof(int32_t));
	offset+=sizeof(rta->tamanio);
	memcpy(buffer,&(rta->codigo),sizeof(codigo_respuesta_pedido_lectura));
	offset+=sizeof(rta->codigo);
	memcpy(buffer+offset, rta->datos, rta->tamanio);


	return buffer;
}

t_respuesta_pedido_lectura* deserializar_respuesta_pedido_lectura(char* rta)
{
	t_respuesta_pedido_lectura* respuesta = malloc(sizeof(t_respuesta_pedido_lectura));

	int offset = 0;
	memcpy(&respuesta->tamanio,(void*)rta,sizeof(int32_t));
	offset+=sizeof(respuesta->tamanio);
	memcpy(&respuesta->codigo,(void*)rta,sizeof(codigo_respuesta_pedido_lectura));
	offset+=sizeof(respuesta->codigo);
	memcpy(respuesta->datos, (void*)rta+offset, respuesta->tamanio);

	return respuesta;
}

t_pedido_cerrar_archivo* deserializar_pedido_cerrar_archivo(char* pedido_serializado){
	t_pedido_cerrar_archivo* pedido = malloc(sizeof(t_pedido_cerrar_archivo));
	int offset = 0;

	memcpy(&pedido->pid,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->fd,pedido_serializado+offset,sizeof(int32_t));

	return pedido;
}

char* serializar_pedido_cerrar_archivo(t_pedido_cerrar_archivo *pedido){
	char* buffer = malloc(sizeof(t_pedido_cerrar_archivo));

	int offset=0;
	memcpy(buffer,&pedido->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->fd,sizeof(int32_t));

	return buffer;
}

t_pedido_borrar_archivo* deserializar_pedido_borrar_archivo(char* pedido_serializado){
	t_pedido_borrar_archivo* pedido = malloc(sizeof(t_pedido_borrar_archivo));
	int offset = 0;

	memcpy(&pedido->pid,pedido_serializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->fd,pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&(pedido->tamanio),pedido_serializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->direccion=malloc(pedido->tamanio+1);
	pedido->direccion[pedido->tamanio]='\0';
	memcpy(pedido->direccion,pedido_serializado+offset,pedido->tamanio);

	return pedido;
}


char* serializar_pedido_borrar_archivo(t_pedido_borrar_archivo *pedido){
	char *buffer = malloc((sizeof(int32_t)*3)+pedido->tamanio);
	int offset =0;

	memcpy(buffer,&pedido->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&pedido->fd,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&(pedido->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,pedido->direccion,pedido->tamanio);

	return buffer;
}

char* serializar_respuesta_cerrar_archivo(t_respuesta_cerrar_archivo* rta){
	char *buffer = malloc(sizeof(t_respuesta_cerrar_archivo));

	memcpy(buffer,&(rta->codigoRta),sizeof(int32_t));

	return buffer;
}

t_respuesta_cerrar_archivo* deserializar_respuesta_cerrar_archivo(char* rta){
	t_respuesta_cerrar_archivo* respuesta = malloc(sizeof(t_respuesta_cerrar_archivo));

	memcpy(&respuesta->codigoRta,rta,sizeof(codigo_cerrar_archivo));

	return respuesta;
}
char* serializar_pedido_lectura_datos(t_pedido_lectura_datos *pedido){
	char *buffer = malloc(sizeof(int32_t)*3+pedido->tamanio);

	int offset=0;
	memcpy(buffer,&(pedido->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&(pedido->offset),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&(pedido->tamanioRuta),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset, pedido->ruta, pedido->tamanioRuta);

	return buffer;
}
t_pedido_lectura_datos* deserializar_pedido_lectura_datos(char *pedidoSerializado){
	t_pedido_lectura_datos* pedido = malloc(sizeof(t_pedido_lectura_datos));

	int offset=0;
	memcpy(&pedido->tamanio,(void*)pedidoSerializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->offset,(void*)pedidoSerializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->tamanioRuta,(void*)pedidoSerializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->ruta = malloc(pedido->tamanioRuta);
	memcpy(pedido->ruta, (void*)pedidoSerializado+offset, pedido->tamanioRuta);

	return pedido;
}

t_pedido_escritura_datos* deserializar_pedido_escritura_datos(char *pedidoSerializado){
	t_pedido_escritura_datos* pedido = malloc(sizeof(t_pedido_escritura_datos));

	int offset=0;
	memcpy(&pedido->tamanio,(void*)pedidoSerializado,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->offset,(void*)pedidoSerializado+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pedido->tamanioRuta, (void*)pedidoSerializado+offset, sizeof(int32_t));
	offset+=sizeof(int32_t);
	pedido->ruta = malloc(pedido->tamanioRuta+1);
	pedido->ruta[pedido->tamanioRuta]='\0';
	memcpy(pedido->ruta, (void*)pedidoSerializado+offset, pedido->tamanioRuta);
	offset+=pedido->tamanioRuta;
	pedido->buffer = malloc(pedido->tamanio);
	memcpy(pedido->buffer, (void*)pedidoSerializado+offset, pedido->tamanio);

	return pedido;
}

char* serializar_pedido_escritura_datos(t_pedido_escritura_datos *pedido){
	char *buffer = malloc(sizeof(int32_t)*3 + pedido->tamanio + pedido->tamanioRuta);

	int offset=0;
	memcpy(buffer,&(pedido->tamanio),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset,&(pedido->offset),sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset, &(pedido->tamanioRuta), sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(buffer+offset, pedido->ruta, pedido->tamanioRuta);
	offset+=pedido->tamanioRuta;
	memcpy(buffer+offset, pedido->buffer, pedido->tamanio);

	return buffer;
}

char* serializar_respuesta_pedido_escritura(t_respuesta_pedido_escritura* rta)
{
	char *buffer = malloc(sizeof(t_respuesta_pedido_escritura));

	int offset = 0;
	memcpy(buffer,&(rta->codigoRta),sizeof(int32_t));

	return buffer;
}

t_respuesta_pedido_escritura* deserializar_respuesta_pedido_escritura(char* rta)
{
	t_respuesta_pedido_escritura* respuesta = malloc(sizeof(t_respuesta_pedido_escritura));

	int offset = 0;
	memcpy(&respuesta->codigoRta,(void*)rta,sizeof(int32_t));

	return respuesta;
}
