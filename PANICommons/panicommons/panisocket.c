/*
 * socket.c
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */
#include "panisocket.h"


int handshake(int socket, char * keyEnviada, char * keyEsperada){
	//TODO loguear handshake
	empaquetarEnviarMensaje(socket,"HANDSHAKE",1,keyEnviada);

	t_package* package;
	package = recibirPaquete(socket,NULL);

	if(strcmp(package->key,keyEsperada) == 0){
		borrarPaquete(package);
		return 1;
	}
	borrarPaquete(package);

	return 0;
}

void realizarHandshake(int socket, char* keyRecibida,t_dictionary* diccionarioHandshakes){
	//TODO loguear envio de handshake
	void* returnValue = dictionary_get(diccionarioHandshakes,keyRecibida);

	if(returnValue != NULL)
		empaquetarEnviarMensaje(socket,(char*)returnValue,1,(char*)returnValue);
	else
		empaquetarEnviarMensaje(socket,"HD_NOT",1,"HD_NOT");

}

uint32_t tamanioPaquete(t_package paquete){
	return paquete.longitud+sizeof(uint32_t);
}

char*empaquetar(t_package* paquete){
	char* paqueteSalida =  (char*)malloc(sizeof(uint32_t)+paquete->longitud);
	if(paqueteSalida){
		memcpy(paqueteSalida, &paquete->longitud, sizeof(uint32_t));
		char* keyArguments = string_new();
		string_append(&keyArguments,paquete->key);
		string_append(&keyArguments,";");
		string_append(&keyArguments,paquete->datos);
		memcpy(paqueteSalida+sizeof(uint32_t),keyArguments,paquete->longitud);
		free(keyArguments);
	}
	return paqueteSalida;
}

int enviarMensaje(int socket, char * mensaje,uint32_t tamanioPaquete){
	int total = 0;
	int enviados = 0;

	//bucle hasta enviar todoo el packete (send devuelve los byte que envio)
	while(total < tamanioPaquete){
		if ((enviados = send(socket, mensaje + enviados, tamanioPaquete - enviados, 0)) == -1){
			perror("send");
			close(socket);
			return false;
		}

		total += enviados;
	}

	free(mensaje);
	return 1;
}


int empaquetarEnviarMensaje(int socketServidor, char* key, int cantParams, ...){

	va_list arguments;
	int i;
	char* argumento;
	va_start(arguments, cantParams);

	char * cuerpo = string_new();

	string_append(&cuerpo, key);
	string_append(&cuerpo, ";");

	for (i = 0; i < cantParams; i++){
		argumento = va_arg(arguments, char*);
		string_append(&cuerpo, argumento);
		if(i<(cantParams-1))
			string_append(&cuerpo, ",");
	}

	va_end(arguments);

	t_package *paquete = malloc(sizeof(t_package));
	*paquete = crearPaquete(cuerpo,strlen(cuerpo));

	free(cuerpo);

	char* mensaje = empaquetar(paquete);
	uint32_t tamanio;
	tamanio = tamanioPaquete(*paquete);

	borrarPaquete(paquete);

	return enviarMensaje(socketServidor, mensaje,tamanio);

}

int aceptarCliente(int socket) {
	struct sockaddr direccionCliente;
	unsigned int len = 0;

	int cliente = accept(socket, &direccionCliente,  &len);
	if (cliente == -1)
		perror("Error al aceptar cliente en socket");
	return cliente;
}

int aceptarClienteMultiConexion(int socket,fd_set* fds, int* fdmax) {
	int nuevaConn = aceptarCliente(socket);
	if (nuevaConn != -1){
		FD_SET(nuevaConn, fds);
		if ((*fdmax) < nuevaConn)
			*fdmax = nuevaConn;
	}
	return nuevaConn;
}

t_package crearPaquete(char*datos,uint32_t longitud){
	t_package paquete;
	char ** keyDatos = string_split(datos, ";");
	paquete.datos=keyDatos[1];
	paquete.key=keyDatos[0];
	paquete.longitud=longitud;
	free(keyDatos);
	return paquete;
}

t_package crearPaqueteDeError(){
	t_package paquete;
	paquete.datos="Error";
	paquete.key="ERROR_FUNC";
	paquete.longitud=5;
	return paquete;
}

t_package* recibirPaquete(int socket, void (*desconexion) (int)){
	t_package *paquete = malloc(sizeof(t_package));
	*paquete = crearPaqueteDeError();
	ssize_t recibidos;
	uint32_t longitud;

	recibidos = recv(socket,&longitud,sizeof(uint32_t),MSG_WAITALL);

	if(recibidos <= 0){
			if(recibidos == -1)
				perror("Error al recibir la longitud del paquete");
			else{
				close(socket);
				if (desconexion != NULL)
					desconexion(socket);
			}
			return paquete;
	}

	char* data = (char *)malloc(longitud+1);

	if(data == NULL)
		return paquete;

	recibidos=0;

	while(recibidos<longitud){
		ssize_t recibido = recv(socket, data + recibidos, longitud - recibidos, 0);
		if(recibido <= 0){
			if(recibido == -1)
				perror("Error al recibir datos");
			else
				close(socket);
			return paquete;
		}
		recibidos+=recibido;
	}
	data[recibidos] = '\0';
	*paquete = crearPaquete(data,longitud);
	free(data);
	return paquete;
}


void correrFuncion(void* funcion(),char* datos){

	if(string_contains(datos,",")){
		char** parametros = string_split(datos,",");
		funcion(parametros);
	}else
		funcion(datos);
}

void borrarPaquete(t_package* package){
	if(!package)
			return;
	free(package->datos);
	free(package->key);
	free(package);
}

int correrServidorMultiConexion(int socket, void (*nuevaConexion)(int),void (*desconexion)(int),t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes){
	fd_set master;   // conjunto maestro de descriptores de fichero
	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
	int fdmax;        // número máximo de descriptores de fichero
	int i; // iterador
	FD_ZERO(&master);    // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);// añadir listener al conjunto maestro
	FD_SET(socket, &master);
	fdmax = socket; // por ahora es éste
	// bucle principal
	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == socket) {//nueva conexion
					int nueva = NULL;
					nueva = aceptarClienteMultiConexion(socket,&master,&fdmax);
					if(nueva != -1 && nuevaConexion != NULL)
						nuevaConexion(nueva);
				} else {
					// gestionar datos de un cliente
					char c;
					if(recv(i,&c,sizeof(c),MSG_PEEK)>0){//Mira el primer byte y lo vuelve a dejar
						t_package* paquete = recibirPaquete(i,desconexion);
						if(strcmp(paquete->key,"HANDSHAKE") != 0){
							void* funcion;
							funcion = dictionary_get(diccionarioFunciones,paquete->key);
							if(funcion != NULL){
								correrFuncion(funcion,paquete->datos);
							}else{
								perror("Key de funcion no encontrada");
							}
						}else{
							realizarHandshake(i,paquete->datos,diccionarioHandshakes);
						}
						borrarPaquete(paquete);
					}else{
						FD_CLR(i, &master);
						close(i);
						if (desconexion != NULL)
							desconexion(i);
					}
				 }
			}
		}
	}

}


int crearHostMultiConexion(int puerto){
    fd_set master;   // conjunto maestro de descriptores de fichero
    fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
    struct sockaddr_in myaddr;     // dirección del servidor
    int socketListener;     // descriptor de socket a la escucha
    int yes=1;        // para setsockopt() SO_REUSEADDR, más abajo
    FD_ZERO(&master);    // borra los conjuntos maestro y temporal
    FD_ZERO(&read_fds);
    // obtener socket a la escucha
    if ((socketListener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error durante la creacion del socket");
        exit(1);
    }
    // obviar el mensaje "address already in use" (la dirección ya se está usando)
    if (setsockopt(socketListener, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    // enlazar
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(puerto);
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(socketListener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        perror("Error durante el bind");
        exit(1);
    }
    // escuchar
    if (listen(socketListener, 100) == -1) {
        perror("Error durante el listen");
        exit(1);
    }

    return socketListener;
}

int conectar(char*direccion,int puerto){
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(direccion);
	direccionServidor.sin_port = htons(puerto);

	int socketCliente = socket(AF_INET, SOCK_STREAM, 0);

	if (socketCliente < 0){
		perror("Error al crear socket");
		return -1;
	}

	if (connect(socketCliente, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("No se pudo conectar\n");
		return 1;
	}

	return socketCliente;
}
