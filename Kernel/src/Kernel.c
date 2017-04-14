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
#include "Kernel.h"

void mostrarMensaje(char** mensajes){
	printf("Mensaje %s \n", mensajes[0]);
}


int main(int argc, char** argv) {
	int socketMemoria;
	int socketFS;

	cargarConfiguracion(argv[1]);

    t_dictionary* diccionarioFunciones = dictionary_create();
    dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);

    t_dictionary* diccionarioHandshakes = dictionary_create();
    dictionary_put(diccionarioHandshakes,"HCPKE","HKECP");
    dictionary_put(diccionarioHandshakes,"HCSKE","HKECS");

    int socket = crearHostMultiConexion(PuertoKernel);

    if ((socketMemoria = conectar(IpMemoria,PuertoMemoria)) == -1)
    	exit(EXIT_FAILURE);
    if(handshake(socketMemoria,"HKEME","HMEKE")){
    		empaquetarEnviarMensaje(socketMemoria,"KEY_PRINT",2,"p1","p2");
    		puts("Se pudo realizar handshake");
    	}else
    		puts("No se pudo realizar handshake");

    if ((socketFS = conectar(IpFS,PuertoFS)) == -1)
        exit(EXIT_FAILURE);
    if(handshake(socketFS,"HKEFS","HFSKE")){
     		empaquetarEnviarMensaje(socketFS,"KEY_PRINT",2,"p1","p2");
     		puts("Se pudo realizar handshake");
     	}else
     		puts("No se pudo realizar handshake");

    correrServidorMultiConexion(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	return EXIT_SUCCESS;
}

void cargarConfiguracion(char* archivo){
	t_config* archivo_cnf;
		char *aux;

		archivo_cnf = config_create(archivo);

		if(config_has_property(archivo_cnf, "PUERTO_KERNEL") == true)
			PuertoKernel = config_get_int_value(archivo_cnf, "PUERTO_KERNEL");
		else{
			printf("ERROR archivo config sin puerto KERNEL\n");
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

		if(config_has_property(archivo_cnf, "STACK_SIZE") == true)
			StackSize = config_get_int_value(archivo_cnf, "STACK_SIZE");
		else{
			printf("ERROR archivo config sin Stack Size\n");
			exit(EXIT_FAILURE);
			}
}
