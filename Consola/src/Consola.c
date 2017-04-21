#include "Consola.h"


void init(int sizeArgs, char** path){
	if(sizeArgs != 2){
		printf("Numero de argumentos incorrectos, el init solo debe recibir el path del archivo\n\r");
		freeElementsArray(path,sizeArgs);
		return;
	}
	printf("%s\n",path[1]);

	freeElementsArray(path,sizeArgs);
}

void clear(int sizeArgs, char** args){
	system("clear");
	freeElementsArray(args,sizeArgs);
}

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);

	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"init",&init);
	dictionary_put(commands,"clear",&clear);

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

	waitCommand(commands);

	dictionary_destroy(commands);
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
