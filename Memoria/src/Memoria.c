#include "Memoria.h"

void mostrarMensaje(char* mensaje,int socket){
	printf("Error: %s \n",mensaje);
}

//COMMONS

void showInScreenAndLog(char*message){
	printf("%s\n\r",message);
	log_info(logDumpFile,message);
}

int cantPaginasAdms(){
	return divAndRoundUp(TAM_ELM_TABLA_INV*marcos,marcoSize);
}

//COMMONS

//HASH

int32_t hash(int32_t pid,int32_t nroPag){
	if(nroPag != 0)
		return ((pid*nroPag)+(pid/nroPag));
	else
		return pid;
}

int32_t getHash(int32_t pid,int32_t nroPag){
	int hashResult = hash(pid,nroPag);
	int cantPags = (cantPaginasAdms()-1);//Es base 0 por eso le restamos uno a la cantidad de paginas
	if (hashResult > cantPags) {
	  return hashResult % cantPags;
	}else{
		return hashResult;
	}
}

//HASH

//CACHE

void freeCache(t_cache*cache){
	free(cache->contenido);
	free(cache);
}

t_cache* getPaginaCache(int indice){
	int offset = indice*(marcoSize+(sizeof(int32_t)*2));
	t_cache* cache = malloc(sizeof(t_cache));
	memcpy(&cache->pid,bloqueCache+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(&cache->nroPagina,bloqueCache+offset,sizeof(int32_t));
	offset+=sizeof(int32_t);
	cache->contenido = malloc(marcoSize);
	memcpy(cache->contenido,bloqueCache+offset,marcoSize);
	return cache;
}

int32_t cantEntradasCachePID(int32_t pid){
	bool matchPID(void* entrada){
			return ((t_cache_admin*)entrada)->pid==pid;
	}
	return list_count_satisfying(cacheEntradas,matchPID);
}


t_cache_admin* findMinorEntradas(){//Si hay alguna libre le doy esa sino busco la de menor entradas

	bool isFreeCache(void*e1){
		return ((t_cache_admin*)e1)->pid==-1;
	}

	t_cache_admin* posible = list_find(cacheEntradas,isFreeCache);

	if(posible!=NULL)
		return posible;

	bool minorEntradas(void* e1,void* e2){
		double diff1 = difftime(time(0),((t_cache_admin*)e1)->tiempoEntrada);
		double diff2 = difftime(time(0),((t_cache_admin*)e2)->tiempoEntrada);

		if(diff1>diff2)
			return true;
		else
			return false;
	}

	list_sort(cacheEntradas,minorEntradas);

	return list_get(cacheEntradas,0);
}

void clearEntradasCache(int32_t pid,int32_t nroPagina,int32_t pidReplace,int32_t nroPaginaReplace){

	void clearEntradas(void* entrada){
		if( (((t_cache_admin*)entrada)->pid == pid) && (nroPagina==-1 || (((t_cache_admin*)entrada)->nroPagina==nroPagina))){
			((t_cache_admin*)entrada)->tiempoEntrada=time(0);
			if(pidReplace != -1 && nroPaginaReplace != -1){
				((t_cache_admin*)entrada)->pid=pidReplace;
				((t_cache_admin*)entrada)->nroPagina=nroPaginaReplace;
			}else{
				((t_cache_admin*)entrada)->pid=-1;
				((t_cache_admin*)entrada)->nroPagina=0;
			}
		}
	}

	list_iterate(cacheEntradas,clearEntradas);
}

bool anyEntradaInCache(int32_t pid){
	bool matchPID(void* entrada){
		return ((t_cache_admin*)entrada)->pid==pid;
	}

	return list_any_satisfy(cacheEntradas,matchPID);
}

void findAndReplaceInCache(int32_t oldPID, int32_t oldNroPagina, int32_t pid, int32_t nroPagina, char* contenido){
	int i;
	int offset=0;
	for(i=0;i<entradasCache;i++){
		t_cache* cache= getPaginaCache(i);
		if(cache->pid==oldPID && (cache->nroPagina==oldNroPagina || oldNroPagina == -1)){//se manda en -1 cuando se quiere eliminar todas -> el contenido viene en null
			memcpy(bloqueCache+offset,&pid,sizeof(int32_t));
			offset+=sizeof(int32_t);
			memcpy(bloqueCache+offset,&nroPagina,sizeof(int32_t));
			offset+=sizeof(int32_t);
			if(contenido != NULL){
				memcpy(bloqueCache+offset,contenido,marcoSize);
				break;
			}else
				memset(bloqueCache+offset,0,marcoSize);
		}
		offset+=(sizeof(int32_t)*2)+marcoSize;
		freeCache(cache);
	}
}


void cacheMiss(int32_t pid, int32_t nroPagina,char* contenido){

	log_info(logFile,"Se produjo un cache miss de la pagina:%d del PID:%d ",nroPagina,pid);

	int32_t cantActuales = cantEntradasCachePID(pid);

	log_info(logFile,"El PID:%d tiene %d entradas en cache y el maximo es %d ",pid,cantActuales,cacheXproc);

	if (cantActuales < cacheXproc) {// Si tiene menos entradas que las permitidas por proceso en cache
		t_cache_admin* menorEntradas = findMinorEntradas();
		log_info(logFile,"Se selecciona como victima de la pagina:%d del PID:%d ",menorEntradas->nroPagina,menorEntradas->pid);
		clearEntradasCache(menorEntradas->pid,menorEntradas->nroPagina,pid,nroPagina);
		findAndReplaceInCache(menorEntradas->pid,menorEntradas->nroPagina,pid,nroPagina,contenido);
	}

}

t_cache* findInCache(int32_t pid,int32_t nroPagina){
	int i;
	for(i=0;i<entradasCache;i++){
		t_cache* cache= getPaginaCache(i);
		if(cache->pid==pid && cache->nroPagina==nroPagina)
			return cache;
	}
	return NULL;
}


t_cache crearCache(int32_t pid,int32_t nroPagina,char*contenido){
	t_cache cache;
	cache.pid=pid;
	cache.nroPagina=nroPagina;
	if(contenido!=NULL){
		cache.contenido=malloc(marcoSize);
		memcpy(cache.contenido,contenido,marcoSize);
	}
	return cache;
}

void inicializarCache(){
	int i;
	int offset=0;
	for(i=0;i<entradasCache;i++){
		t_cache* cache=malloc(sizeof(t_cache));//porque va sin contenido
		*cache = crearCache(-1,0,NULL);
		memcpy(bloqueCache+offset,&cache->pid,sizeof(int32_t));
		offset+=sizeof(int32_t);
		memcpy(bloqueCache+offset,&cache->nroPagina,sizeof(int32_t));
		offset+=sizeof(int32_t);
		memset(bloqueCache+offset,0,marcoSize);
		offset+=marcoSize;
	}
}

void inicializarEntradasCache(){
	list_clean_and_destroy_elements(cacheEntradas,free);
	int i;
	for(i=0;i<entradasCache;i++){
		t_cache_admin* admin = malloc(sizeof(t_cache_admin));
		admin->pid=-1;
		admin->nroPagina=0;
		admin->tiempoEntrada=time(0);
		list_add(cacheEntradas,admin);
	}
}

void addEntradaCache(int32_t pid, int32_t nroPagina){

	bool findByPID(void*entrada){
		return ((t_cache_admin*)entrada)->pid==pid && ((t_cache_admin*)entrada)->nroPagina==nroPagina;
	}

	t_cache_admin* entrada = (t_cache_admin*)list_find(cacheEntradas,findByPID);
	entrada->tiempoEntrada=time(0);
}

//CACHE

//INICIO FUNCIONES SOBRE PAGINAS ADMS

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

int paginasLibres(int *paginasLibres){
	int cantPaginasLibres=0;
	int i;
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		if(pag->pid==-1){
			cantPaginasLibres++;
			if(paginasLibres != NULL){
				paginasLibres[pag->indice]=1;
			}
		}else{
			if(paginasLibres != NULL){
				paginasLibres[pag->indice]=0;
			}
		}
		free(pag);
	}
	return cantPaginasLibres;
}

int cantPaginasPID(int32_t pid){
	int cantidad=0;
	int i;
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		if(pag->pid==pid)
			cantidad++;
		free(pag);
	}
	return cantidad;
}

void escribirPaginaEnTabla(t_pagina* pagina){
	int offset = (pagina->indice*sizeof(t_pagina))+sizeof(int32_t);//se agrega el tamaño del indice porque no se vuelve a escribir
	memcpy(bloqueMemoria+offset,&pagina->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,&pagina->numeroPag,sizeof(int32_t));
}

void escribirEnEstrucAdmin(t_pagina* pagina){
	int offset = pagina->indice*sizeof(t_pagina);
	memcpy(bloqueMemoria+offset,(void *)&pagina->indice,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,(void *)&pagina->pid,sizeof(int32_t));
	offset+=sizeof(int32_t);
	memcpy(bloqueMemoria+offset,(void *)&pagina->numeroPag,sizeof(int32_t));
}

int asignarPaginasPID(int32_t pid,int32_t paginasRequeridas,bool isNew){
	int cantPaginasLibres=0;
	int32_t* pagLibres = malloc(sizeof(int32_t)*cantPaginasAdms());
	sleep(retardoMemoria/1000);//pasamos a milisegundos
	pthread_mutex_lock(&mutexMemoriaPrincipal);

	cantPaginasLibres=paginasLibres(pagLibres);

	if(cantPaginasLibres>=paginasRequeridas){
		//Hay paginas suficientes
		int i;
		int offset = isNew?0:cantPaginasPID(pid);//Se usa en el caso de asignar nuevas paginas ->Para contar desde donde se quedo
		for(i=0;i<paginasRequeridas;i++){
			int hashIndice = getHash(pid,offset+i);//Buscamos el indice correspondiente a ese pid y nroPagina
			int indice=hashIndice;
			int reverse=0;//Para buscar para atras

			while(!pagLibres[indice]){//Recorremos si no esta libre la pagina del hash hasta encontrar una que si
				if(indice<(cantPaginasAdms()-1) && !reverse)
					indice++;
				else{
					if(indice>=hashIndice){
						indice=hashIndice;
						reverse=1;
					}
					indice--;
				}

			}

			t_pagina* pagina = malloc(sizeof(t_pagina));
			*pagina = crearPagina(indice,pid,offset+i);
			escribirPaginaEnTabla(pagina);
			pagLibres[indice]=0;//Marcamos la pagina como ocupada

			free(pagina);
		}
		pthread_mutex_unlock(&mutexMemoriaPrincipal);
		free(pagLibres);

		return 1;
	}else{
		return 0;
	}

}

t_pagina* encontrarPagina(int32_t pid,int32_t nroPagina){
	int32_t hashIndice = getHash(pid,nroPagina);
	t_pagina* pag= getPagina(hashIndice);
	int indice=hashIndice;
	int reverse=0;//Para buscar para atras

	while(pag->pid!=pid && pag->numeroPag!=nroPagina){//Recorremos si la pagina que retorna el hash no es la que corresponde
		free(pag);
		if(indice<(cantPaginasAdms()-1) && !reverse)
			indice++;
		else{
			if(indice>=hashIndice){
				indice=hashIndice;
				reverse=1;
			}
			if(indice>0)
				indice--;
			else{
				free(pag);
				pag=NULL;
				break;
			}
		}
		pag=getPagina(indice);
	}

	return pag;
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
	if(size>2){
		printf("La funcion de dumpProcesos no debe recibir parametros o solo el PID del proceso a filtrar.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	int32_t pid = -1;

	if(size==2){
		pid = atoi(functionAndParams[1]);
	}

	showInScreenAndLog("-----------------------------------------------------------------------------------------------");
	showInScreenAndLog("| #FRAME |					CONTENIDO					|");
	showInScreenAndLog("-----------------------------------------------------------------------------------------------");

	int i;
	int32_t offsetEstrucAdmin=cantPaginasAdms()*TAM_ELM_TABLA_INV;

	pthread_mutex_lock(&mutexMemoriaPrincipal);
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);

		int32_t offsetHastaPagina= (marcoSize*(pag->indice));
		int32_t offsetTotal = offsetEstrucAdmin+offsetHastaPagina;

		if(pid == -1 || pag->pid == pid){
			char*contenido = malloc(marcoSize);
			memcpy(contenido,bloqueMemoria+offsetTotal,marcoSize);

			char* message = string_from_format("|   %d	 |",pag->indice);

			log_info(logDumpFile,message);
			printf("%s",message);

			fwrite(contenido,marcoSize,1,stdout);
			log_info(logDumpFile,contenido);

			printf("\r\n");
			showInScreenAndLog("-----------------------------------------------------------------------------------------------");

			free(pag);
			free(message);
			free(contenido);
		}else{
			free(pag);
			continue;
		}
	}
	pthread_mutex_unlock(&mutexMemoriaPrincipal);



	freeElementsArray(functionAndParams,size);
}

void dumpTabla(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de dumpTabla no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	showInScreenAndLog("----------------------");
	showInScreenAndLog("|  I  |  PID | NRO |");
	showInScreenAndLog("----------------------");

	int i;
	pthread_mutex_lock(&mutexMemoriaPrincipal);
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		char* message = string_from_format("|  %d  |  %d  |  %d  |",pag->indice,pag->pid,pag->numeroPag);
		showInScreenAndLog(message);
		showInScreenAndLog("----------------------");
		free(pag);
		free(message);
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
	inicializarCache();
	inicializarEntradasCache();
	pthread_mutex_unlock(&mutexCache);

	freeElementsArray(functionAndParams,size);
}

void sizeMemory(int size, char** functionAndParams){
	if(size!=1){
		printf("La funcion de sizeMemory no debe recibir parametros.\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	int framesTotales = cantPaginasAdms();

	pthread_mutex_lock(&mutexMemoriaPrincipal);
	int framesLibres = paginasLibres(NULL);
	pthread_mutex_unlock(&mutexMemoriaPrincipal);

	int framesOcupados = framesTotales-framesLibres;
	printf("Frames Totales:%d\n\r",framesTotales);
	printf("Frames Libres:%d\n\r",framesLibres);
	printf("Frames Ocupados:%d\n\r",framesOcupados);

	freeElementsArray(functionAndParams,size);
}

void sizePID(int size, char** functionAndParams){
	if(size!=2){
		printf("La funcion de sizePID debe recibir solo 1 parametro (el PID del proceso).\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	int32_t pid = atoi(functionAndParams[1]);

	pthread_mutex_lock(&mutexMemoriaPrincipal);
	printf("Cantidad de Paginas :%d\n\r",cantPaginasPID(pid));
	pthread_mutex_unlock(&mutexMemoriaPrincipal);

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

//INICIO INTERFAZ MEMORIA

void iniciarPrograma(char* data,int socket){
	int hayEspacio;
	t_pedido_inicializar* pedido = deserializar_pedido_inicializar(data);

	log_info(logFile,"Pedido de inicio de programa PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);

	hayEspacio=asignarPaginasPID(pedido->idPrograma,pedido->pagRequeridas,false);

	t_respuesta_inicializar* respuesta = malloc(sizeof(t_respuesta_inicializar));
	respuesta->idPrograma=pedido->idPrograma;

	if(hayEspacio){
		respuesta->codigoRespuesta= OK_INICIALIZAR;
		log_info(logFile,"Pedido de inicio de programa exitoso PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);
	}else{
		log_info(logFile,"Pedido de inicio de programa sin espacio PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);
		respuesta->codigoRespuesta= SIN_ESPACIO_INICIALIZAR;
	}
	char* buffer = serializar_respuesta_inicializar(respuesta);
	empaquetarEnviarMensaje(socket,"RES_INICIALIZAR",sizeof(t_respuesta_inicializar),buffer);

	free(buffer);
	free(pedido);
	free(respuesta);
}

void solicitarBytes(char* data,int socket){
	t_pedido_solicitar_bytes* pedido = deserializar_pedido_solicitar_bytes(data);

	t_respuesta_solicitar_bytes* respuesta = malloc(sizeof(t_respuesta_solicitar_bytes));
	respuesta->pid=pedido->pid;
	respuesta->pagina=pedido->pagina;

	log_info(logFile,"Solicitud de bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);

	pthread_mutex_lock(&mutexCache);
	t_cache* cache = findInCache(pedido->pid,pedido->pagina);

	if(cache!=NULL){
		log_info(logFile,"Pagina encontrada en cache PID:%d Pagina:%d",pedido->pid,pedido->pagina);

		char* dataRecuperada=NULL;

		if(marcoSize -(pedido->offsetPagina+pedido->tamanio) <0){
			respuesta->codigo=PAGINA_SOLICITAR_OVERFLOW;
			respuesta->tamanio=5;
			respuesta->data="ERROR";
			log_info(logFile,"Overflow al solicitar bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);
		}else{
			addEntradaCache(pedido->pid,pedido->pagina);

			respuesta->codigo=OK_SOLICITAR;
			respuesta->tamanio=pedido->tamanio;
			dataRecuperada = malloc(pedido->tamanio);
			memcpy(dataRecuperada,cache->contenido+pedido->offsetPagina,pedido->tamanio);
			freeCache(cache);
			respuesta->data=dataRecuperada;
			log_info(logFile,"Exito al solicitar bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);
		}

		pthread_mutex_unlock(&mutexCache);

		char* buffer = serializar_respuesta_solicitar_bytes(respuesta);
		empaquetarEnviarMensaje(socket,"RTA_SL_BYTES",sizeof(int32_t)*3+sizeof(codigo_solicitar_bytes)+respuesta->tamanio,buffer);
		free(buffer);
		free(respuesta);
		free(pedido);
		if(dataRecuperada != NULL)
			free(dataRecuperada);

		return;
	}

	sleep(retardoMemoria/1000);
	pthread_mutex_lock(&mutexMemoriaPrincipal);

	t_pagina* pag = encontrarPagina(pedido->pid,pedido->pagina);
	char* dataRecuperada = NULL;

	if (pag==NULL){
		respuesta->codigo=PAGINA_SOL_NOT_FOUND;
		respuesta->tamanio=5;
		respuesta->data="ERROR";
		log_info(logFile,"Pagina no encontrada PID:%d PAG:%d",pedido->pid,pedido->pagina);
	}else{
		int32_t offsetEstrucAdmin=cantPaginasAdms()*TAM_ELM_TABLA_INV;
		int32_t offsetHastaData= (marcoSize*(pag->indice))+pedido->offsetPagina;
		int32_t offsetTotal = offsetEstrucAdmin+offsetHastaData;

		if(marcoSize -(pedido->offsetPagina+pedido->tamanio) <0){
			respuesta->codigo=PAGINA_SOLICITAR_OVERFLOW;
			respuesta->tamanio=5;
			respuesta->data="ERROR";
			log_info(logFile,"Overflow al solicitar bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);
		}else{
			respuesta->codigo=OK_SOLICITAR;
			respuesta->tamanio=pedido->tamanio;
			dataRecuperada = malloc(pedido->tamanio);
			memcpy(dataRecuperada,bloqueMemoria+offsetTotal,pedido->tamanio);
			cacheMiss(pedido->pid,pedido->pagina,bloqueMemoria+(offsetEstrucAdmin+(marcoSize*(pag->indice))));

			respuesta->data=dataRecuperada;
			log_info(logFile,"Exito al solicitar bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);
		}
	}

	pthread_mutex_unlock(&mutexCache);
	pthread_mutex_unlock(&mutexMemoriaPrincipal);
	char* buffer = serializar_respuesta_solicitar_bytes(respuesta);
	empaquetarEnviarMensaje(socket,"RTA_SL_BYTES",sizeof(int32_t)*3+sizeof(codigo_solicitar_bytes)+respuesta->tamanio,buffer);

	if(dataRecuperada!=NULL)
		free(dataRecuperada);
	if(pag!=NULL)
		free(pag);
	free(buffer);
	free(respuesta);
	free(pedido);

}

void almacenarBytes(char* data,int socket){
	t_pedido_almacenar_bytes* pedido = deserializar_pedido_almacenar_bytes(data);

	log_info(logFile,"Solicitud de almacenamiento bytes PID:%d PAG:%d OFFSET:%d TAMANIO:%d",pedido->pid,pedido->pagina,pedido->offsetPagina,pedido->tamanio);

	t_respuesta_almacenar_bytes* respuesta = malloc(sizeof(t_respuesta_almacenar_bytes));
	respuesta->pid=pedido->pid;

	bool inCache = true;

	pthread_mutex_lock(&mutexCache);
	t_cache* cache = findInCache(pedido->pid,pedido->pagina);
	if(cache==NULL){
		inCache=false;
		pthread_mutex_unlock(&mutexCache);//Si no esta en cache desbloqueamos el acceso sino se espera hasta que escribamos en memoria
	}
	sleep(retardoMemoria/1000);
	pthread_mutex_lock(&mutexMemoriaPrincipal);

	t_pagina* pag = encontrarPagina(pedido->pid,pedido->pagina);
	if(pag==NULL){
		respuesta->codigo=PAGINA_ALM_NOT_FOUND;
		log_info(logFile,"Pagina no encontrada PID:%d PAG:%d",pedido->pid,pedido->pagina);
	}else{
		int32_t offsetEstrucAdmin=cantPaginasAdms()*TAM_ELM_TABLA_INV;
		int32_t offsetHastaData= (marcoSize*(pag->indice))+pedido->offsetPagina;
		int32_t offsetTotal = offsetEstrucAdmin+offsetHastaData;

		if(marcoSize-(pedido->offsetPagina + pedido->tamanio) < 0){
			respuesta->codigo=PAGINA_ALM_OVERFLOW;
			log_info(logFile,"Overflow al escribir en pagina PID:%d PAG:%d TAMANIO:%d OFFSET:%d",pedido->pid,pedido->pagina,pedido->tamanio,pedido->offsetPagina);
		}else{
			memcpy(bloqueMemoria+offsetTotal,pedido->data,pedido->tamanio);
			if(cache!=NULL){
				log_info(logFile,"Se actualiza la pagina de cache del PID:%d NRO:%d",cache->pid,cache->nroPagina);
				memcpy(cache->contenido+pedido->offsetPagina,pedido->data,pedido->tamanio);
				freeCache(cache);
			}
			log_info(logFile,"Pedido correcto escribir en pagina PID:%d PAG:%d TAMANIO:%d OFFSET:%d",pedido->pid,pedido->pagina,pedido->tamanio,pedido->offsetPagina);
			respuesta->codigo=OK_ALMACENAR;
		}
	}

	if(inCache)
		pthread_mutex_unlock(&mutexCache);
	pthread_mutex_unlock(&mutexMemoriaPrincipal);

	char*buffer = serializar_respuesta_almacenar_bytes(respuesta);
	empaquetarEnviarMensaje(socket,"RES_ALMC",sizeof(t_respuesta_almacenar_bytes),buffer);

	free(buffer);
	free(pedido->data);
	free(pedido);
	free(respuesta);
	if(pag!=NULL)
		free(pag);
}

void asignarPaginas(char* data,int socket){
	int hayEspacio;

	t_pedido_inicializar* pedido = malloc(sizeof(t_pedido_inicializar));
	pedido = deserializar_pedido_inicializar(data);

	log_info(logFile,"Pedido de paginas de programa PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);

	hayEspacio=asignarPaginasPID(pedido->idPrograma,pedido->pagRequeridas,false);

	t_respuesta_inicializar* respuesta = malloc(sizeof(t_respuesta_inicializar));
	respuesta->idPrograma=pedido->idPrograma;

	if(hayEspacio){
		respuesta->codigoRespuesta= OK_INICIALIZAR;
		log_info(logFile,"Pedido de paginas de programa exitoso PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);
	}else{
		respuesta->codigoRespuesta= SIN_ESPACIO_INICIALIZAR;
		log_info(logFile,"Pedido de paginas de programa sin espacio PID:%d PAGS:%d",pedido->idPrograma,pedido->pagRequeridas);
	}

	char* buffer = serializar_respuesta_inicializar(respuesta);

	empaquetarEnviarMensaje(socket,"RES_ASIGNAR",sizeof(t_respuesta_inicializar),buffer);

	free(respuesta);
	free(buffer);
	free(pedido);

}

void finalizarPrograma(char* data,int socket){
	t_pedido_finalizar_programa* pedido = deserializar_pedido_finalizar_programa(data);

	pthread_mutex_lock(&mutexCache);

	if(anyEntradaInCache(pedido->pid)){
		clearEntradasCache(pedido->pid,-1,-1,-1);
		findAndReplaceInCache(pedido->pid,-1,-1,0,NULL);
	}

	pthread_mutex_unlock(&mutexCache);

	log_info(logFile,"Pedido para finalizar programa PID:%d",pedido->pid);

	int i;
	sleep(retardoMemoria/1000);
	pthread_mutex_lock(&mutexMemoriaPrincipal);
	for(i=0;i<cantPaginasAdms();i++){
		t_pagina* pag = getPagina(i);
		if(pag->pid==pedido->pid){
			pag->pid=-1;

			int32_t offsetEstrucAdmin=cantPaginasAdms()*TAM_ELM_TABLA_INV;
			int32_t offsetHastaData= (marcoSize*(pag->indice));
			int32_t offsetTotal = offsetEstrucAdmin+offsetHastaData;

			memset(bloqueMemoria+offsetTotal,0,marcoSize);

			pag->numeroPag=0;
			escribirPaginaEnTabla(pag);
		}
		free(pag);
	}
	pthread_mutex_unlock(&mutexMemoriaPrincipal);

	t_respuesta_finalizar_programa* respuesta = malloc(sizeof(t_respuesta_finalizar_programa));
	respuesta->pid=pedido->pid;
	respuesta->codigo=OK_FINALIZAR;

	char*buffer = serializar_respuesta_finalizar_programa(respuesta);

	empaquetarEnviarMensaje(socket,"RES_FINALIZAR",sizeof(t_respuesta_finalizar_programa),buffer);

	free(buffer);
	free(respuesta);
	free(pedido);
}

void getMarcos(char* data,int socket){
	char* buffer = string_itoa(marcoSize);
	empaquetarEnviarMensaje(socket,"RECB_MARCOS",strlen(buffer),buffer);
	free(buffer);
}

//FIN INTERFAZ MEMORIA

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
	logDumpFile = log_create("MemoriaDump.log","Memoria",0,LOG_LEVEL_TRACE);

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

	bloqueCache = (char*) calloc(entradasCache*(marcoSize+(sizeof(int32_t)*2)),sizeof(char));
	cacheEntradas= list_create();
	inicializarEntradasCache();
	inicializarCache();

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
	dictionary_put(diccionarioFunciones,"GET_MARCOS",&getMarcos);

	t_dictionary* diccionarioHandshakes = dictionary_create();
	dictionary_put(diccionarioHandshakes,"HCPME","HMECP");
	dictionary_put(diccionarioHandshakes,"HKEME","HMEKE");

	pthread_mutex_init(&mutexCache,NULL);
	pthread_mutex_init(&mutexMemoriaPrincipal,NULL);

	pthread_t threadConsola;
	pthread_create(&threadConsola,NULL,(void *)correrConsola,NULL);

	int socket = crearHostMultiConexion(puerto);
	correrServidorThreads(socket,NULL,NULL,diccionarioFunciones,diccionarioHandshakes);

	//free
	dictionary_destroy(diccionarioFunciones);
	dictionary_destroy(diccionarioHandshakes);

	pthread_mutex_destroy(&mutexCache);
	pthread_mutex_destroy(&mutexMemoriaPrincipal);

	config_destroy(configFile);

	log_destroy(logFile);
	log_destroy(logDumpFile);

	list_destroy_and_destroy_elements(cacheEntradas,free);

	free(bloqueMemoria);
	free(bloqueCache);

	return EXIT_SUCCESS;
}
