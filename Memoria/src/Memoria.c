#include <stdio.h>
#include <stdlib.h>

#include <panicommons/panisocket.h>
#include <commons/config.h>
#include "Memoria.h"

void mostrarMensaje(char* mensaje,int socket){
	printf("Mensaje recibido: %s \n",mensaje);
}

t_config* cargarConfiguracion(char * nombreArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreArchivo);
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
	if (config_has_property(configFile, "RETARDO_MEMORIA")) {
		retardoMemoria = config_get_int_value(configFile, "RETARDO_MEMORIA");
	} else {
		perror("La key RETARDO_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}
	return configFile;
}

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);

	printf("PUERTO: %d\n",puerto);
	printf("MARCOS: %d\n",marcos);
	printf("MARCO_SIZE: %d\n",marcoSize);
	printf("ENTRADAS_CACHE: %d\n",entradasCache);
	printf("CACHE_X_PROC: %d\n",cacheXproc);
	printf("RETARDO_MEMORIA: %d\n",retardoMemoria);

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);

	t_dictionary* diccionarioHandshakes = dictionary_create();
	dictionary_put(diccionarioHandshakes,"HCPME","HMECP");
	dictionary_put(diccionarioHandshakes,"HKEME","HMEKE");

	int socket = crearHostMultiConexion(puerto);
	correrServidorThreads(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);

	config_destroy(configFile);

	return EXIT_SUCCESS;
}
