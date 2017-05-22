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

/* Global Variables*/
char* IpKernel;
int PuertoKernel;
int socketKernel;
pthread_mutex_t mutexLog;
t_log* logConsola;
t_dictionary*semaforosPID;
t_aviso_consola* avisoKernel;

/*Function prototypes*/
t_config* cargarConfiguracion(char * nombreArchivo);


#endif /* CONSOLA_H_ */




