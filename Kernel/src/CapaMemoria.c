/*
 * CapaMemoria.c
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#include "CapaMemoria.h"
//Capa de memoria

void getVariableCompartida(char* data, int socket){
	t_pedido_variable_compartida* pedido = deserializar_pedido_variable_compartida(data);

	log_info(logNucleo,"Se recibió un mensaje de la CPU:%d por el PID:%d para obtener la variable:%s",socket,pedido->pid,pedido->nombre_variable_compartida);

	t_respuesta_variable_compartida respuesta;

	if(!dictionary_has_key(variablesCompartidas,pedido->nombre_variable_compartida)){
		log_info(logNucleo,"Variable:%s no encontrada",pedido->nombre_variable_compartida);
		respuesta.valor_variable_compartida=-1;
		respuesta.codigo=ERROR_VARIABLE;
	}else{
		log_info(logNucleo,"Variable:%s encontrada",pedido->nombre_variable_compartida);

		respuesta.valor_variable_compartida=*((int32_t*)dictionary_get(variablesCompartidas,pedido->nombre_variable_compartida));
		respuesta.codigo=OK_VARIABLE;
	}

	char* buffer = serializar_respuesta_variable_compartida(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_VARIABLE",sizeof(t_respuesta_variable_compartida),buffer);

	free(buffer);
	free(pedido->nombre_variable_compartida);
	free(pedido);
}

void setVariableCompartida(char* data, int socket){


}

void wait(char* data,int socket){


}

void signal(char* data,int socket){


}


//Capa de memoria
