/*
 ============================================================================
 Name        : Memoria.c
 Author      : mpicollo
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include <panicommons/panisocket.h>
#include <commons/config.h>
#include "Memoria.h"

void mostrarMensaje(char** mensajes){
	printf("Mensaje recibido: %s \n",mensajes[0]);

	free(mensajes[0]);
	free(mensajes);
}

void cargarConfiguracion(char * nombreArchivo){
	int configFileSize = strlen("/home/utnso/workspace/tp-2017-1c-utn-panic/Memoria/Debug/") + strlen(nombreArchivo);
	char* configFilePath = (char *)malloc(configFileSize * sizeof(char));
	strcat(configFilePath, "/home/utnso/workspace/tp-2017-1c-utn-panic/Memoria/Debug/");
	strcat(configFilePath, nombreArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);

	if (config_has_property(configFile, "PUERTO")) {
		puerto = config_get_int_value(configFile, "PUERTO");
	} else {
		perror("La key PUERTO no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "MARCOS")) {
		marcos = config_get_int_value(configFile, "MARCOS");
	} else {
		perror("La key MARCOS no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "MARCO_SIZE")) {
		marcoSize = config_get_int_value(configFile, "MARCO_SIZE");
	} else {
		perror("La key MARCO_SIZE no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "ENTRADAS_CACHE")) {
		entradasCache = config_get_int_value(configFile, "ENTRADAS_CACHE");
	} else {
		perror("La key ENTRADAS_CACHE no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "CACHE_X_PROC")) {
		cacheXproc = config_get_int_value(configFile, "CACHE_X_PROC");
	} else {
		perror("La key CACHE_X_PROC no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "REEMPLAZO_CACHE")) {
		reemplazoCache = config_get_string_value(configFile, "REEMPLAZO_CACHE");
	} else {
		perror("La key REEMPLAZO_CACHE no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "RETARDO_MEMORIA")) {
		retardoMemoria = config_get_int_value(configFile, "RETARDO_MEMORIA");
	} else {
		perror("La key RETARDO_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}
	config_destroy(configFile);
}

int main(int argc, char** argv) {

	cargarConfiguracion(argv[1]);

	printf("PUERTO: %d\n",puerto);
	printf("MARCOS: %d\n",marcos);
	printf("MARCO_SIZE: %d\n",marcoSize);
	printf("ENTRADAS_CACHE: %d\n",entradasCache);
	printf("CACHE_X_PROC: %d\n",cacheXproc);
	printf("REEMPLAZO_CACHE: %s\n",reemplazoCache);
	printf("RETARDO_MEMORIA: %d\n",retardoMemoria);

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);

	t_dictionary* diccionarioHandshakes = dictionary_create();
	dictionary_put(diccionarioHandshakes,"HCPME","HMECP");
	dictionary_put(diccionarioHandshakes,"HKEME","HMEKE");

	int socket = crearHostMultiConexion(puerto);
	correrServidorMultiConexion(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);

	return EXIT_SUCCESS;
}
