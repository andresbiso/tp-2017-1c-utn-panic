/*
 * CapaFS.c
 *
 *  Created on: 10/6/2017
 *      Author: utnso
 */

#include "CapaFS.h"

int obtenerEIncrementarGlobalFD()
{
	int	globalfd;

	pthread_mutex_lock(&mutexGlobalFD);
	globalfd = ultimoGlobalFD;
	ultimoGlobalFD++;
	pthread_mutex_unlock(&mutexGlobalFD);

	return globalfd;
}

void abrirArchivo(char* data, int socket){
	t_pedido_abrir_archivo* pedido = deserializar_pedido_abrir_archivo(data);

	t_archivos_proceso* archivos_proceso;
	archivos_proceso = malloc(sizeof(t_archivos_proceso));

	t_archivos_global* archivos_global;
	archivos_global = malloc(sizeof(t_archivos_global));

	char* pidKey = string_itoa(pedido->pid);

	t_list* listaArchivos = dictionary_get(tablaArchivosPorProceso,pidKey);
	int posinicial = listaArchivos == NULL ? 0 : list_size(listaArchivosPorProceso);

    char banderas[2];
	if(pedido->flags->lectura && pedido->flags->escritura){
		strcpy(banderas,"rw");
	}else if(pedido->flags->escritura){
		strcpy(banderas,"w");
	}else strcpy(banderas,"r");

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
							  return ((t_archivos_global*)archivoGlobal)->file == pedido->direccion;
							}

			t_archivos_global* archivoGlobal = list_find(tablaArchivosGlobales,matchFile);

			if(archivoGlobal){
				archivoGlobal->open++;

				archivos_proceso->fd = posinicial + FD_START;
				archivos_proceso->flags = banderas;
				archivos_proceso->globalFD = archivoGlobal->globalFD;
				archivos_proceso->cursor = 0;
				list_add(listaArchivosPorProceso,archivos_proceso);
				dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
				pthread_mutex_lock(&listaArchivosGlobalMutex);
				list_add(tablaArchivosGlobales,archivoGlobal);
				pthread_mutex_unlock(&listaArchivosGlobalMutex);
			}else{
				archivos_global->globalFD = obtenerEIncrementarGlobalFD();
				archivos_global->file = pedido->direccion;
				archivos_global->open = 1;
				pthread_mutex_lock(&listaArchivosGlobalMutex);
				list_add(tablaArchivosGlobales,archivos_global);
				pthread_mutex_unlock(&listaArchivosGlobalMutex);

				archivos_proceso->fd = posinicial + FD_START;
				archivos_proceso->flags = banderas;
				archivos_proceso->globalFD = archivos_global->globalFD;
				archivos_proceso->cursor = 0;
				list_add(listaArchivosPorProceso,archivos_proceso);
				dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
			}

			t_respuesta_abrir_archivo respuesta;
			respuesta.fd = archivos_proceso->fd;
			respuesta.codigo = ABRIR_OK;

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

				t_respuesta_abrir_archivo respuesta;

				switch(respuestaCrear->codigoRta){
					case CREAR_OK:
						log_info(logNucleo,"Creacion correcta de archivo");
						archivos_global->globalFD = obtenerEIncrementarGlobalFD();
						archivos_global->file = pedido->direccion;
						archivos_global->open = 1;
						pthread_mutex_lock(&listaArchivosGlobalMutex);
						list_add(tablaArchivosGlobales,archivos_global);
						pthread_mutex_unlock(&listaArchivosGlobalMutex);

						archivos_proceso->fd = posinicial + FD_START;
						archivos_proceso->flags = banderas;
						archivos_proceso->globalFD = archivos_global->globalFD;
						archivos_proceso->cursor = 0;
						list_add(listaArchivosPorProceso,archivos_proceso);
						dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);

						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_OK;

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
				free(respuestaCrear);
			}else{
			log_error(logNucleo,"No existe el archivo solicitado");
			}
			break;
	}

	free(respuestaValidar);
	free(pidKey);
	free(pedido->direccion);
	free(pedido);
}

void cerrarArchivo(char* data, int socket){
	t_pedido_cerrar_archivo* pedido = deserializar_pedido_cerrar_archivo(data);

	char* pidKey = string_itoa(pedido->pid);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	t_respuesta_cerrar_archivo respuesta;

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_get(listaArchivosPorProceso,pedido->fd);
		t_archivos_global* archivo_global = list_get(tablaArchivosGlobales,archivo_proceso->globalFD);

		archivo_global->open--;

		pthread_mutex_lock(&listaArchivosGlobalMutex);
		archivo_global->open == 0 ? list_remove_and_destroy_element(tablaArchivosGlobales,archivo_proceso->globalFD,free) :
				list_replace_and_destroy_element(tablaArchivosGlobales,archivo_proceso->globalFD,archivo_global,free);
		pthread_mutex_unlock(&listaArchivosGlobalMutex);

		list_remove_and_destroy_element(listaArchivosPorProceso,pedido->fd,free);
		dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);

		respuesta.codigoRta = CERRAR_OK;
		log_info(logNucleo,"Se cerro el archivo abierto por el PID: %d con FD: %d",pedido->pid,pedido->fd);
	}else{
		respuesta.codigoRta = ERROR_CERRAR;
		log_info(logNucleo,"El archivo a cerrar nunca fue abierto por el PID: %d",pedido->pid);
	}

	char* buffer = serializar_respuesta_cerrar_archivo(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_CERRAR_ARCH",sizeof(t_respuesta_cerrar_archivo),buffer);
	free(buffer);

	free(pedido);
	free(pidKey);
}

void borrarArchivo(char* data, int socket){
	t_pedido_borrar_archivo* pedido = deserializar_pedido_borrar_archivo(data);

	char* pidKey = string_itoa(pedido->pid);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_get(listaArchivosPorProceso,pedido->fd);
		t_archivos_global* archivo_global = list_get(tablaArchivosGlobales,archivo_proceso->globalFD);

		if(archivo_global->globalFD == 1){
			t_pedido_validar_crear_borrar_archivo_fs pedidoBorrar;
			pedidoBorrar.tamanio = pedido->tamanio;
			pedidoBorrar.direccion = malloc((pedido->tamanio)+1);
			memcpy(pedidoBorrar.direccion,pedido->direccion,pedidoBorrar.tamanio);

			char* buffer = serializar_pedido_validar_crear_borrar_archivo(&pedidoBorrar);
			empaquetarEnviarMensaje(socketFS,"BORRAR_ARCH",sizeof(int32_t)+pedidoBorrar.tamanio,buffer);
			free(buffer);

			t_package* paqueteBorrar = recibirPaquete(socketFS,NULL);
			t_respuesta_crear_archivo* respuestaBorrar = deserializar_respuesta_crear_archivo(paqueteBorrar->datos);
			borrarPaquete(paqueteBorrar);

			switch(respuestaBorrar->codigoRta){
				case BORRAR_OK:
					pthread_mutex_lock(&listaArchivosGlobalMutex);
					list_remove_and_destroy_by_condition(tablaArchivosGlobales,archivo_proceso->globalFD,free);
					pthread_mutex_unlock(&listaArchivosGlobalMutex);

					list_remove_by_condition(listaArchivosPorProceso,pedido->fd);
					dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
					log_info(logNucleo,"Se borro correctamente el archivo");
					break;
				case BORRAR_ERROR:
					log_error(logNucleo,"No se pudo borrar el archivo");
					break;
			}
			free(respuestaBorrar);
		}else{
			log_error(logNucleo,"El archivo se encuentra abierto por mas de un proceso");
		}
	}else{
		log_error(logNucleo,"El archivo a borrar nunca fue abierto por el PID: %d",pedido->pid);
	}

	free(pedido->direccion);
	free(pedido);
	free(pidKey);
}