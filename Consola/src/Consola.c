#include "Consola.h"

#include <panicommons/panicommons.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

void esperarMensajePID(void*paramPid){
	int32_t pid = (int32_t)paramPid;
	sem_t* mutexPID;
	mutexPID = (sem_t*)dictionary_get(semaforosPID,string_itoa(pid));

	while(1){
		sem_wait(mutexPID);
		log_info(logConsola,"El PID:%d recibió un mensaje :%s",pid,avisoKernel->mensaje);
		/*if(avisoKernel->terminoProceso ==1){
						break;
		}*/
	}
}

void newPID(char*data,int socket){
	avisoKernel = deserializar_aviso_consola(data);

	sem_t* sem=malloc(sizeof(sem_t));
	sem_init(sem,0,1);

	dictionary_put(semaforosPID,string_itoa(avisoKernel->idPrograma),sem);

	pthread_t hiloPID;
	pthread_create(&hiloPID,NULL,(void*)esperarMensajePID,(void*)avisoKernel->idPrograma);
}

void esperarKernel(void* args){
	t_dictionary* diccionario = dictionary_create();
	dictionary_put(diccionario,"NEW_PID",&newPID);
	while(1){
		t_package* paqueteKernel = recibirPaquete(socketKernel,NULL);
		if(strcmp(paqueteKernel->key,"NEW_PID")==0)
			procesarPaquete(paqueteKernel,socketKernel,diccionario,NULL);
		else{
			avisoKernel = deserializar_aviso_consola(paqueteKernel->datos);
			sem_post((sem_t*)dictionary_get(semaforosPID,string_itoa(avisoKernel->idPrograma)));

		}
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

	fclose(arch);
	free(buffer);
	freeElementsArray(path,sizeArgs);
}


void clear(int sizeArgs, char** args){
	system("clear");
	freeElementsArray(args,sizeArgs);
}
/*
void end(int sizeArgs, char** path){
	if(sizeArgs != 2){
			printf("Numero de argumentos incorrectos, el end solo debe recibir el path del archivo\n\r");
			freeElementsArray(path,sizeArgs);
			return;
		}
	char* pid= path[1];
	if(!dictionary_has_key(semaforosPID,pid)){  //controla que el pid este en el diccionario
		printf("ṔÍD no encontrado\n\r");
		return;
			};
	empaquetarEnviarMensaje(socketKernel,"END_PROG",strlen(pid) ,pid);
	if(string_itoa(avisoKernel->idPrograma)==pid){//controla que el kernel tenga el mismo pid
		dictionary_remove(semaforosPID, pid);
		log_info(logConsola,"Se finalizo el proceso, con el PID:%d",avisoKernel->idPrograma);
	}
	else{
		printf("No se logro finalizar el proceso");
		return;
	}
	freeElementsArray(path,sizeArgs);
 }

void cerrarConsola(int sizeArgs,char** args){
	while (!dictionary_is_empty(semaforosPID)){
		int32_t pid= 1;
		if(dictionary_has_key(semaforosPID,string_itoa(pid))){
			char* pid2 =string_itoa(pid);
			empaquetarEnviarMensaje(socketKernel,"END_PROG",strlen(pid2) ,pid2);
			if(string_itoa(avisoKernel->idPrograma)==pid2){//controla que el kernel tenga el mismo pid
				dictionary_remove(semaforosPID, pid2);
				log_info(logConsola,"Se finalizo el proceso, con el PID:%d",avisoKernel->idPrograma);
			}else{
				printf("No se logro finalizar el proceso");
				return;
			}
			pid++;
		};
		pid++;
	}

	freeElementsArray(args,sizeArgs);
}
*/
int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);
	logConsola = log_create("Consola.log","Consola",false,LOG_LEVEL_TRACE);

	semaforosPID = dictionary_create();

	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"init",&init);
	dictionary_put(commands,"clear",&clear);
	//dictionary_put(commands,"end",&end);
	//dictionary_put(commands,"shutdown",&cerrarConsola);

	printf("IP_Kernel: %s\n", IpKernel);
	printf("Puerto_Kernel: %d\n", PuertoKernel);

	if ((socketKernel = conectar(IpKernel, PuertoKernel)) == -1) {
		perror("No se encontro kernel");
		exit(EXIT_FAILURE);
	}
	while (!handshake(socketKernel, "HCSKE", "HKECS")) {
		perror("Fallo la conexion con el Kernel");
		exit(EXIT_FAILURE);
	}
	puts("Conectado con kernel");

	pthread_t hilo;
	pthread_create(&hilo,NULL,(void*)esperarKernel,NULL);

	waitCommand(commands);

	dictionary_destroy(commands);
	dictionary_destroy_and_destroy_elements(semaforosPID,free);
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
