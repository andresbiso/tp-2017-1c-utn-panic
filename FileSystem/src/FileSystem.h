/*
 * FileSystem.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <panicommons/panisocket.h>
#include <panicommons/serializacion.h>
#include <commons/config.h>

//#define foreach(item, array) \
//    for(int keep = 1, \
//            count = 0,\
//            size = sizeof (array) / sizeof *(array); \
//        keep && count != size; \
//        keep = !keep, count++) \
//      for(item = (array) + count; keep; keep = !keep)

typedef struct
{
	int32_t tamanio;
	char** bloques;
} t_metadata_archivo;

int puerto;
char * punto_montaje;
char* rutaBloques;

void mostrarMensaje(char* mensajes,int socket);
int validarArchivo(char* ruta);
void crearArchivo(char* ruta);
void borrarArchivo(char* ruta);
void leerDatosArchivo(t_pedido_datos_fs datosFs);

#endif /* FILESYSTEM_H_ */
