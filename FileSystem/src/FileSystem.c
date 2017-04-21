#include <stdio.h>
#include <stdlib.h>
#include <panicommons/panisocket.h>
#include "FileSystem.h"
#include <commons/config.h>

void error(char** args){
	printf("%s",args[0]);
}
void mostrarMensaje(char* mensajes,int socket){
 	printf("Mensaje recibido: %s \n",mensajes);
}
t_config* cargarConfiguracion(char* nombreDelArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreDelArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);
	if (config_has_property(configFile, "PUERTO")) {
		puerto = config_get_int_value(configFile, "PUERTO");
	} else {
	 	perror("La key PUERTO no existe");
	 	exit(EXIT_FAILURE);
	 }
	if (config_has_property(configFile,"PUNTO_MONTAJE")){
		punto_montaje= config_get_string_value(configFile,"PUNTO_MONTAJE");
	}else {
		perror("La key PUNTO_MONTAJE no existe");
		exit(EXIT_FAILURE);
	}
	return configFile;
}
int main(int argc, char** argv) {
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",punto_montaje);
	t_dictionary* diccionarioFunc= dictionary_create();
	t_dictionary* diccionarioHands= dictionary_create();
	dictionary_put(diccionarioHands,"HKEFS","HFSKE");
	dictionary_put(diccionarioFunc,"ERROR_FUNC",&error);
	dictionary_put(diccionarioFunc,"KEY_PRINT",&mostrarMensaje);
	int sock= crearHostMultiConexion(puerto);
	correrServidorMultiConexion(sock,NULL,NULL,diccionarioFunc,diccionarioHands);
	dictionary_destroy(diccionarioFunc);
	dictionary_destroy(diccionarioHands);
	config_destroy(configFile);
	return EXIT_SUCCESS;
}
