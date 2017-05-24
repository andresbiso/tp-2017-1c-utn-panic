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
#include <commons/bitarray.h>

//#define foreach(item, array) \
//    for(int keep = 1, \
//            count = 0,\
//            size = sizeof (array) / sizeof *(array); \
//        keep && count != size; \
//        keep = !keep, count++) \
//      for(item = (array) + count; keep; keep = !keep)


char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

typedef struct
{
	int32_t tamanio;
	char* bloques;
} t_metadata_archivo;

typedef struct
{
	int32_t tamanioBloque;
	int32_t cantidadBloques;
	char* magicNumber;
} t_metadata_fs;

int puerto;
char *punto_montaje;
char* rutaBloques;
char* rutaArchivos;
char* rutaBitmap;
t_metadata_fs metadataFS;

void mostrarMensaje(char* mensajes, int socket);
int validarArchivo(char* ruta, int socket);
void crearArchivo(char* ruta, int socket);
void borrarArchivo(char* ruta, int socket);
void leerDatosArchivo(t_pedido_datos_fs datosFs, int socket);

#endif /* FILESYSTEM_H_ */
