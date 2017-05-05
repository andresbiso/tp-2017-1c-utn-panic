#include <stdio.h>
#include <stdlib.h>

#include <panicommons/panisocket.h>
#include <commons/config.h>
#include <commons/log.h>
#include "CPU.h"

void nuevoPCB(char* pcb){
	pcb_ejecutandose = deserializar(pcb);
}

void mostrarMensaje(char* mensaje){
	printf("Mensaje recibido: %s \n",mensaje);
}

t_config* cargarConfiguracion(char * nombreArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);

	if (config_has_property(configFile, "PUERTO_KERNEL")) {
		puertoKernel = config_get_int_value(configFile, "PUERTO_KERNEL");
	} else {
		perror("La key PUERTO_KERNEL no existe");
		exit(EXIT_FAILURE);
	}

	if(config_has_property(configFile, "IP_KERNEL") == true){
		ipKernel = config_get_string_value(configFile,"IP_KERNEL");
	}else{
		perror("La key IP_KERNEL no existe");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(configFile, "PUERTO_MEMORIA")) {
		puertoMemoria = config_get_int_value(configFile, "PUERTO_MEMORIA");
	} else {
		perror("La key PUERTO_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}

	if(config_has_property(configFile, "IP_MEMORIA") == true){
		ipMemoria = config_get_string_value(configFile,"IP_MEMORIA");
	}else{
		perror("La key IP_MEMORIA no existe");
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

	printf("PUERTO KERNEL: %d\n",puertoKernel);
	printf("IP KERNEL: %s\n",ipKernel);
	printf("PUERTO MEMORIA: %d\n",puertoMemoria);
	printf("IP MEMORIA: %s\n",ipMemoria);

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"NUEVO_PCB",&nuevoPCB);

	t_log* log = log_create("cpu.log","CPU",0,LOG_LEVEL_TRACE);

	int socketKernel = conectar(ipKernel,puertoKernel);
	if(!handshake(socketKernel,"HCPKE","HKECP")){
		log_error(log,"No se pudo realizar la conexion con el kernel");
		exit(EXIT_FAILURE);
	}

	int socketMemoria = conectar(ipMemoria,puertoMemoria);
	if(!handshake(socketMemoria,"HCPME","HMECP")){
		log_error(log,"No se pudo realizar la conexion con la memoria");
		exit(EXIT_FAILURE);
	}

	dictionary_destroy(diccionarioFunciones);
	config_destroy(configFile);
	log_destroy(log);

	return EXIT_SUCCESS;
}
