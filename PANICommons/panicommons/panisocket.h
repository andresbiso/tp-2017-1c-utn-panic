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
#include <pthread.h>
#include "panicommons.h"

typedef struct{
	uint32_t longitud;
	char* key;
	char* datos;
}t_package;

typedef struct{
	int socket;
	void (*desconexion) (int);
	t_dictionary* funciones;
	t_dictionary* handshakes;
}t_threadSocket;

int handshake(int socket, char * keyEnviada, char * keyEsperada);
void realizarHandshake(int socket, char* keyRecibida,t_dictionary* diccionarioHandshakes);
uint32_t tamanioPaquete(t_package paquete);
char*empaquetar(t_package* paquete);
int enviarMensaje(int socket, char * mensaje,uint32_t tamanioPaquete);
int empaquetarEnviarMensaje(int socketServidor, char* key, int cantParams, ...);
int aceptarCliente(int socket);
int aceptarClienteMultiConexion(int socket,fd_set* fds, int* fdmax);
t_package crearPaquete(char*datos,uint32_t longitud);
t_package crearPaqueteDeError();
t_package* recibirPaquete(int socket, void (*desconexion) (int));
void correrFuncion(void* (*funcion)(),char* datos, char* key, int socket);
void borrarPaquete(t_package* package);
void procesarPaquete(t_package* paquete,int socket,t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes);
void recibirMensajesThread(void* paramsServidor);
void correrServidorThreads(int socket, void (*nuevaConexion)(int), void (*desconexion)(int),t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes);
int correrServidorMultiConexion(int socket, void (*nuevaConexion)(int),void (*desconexion)(int),t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes);
int crearHostMultiConexion(int puerto);
int conectar(char*direccion,int puerto);
