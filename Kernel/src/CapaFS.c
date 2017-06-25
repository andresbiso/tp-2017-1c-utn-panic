/*
 * CapaFS.c
 *
 *  Created on: 10/6/2017
 *      Author: utnso
 */

#include "CapaFS.h"

int obtenerEIncrementarGlobalFD(){

	int	globalfd;

	pthread_mutex_lock(&mutexGlobalFD);
	globalfd = ultimoGlobalFD;
	ultimoGlobalFD++;
	pthread_mutex_unlock(&mutexGlobalFD);

	return globalfd;
}

void abrirArchivo(char* data, int socket){
	t_pedido_abrir_archivo* pedido = deserializar_pedido_abrir_archivo(data);

	if(pedido->direccion[pedido->tamanio-1]=='\n')
		pedido->direccion[pedido->tamanio-1]='\0';

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivos = dictionary_get(tablaArchivosPorProceso,pidKey);
	int32_t posinicial = listaArchivos == NULL ? 0 : list_size(listaArchivos);

	if(listaArchivos==NULL){
		listaArchivos = list_create();
		dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivos);
	}

	free(pidKey);

	t_pedido_validar_crear_borrar_archivo_fs pedidoValidar;
	pedidoValidar.tamanio = pedido->tamanio;
	pedidoValidar.direccion = malloc((pedido->tamanio)+1);
	memcpy(pedidoValidar.direccion,pedido->direccion,pedidoValidar.tamanio);

	char* buffer = serializar_pedido_validar_crear_borrar_archivo(&pedidoValidar);
	empaquetarEnviarMensaje(socketFS,"VALIDAR_ARCH",sizeof(int32_t)+pedidoValidar.tamanio,buffer);
	free(buffer);

	t_package* paqueteValidar = recibirPaquete(socketFS,NULL);
	t_respuesta_validar_archivo* respuestaValidar = deserializar_respuesta_validar_archivo(paqueteValidar->datos);
	borrarPaquete(paqueteValidar);

	switch (respuestaValidar->codigoRta){
		case VALIDAR_OK:
			log_info(logNucleo,"Validacion correcta de archivo");

			bool matchFile(void *archivoGlobal) {
					return strcmp(((t_archivos_global*)archivoGlobal)->file,pedido->direccion)==0;
			}

			char* pid = string_itoa(pedido->pid);

			char banderas[2];
			if(pedido->flags->lectura && pedido->flags->escritura){
				strcpy(banderas,"rw");
			}else if(pedido->flags->escritura){
				strcpy(banderas,"w");
			}else strcpy(banderas,"r");

			t_archivos_global* archivoGlobal = list_find(tablaArchivosGlobales,matchFile);

			t_archivos_global* archivos_global= malloc(sizeof(t_archivos_global));

			t_archivos_proceso* archivos_proceso= malloc(sizeof(t_archivos_proceso));

			t_respuesta_abrir_archivo respuesta;

			if(archivoGlobal){
				archivoGlobal->open++;

				archivos_proceso->fd = posinicial + FD_START;
				archivos_proceso->flags = malloc(sizeof(banderas)+1);
				archivos_proceso->flags[sizeof(banderas)] = '\0';
				memcpy(archivos_proceso->flags,banderas,sizeof(banderas));
				archivos_proceso->globalFD = archivoGlobal->globalFD;
				archivos_proceso->cursor = 0;
				list_add(listaArchivos,archivos_proceso);

				respuesta.fd = archivos_proceso->fd;
				respuesta.codigo = ABRIR_OK;
				list_add(tablaArchivosGlobales,archivoGlobal);
			}else{
				archivos_global->globalFD = obtenerEIncrementarGlobalFD();
				archivos_global->file = malloc(strlen(pedido->direccion));
				memcpy(archivos_global->file,pedido->direccion,strlen(pedido->direccion));
				archivos_global->open = 1;

				archivos_proceso->fd = posinicial + FD_START;
				archivos_proceso->flags = malloc(sizeof(banderas)+1);
				archivos_proceso->flags[sizeof(banderas)] = '\0';
				memcpy(archivos_proceso->flags,banderas,sizeof(banderas));
				archivos_proceso->globalFD = archivos_global->globalFD;
				archivos_proceso->cursor = 0;
				list_add(tablaArchivosGlobales,archivos_global);

				respuesta.fd = archivos_proceso->fd;
				respuesta.codigo = ABRIR_OK;
				list_add(listaArchivos,archivos_proceso);
			}

			free(pid);

			char* buffer = serializar_respuesta_abrir_archivo(&respuesta);
			empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
			free(buffer);
			break;
		case NO_EXISTE_ARCHIVO:
			if(pedido->flags->creacion){
				log_info(logNucleo,"No existe el archivo, se intentara crearlo");
				t_pedido_validar_crear_borrar_archivo_fs pedidoCrear;
				pedidoCrear.tamanio = pedido->tamanio;
				pedidoCrear.direccion = malloc((pedido->tamanio)+1);
				memcpy(pedidoCrear.direccion,pedido->direccion,pedidoCrear.tamanio);

				char* buffer = serializar_pedido_validar_crear_borrar_archivo(&pedidoCrear);
				empaquetarEnviarMensaje(socketFS,"CREAR_ARCH",sizeof(int32_t)+pedidoCrear.tamanio,buffer);
				free(buffer);

				t_package* paqueteCrear = recibirPaquete(socketFS,NULL);
				t_respuesta_crear_archivo* respuestaCrear = deserializar_respuesta_crear_archivo(paqueteCrear->datos);
				borrarPaquete(paqueteCrear);

				char* pid = string_itoa(pedido->pid);

				char banderas[2];
				if(pedido->flags->lectura && pedido->flags->escritura){
					strcpy(banderas,"rw");
				}else if(pedido->flags->escritura){
					strcpy(banderas,"w");
				}else strcpy(banderas,"r");

				t_archivos_global* archivos_global;
				archivos_global = malloc(sizeof(t_archivos_global));

				t_archivos_proceso* archivos_proceso;
				archivos_proceso = malloc(sizeof(t_archivos_proceso));

				t_respuesta_abrir_archivo respuesta;

				switch(respuestaCrear->codigoRta){
					case CREAR_OK:
						log_info(logNucleo,"Creacion correcta de archivo");
						archivos_global->globalFD = obtenerEIncrementarGlobalFD();
						archivos_global->file = pedido->direccion;
						archivos_global->open = 1;

						archivos_proceso->fd = posinicial + FD_START;
						archivos_proceso->flags = malloc(sizeof(banderas)+1);
						archivos_proceso->flags[sizeof(banderas)] = '\0';
						memcpy(archivos_proceso->flags,banderas,sizeof(banderas));
						archivos_proceso->globalFD = archivos_global->globalFD;
						archivos_proceso->cursor = 0;
						list_add(tablaArchivosGlobales,archivos_global);

						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_OK;
						list_add(listaArchivos,archivos_proceso);

						buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						break;
					case CREAR_ERROR:
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ERROR_ABRIR;

						buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						log_error(logNucleo,"No se pudo crear el archivo");
						break;
					case NO_HAY_BLOQUES:
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ERROR_ABRIR;

						buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						log_error(logNucleo,"No hay bloques disponibles para crear el archivo");
						break;
				}
				free(pid);
				free(respuestaCrear);
			}else{
				log_error(logNucleo,"No existe el archivo solicitado");
			}
			break;
	}

	pthread_mutex_unlock(&capaFSMutex);

	free(respuestaValidar);
	free(pedido->direccion);
	free(pedido->flags);
	free(pedido);
}

void cerrarArchivo(char* data, int socket){
	t_pedido_cerrar_archivo* pedido = deserializar_pedido_cerrar_archivo(data);

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	t_respuesta_cerrar_archivo respuesta;

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_get(listaArchivosPorProceso,pedido->fd);
		t_archivos_global* archivo_global = list_get(tablaArchivosGlobales,archivo_proceso->globalFD);

		archivo_global->open--;

		if(archivo_global->open == 0){
			list_remove_and_destroy_element(tablaArchivosGlobales,archivo_proceso->globalFD,free);
		}

		list_remove_and_destroy_element(listaArchivosPorProceso,pedido->fd,free);

		respuesta.codigoRta = CERRAR_OK;
		log_info(logNucleo,"Se cerro el archivo abierto por el PID: %d con FD: %d",pedido->pid,pedido->fd);
	}else{
		respuesta.codigoRta = ERROR_CERRAR;
		log_info(logNucleo,"El archivo a cerrar nunca fue abierto por el PID: %d",pedido->pid);
	}

	pthread_mutex_unlock(&capaFSMutex);

	char* buffer = serializar_respuesta_cerrar_archivo(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_CERRAR_ARCH",sizeof(t_respuesta_cerrar_archivo),buffer);
	free(buffer);

	free(pedido);
	free(pidKey);
}

void borrarArchivo(char* data, int socket){
	t_pedido_borrar_archivo* pedido = deserializar_pedido_borrar_archivo(data);

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_get(listaArchivosPorProceso,pedido->fd);

		if(archivo_proceso != NULL){

			t_archivos_global* archivo_global = list_get(tablaArchivosGlobales,archivo_proceso->globalFD);
			if(archivo_global->open == 1){
				t_pedido_validar_crear_borrar_archivo_fs pedidoBorrar;
				pedidoBorrar.tamanio = strlen(archivo_global->file);
				pedidoBorrar.direccion = malloc(pedidoBorrar.tamanio);
				memcpy(pedidoBorrar.direccion,archivo_global->file,pedidoBorrar.tamanio);

				char* buffer = serializar_pedido_validar_crear_borrar_archivo(&pedidoBorrar);
				empaquetarEnviarMensaje(socketFS,"BORRAR_ARCH",sizeof(int32_t)+pedidoBorrar.tamanio,buffer);
				free(buffer);

				t_package* paqueteBorrar = recibirPaquete(socketFS,NULL);
				t_respuesta_borrar_archivo* respuestaBorrar = deserializar_respuesta_borrar_archivo(paqueteBorrar->datos);
				borrarPaquete(paqueteBorrar);

				switch(respuestaBorrar->codigoRta){
					case BORRAR_OK:

						list_remove_and_destroy_element(tablaArchivosGlobales,archivo_proceso->globalFD,free);
						log_info(logNucleo,"Se borro correctamente el archivo GLOBAL FD:%d ",archivo_proceso->globalFD);

						list_remove_and_destroy_element(listaArchivosPorProceso,pedido->fd,free);

						break;
					case BORRAR_ERROR:
						log_error(logNucleo,"No se pudo borrar el archivo GLOBAL FD:%d",archivo_global->globalFD);
						break;
				}
				free(respuestaBorrar);
			}else{
				log_error(logNucleo,"El archivo GLOBAL FD:%d se encuentra abierto por mas de un proceso",archivo_global->globalFD);
			}
		}else{
			log_error(logNucleo,"El archivo a borrar nunca fue abierto por el PID: %d",pedido->pid);
		}
	}else{
		log_error(logNucleo,"El archivo a borrar nunca fue abierto por el PID: %d",pedido->pid);
	}
	pthread_mutex_unlock(&capaFSMutex);

	//TODO respuesta a CPU

	free(pedido);
	free(pidKey);
}
