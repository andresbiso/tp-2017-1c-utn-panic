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
#include <commons/log.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

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

void _mkdir(const char *dir) {
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU);
}

typedef struct
{
	int32_t tamanio;
	int* bloques;
} t_metadata_archivo;

typedef struct
{
	int32_t tamanioBloque;
	int32_t cantidadBloques;
	char* magicNumber;
} t_metadata_fs;

typedef struct
{
	int32_t tamanio;
	char* datos;
} t_bloque;

int puerto;
char *puntoMontaje;
char* rutaBloques;
char* rutaArchivos;
char* rutaBitmap;
char* rutaMetadata;
t_metadata_fs* metadataFS;
t_bitarray* bitmap;
FILE* archivoBitmap;
t_log* logFS;

void mostrarMensaje(char* mensajes, int socket);
void validarArchivo(char* ruta, int socket);
void crearArchivo(char* ruta, int socket);
void borrarArchivo(char* ruta, int socket);
void leerDatosArchivo(t_pedido_lectura_datos* pedidoDeLectura, int socket);

#endif /* FILESYSTEM_H_ */
