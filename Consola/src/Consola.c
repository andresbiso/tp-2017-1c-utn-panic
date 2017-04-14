/*
 ============================================================================
 Name        : Consola.c
 Author      : mpicollo
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <panicommons/panisocket.h>
#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* IpKernel;
int PuertoKernel;


int main(int argc, char** argv) {
	int configFileSize = strlen("/home/utnso/workspace/tp-2017-1c-utn-panic/Consola/Debug/") + strlen(argv[1]);
	char* configFilePath = (char *)malloc(configFileSize * sizeof(char));
	strcat(configFilePath, "/home/utnso/workspace/tp-2017-1c-utn-panic/Consola/Debug/");
	strcat(configFilePath, argv[1]);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);
	if (config_has_property(configFile, "IP_KERNEL")) {
		IpKernel = config_get_string_value(configFile, "IP_KERNEL");
	} else {
		perror("La key IP_KERNEL no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "PUERTO_KERNEL")) {
		PuertoKernel = config_get_int_value(configFile, "PUERTO_KERNEL");
	} else {
		perror("La key PUERTO_KERNEL no existe");
		exit(EXIT_FAILURE);
	}
	int socketConsola;
	if ((socketConsola = conectar(IpKernel, PuertoKernel)) == -1) {
		puts("No se encontro kernel");
		exit(EXIT_FAILURE);
	}
	while (!handshake(socket, "HCSKE", "HKECS")) {
		puts("Fallo la conexion con el Kernel");
	}
	puts("Conectado con kernel");
	while(1) {
		char* input = (char *)malloc(255 * sizeof(char));
		scanf("%s", input);
		puts(input);
		free(input);
	}
	return EXIT_SUCCESS;
}
