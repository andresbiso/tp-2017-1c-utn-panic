/*
 * CapaMemoria.c
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#include "CapaMemoria.h"
//Capa de memoria

void getVariableCompartida(char* data, int socket){
	t_pedido_obtener_variable_compartida* pedido = deserializar_pedido_obtener_variable_compartida(data);

	if(pedido->nombre_variable_compartida[pedido->tamanio]=='\n')
		pedido->nombre_variable_compartida[pedido->tamanio]='\0';

	log_info(logNucleo,"Se recibio un mensaje de la CPU:%d por el PID:%d para obtener la variable:%s",socket,pedido->pid,pedido->nombre_variable_compartida);

	t_respuesta_obtener_variable_compartida respuesta;

	if(!dictionary_has_key(variablesCompartidas,pedido->nombre_variable_compartida)){
		log_info(logNucleo,"Variable:%s no encontrada",pedido->nombre_variable_compartida);
		respuesta.valor_variable_compartida=-1;
		respuesta.codigo=ERROR_VARIABLE;
	}else{
		log_info(logNucleo,"Variable:%s encontrada",pedido->nombre_variable_compartida);

		respuesta.valor_variable_compartida=*((int32_t*)dictionary_get(variablesCompartidas,pedido->nombre_variable_compartida));
		respuesta.codigo=OK_VARIABLE;
	}

	char* buffer = serializar_respuesta_obtener_variable_compartida(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_VARIABLE",sizeof(t_respuesta_obtener_variable_compartida),buffer);

	free(buffer);
	free(pedido->nombre_variable_compartida);
	free(pedido);
}

void setVariableCompartida(char* data, int socket){
	t_pedido_asignar_variable_compartida* pedido = deserializar_pedido_asignar_variable_compartida(data);

	if(pedido->nombre_variable_compartida[pedido->tamanio]=='\n')
		pedido->nombre_variable_compartida[pedido->tamanio]='\0';

	log_info(logNucleo,"Se recibio un mensaje de la CPU:%d por el PID:%d para asignar la variable:%s",socket,pedido->pid,pedido->nombre_variable_compartida);

	t_respuesta_asignar_variable_compartida respuesta;

	if(!dictionary_has_key(variablesCompartidas,pedido->nombre_variable_compartida)){
		log_info(logNucleo,"Variable:%s no encontrada",pedido->nombre_variable_compartida);
		respuesta.codigo=ERROR_ASIGNAR_VARIABLE;
	}else{
		log_info(logNucleo,"Variable:%s encontrada",pedido->nombre_variable_compartida);

		int32_t* valorActual = ((int32_t*)dictionary_get(variablesCompartidas,pedido->nombre_variable_compartida));
		*valorActual=pedido->valor_variable_compartida;
		respuesta.codigo=OK_ASIGNAR_VARIABLE;
	}

	char* buffer = serializar_respuesta_asignar_variable_compartida(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_ASIG_VARIABLE",sizeof(t_respuesta_asignar_variable_compartida),buffer);

	free(buffer);
	free(pedido->nombre_variable_compartida);
	free(pedido);
}

void wait(char* data,int socket){
	t_pedido_wait* pedido = deserializar_pedido_wait(data);
	t_respuesta_wait respuesta;

	if(pedido->semId[pedido->tamanio-1]=='\n')
		pedido->semId[pedido->tamanio-1]='\0';

	log_info(logNucleo,"Pedido de WAIT en SEM:%s",pedido->semId);

	t_semaforo* sem = dictionary_get(semaforos,pedido->semId);

	if(sem==NULL){
		log_info(logNucleo,"El SEM:%s no existe",pedido->semId);
		respuesta.respuesta=WAIT_NOT_EXIST;
	}else{
		log_info(logNucleo,"Cantidad de procesos en cola del SEM:%d",sem->valor,queue_size(sem->cola));
		sem->valor--;
		if(sem->valor <0){
			log_info(logNucleo,"El SEM:%s queda con valor:%d y se bloquea al PID:%d",pedido->semId,sem->valor,pedido->pcb->pid);
			respuesta.respuesta=WAIT_BLOCKED;
			bloquear_pcb(pedido->pcb);
			int32_t* pidBloqueado = malloc(sizeof(int32_t));
			*pidBloqueado=pedido->pcb->pid;
			queue_push(sem->cola,pidBloqueado);

			cpu_change_running(socket,false);
			program_change_running(pedido->pcb->pid,false);
		}else{
			log_info(logNucleo,"El SEM:%s queda con valor:%d y no se bloquea al PID:%d",pedido->semId,sem->valor,pedido->pcb->pid);
			respuesta.respuesta=WAIT_OK;
		}
	}
	char*buffer = serializar_respuesta_wait(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_WAIT",sizeof(t_respuesta_wait),buffer);
	free(buffer);

	if(respuesta.respuesta!=WAIT_BLOCKED)
		destruir_pcb(pedido->pcb);//Se destruye porque CPU se encarga del pcb
	free(pedido->semId);
	free(pedido);
}

void signal(char* data,int socket){
	t_pedido_signal* pedido = deserializar_pedido_signal(data);

	t_respuesta_signal respuesta;

	if(pedido->semId[pedido->tamanio-1]=='\n')
			pedido->semId[pedido->tamanio-1]='\0';

	log_info(logNucleo,"Pedido de SIGNAL en SEM:%s",pedido->semId);

	t_semaforo* sem = dictionary_get(semaforos,pedido->semId);

	if(sem==NULL){
		log_info(logNucleo,"El SEM:%s no existe",pedido->semId);
		respuesta.respuesta=SIGNAL_NOT_EXIST;
	}else{
		respuesta.respuesta=SIGNAL_OK;
		log_info(logNucleo,"Cantidad de procesos en cola del SEM:%d",sem->valor,queue_size(sem->cola));
		sem->valor++;
		log_info(logNucleo,"El SEM:%s queda con valor:%d",pedido->semId,sem->valor);
		if(sem->valor >=0){
			if(queue_size(sem->cola)!=0){
				int32_t* pidADesbloquear = queue_pop(sem->cola);
				desbloquear_pcb(*pidADesbloquear);
				free(pidADesbloquear);
			}else{
				log_info(logNucleo,"El SEM:%s con cola de espera vacia");
			}
		}
	}

	char*buffer = serializar_respuesta_signal(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_SIGNAL",sizeof(t_respuesta_signal),buffer);
	free(buffer);

	free(pedido->semId);
	free(pedido);
}

void reservar(void* data,int socket){
	t_pedido_reservar* pedido = deserializar_pedido_reservar(data);

	t_respuesta_reservar respuesta;

	log_info(logNucleo,"Se recibio un pedido de reserva de memoria del socket:%d por el PID:%d por bytes:%d",socket,pedido->pid,pedido->bytes);

	if((pedido->bytes) > (tamanio_pag_memoria-10)){ //Pedido mayor al disponible en una pagina
		respuesta.codigo=RESERVAR_OVERFLOW;
	}else{
		//TODO
	}

	char* buffer = serializar_respuesta_reservar(&respuesta);
	empaquetarEnviarMensaje(socket,"RET_RESERVAR",sizeof(t_respuesta_reservar),buffer);
	free(buffer);

	free(pedido);
}

void liberar(void* data,int socket){
	//TODO
}

//Capa de memoria
