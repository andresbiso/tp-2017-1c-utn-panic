/*
 * socket.h
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */

#ifndef SOCKET_H_
#define SOCKET_H_
#endif /* SOCKET_H_ */

#include <commons/collections/dictionary.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/string.h>

typedef struct{
	uint32_t longitud;
	char* key;
	char* datos;
}t_package;

t_dictionary* diccionarioFunciones;/*Cada proceso debe definir el diccionario y
									como minimo la funcion con la key ERROR_FUNC*/

uint32_t tamanioPaquete(t_package paquete);
char*empaquetar(t_package* paquete);
int enviarMensaje(int socket, char * mensaje,uint32_t tamanioPaquete);
int empaquetarEnviarMensaje(int socketServidor, char* key, int cantParams, ...);
int aceptarCliente(int socket);
int aceptarClienteMultiConexion(int socket,fd_set* fds, int* fdmax);
t_package crearPaquete(char*datos,uint32_t longitud);
t_package crearPaqueteDeError();
t_package* recibirPaquete(int socket);
void correrFuncion(void* funcion(),char* datos);
void borrarPaquete(t_package* package);
int correrServidorMultiConexion(int socket);
int crearHostMultiConexion(int puerto);
int conectar(char*direccion,int puerto);
