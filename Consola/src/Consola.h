#ifndef CONSOLA_H_
#define CONSOLA_H_


#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <panicommons/panisocket.h>
#include <panicommons/paniconsole.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <panicommons/serializacion.h>
#include <panicommons/panicommons.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

typedef struct{
	struct tm* inicio;
	int32_t cantMensajesPantalla;
}t_stats;

/* Global Variables*/
char* IpKernel;
int PuertoKernel;
int socketKernel;
pthread_mutex_t mutexSemaforosPID = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexStatsPID = PTHREAD_MUTEX_INITIALIZER;
sem_t avisoProcesado;
t_log* logConsola;
t_dictionary*semaforosPID;
t_dictionary*commands;
t_aviso_consola* avisoKernel;
t_dictionary* statsPID;

/*Function prototypes*/
t_config* cargarConfiguracion(char * nombreArchivo);


#endif /* CONSOLA_H_ */




