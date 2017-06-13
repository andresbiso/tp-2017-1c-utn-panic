/*
 * CapaFS.c
 *
 *  Created on: 10/6/2017
 *      Author: utnso
 */

#include "CapaFS.h"

ultimoGlobalFD = 0;

void abrirArchivo(char* data, int socket){
	t_pedido_abrir_archivo* pedido = deserializar_pedido_abrir_archivo(data);

	t_archivos_proceso* archivos_proceso;
	archivos_proceso = malloc(sizeof(t_archivos_proceso));

	t_archivos_global* archivos_global;
	archivos_global = malloc(sizeof(t_archivos_global));

	char* pidKey = string_itoa(pedido->pid);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);
	int posinicial = listaArchivosPorProceso == NULL ? 0 : list_size(listaArchivosPorProceso);

	char banderas[2];
	if(pedido->flags->lectura && pedido->flags->escritura){
		strcpy(banderas,"rw");
	}else if(pedido->flags->escritura){
		strcpy(banderas,"w");
	}else strcpy(banderas,"r");

	t_pedido_validar_crear_archivo_fs pedidoValidar;
	pedidoValidar.tamanio = strlen(pedido->direccion);
	pedidoValidar.direccion = malloc(strlen(pedido->direccion)+1);
	memcpy(pedidoValidar.direccion,pedido->direccion,strlen(pedido->direccion));

	char* buffer = serializar_pedido_validar_crear_archivo(&pedidoValidar);
	empaquetarEnviarMensaje(socketFS,"VALIDAR_ARCH",sizeof(int32_t)+pedidoValidar.tamanio,buffer);
	free(buffer);

	t_package* paqueteValidar = recibirPaquete(socketFS,NULL);
	t_respuesta_validar_archivo* respuestaValidar = deserializar_respuesta_validar_archivo(paqueteValidar->datos);
	borrarPaquete(paqueteValidar);

	switch (respuestaValidar->codigoRta){
		case VALIDAR_OK:
			if(dictionary_has_key(tablaArchivosGlobales,pedido->direccion)){
				t_archivos_global* archivoGlobal = dictionary_get(tablaArchivosGlobales,pedido->direccion);
				archivoGlobal->open++;

				archivos_proceso->fd = posinicial + FD_START;
				archivos_proceso->flags = banderas;
				archivos_proceso->globalFD = archivoGlobal->globalFD;
				archivos_proceso->cursor = 0;
				list_add(listaArchivosPorProceso,archivos_proceso);
				dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
				dictionary_put(tablaArchivosGlobales,pedido->direccion,archivoGlobal);
			}

			archivos_global->globalFD = ultimoGlobalFD;
			archivos_global->file = pedido->direccion;
			archivos_global->open = 1;
			dictionary_put(tablaArchivosGlobales,pedido->direccion,archivos_global);

			archivos_proceso->fd = posinicial + FD_START;
			archivos_proceso->flags = banderas;
			archivos_proceso->globalFD = ultimoGlobalFD;
			archivos_proceso->cursor = 0;
			list_add(listaArchivosPorProceso,archivos_proceso);
			dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
			ultimoGlobalFD++;

			t_respuesta_abrir_archivo respuesta;
			respuesta.fd = archivos_proceso->fd;
			respuesta.codigo = ABRIR_OK;

			char* buffer = serializar_respuesta_abrir_archivo(&respuesta);
			empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_reservar),buffer);
			free(buffer);
			break;
		case NO_EXISTE_ARCHIVO:
			if(pedido->flags->creacion){
				t_pedido_validar_crear_archivo_fs pedidoCrear;
				pedidoCrear.tamanio = strlen(pedido->direccion);
				pedidoCrear.direccion = malloc(strlen(pedido->direccion)+1);
				memcpy(pedidoCrear.direccion,pedido->direccion,strlen(pedido->direccion));

				char* buffer = serializar_pedido_validar_crear_archivo(&pedidoCrear);
				empaquetarEnviarMensaje(socketFS,"CREAR_ARCH",sizeof(int32_t)+pedidoCrear.tamanio,buffer);
				free(buffer);

				t_package* paqueteCrear = recibirPaquete(socketFS,NULL);
				t_respuesta_crear_archivo* respuestaCrear = deserializar_respuesta_crear_archivo(paqueteCrear->datos);
				borrarPaquete(paqueteCrear);

				switch(respuestaCrear->codigoRta){
					case CREAR_OK:
						archivos_global->globalFD = ultimoGlobalFD;
						archivos_global->file = pedido->direccion;
						archivos_global->open = 1;
						dictionary_put(tablaArchivosGlobales,pedido->direccion,archivos_global);

						archivos_proceso->fd = posinicial + FD_START;
						archivos_proceso->flags = banderas;
						archivos_proceso->globalFD = ultimoGlobalFD;
						archivos_proceso->cursor = 0;
						list_add(listaArchivosPorProceso,archivos_proceso);
						dictionary_put(tablaArchivosPorProceso,pidKey,listaArchivosPorProceso);
						ultimoGlobalFD++;

						t_respuesta_abrir_archivo respuesta;
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_OK;

						char* buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_reservar),buffer);
						free(buffer);
						break;
					case CREAR_ERROR:
						// TODO error
						break;
					case NO_HAY_BLOQUES:
						// TODO error
						break;
				}

			}else{
			// TODO error
			}
			break;
	}

	free(pidKey);

}
