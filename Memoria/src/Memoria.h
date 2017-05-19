/*
 * Memoria.h
 *
 *  Created on: 1/4/2017
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <panicommons/panisocket.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <pthread.h>
#include <panicommons/paniconsole.h>
#include <panicommons/panicommons.h>
#include <panicommons/serializacion.h>
#include <math.h>

int puerto;
int marcos;
int32_t marcoSize;
int entradasCache;
int cacheXproc;
int retardoMemoria;
char* bloqueMemoria;
char* bloqueCache;
t_list* cacheEntradas;
t_log* logFile;
t_log* logDumpFile;
pthread_mutex_t mutexCache;
pthread_mutex_t mutexMemoriaPrincipal;

typedef struct{
	int32_t pid;
	int32_t nroPagina;
	int32_t entradas;
}t_cache_admin;

typedef struct{
	int32_t pid;
	int32_t nroPagina;
	char* contenido;
}t_cache;

typedef struct{
	int32_t indice;
	int32_t pid;
	int32_t numeroPag;
}t_pagina;

#define TAM_ELM_TABLA_INV 12

#endif /* MEMORIA_H_ */
