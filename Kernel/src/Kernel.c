/*
 ============================================================================
 Name        : Kernel.c
 Author      : mpicollo
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "Kernel.h"

typedef struct {
    int socketEscucha;
    void (*nuevaConexion) (int);
    void (*desconexion) (int);
    t_dictionary* funciones;
    t_dictionary* handshakes;
}threadParams;

void mostrarMensaje(char* mensaje){
	printf("Mensaje recibido: %s \n",mensaje);
}


void correrServidor(void* arg){
	threadParams* params = arg;
	correrServidorMultiConexion(params->socketEscucha,params->nuevaConexion,params->desconexion,params->funciones,params->handshakes);
}

int main(int argc, char** argv) {
	pthread_t thread_consola, thread_cpu;
	int socketMemoria;
	int socketFS;

	cargarConfiguracion(argv[1]);

    t_dictionary* diccionarioFunciones = dictionary_create();
    dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
    dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);

    t_dictionary* diccionarioHandshakes = dictionary_create();
    dictionary_put(diccionarioHandshakes,"HCPKE","HKECP");
    dictionary_put(diccionarioHandshakes,"HCSKE","HKECS");

    int socketConsola = crearHostMultiConexion(PuertoConsola);
    int socketCPU = crearHostMultiConexion(PuertoCpu);

    threadParams parametrosConsola;
    parametrosConsola.socketEscucha = socketConsola;
    parametrosConsola.nuevaConexion = NULL;
    parametrosConsola.desconexion = NULL;
    parametrosConsola.handshakes = diccionarioHandshakes;
    parametrosConsola.funciones = diccionarioFunciones;

    threadParams parametrosCpu;
    parametrosCpu.socketEscucha = socketCPU;
    parametrosCpu.nuevaConexion = NULL;
    parametrosCpu.desconexion = NULL;
    parametrosCpu.handshakes = diccionarioHandshakes;
    parametrosCpu.funciones = diccionarioFunciones;

    if ((socketMemoria = conectar(IpMemoria,PuertoMemoria)) == -1)
    	exit(EXIT_FAILURE);
    if(handshake(socketMemoria,"HKEME","HMEKE")){
    		empaquetarEnviarMensaje(socketMemoria,"KEY_PRINT",1,"hola");
    		puts("Se pudo realizar handshake");
    }
    else{
    	puts("No se pudo realizar handshake");
    	exit(EXIT_FAILURE);
    	}

    if ((socketFS = conectar(IpFS,PuertoFS)) == -1)
        exit(EXIT_FAILURE);
    if(handshake(socketFS,"HKEFS","HFSKE")){
     		empaquetarEnviarMensaje(socketFS,"KEY_PRINT",2,"p1","p2");
     		puts("Se pudo realizar handshake");
     	}else
     		puts("No se pudo realizar handshake");

    if (pthread_create(&thread_consola, NULL, (void*)correrServidor, &parametrosCpu)){
    		        perror("Error el crear el thread consola.");
    		        exit(EXIT_FAILURE);
    		}

    if (pthread_create(&thread_cpu, NULL, (void*)correrServidor, &parametrosConsola)){
      		        perror("Error el crear el thread consola.");
      		        exit(EXIT_FAILURE);
      		}

    pthread_join(thread_consola, NULL);

    pthread_join(thread_cpu, NULL);


//    correrServidorMultiConexion(socketConsola,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);
//
//    correrServidorMultiConexion(socketCPU,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

    dictionary_destroy(diccionarioFunciones);
    dictionary_destroy(diccionarioHandshakes);

	return EXIT_SUCCESS;
}

void cargarConfiguracion(char* archivo){
	t_config* archivo_cnf;
		char *aux;

		archivo_cnf = config_create(archivo);

		if(config_has_property(archivo_cnf, "PUERTO_CONSOLA") == true)
			PuertoConsola = config_get_int_value(archivo_cnf, "PUERTO_CONSOLA");
		else{
			printf("ERROR archivo config sin puerto Consola\n");
			exit(EXIT_FAILURE);
		}

		if(config_has_property(archivo_cnf, "PUERTO_CPU") == true)
			PuertoCpu = config_get_int_value(archivo_cnf, "PUERTO_CPU");
		else{
			printf("ERROR archivo config sin puerto CPU\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "PUERTO_MEMORIA") == true)
			PuertoMemoria = config_get_int_value(archivo_cnf, "PUERTO_MEMORIA");
		else{
			printf("ERROR archivo config sin puerto Memoria\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "PUERTO_FS") == true)
			PuertoFS = config_get_int_value(archivo_cnf, "PUERTO_FS");
		else{
			printf("ERROR archivo config sin puerto FileSystem\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "IP_MEMORIA") == true){
			aux = config_get_string_value(archivo_cnf,"IP_MEMORIA");
			IpMemoria = malloc(strlen(aux) + 1);
			memcpy(IpMemoria, aux, strlen(aux) + 1);
			free(aux);
		}
		else{
			printf("ERROR archivo config sin IP Memoria\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "IP_FS") == true){
			aux = config_get_string_value(archivo_cnf,"IP_FS");
			IpFS = malloc(strlen(aux) + 1);
			memcpy(IpFS, aux, strlen(aux) + 1);
			free(aux);
		}
		else{
			printf("ERROR archivo config sin IP FILESYSTEM\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "QUANTUM") == true)
			Quantum = config_get_int_value(archivo_cnf, "QUANTUM");
		else{
			printf("ERROR archivo config sin Quantum\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "QUANTUM_SLEEP") == true)
			QuantumSleep = config_get_int_value(archivo_cnf, "QUANTUM_SLEEP");
		else{
			printf("ERROR archivo config sin Quantum Sleep\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "GRADO_MULTIPROG") == true)
			GradoMultiprog = config_get_int_value(archivo_cnf, "GRADO_MULTIPROG");
		else{
			printf("ERROR archivo config sin Grado Multiprogramacion\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "SEM_IDS") == true)
			SemIds = config_get_array_value(archivo_cnf, "SEM_IDS");
		else{
			printf("ERROR archivo config sin ID de Semaforos\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "SEM_INIT") == true)
			SemInit = config_get_array_value(archivo_cnf, "SEM_INIT");
		else{
			printf("ERROR archivo config sin Inicializacion de Semaforos\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "SHARED_VARS") == true)
			SharedVars = config_get_array_value(archivo_cnf, "SHARED_VARS");
		else{
			printf("ERROR archivo config sin Variables Compartidas\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "STACK_SIZE") == true)
			StackSize = config_get_int_value(archivo_cnf, "STACK_SIZE");
		else{
			printf("ERROR archivo config sin Stack Size\n");
			exit(EXIT_FAILURE);
			}

		config_destroy(archivo_cnf);
}
