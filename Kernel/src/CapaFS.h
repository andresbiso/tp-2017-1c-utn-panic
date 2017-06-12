/*
 * CapaFS.h
 *
 *  Created on: 10/6/2017
 *      Author: utnso
 */

#ifndef SRC_CAPAFS_H_
#define SRC_CAPAFS_H_

#include "estados.h"

#define FD_START 3

t_dictionary* tablaArchivosPorProceso;
t_dictionary* tablaArchivosGlobales;
t_list* listaArchivosPorProceso;
t_list* listaArchivosGlobal;

int ultimoGlobalFD;

typedef struct {
	int32_t fd;
	char* flags;
	int32_t globalFD;
	int32_t cursor;
}t_archivos_proceso;

typedef struct {
	int32_t globalFD;
	char* file;
	int32_t open;
}t_archivos_global;

void abrirArchivo(char* data, int socket);

#endif /* SRC_CAPAFS_H_ */
