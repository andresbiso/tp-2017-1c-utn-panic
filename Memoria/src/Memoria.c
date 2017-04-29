#include "Memoria.h"

void mostrarMensaje(char* mensaje,int socket){
	printf("Error: %s \n",mensaje);
}

//INICIO FUNCIONES SOBRE PAGINAS ADMS

int cantPaginasAdms(){
	return divAndRoundUp(TAM_ELM_TABLA_INV*marcos,marcoSize);
}

t_pagina crearPagina(int32_t indice,int32_t pid,int32_t numeroPag){
	t_pagina pagina;
	pagina.indice = indice;
	pagina.pid = pid;
	pagina.numeroPag = numeroPag;
	return pagina;
}

t_pagina* getPagina(int indice){
	int offset = indice*sizeof(t_pagina);
	t_pagina* pagina = malloc(sizeof(t_pagina));
	memcpy(&pagina->indice,bloqueMemoria+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pagina->pid,bloqueMemoria+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&pagina->numeroPag,bloqueMemoria+offset,sizeof(int32_t));
	return pagina;
}

void escribirPaginaEnTabla(t_pagina* pagina){
	int offset = (pagina->indice*sizeof(t_pagina))+sizeof(int32_t);//se agrega el tamaño del indice porque no se vuelve a escribir
	memcpy(bloqueMemoria+offset,&pagina->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,&pagina->numeroPag,sizeof(int32_t));
}

void escribirEnEstrucAdmin(t_pagina* pagina){
	int offset = pagina->indice*sizeof(t_pagina);
	pthread_mutex_lock(&mutexMemoriaPrincipal);
	memcpy(bloqueMemoria+offset,(void *)&pagina->indice,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,(void *)&pagina->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,(void *)&pagina->numeroPag,sizeof(int32_t));
	pthread_mutex_unlock(&mutexMemoriaPrincipal);
}

//FIN FUNCIONES SOBRE PAGINAS ADMS

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
	printf("----------------------\n\r");
	printf("|  I  |  PID | NRO |\n\r");
	printf("----------------------\n\r");

	int i;
	pthread_mutex_lock(&mutexMemoriaPrincipal);
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		printf("|  %d  |  %d  |  %d  |\n\r",pag->indice,pag->pid,pag->numeroPag);
		printf("----------------------\n\r");
		free(pag);
	}
	pthread_mutex_unlock(&mutexMemoriaPrincipal);

	freeElementsArray(functionAndParams,size);
}

void retardo(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de retardo debe recibir solo 1 parametro (tiempo de retardo en ms).\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
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
	int32_t pid;
	int32_t paginasRequeridas;
	int32_t cantPaginasLibres = 0;
	t_list* posiblesPags = list_create();
	int hayEspacio=0;
	int i;

	memcpy(&pid,(void*)data,sizeof(int32_t));
	memcpy(&paginasRequeridas,(void*)data+sizeof(int32_t),sizeof(int32_t));

	pthread_mutex_lock(&mutexLog);
	log_info(logFile,string_from_format("Pedido de inicio de programa PID:%d PAGS:%d",pid,paginasRequeridas));
	pthread_mutex_unlock(&mutexLog);

	sleep(retardoMemoria/1000);//pasamos a milisegundos
	pthread_mutex_lock(&mutexMemoriaPrincipal);

	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		if(pag->pid==-1){
			cantPaginasLibres++;
			list_add(posiblesPags,(void*)pag->indice);
		}
		free(pag);
		if(cantPaginasLibres>=paginasRequeridas)
			break;
	}

	if(cantPaginasLibres>=paginasRequeridas){
		//Hay paginas suficientes
		hayEspacio=1;
		i=0;
		for(i=0;i<paginasRequeridas;i++){
			int indice = (int)list_get(posiblesPags,i);

			t_pagina* pagina = malloc(sizeof(t_pagina));
			*pagina = crearPagina(indice,pid,i);
			escribirPaginaEnTabla(pagina);

			free(pagina);
		}
	}

	pthread_mutex_unlock(&mutexMemoriaPrincipal);
	list_destroy(posiblesPags);

	//TODO Avisar al kernel si pudo o no asignar segun la variable hayEspacio
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

void crearEstructurasAdministrativas(){
	int pagAdminis = cantPaginasAdms();
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
	logFile = log_create("Memoria.log","Memoria",0,LOG_LEVEL_TRACE);

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
		log_error(logFile,"No se pudo reservar memoria para el bloque principal");
		exit(-1);
	}

	bloqueCache = (char*) calloc(entradasCache*marcoSize,sizeof(char));

	if (bloqueCache == NULL){
		log_error(logFile,"No se pudo reservar memoria para el bloque de la cache");
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
	pthread_mutex_init(&mutexLog,NULL);

	pthread_t threadConsola;
	pthread_create(&threadConsola,NULL,(void *)correrConsola,NULL);

	int socket = crearHostMultiConexion(puerto);
	correrServidorThreads(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);
	pthread_mutex_destroy(&mutexCache);
	pthread_mutex_destroy(&mutexMemoriaPrincipal);
	pthread_mutex_destroy(&mutexLog);
	config_destroy(configFile);
	log_destroy(logFile);
	free(bloqueMemoria);
	free(bloqueCache);

	return EXIT_SUCCESS;
}
