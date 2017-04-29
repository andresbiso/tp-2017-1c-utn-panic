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
#include <math.h>

int puerto;
int marcos;
int marcoSize;
int entradasCache;
int cacheXproc;
int retardoMemoria;
char* bloqueMemoria;
char* bloqueCache;
t_log* logFile;
pthread_mutex_t mutexLog;
pthread_mutex_t mutexCache;
pthread_mutex_t mutexMemoriaPrincipal;

typedef struct{
	int32_t indice;
	int32_t pid;
	int32_t numeroPag;
}t_pagina;

#define TAM_ELM_TABLA_INV 12

#endif /* MEMORIA_H_ */
