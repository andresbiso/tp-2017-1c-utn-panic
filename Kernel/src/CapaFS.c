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

	t_pedido_validar_crear_archivo_fs pedidoValidar;
	pedidoValidar.tamanio = pedido->tamanio;
	pedidoValidar.direccion = malloc((pedido->tamanio)+1);
	memcpy(pedidoValidar.direccion,pedido->direccion,pedidoValidar.tamanio);

	char* buffer = serializar_pedido_validar_crear_archivo(&pedidoValidar);
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
				t_pedido_validar_crear_archivo_fs pedidoCrear;
				pedidoCrear.tamanio = pedido->tamanio;
				pedidoCrear.direccion = malloc((pedido->tamanio)+1);
				memcpy(pedidoCrear.direccion,pedido->direccion,pedidoCrear.tamanio);

				char* buffer = serializar_pedido_validar_crear_archivo(&pedidoCrear);
				empaquetarEnviarMensaje(socketFS,"CREAR_ARCH",sizeof(int32_t)+pedidoCrear.tamanio,buffer);
				free(buffer);

				t_package* paqueteCrear = recibirPaquete(socketFS,NULL);
				t_respuesta_crear_archivo* respuestaCrear = deserializar_respuesta_crear_archivo(paqueteCrear->datos);
				borrarPaquete(paqueteCrear);

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

						t_respuesta_abrir_archivo respuesta;
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_OK;

						char* buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						break;
					case CREAR_ERROR:
						// TODO error
						break;
					case NO_HAY_BLOQUES:
						// TODO error
						break;
				}
				free(respuestaCrear);
			}else{
			// TODO error
			}
			break;
	}

	free(respuestaValidar);
	free(pidKey);

}

void cerrarArchivo(char* data, int socket){
	t_pedido_cerrar_archivo* pedido = deserializar_pedido_cerrar_archivo(data);

	char* pidKey = string_itoa(pedido->pid);

	bool matchFile(void *archivoGlobal) {
					  return ((t_archivos_global*)archivoGlobal)->file == pedido->direccion;
					}

	t_archivos_global* archivo_global = list_find(tablaArchivosGlobales,matchFile);

	if(archivo_global){
		archivo_global->open--;

		pthread_mutex_lock(&listaArchivosGlobalMutex);
		archivo_global->open == 0 ? list_remove_and_destroy_by_condition(tablaArchivosGlobales,matchFile,free) :
				list_replace_and_destroy_element(tablaArchivosGlobales,archivo_global->globalFD,archivo_global,free);
		pthread_mutex_unlock(&listaArchivosGlobalMutex);

		t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

		bool matchGlobalFD(void *archivo_proceso) {
						return ((t_archivos_proceso*)archivo_proceso)->globalFD == archivo_global->globalFD;
					 }

		list_remove_by_condition(listaArchivosPorProceso,matchGlobalFD);
	}

	free(pedido->direccion);
	free(pedido);
	free(pidKey);
}
