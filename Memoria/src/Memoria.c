#include "Memoria.h"

void mostrarMensaje(char* mensaje,int socket){
	printf("Error: %s \n",mensaje);
}

//INICIO COMANDOS

void dumpCache(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de dumpCache no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO
	printf("Do dump cache\n\r");

	freeElementsArray(functionAndParams,size);
}

void dumpProcesos(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de dumpCache no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO
	printf("Do dump procesos\n\r");

	freeElementsArray(functionAndParams,size);
}

void dumpTabla(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de dumpCache no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO
	printf("Do dump tabla\n\r");

	freeElementsArray(functionAndParams,size);
}

void retardo(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de retardo debe recibir solo 1 parametro (tiempo de retardo en ms).\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO chequear concurrencia !!!
	retardoMemoria = atoi(functionAndParams[1]);
	printf("Retardo modificado a %d\n\r",retardoMemoria);

	freeElementsArray(functionAndParams,size);
}

void flush(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de flush no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	pthread_mutex_lock(&mutexCache);
	memset(bloqueCache,0,entradasCache*marcoSize);
	pthread_mutex_unlock(&mutexCache);

	freeElementsArray(functionAndParams,size);
}

void sizeMemory(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de sizeMemory no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO chequear concurrencia !!!
	retardoMemoria = atoi(functionAndParams[1]);
	printf("Retardo modificado a %d\n\r",retardoMemoria);

	freeElementsArray(functionAndParams,size);
}

void sizePID(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de sizePID debe recibir solo 1 parametro (el PID del proceso).\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	//TODO
	printf("TODO size pid\n\r");

	freeElementsArray(functionAndParams,size);
}

//FIN COMANDOS

void correrConsola(){
	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"dumpCache",&dumpCache);
	dictionary_put(commands,"dumpTabla",&dumpTabla);
	dictionary_put(commands,"dumpProcesos",&dumpProcesos);
	dictionary_put(commands,"retardo",&retardo);
	dictionary_put(commands,"flush",&flush);
	dictionary_put(commands,"sizem",&sizeMemory);
	dictionary_put(commands,"sizep",&sizePID);
	waitCommand(commands);
	dictionary_destroy(commands);
}

void escribirEnEstrucAdmin(t_pagina* pagina){
	int offset = pagina->indice*sizeof(t_pagina);
	pthread_mutex_lock(&mutexMemoriaPrincipal);
	memcpy(bloqueMemoria+offset,(void *)&pagina->indice,TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
	offset+=TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV;
	memcpy(bloqueMemoria+offset,(void *)&pagina->pid,TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
	offset+=TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV;
	memcpy(bloqueMemoria+offset,(void *)&pagina->numeroPag,TAM_ELM_TABLA_INV/CANT_ELM_TABLA_INV);
	pthread_mutex_unlock(&mutexMemoriaPrincipal);
}


t_config* cargarConfiguracion(char * nombreArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);

	if (config_has_property(configFile, "PUERTO")) {
		puerto = config_get_int_value(configFile, "PUERTO");
	} else {
		perror("La key PUERTO no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "MARCOS")) {
		marcos = config_get_int_value(configFile, "MARCOS");
	} else {
		perror("La key MARCOS no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "MARCO_SIZE")) {
		marcoSize = config_get_int_value(configFile, "MARCO_SIZE");
	} else {
		perror("La key MARCO_SIZE no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "ENTRADAS_CACHE")) {
		entradasCache = config_get_int_value(configFile, "ENTRADAS_CACHE");
	} else {
		perror("La key ENTRADAS_CACHE no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "CACHE_X_PROC")) {
		cacheXproc = config_get_int_value(configFile, "CACHE_X_PROC");
	} else {
		perror("La key CACHE_X_PROC no existe");
		exit(EXIT_FAILURE);
	}
	if (config_has_property(configFile, "RETARDO_MEMORIA")) {
		retardoMemoria = config_get_int_value(configFile, "RETARDO_MEMORIA");
	} else {
		perror("La key RETARDO_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}
	return configFile;
}

void iniciarPrograma(char* data,int socket){
	//TODO
}

void solicitarBytes(char* data,int socket){
	//TODO
}

void almacenarBytes(char* data,int socket){
	//TODO
}

void asignarPaginas(char* data,int socket){
	//TODO
}

void finalizarPrograma(char* data,int socket){
	//TODO
}

t_pagina crearPagina(int32_t indice,int32_t pid,int32_t numeroPag){
	t_pagina pagina;
	pagina.indice = indice;
	pagina.pid = pid;
	pagina.numeroPag = numeroPag;
	return pagina;
}

void crearEstructurasAdministrativas(){
	int pagAdminis = divAndRoundUp(TAM_ELM_TABLA_INV*marcos,marcoSize);
	int i;

	for(i=0;i<pagAdminis;i++){
		t_pagina* pagina = malloc(sizeof(t_pagina));
		*pagina = crearPagina(i,-1,0);
		escribirEnEstrucAdmin(pagina);
		free(pagina);
	}

	return;
}

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);

	printf("PUERTO: %d\n",puerto);
	printf("MARCOS: %d\n",marcos);
	printf("MARCO_SIZE: %d\n",marcoSize);
	printf("ENTRADAS_CACHE: %d\n",entradasCache);
	printf("CACHE_X_PROC: %d\n",cacheXproc);
	printf("RETARDO_MEMORIA: %d\n",retardoMemoria);

	//La memoria a utilizar es MARCO * MARCO_SIZE
	//La cantidad de marcos que ocupan las estructuras administrativas son (TAM_ELM_TABLA_INV*MARCOS)/MARCO_SIZE <- redondeado para arriba

	bloqueMemoria = (char*) calloc(marcos*marcoSize,sizeof(char));

	if (bloqueMemoria == NULL){
		perror("No se pudo reservar memoria para el bloque principal");
		exit(-1);
	}

	bloqueCache = (char*) calloc(entradasCache*marcoSize,sizeof(char));

	if (bloqueCache == NULL){
		perror("No se pudo reservar memoria para el bloque de la cache");
		exit(-1);
	}

	crearEstructurasAdministrativas();

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"INIT_PROGM",&iniciarPrograma);
	dictionary_put(diccionarioFunciones,"SOLC_BYTES",&solicitarBytes);
	dictionary_put(diccionarioFunciones,"ALMC_BYTES",&almacenarBytes);
	dictionary_put(diccionarioFunciones,"ASIG_PAGES",&asignarPaginas);
	dictionary_put(diccionarioFunciones,"FINZ_PROGM",&finalizarPrograma);

	t_dictionary* diccionarioHandshakes = dictionary_create();
	dictionary_put(diccionarioHandshakes,"HCPME","HMECP");
	dictionary_put(diccionarioHandshakes,"HKEME","HMEKE");

	pthread_mutex_init(&mutexCache,NULL);
	pthread_mutex_init(&mutexMemoriaPrincipal,NULL);

	pthread_t threadConsola;
	pthread_create(&threadConsola,NULL,(void *)correrConsola,NULL);

	int socket = crearHostMultiConexion(puerto);
	correrServidorThreads(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);
	pthread_mutex_destroy(&mutexCache);
	pthread_mutex_destroy(&mutexMemoriaPrincipal);
	config_destroy(configFile);
	free(bloqueMemoria);
	free(bloqueCache);

	return EXIT_SUCCESS;
}
