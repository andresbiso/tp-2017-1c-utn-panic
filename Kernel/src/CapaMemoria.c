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
		pedido_memoria.tamanio=tamanio_pag_memoria;
		pedido_memoria.pagina=paginas_proceso->maxPaginas;
		pedido_memoria.data=malloc(tamanio_pag_memoria);
		memset(pedido_memoria.data,'\0',tamanio_pag_memoria);


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
			free(respuesta_alm);
			return false;
		}else{
			t_pagina_heap* pagina = malloc(sizeof(t_pagina_heap));
			pagina->espacioDisponible=tamanio_pag_memoria-sizeof(t_heap_metadata);
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

bool tryAllocate(t_pedido_reservar* pedido,t_respuesta_reservar* respuesta,t_pagina_heap* pag_heap){
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

	int offset=0;

	while(offset<tamanio_pag_memoria){
		t_heap_metadata metadata;
		memcpy(&metadata.isFree,(rta_sol_bytes->data)+offset,sizeof(bool));
		offset+=sizeof(bool);
		memcpy(&metadata.size,(rta_sol_bytes->data)+offset,sizeof(int32_t));
		offset+=sizeof(int32_t);

		if(metadata.isFree && (metadata.size > pedido->bytes )){
			int oldOffset=offset-sizeof(t_heap_metadata);
			int diff=metadata.size-pedido->bytes;//La diferencia de bytes entre el pedido y lo disponible

			respuesta->puntero=(pag_heap->nroPagina*tamanio_pag_memoria)+offset;
			respuesta->codigo=RESERVAR_OK;

			t_heap_metadata newMetadata;
			newMetadata.isFree=false;
			newMetadata.size=(diff>=sizeof(t_heap_metadata)?pedido->bytes:metadata.size);

			memcpy((rta_sol_bytes->data)+oldOffset,&newMetadata.isFree,sizeof(bool));//Escribimos la nueva metadata
			oldOffset+=sizeof(bool);
			memcpy((rta_sol_bytes->data)+oldOffset,&newMetadata.size,sizeof(int32_t));
			oldOffset+=sizeof(int32_t);

			if(diff>=sizeof(t_heap_metadata)){//Esto es para poner el flag al final
				t_heap_metadata lastMetadata;
				lastMetadata.isFree=true;
				lastMetadata.size=metadata.size-newMetadata.size-sizeof(t_heap_metadata);//Se resta uno para el flag

				memcpy((rta_sol_bytes->data)+oldOffset+newMetadata.size,&lastMetadata.isFree,sizeof(bool));//Escribimos la nueva metadata
				memcpy((rta_sol_bytes->data)+oldOffset+1+newMetadata.size,&lastMetadata.size,sizeof(int32_t));

				pag_heap->espacioDisponible-=(newMetadata.size+sizeof(t_heap_metadata));
			}else{
				pag_heap->espacioDisponible-=newMetadata.size;
			}


			t_pedido_almacenar_bytes pedido_memoria;
			pedido_memoria.pid=pedido->pid;
			pedido_memoria.offsetPagina=0;
			pedido_memoria.tamanio=tamanio_pag_memoria;
			pedido_memoria.pagina=pag_heap->nroPagina;
			pedido_memoria.data=rta_sol_bytes->data;

			char* buffer = serializar_pedido_almacenar_bytes(&pedido_memoria);
			empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",sizeof(int32_t)*4+pedido_memoria.tamanio,buffer);
			free(buffer);

			t_package* paquete_alm = recibirPaqueteMemoria();
			t_respuesta_almacenar_bytes* respuesta_alm = deserializar_respuesta_almacenar_bytes(paquete_alm->datos);
			borrarPaquete(paquete_alm);

			if(respuesta_alm->codigo != OK_ALMACENAR){
				free(respuesta_alm);
				free(rta_sol_bytes->data);
				free(rta_sol_bytes);

				return false;
			}

			free(respuesta_alm);
			free(rta_sol_bytes->data);
			free(rta_sol_bytes);

			return true;
		}

		offset+=metadata.size;

	}

	free(rta_sol_bytes->data);
	free(rta_sol_bytes);
	return false;
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
		char* pidKey = string_itoa(pedido->pid);
		t_paginas_proceso* paginas_proceso = dictionary_get(paginasGlobalesHeap,pidKey);

		bool pageWithNoSpace(void* elem){
			return ((t_pagina_heap*)elem)->espacioDisponible<pedido->bytes;
		}

		if(paginas_proceso == NULL || list_all_satisfy(paginas_proceso->paginas,pageWithNoSpace)){//Si no hay paginas o no hay ninguna con espacio
			if (!pedirPaginaHeap(paginas_proceso,pedido->paginasTotales, pedido->pid,pidKey)){
				respuesta.puntero = -1;
				respuesta.codigo = RESERVAR_SIN_ESPACIO;
			}
		}

		if (paginas_proceso == NULL){
			paginas_proceso = dictionary_get(paginasGlobalesHeap,pidKey);
		}

		if(respuesta.puntero != -1){

			bool pageWithSpace(void* elem){
				return ((t_pagina_heap*)elem)->espacioDisponible>pedido->bytes;
			}

			int index=0;
			t_list* all_pages_with_space = list_filter(paginas_proceso->paginas,pageWithSpace);
			t_pagina_heap* pag_heap = list_get(all_pages_with_space,index);

			bool pageWrite = false;

			while(!pageWrite){
				pageWrite = tryAllocate(pedido,&respuesta,pag_heap);
				if(!pageWrite){
					index++;
					pag_heap = list_get(all_pages_with_space,index);
					if (pag_heap==NULL && !pedirPaginaHeap(paginas_proceso,pedido->paginasTotales, pedido->pid,pidKey)){
						respuesta.puntero = -1;
						respuesta.codigo = RESERVAR_SIN_ESPACIO;
						break;
					}
				}
			}

			list_clean(all_pages_with_space);
		}

		free(pidKey);
	}

	if(respuesta.puntero==-1){
		log_info(logNucleo,"No se pudo reservar memoria del pedido del socket:%d por el PID:%d BYTES:%d",socket,pedido->pid,pedido->bytes);
	}else{
		log_info(logNucleo,"Memoria reservada del pedido del socket:%d por el PID:%d BYTES:%d",socket,pedido->pid,pedido->bytes);
	}

	char* buffer = serializar_respuesta_reservar(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_RESERVAR",sizeof(t_respuesta_reservar),buffer);
	free(buffer);

	free(pedido);
}

bool compressPageHeap(char* page,int32_t pid,int32_t pagina){//retorna un booleano si salió bien la compresion
	t_heap_metadata metadata;

	int32_t startMetadata = 0;
	bool anterior_is_free=false;
	int32_t anterior_offset=0;

	while(startMetadata<tamanio_pag_memoria){
		memcpy(&metadata.isFree,page+startMetadata,sizeof(bool));
		memcpy(&metadata.size,page+1+startMetadata,sizeof(int32_t));

		if(metadata.isFree && anterior_is_free){
			metadata.isFree=true;
			int32_t anteriorSize=0;
			memcpy(&anteriorSize,page+1+anterior_offset,sizeof(int32_t));

			metadata.size+=(anteriorSize+sizeof(t_heap_metadata));//Con el size de metadata porque junta de a dos

			memset(page+anterior_offset+sizeof(t_heap_metadata),0,metadata.size);

			memcpy(page+anterior_offset,&metadata.isFree,sizeof(bool));
			memcpy(page+anterior_offset+1,&metadata.size,sizeof(int32_t));

			startMetadata=anterior_offset;
		}

		anterior_is_free=metadata.isFree;
		anterior_offset=startMetadata;

		startMetadata+=(metadata.size+sizeof(t_heap_metadata));
	}

	//Me fijo si la pagina quedó totalmente libre
	memcpy(&metadata.isFree,page,sizeof(bool));
	memcpy(&metadata.size,page+1,sizeof(int32_t));

	if(metadata.size==(tamanio_pag_memoria-sizeof(t_heap_metadata))){//Quedo toda liberada
		t_pedido_liberar_pagina pedido_liberar;
		pedido_liberar.pagina=pagina;
		pedido_liberar.pid=pid;

		char * buffer = serializar_pedido_liberar_pagina(&pedido_liberar);
		empaquetarEnviarMensaje(socketMemoria,"LIBERAR_PAG",sizeof(t_pedido_liberar_pagina),buffer);
		free(buffer);

		t_package* paquete = recibirPaqueteMemoria();
		t_respuesta_liberar_pagina* respuesta_liberar = deserializar_respuesta_liberar_pagina(paquete->datos);
		borrarPaquete(paquete);

		if(respuesta_liberar->codigo!=OK_LIBERAR){//Fallo por algo
			free(respuesta_liberar);
			return false;
		}
	}else{//Hay espacio alocado en algun lado => Escribimos la pagina a memoria
		t_pedido_almacenar_bytes pedido_memoria;
		pedido_memoria.pid=pid;
		pedido_memoria.offsetPagina=0;
		pedido_memoria.tamanio=tamanio_pag_memoria;
		pedido_memoria.pagina=pagina;
		pedido_memoria.data=page;

		char* buffer = serializar_pedido_almacenar_bytes(&pedido_memoria);
		empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",sizeof(int32_t)*4+pedido_memoria.tamanio,buffer);
		free(buffer);

		t_package* paquete_alm = recibirPaqueteMemoria();
		t_respuesta_almacenar_bytes* respuesta_alm = deserializar_respuesta_almacenar_bytes(paquete_alm->datos);
		borrarPaquete(paquete_alm);

		if(respuesta_alm->codigo != OK_ALMACENAR){
			free(respuesta_alm);
			return false;
		}
	}
	return true;
}

void liberar(void* data,int socket){
	t_pedido_liberar* pedido = deserializar_pedido_liberar(data);
	t_respuesta_liberar respuesta;

	log_info(logNucleo,"Se recibio un pedido de liberar memoria del socket:%d por el PID:%d PAG:%d OFFSET:%d",socket,pedido->pid,pedido->pagina,pedido->offset);

	if (pedido->offset<tamanio_pag_memoria){//Por si se va de offset
		t_pedido_solicitar_bytes pedido_sol_bytes;
		pedido_sol_bytes.pid=pedido->pid;
		pedido_sol_bytes.pagina=pedido->pagina;
		pedido_sol_bytes.offsetPagina=0;
		pedido_sol_bytes.tamanio=tamanio_pag_memoria;

		char* buffer = serializar_pedido_solicitar_bytes(&pedido_sol_bytes);
		empaquetarEnviarMensaje(socketMemoria,"SOLC_BYTES",sizeof(t_pedido_solicitar_bytes),buffer);
		free(buffer);

		t_package* paquete = recibirPaqueteMemoria();
		t_respuesta_solicitar_bytes* rta_sol_bytes = deserializar_respuesta_solicitar_bytes(paquete->datos);
		borrarPaquete(paquete);

		if(rta_sol_bytes->codigo!=OK_SOLICITAR){//Por si el pedido no es correcto
			respuesta.codigo=LIBERAR_ERROR;
		}else{
			t_heap_metadata metadata;
			int32_t startMetadata = pedido->offset-sizeof(t_heap_metadata);

			memcpy(&metadata.isFree,(rta_sol_bytes->data)+startMetadata,sizeof(bool));
			memcpy(&metadata.size,(rta_sol_bytes->data)+1+startMetadata,sizeof(int32_t));

			if(!metadata.isFree){
				metadata.isFree=true;
				memcpy((rta_sol_bytes->data)+startMetadata,&metadata.isFree,sizeof(bool));//No hace falta sobre-escribir el size ya que por ahora es el mismo
				memset((rta_sol_bytes->data)+pedido->offset,0,metadata.size);

				bool compressSuccess = compressPageHeap(rta_sol_bytes->data,pedido->pid,pedido->pagina);
				if(compressSuccess){
					respuesta.codigo=LIBERAR_OK;
				}else{
					respuesta.codigo=LIBERAR_ERROR;
				}
			}else{
				respuesta.codigo=LIBERAR_ERROR;//Ya estaba liberada
			}
		}
		free(rta_sol_bytes->data);
		free(rta_sol_bytes);
	}else{
		respuesta.codigo=LIBERAR_ERROR;
	}

	if(respuesta.codigo==LIBERAR_ERROR){
		log_info(logNucleo,"No se pudo liberar memoria del pedido del socket:%d por el PID:%d PAG:%d OFFSET:%d",socket,pedido->pid,pedido->pagina,pedido->offset);
	}else{
		log_info(logNucleo,"Memoria liberada del pedido del socket:%d por el PID:%d PAG:%d OFFSET:%d",socket,pedido->pid,pedido->pagina,pedido->offset);
	}

	char* buffer = serializar_respuesta_liberar(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_LIBERAR",sizeof(t_respuesta_liberar),buffer);
	free(buffer);

	free(pedido);
}

//Capa de memoria
