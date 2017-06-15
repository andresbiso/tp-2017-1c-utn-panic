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

	if(respuesta.respuesta==WAIT_BLOCKED)
		enviar_a_cpu();
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
		if(sem->valor <=0){
			if(queue_size(sem->cola)!=0){
				int32_t* pidADesbloquear = queue_pop(sem->cola);
				desbloquear_pcb(*pidADesbloquear);
				free(pidADesbloquear);
			}else{
				log_info(logNucleo,"El SEM:%s con cola de espera vacia",pedido->semId);
			}
		}
	}

	char * buffer = serializar_respuesta_signal(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_SIGNAL",sizeof(t_respuesta_signal),buffer);
	free(buffer);

	free(pedido->semId);
	free(pedido);

}

bool pedirPaginaHeap(t_paginas_proceso* paginas_proceso, int paginasTotales, int pid,char* pidKey){//Devuelve si pudo o no asignar una nueva pagina
	if(paginas_proceso==NULL){
		paginas_proceso= malloc(sizeof(t_paginas_proceso));
		paginas_proceso->maxPaginas=paginasTotales-1;//Para incrementarlo despues (para el caso que tiene paginas o no)
		paginas_proceso->paginas = list_create();
		dictionary_put(paginasGlobalesHeap,pidKey,paginas_proceso);
	}

	t_pedido_inicializar pedido_memoria;
	pedido_memoria.pagRequeridas=1;
	pedido_memoria.idPrograma=pid;

	char* buffer = serializar_pedido_inicializar(&pedido_memoria);
	empaquetarEnviarMensaje(socketMemoria,"ASIG_PAGES",sizeof(t_pedido_inicializar),buffer);
	free(buffer);

	t_package* paquete_asig = recibirPaqueteMemoria();
	t_respuesta_inicializar* respuesta_memoria = deserializar_respuesta_inicializar(paquete_asig->datos);
	borrarPaquete(paquete_asig);

	if(respuesta_memoria->codigoRespuesta == OK_INICIALIZAR){
		free(respuesta_memoria);
		paginas_proceso->maxPaginas++;

		t_pedido_almacenar_bytes pedido_memoria;
		pedido_memoria.pid=pid;
		pedido_memoria.offsetPagina=0;
		pedido_memoria.tamanio=sizeof(t_heap_metadata);
		pedido_memoria.pagina=paginas_proceso->maxPaginas;
		pedido_memoria.data=malloc(sizeof(t_heap_metadata));

		t_heap_metadata metadata;
		metadata.isFree=1;
		metadata.size=(tamanio_pag_memoria-sizeof(t_heap_metadata));

		memcpy(pedido_memoria.data,(void*)&(metadata.isFree),sizeof(bool));
		memcpy((pedido_memoria.data+sizeof(bool)),(void*)&(metadata.size),sizeof(int32_t));

		char* buffer = serializar_pedido_almacenar_bytes(&pedido_memoria);
		empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",sizeof(int32_t)*4+pedido_memoria.tamanio,buffer);
		free(buffer);
		free(pedido_memoria.data);

		t_package* paquete_alm = recibirPaqueteMemoria();
		t_respuesta_almacenar_bytes* respuesta_alm = deserializar_respuesta_almacenar_bytes(paquete_alm->datos);
		borrarPaquete(paquete_alm);

		if(respuesta_alm->codigo != OK_ALMACENAR){
			return false;
		}else{
			t_pagina_heap* pagina = malloc(sizeof(t_pagina_heap));
			pagina->espacioContiguoDisponible=tamanio_pag_memoria-sizeof(t_heap_metadata);
			pagina->nroPagina=paginas_proceso->maxPaginas;
			list_add(paginas_proceso->paginas,pagina);
		}
		free(respuesta_alm);
		return true;
	}else{
		free(respuesta_memoria);
		return false;
	}
}

void reservar(void* data,int socket){
	t_pedido_reservar* pedido = deserializar_pedido_reservar(data);

	t_respuesta_reservar respuesta;
	respuesta.puntero=0;

	log_info(logNucleo,"Se recibio un pedido de reserva de memoria del socket:%d por el PID:%d por bytes:%d",socket,pedido->pid,pedido->bytes);

	if((pedido->bytes) > (tamanio_pag_memoria-(sizeof(t_heap_metadata)*2))){ //Pedido mayor al disponible en una pagina
		respuesta.codigo=RESERVAR_OVERFLOW;
		respuesta.puntero=-1;
	}else{
		//TODO
		char* pidKey = string_itoa(pedido->pid);
		t_paginas_proceso* paginas_proceso = dictionary_get(paginasGlobalesHeap,pidKey);

		bool pageWithNoSpace(void* elem){
			return ((t_pagina_heap*)elem)->espacioContiguoDisponible<(pedido->bytes+5);
		}

		if(paginas_proceso == NULL || list_all_satisfy(paginas_proceso->paginas,pageWithNoSpace)){//Si no hay paginas o no hay ninguna con espacio
			if (!pedirPaginaHeap(paginas_proceso,pedido->paginasTotales, pedido->pid,pidKey)){
				respuesta.puntero = -1;
				respuesta.codigo = RESERVAR_SIN_ESPACIO;
			}else if (paginas_proceso == NULL){
				paginas_proceso = dictionary_get(paginasGlobalesHeap,pidKey);
			}
		}

		if(respuesta.puntero != -1){

			bool pageWithSpace(void* elem){
				return ((t_pagina_heap*)elem)->espacioContiguoDisponible>(pedido->bytes+sizeof(t_heap_metadata));
			}

			t_pagina_heap* pag_heap = list_find(paginas_proceso->paginas,pageWithSpace);

			t_pedido_solicitar_bytes pedido_sol_bytes;
			pedido_sol_bytes.pid=pedido->pid;
			pedido_sol_bytes.pagina=pag_heap->nroPagina;
			pedido_sol_bytes.offsetPagina=0;
			pedido_sol_bytes.tamanio=tamanio_pag_memoria;

			char* buffer = serializar_pedido_solicitar_bytes(&pedido_sol_bytes);
			empaquetarEnviarMensaje(socketMemoria,"SOLC_BYTES",sizeof(t_pedido_solicitar_bytes),buffer);
			free(buffer);

			t_package* paquete_sol_bytes = recibirPaqueteMemoria();
			t_respuesta_solicitar_bytes* rta_sol_bytes = deserializar_respuesta_solicitar_bytes(paquete_sol_bytes->datos);
			borrarPaquete(paquete_sol_bytes);

			//En este punto ya debería haber espacio en la pagina del heap

			int32_t newMaxEspacioContiguo=0;

			int offset=0;
			bool pageFound=false;

			while(offset<tamanio_pag_memoria){
				t_heap_metadata metadata;
				memcpy(&metadata.isFree,rta_sol_bytes+offset,sizeof(bool));
				offset+=sizeof(bool);
				memcpy(&metadata.size,rta_sol_bytes+offset,sizeof(int32_t));
				offset+=sizeof(int32_t);

				if(!pageFound && (metadata.size > (pedido->bytes+sizeof(t_heap_metadata)) )){
					pageFound=true;
					respuesta.puntero=(pag_heap->nroPagina*tamanio_pag_memoria)+offset;
					respuesta.puntero=RESERVAR_OK;
				}

			}

			free(rta_sol_bytes->data);
			free(rta_sol_bytes);

		}

		free(pidKey);
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
