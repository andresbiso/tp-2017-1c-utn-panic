#include <stdio.h>
#include <stdlib.h>

#include <panicommons/panisocket.h>
#include <commons/config.h>
#include "Memoria.h"
#include <pthread.h>
#include <panicommons/paniconsole.h>

void mostrarMensaje(char* mensaje,int socket){
	printf("Error: %s \n",mensaje);
}

void dump(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de dump no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO
	printf("Do dump\n\r");

	freeElementsArray(functionAndParams,size);
}

void retardo(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de retardo debe recibir solo 1 parametro (tiempo de retardo en ms).\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO chequear concurrencia !!!
	retardoMemoria = atoi(functionAndParams[1]);
	printf("Retardo modificado a %d\n\r",retardoMemoria);

	freeElementsArray(functionAndParams,size);
}

void correrConsola(){
	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"dump",&dump);
	dictionary_put(commands,"retardo",&retardo);
	waitCommand(commands);
	dictionary_destroy(commands);
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

void iniciarPrograma(char* data,int socket){
	//TODO
}

void solicitarBytes(char* data,int socket){
	//TODO
}

void almacenarBytes(char* data,int socket){
	//TODO
}

void asignarPaginas(char* data,int socket){
	//TODO
}

void finalizarPrograma(char* data,int socket){
	//TODO
}

void crearEstructurasAdministrativas(){

	int pagAdminis = (int) (TAM_ELM_TABLA_INV*marcos)/marcoSize;
	int offset = 0;
	int i;

	for(i=0;i<pagAdminis;i++){
		memcpy(bloqueMemoria+offset,(void *)&i,TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
		offset+=TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV;
		memcpy(bloqueMemoria+offset,(void *)string_itoa(-1),TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
		offset+=TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV;
		memcpy(bloqueMemoria+offset,(void *)string_itoa(1),TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
		offset+=TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV;
	}

	return;
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

	//La memoria a utilizar es MARCO * MARCO_SIZE
	//La cantidad de marcos que ocupan las estructuras administrativas son (12*MARCOS)/MARCO_SIZE <- redondeado para arriba

	bloqueMemoria = (char*) calloc(marcos*marcoSize,sizeof(char));

	if (bloqueMemoria == NULL){
		perror("No se pudo reservar memoria para el bloque");
		exit(-1);
	}

	crearEstructurasAdministrativas();

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"INIT_PROGM",&iniciarPrograma);
	dictionary_put(diccionarioFunciones,"SOLC_BYTES",&solicitarBytes);
	dictionary_put(diccionarioFunciones,"ALMC_BYTES",&almacenarBytes);
	dictionary_put(diccionarioFunciones,"ASIG_PAGES",&asignarPaginas);
	dictionary_put(diccionarioFunciones,"FINZ_PROGM",&finalizarPrograma);

	t_dictionary* diccionarioHandshakes = dictionary_create();
	dictionary_put(diccionarioHandshakes,"HCPME","HMECP");
	dictionary_put(diccionarioHandshakes,"HKEME","HMEKE");

	pthread_t threadConsola;
	pthread_create(&threadConsola,NULL,(void *)correrConsola,NULL);

	int socket = crearHostMultiConexion(puerto);
	correrServidorThreads(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);
	config_destroy(configFile);
	free(bloqueMemoria);

	return EXIT_SUCCESS;
}
