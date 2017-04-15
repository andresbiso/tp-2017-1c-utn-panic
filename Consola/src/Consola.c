#include <panicommons/panisocket.h>
#include <commons/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Consola.h"

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);

	printf("IP_Kernel: %s\n", IpKernel);
	printf("Puerto_Kernel: %d\n", PuertoKernel);

	int socketConsola;
	if ((socketConsola = conectar(IpKernel, PuertoKernel)) == -1) {
		puts("No se encontro kernel");
		exit(EXIT_FAILURE);
	}
	while (!handshake(socketConsola, "HCSKE", "HKECS")) {
		puts("Fallo la conexion con el Kernel");
	}
	puts("Conectado con kernel");
	while(1) {
		char* input = (char *)malloc(255 * sizeof(char));
		scanf("%s", input);
		if(!empaquetarEnviarMensaje(socketConsola, "KEY_PRINT", 1, input)){
			perror("Hubo un error en la conexión");
			exit(EXIT_FAILURE);
		}
		puts(input);
		free(input);
	}
	config_destroy(configFile);
	return EXIT_SUCCESS;
}

t_config* cargarConfiguracion(char * nombreArchivo){
	char* configFilePath = string_new();
	string_append(&configFilePath,nombreArchivo);
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

	return configFile;
}
