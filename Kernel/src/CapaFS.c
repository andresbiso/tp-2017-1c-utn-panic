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

	t_respuesta_abrir_archivo respuesta;

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
				archivos_global->file = malloc(strlen(pedido->direccion)+1);
				archivos_global->file[strlen(pedido->direccion)]='\0';
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

						buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						break;
					case CREAR_ERROR:
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_ERROR;

						buffer = serializar_respuesta_abrir_archivo(&respuesta);
						empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
						free(buffer);
						log_error(logNucleo,"No se pudo crear el archivo");
						break;
					case NO_HAY_BLOQUES:
						respuesta.fd = archivos_proceso->fd;
						respuesta.codigo = ABRIR_ERROR;

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

				respuesta.fd = 0;
				respuesta.codigo = ABRIR_ERROR;
				buffer = serializar_respuesta_abrir_archivo(&respuesta);
				empaquetarEnviarMensaje(socket,"RES_ABRIR_ARCH",sizeof(t_respuesta_abrir_archivo),buffer);
				free(buffer);
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

	bool matchFileProceso(void* elem){
		return ((t_archivos_proceso*)elem)->fd==pedido->fd;
	}

	t_archivos_proceso* archivo_proceso=NULL;

	if(listaArchivosPorProceso != NULL){
		archivo_proceso = list_find(listaArchivosPorProceso,matchFileProceso);
	}

	if(listaArchivosPorProceso != NULL && archivo_proceso != NULL){

		bool matchFileGlobal(void* elem){
			return ((t_archivos_global*)elem)->globalFD==archivo_proceso->globalFD;
		}

		t_archivos_global* archivo_global = list_find(tablaArchivosGlobales,matchFileGlobal);

		archivo_global->open--;

		if(archivo_global->open == 0){
			list_remove_and_destroy_by_condition(tablaArchivosGlobales,matchFileGlobal,destroyArchivoGlobal);
		}

		list_remove_and_destroy_by_condition(listaArchivosPorProceso,matchFileProceso,destroyArchivoProceso);

		respuesta.codigoRta = CERRAR_OK;
		log_info(logNucleo,"Se cerro el archivo abierto por el PID: %d con FD: %d",pedido->pid,pedido->fd);
	}else{
		respuesta.codigoRta = CERRAR_ERROR;
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

	t_respuesta_borrar respuesta;

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	bool matchFileProceso(void* elem){
		return ((t_archivos_proceso*)elem)->fd==pedido->fd;
	}

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_find(listaArchivosPorProceso,matchFileProceso);

		if(archivo_proceso){

			bool matchFileGlobal(void* elem){
				return ((t_archivos_global*)elem)->globalFD==archivo_proceso->globalFD;
			}

			t_archivos_global* archivo_global = list_find(tablaArchivosGlobales,matchFileGlobal);

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
						list_remove_and_destroy_by_condition(tablaArchivosGlobales,matchFileGlobal,destroyArchivoGlobal);
						list_remove_and_destroy_by_condition(listaArchivosPorProceso,matchFileProceso,destroyArchivoProceso);
						log_info(logNucleo,"Se borro correctamente el archivo GLOBAL FD: %d ",archivo_proceso->globalFD);

						respuesta.codigo = BORRADO_OK;
						break;
					case BORRAR_ERROR:
						log_error(logNucleo,"No se pudo borrar el archivo GLOBAL FD: %d",archivo_global->globalFD);
						respuesta.codigo = BORRADO_ERROR;
						break;
				}
				free(respuestaBorrar);
			}else{
				log_error(logNucleo,"El archivo GLOBAL FD: %d se encuentra abierto por mas de un proceso",archivo_global->globalFD);
				respuesta.codigo = BORRADO_BLOCKED;
			}
		}else{
			log_error(logNucleo,"El archivo a borrar nunca fue abierto por el PID: %d",pedido->pid);
			respuesta.codigo = BORRADO_ERROR;
		}
	}else{
		log_error(logNucleo,"El archivo a borrar nunca fue abierto por el PID: %d",pedido->pid);
		respuesta.codigo = BORRADO_ERROR;
	}
	pthread_mutex_unlock(&capaFSMutex);

	char* buffer = serializar_respuesta_borrar(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_BORRAR_ARCH",sizeof(int32_t),buffer);
	free(buffer);

	free(pedido);
	free(pidKey);
}

void moverCursor(char* data, int socket){
	t_pedido_mover_cursor* pedido = deserializar_pedido_mover_cursor(data);

	t_respuesta_mover_cursor respuesta;

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	bool matchFileProceso(void* elem){
		return ((t_archivos_proceso*)elem)->fd==pedido->fd;
	}

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_find(listaArchivosPorProceso,matchFileProceso);

		if(archivo_proceso){
			archivo_proceso->cursor = pedido->posicion;
			log_info(logNucleo,"Se movio la posicion del cursor en %d posiciones",pedido->posicion);
			respuesta.codigo = MOVER_OK;
		}else{
			respuesta.codigo = MOVER_ERROR;
			log_error(logNucleo,"No se pudo mover la posicion del cursor");
		}
	}else{
		respuesta.codigo = MOVER_ERROR;
		log_error(logNucleo,"El archivo nunca fue abierto por el PID: %d",pedido->pid);
	}

	char* buffer = serializar_respuesta_mover_cursor(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_MOVER_CURSOR",sizeof(t_respuesta_mover_cursor),buffer);
	free(buffer);

	pthread_mutex_unlock(&capaFSMutex);

	free(pedido);
	free(pidKey);
}

void leerArchivo(char* data, int socket){
    t_pedido_leer* pedido = deserializar_pedido_leer_archivo(data);
 
    t_pedido_lectura_datos pedidoLectura;

    t_respuesta_leer respuesta;
 
    agregarSyscall(pedido->pid);
 
    char* pidKey = string_itoa(pedido->pid);
 
    pthread_mutex_lock(&capaFSMutex);
 
    t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);
 
	bool matchFileProceso(void* elem){
		return ((t_archivos_proceso*)elem)->fd==pedido->descriptor_archivo;
	}

    if(listaArchivosPorProceso){
        t_archivos_proceso* archivo_proceso = list_find(listaArchivosPorProceso,matchFileProceso);
 
        if(archivo_proceso){

			bool matchFileGlobal(void* elem){
				return ((t_archivos_global*)elem)->globalFD==archivo_proceso->globalFD;
			}

            t_archivos_global* archivo_global = list_find(tablaArchivosGlobales,matchFileGlobal);
 
            if(string_contains(archivo_proceso->flags,"r")){
            	pedidoLectura.tamanioRuta = strlen(archivo_global->file);
            	pedidoLectura.ruta = malloc(pedidoLectura.tamanioRuta);
            	memcpy(pedidoLectura.ruta,archivo_global->file,pedidoLectura.tamanioRuta);
            	pedidoLectura.tamanio = pedido->tamanio;
            	pedidoLectura.offset = archivo_proceso->cursor;
 
            	char* buffer = serializar_pedido_lectura_datos(&pedidoLectura);
            	empaquetarEnviarMensaje(socketFS,"LEER_ARCH",sizeof(int32_t)*3+pedidoLectura.tamanioRuta,buffer);
            	free(buffer);

            	t_package* paqueteLeer = recibirPaquete(socketFS,NULL);
            	t_respuesta_pedido_lectura* respuestaLeer = deserializar_respuesta_pedido_lectura(paqueteLeer->datos);
            	borrarPaquete(paqueteLeer);

            	switch(respuestaLeer->codigo){
            		case LECTURA_OK:
            			log_info(logNucleo,"Se leyo correctamente el archivo GLOBAL FD: %d",archivo_global->globalFD);
            			respuesta.codigo = LEER_OK;
            			respuesta.tamanio = respuestaLeer->tamanio;
            			respuesta.informacion = malloc(respuesta.tamanio);
            			memcpy(respuesta.informacion,respuestaLeer->datos,respuesta.tamanio);
            			break;
            		case LECTURA_ERROR: //TODO en CPU finalizar el proceso
            			log_error(logNucleo,"No se pudo leer el archivo GLOBAL FD: %d",archivo_global->globalFD);
            			respuesta.codigo = LEER_BLOCKED;
            			respuesta.informacion = "LEER_ERROR";
            			respuesta.tamanio = strlen("LEER_ERROR");
            			break;
            	}
            	free(respuestaLeer->datos);
            	free(respuestaLeer);
            }else{
            	respuesta.codigo = LEER_BLOCKED;
            	respuesta.informacion = "LEER_ERROR";
            	respuesta.tamanio = strlen("LEER_ERROR");
            	log_error(logNucleo,"El archivo GLOBAL FD: %d no fue abierto con permisos de lectura",archivo_global->globalFD); //TODO en CPU finalizar el proceso
            }
        }else{
        	respuesta.codigo = LEER_NO_EXISTE;
        	respuesta.informacion = "NO_EXISTE_ARCHIVO";
        	respuesta.tamanio = strlen("NO_EXISTE_ARCHIVO");
        	log_error(logNucleo,"El archivo a leer nunca fue abierto por el PID: %d",pedido->pid);
        }
    }else{
    	respuesta.codigo = LEER_NO_EXISTE;
    	respuesta.informacion = "NO_EXISTE_ARCHIVO";
    	respuesta.tamanio = strlen("NO_EXISTE_ARCHIVO");
    	log_error(logNucleo,"El archivo a leer nunca fue abierto por el PID: %d",pedido->pid);
    }

    char* buffer = serializar_respuesta_leer_archivo(&respuesta);
    empaquetarEnviarMensaje(socket,"RES_LEER_ARCH",sizeof(int32_t)*2+respuesta.tamanio,buffer);
   	free(buffer);
 
    pthread_mutex_unlock(&capaFSMutex);

    free(respuesta.informacion);
    free(pedidoLectura.ruta);
    free(pedido);
    free(pidKey);
}

void escribirArchivo(char* data, int socket){
	t_pedido_escribir* pedido = deserializar_pedido_escribir_archivo(data);

	t_pedido_escritura_datos pedidoEscritura;

	t_respuesta_escribir respuesta;

	agregarSyscall(pedido->pid);

	char* pidKey = string_itoa(pedido->pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	bool matchFileProceso(void* elem){
		return ((t_archivos_proceso*)elem)->fd==pedido->fd;
	}

	if(listaArchivosPorProceso){
		t_archivos_proceso* archivo_proceso = list_find(listaArchivosPorProceso,matchFileProceso);

	    if(archivo_proceso){

		bool matchFileGlobal(void* elem){
			return ((t_archivos_global*)elem)->globalFD==archivo_proceso->globalFD;
		}

	    t_archivos_global* archivo_global = list_find(tablaArchivosGlobales,matchFileGlobal);

	    	if(string_contains(archivo_proceso->flags,"w")){
	    		pedidoEscritura.tamanioRuta = strlen(archivo_global->file);
	        	pedidoEscritura.ruta = malloc(pedidoEscritura.tamanioRuta);
	        	memcpy(pedidoEscritura.ruta,archivo_global->file,pedidoEscritura.tamanioRuta);
	        	pedidoEscritura.tamanio = pedido->tamanio;
	        	pedidoEscritura.buffer = malloc(pedidoEscritura.tamanio);
	        	memcpy(pedidoEscritura.buffer,pedido->informacion,pedidoEscritura.tamanio);
	        	pedidoEscritura.offset = archivo_proceso->cursor;

	        	char* buffer = serializar_pedido_escritura_datos(&pedidoEscritura);
	        	empaquetarEnviarMensaje(socketFS,"ESCRIBIR_ARCH",sizeof(int32_t)*3+pedidoEscritura.tamanioRuta+pedidoEscritura.tamanio,buffer);
	        	free(buffer);

	        	t_package* paqueteLeer = recibirPaquete(socketFS,NULL);
	        	t_respuesta_pedido_escritura* respuestaEscribir = deserializar_respuesta_pedido_escritura(paqueteLeer->datos);
	        	borrarPaquete(paqueteLeer);

	        	switch(respuestaEscribir->codigoRta){
	        		case ESCRIBIR_OK:
	        			log_info(logNucleo,"Se escribio correctamente el archivo GLOBAL FD: %d",archivo_global->globalFD);
	        	 		respuesta.codigo = ESCRITURA_OK;
	        	 		break;
	        	 	case NO_HAY_ESPACIO:
	        	 		log_error(logNucleo,"No hay espacio para escribir el archivo GLOBAL FD: %d",archivo_global->globalFD);
	        	 		respuesta.codigo = ESCRITURA_SIN_ESPACIO;
	        	 		break;
	        	 	case ESCRIBIR_ERROR:
	        	 		log_error(logNucleo,"No se pudo escribir el archivo GLOBAL FD: %d",archivo_global->globalFD); //TODO en CPU finalizar el proceso
	        	 		respuesta.codigo = ESCRITURA_ERROR;
	        	 		break;
	        	}
	        	free(respuestaEscribir);
	        }else{
	        	log_error(logNucleo,"El archivo GLOBAL FD: %d no fue abierto con permisos de escritura",archivo_global->globalFD); //TODO en CPU finalizar el proceso
	        	respuesta.codigo = ESCRITURA_BLOCKED;
	        }
	    }else{
	    	log_error(logNucleo,"El archivo a escribir nunca fue abierto por el PID: %d",pedido->pid);
	    	respuesta.codigo = ESCRITURA_ERROR;
	    }
	}else{
		log_error(logNucleo,"El archivo a escribir nunca fue abierto por el PID: %d",pedido->pid);
		respuesta.codigo = ESCRITURA_ERROR;
	}

	char* buffer = serializar_respuesta_escribir_archivo(&respuesta);
	empaquetarEnviarMensaje(socket,"RES_ESCR_ARCH",sizeof(t_respuesta_escribir),buffer);
	free(buffer);

	pthread_mutex_unlock(&capaFSMutex);

	free(pedidoEscritura.buffer);
	free(pedidoEscritura.ruta);
	free(pedido->informacion);
	free(pedido);
	free(pidKey);
}

void destroyArchivoGlobal(void* elem){
	free(((t_archivos_global*)elem)->file);
	free(((t_archivos_global*)elem));
}

void destroyArchivoProceso(void* elem){
	free(((t_archivos_proceso*)elem)->flags);
	free(((t_archivos_proceso*)elem));
}

void cleanFilesOpen(int32_t pid){

	char* pidKey = string_itoa(pid);

	pthread_mutex_lock(&capaFSMutex);

	t_list* listaArchivosPorProceso = dictionary_get(tablaArchivosPorProceso,pidKey);

	if(listaArchivosPorProceso){

		int32_t archivosAbiertos=0;

		void fileDestroy(void* elem1){

			bool matchFileGlobal(void* elem){
				return ((t_archivos_global*)elem)->globalFD==((t_archivos_proceso*)elem1)->globalFD;
			}

			t_archivos_global* archivoGlobal= list_find(tablaArchivosGlobales,matchFileGlobal);

			archivoGlobal->open--;

			if(archivoGlobal->open==0){
				list_remove_and_destroy_by_condition(tablaArchivosGlobales,matchFileGlobal,destroyArchivoGlobal);
			}

			free(((t_archivos_proceso*)elem1)->flags);
			free(((t_archivos_proceso*)elem1));

			archivosAbiertos++;
		}
		list_destroy_and_destroy_elements(listaArchivosPorProceso,fileDestroy);

		dictionary_remove(tablaArchivosPorProceso,pidKey);

		log_info(logNucleo,"El PID: %d dejo abiertos %d archivos",pid,archivosAbiertos);
	}


	pthread_mutex_unlock(&capaFSMutex);

	free(pidKey);

}
