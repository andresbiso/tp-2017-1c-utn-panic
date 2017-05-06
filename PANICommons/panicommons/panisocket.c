/*
 * socket.c
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */
#include "panisocket.h"


int handshake(int socket, char * keyEnviada, char * keyEsperada){
	empaquetarEnviarMensaje(socket,"HANDSHAKE",strlen(keyEnviada),keyEnviada);

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
	void* returnValue = dictionary_get(diccionarioHandshakes,keyRecibida);

	if(returnValue != NULL)
		empaquetarEnviarMensaje(socket,(char*)returnValue,strlen((char*)returnValue),(char*)returnValue);
	else
		empaquetarEnviarMensaje(socket,"HD_NOT",6,"HD_NOT");

}

uint32_t tamanioPaquete(t_package paquete){
	return paquete.longitudDatos+strlen(paquete.key)+1+sizeof(uint32_t);//+1 poque suma el separador
}

char*empaquetar(t_package* paquete){
	uint32_t tamPaquete = tamanioPaquete(*paquete);
	char* paqueteSalida =  (char*)malloc(tamPaquete);
	if(paqueteSalida){
		memcpy(paqueteSalida, &tamPaquete, sizeof(uint32_t));
		char* keyArguments = string_new();
		string_append(&keyArguments,paquete->key);
		string_append(&keyArguments,";");
		memcpy(paqueteSalida+sizeof(uint32_t),keyArguments,strlen(keyArguments));
		int offset=sizeof(uint32_t)+strlen(keyArguments);
		memcpy(paqueteSalida+offset,paquete->datos,paquete->longitudDatos);
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
			return 0;
		}

		total += enviados;
	}

	free(mensaje);
	return 1;
}


int empaquetarEnviarMensaje(int socketServidor, char* key,int longitudDatos, char* data){

	char * cuerpo = malloc(strlen(key)+2+longitudDatos);//1 de separador 1 de /0
	memcpy(cuerpo,key,strlen(key));
	memcpy(cuerpo+strlen(key),";",1);

	int longKey=strlen(key)+1;
	memcpy(cuerpo+longKey,(void*)data,longitudDatos);

	cuerpo[longKey+longitudDatos]='\0';

	t_package *paquete = malloc(sizeof(t_package));
	*paquete = crearPaquete(cuerpo,longitudDatos+longKey);//(cantParams-1) para sumar el separador

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
	paquete.key=keyDatos[0];
	paquete.longitudDatos=abs(longitud-(strlen(paquete.key)+1));//A la longitud se le resta la key ya que la incluye
	paquete.datos = malloc(paquete.longitudDatos+1);
	memcpy(paquete.datos,datos+strlen(paquete.key)+1,paquete.longitudDatos);
	paquete.datos[paquete.longitudDatos]='\0';
	free(keyDatos[1]);
	free(keyDatos);
	return paquete;
}

t_package crearPaqueteDeError(){
	t_package paquete;
	paquete.datos="Error";
	paquete.key="ERROR_FUNC";
	paquete.longitudDatos=5;
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

	char* data = (char *)malloc(longitud);

	if(data == NULL)
		return paquete;

	recibidos=0;

	while(recibidos<(longitud-sizeof(uint32_t))){
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
	*paquete = crearPaquete(data,(longitud-sizeof(int32_t)));// (sizeof(int32_t)-1) se le resta el tamanio de la longitud y 1 por el separador
	free(data);
	return paquete;
}


void correrFuncion(void* (*funcion)(),char* datos, int socket){
	funcion(datos,socket);
}

void borrarPaquete(t_package* package){
	if(!package)
			return;
	if (strcmp(package->key,"ERROR_FUNC")!=0){
		free(package->datos);
		free(package->key);
	}
	free(package);
}

void recibirMensajesThread(void* paramsServidor){
	t_threadSocket* threadSocket = paramsServidor;
	while(1){
		t_package* paquete = recibirPaquete(threadSocket->socket,threadSocket->desconexion);
		int error = strcmp(paquete->key,"ERROR_FUNC")==0;
		procesarPaquete(paquete,threadSocket->socket,threadSocket->funciones,threadSocket->handshakes);
		if(error){
			if(threadSocket->desconexion != NULL)
				threadSocket->desconexion(threadSocket->socket);
			close(threadSocket->socket);
			break;
		}
	}
	free(threadSocket);
}

void correrServidorThreads(int socket, void (*nuevaConexion)(int), void (*desconexion)(int),t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes){
	while(1){
			int cliente = aceptarCliente(socket);
			if(cliente != -1 && nuevaConexion != NULL)
				nuevaConexion(cliente);

			t_threadSocket* threadSocket = malloc(sizeof(t_threadSocket));
			threadSocket->socket=cliente;
			threadSocket->desconexion=desconexion;
			threadSocket->handshakes=diccionarioHandshakes;
			threadSocket->funciones=diccionarioFunciones;

			pthread_t hiloMensajes;
			pthread_create(&hiloMensajes,NULL,(void *) recibirMensajesThread, threadSocket);
	}

}

void procesarPaquete(t_package* paquete,int socket,t_dictionary* diccionarioFunciones, t_dictionary* diccionarioHandshakes){
	if(strcmp(paquete->key,"HANDSHAKE") != 0){
		void* funcion;
		funcion = dictionary_get(diccionarioFunciones,paquete->key);
		if(funcion != NULL){
			correrFuncion(funcion,paquete->datos,socket);
		}else{
			perror("Key de funcion no encontrada");
		}
	}else{
		realizarHandshake(socket,paquete->datos,diccionarioHandshakes);
	}
	borrarPaquete(paquete);
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
						procesarPaquete(paquete,i,diccionarioFunciones,diccionarioHandshakes);
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
		return -1;
	}

	return socketCliente;
}
