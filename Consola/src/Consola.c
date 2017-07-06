#include "Consola.h"

void esperarMensajePID(void*paramPid){
	int32_t pid = (int32_t)paramPid;
	sem_t* mutexPID;
	char* pidKey = string_itoa(pid);
	mutexPID = (sem_t*)dictionary_get(semaforosPID,pidKey);
	free(pidKey);

	while(1){
		sem_wait(mutexPID);
		log_info(logConsola,"El PID:%d recibió un mensaje :%s",pid,avisoKernel->mensaje);

		if(avisoKernel->mostrarPorPantalla){
			printf("%s\n\r",avisoKernel->mensaje);
			char* pidKey = string_itoa(pid);
			pthread_mutex_lock(&mutexStatsPID);
			t_stats* stats = dictionary_get(statsPID,pidKey);
			stats->cantMensajesPantalla++;
			pthread_mutex_unlock(&mutexStatsPID);
			free(pidKey);
		}

		if(avisoKernel->terminoProceso){
			log_info(logConsola,"Se finalizo el proceso, con el PID:%d",avisoKernel->idPrograma);
			char* pidKey = string_itoa(pid);

			pthread_mutex_lock(&mutexSemaforosPID);
			dictionary_remove_and_destroy(semaforosPID, pidKey,free);
			pthread_mutex_unlock(&mutexSemaforosPID);

			pthread_mutex_lock(&mutexStatsPID);
			t_stats* stats = dictionary_remove(statsPID,pidKey);
			pthread_mutex_unlock(&mutexStatsPID);

			time_t timerow = time(0);
			struct tm* fin = malloc(sizeof(struct tm));
			localtime_r(&timerow,fin);

			printf("*******STATS PID:%s*******\n\r",pidKey);
			printf("Cantidad mensajes mostrados :%d\n\r",stats->cantMensajesPantalla);
			printf("Inicio :%s\n\r",asctime(stats->inicio));
			printf("Fin :%s\n\r",asctime(fin));
			printf("Tiempo de ejecución (segundos) :%0.0lf \n\r",difftime(mktime(fin),mktime(stats->inicio)));
			printf("*******FIN*******\n\r");

			free(fin);
			free(stats->inicio);
			free(stats);

			free(pidKey);
			sem_post(&avisoProcesado);
			break;
		}

		sem_post(&avisoProcesado);
	}
}

void newPID(char*data,int socket){
	avisoKernel = deserializar_aviso_consola(data);

	sem_t* sem=malloc(sizeof(sem_t));
	sem_init(sem,0,1);

	char* pid = string_itoa(avisoKernel->idPrograma);
	pthread_mutex_lock(&mutexSemaforosPID);
	dictionary_put(semaforosPID,pid,sem);
	pthread_mutex_unlock(&mutexSemaforosPID);

	time_t timerow = time(0);
	t_stats* stats = malloc(sizeof(t_stats));
	stats->cantMensajesPantalla=0;
	stats->inicio= malloc(sizeof(struct tm));
	localtime_r(&timerow,stats->inicio);


	pthread_mutex_lock(&mutexStatsPID);
	dictionary_put(statsPID,pid,stats);
	pthread_mutex_unlock(&mutexStatsPID);

	free(pid);

	pthread_t hiloPID;
	pthread_create(&hiloPID,NULL,(void*)esperarMensajePID,(void*)avisoKernel->idPrograma);
	pthread_detach(hiloPID);
}

void desconexionKernel(int socket){
	log_info(logConsola,"El kernel se desconectó");
	exit(0);
}

void esperarKernel(void* args){
	while(1){
		t_package* paqueteKernel = recibirPaquete(socketKernel,desconexionKernel);
		if(strcmp(paqueteKernel->key,"NEW_PID")==0){
			t_dictionary* diccionario = dictionary_create();
			dictionary_put(diccionario,"NEW_PID",&newPID);
			procesarPaquete(paqueteKernel,socketKernel,diccionario,NULL,NULL);
			dictionary_destroy(diccionario);
			sem_wait(&avisoProcesado);
			free(avisoKernel->mensaje);
			free(avisoKernel);
		}else{
			avisoKernel = deserializar_aviso_consola(paqueteKernel->datos);
			char* pid = string_itoa(avisoKernel->idPrograma);

			if(dictionary_get(semaforosPID,pid)!=NULL){
				sem_post((sem_t*)dictionary_get(semaforosPID,pid));
				sem_wait(&avisoProcesado);
			}

			free(pid);
			borrarPaquete(paqueteKernel);
			free(avisoKernel->mensaje);
			free(avisoKernel);
		}
	}
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
	char* buffer=(char*)malloc(tamanio+1);

	fseek(arch,0L,SEEK_SET); // me ubico al principio para empezar a leer
	fread (buffer, sizeof(char), tamanio, arch);

	buffer[tamanio]='\0';

	empaquetarEnviarMensaje(socketKernel,"NUEVO_PROG",strlen(buffer),buffer);

	fclose(arch);
	free(buffer);
	freeElementsArray(path,sizeArgs);
}


void clear(int sizeArgs, char** args){
	system("clear");
	freeElementsArray(args,sizeArgs);
}

void end(int sizeArgs, char** path){
	if(sizeArgs != 2){
			printf("Numero de argumentos incorrectos, el end solo debe recibir el pid del proceso\n\r");
			freeElementsArray(path,sizeArgs);
			return;
	}
	char* pid= path[1];

	pthread_mutex_lock(&mutexSemaforosPID);
	if(!dictionary_has_key(semaforosPID,pid)){  //controla que el pid este en el diccionario
		pthread_mutex_unlock(&mutexSemaforosPID);

		printf("PID no encontrado\n\r");
		freeElementsArray(path,sizeArgs);
		return;
	}
	pthread_mutex_unlock(&mutexSemaforosPID);

	empaquetarEnviarMensaje(socketKernel,"END_PROG",strlen(pid) ,pid);
	freeElementsArray(path,sizeArgs);
 }

void cerrarConsola(int sizeArgs,char** args){
	void llamarEnd(char* key, void* value){
		empaquetarEnviarMensaje(socketKernel,"END_PROG",strlen(key) ,key);
	}
	pthread_mutex_lock(&mutexSemaforosPID);
	dictionary_iterator(semaforosPID,llamarEnd);
	pthread_mutex_unlock(&mutexSemaforosPID);

	freeElementsArray(args,sizeArgs);
}


int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);
	logConsola = log_create("Consola.log","Consola",false,LOG_LEVEL_TRACE);

	sem_init(&avisoProcesado,0,0);

	semaforosPID = dictionary_create();
	statsPID = dictionary_create();
	commands = dictionary_create();
	dictionary_put(commands,"init",&init);
	dictionary_put(commands,"clear",&clear);
	dictionary_put(commands,"end",&end);
	dictionary_put(commands,"shutdown",&cerrarConsola);

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

	pthread_detach(hilo);
	dictionary_destroy(commands);
	dictionary_destroy(statsPID);
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
