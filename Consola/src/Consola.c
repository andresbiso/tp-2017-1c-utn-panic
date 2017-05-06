#include "Consola.h"

#include <panicommons/panicommons.h>
#include <pthread.h>
#include <string.h>

void logMessage(char*data,int socket){
	pthread_mutex_lock(&mutexLog);
	log_info(logConsola,data);
	pthread_mutex_unlock(&mutexLog);
}

void esperarKernel(void* args){
	t_dictionary* diccionario = dictionary_create();
	dictionary_put(diccionario,"LOG_MESSAGE",&logMessage);
	while(1){
		t_package* paquete = recibirPaquete(socketKernel,NULL);
		procesarPaquete(paquete,socketKernel,diccionario,NULL);
	}
	dictionary_destroy(diccionario);
}


void init(int sizeArgs, char** path){
	if(sizeArgs != 2){
		printf("Numero de argumentos incorrectos, el init solo debe recibir el path del archivo\n\r");
		freeElementsArray(path,sizeArgs);
		return;
	}
	FILE *arch = fopen(path[1], "r");

	if (arch==NULL){
		   perror("No se puede abrir el archivo");
		   return;
	}

	fseek(arch,0L, SEEK_END);            // me ubico en el final del archivo.
	int tamanio= ftell(arch);
	char* buffer=(char*)malloc(tamanio);

	fseek(arch,0L,SEEK_SET); // me ubico al principio para empezar a leer
	fread (buffer, sizeof(char), tamanio, arch);

	buffer[tamanio-1]='\0';

	empaquetarEnviarMensaje(socketKernel,"NUEVO_PROG",strlen(buffer),buffer);

	pthread_t hilo;
	pthread_create(&hilo,NULL,(void*)esperarKernel,NULL);

	fclose(arch);
	free(buffer);
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
	logConsola = log_create("Consola.log","Consola",false,LOG_LEVEL_TRACE);

	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"init",&init);
	dictionary_put(commands,"clear",&clear);

	printf("IP_Kernel: %s\n", IpKernel);
	printf("Puerto_Kernel: %d\n", PuertoKernel);

	if ((socketKernel = conectar(IpKernel, PuertoKernel)) == -1) {
		puts("No se encontro kernel");
		exit(EXIT_FAILURE);
	}
	while (!handshake(socketKernel, "HCSKE", "HKECS")) {
		puts("Fallo la conexion con el Kernel");
	}
	puts("Conectado con kernel");

	waitCommand(commands);

	dictionary_destroy(commands);
	config_destroy(configFile);
	log_destroy(logConsola);
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
