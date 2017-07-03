#include "CPU.h"


void recibirTamanioPagina(int socket){
	empaquetarEnviarMensaje(socket,"GET_MARCOS",strlen("GET_MARCOS\0"),"GET_MARCOS");
	t_package* paquete = recibirPaquete(socket,NULL);
	pagesize = atoi(paquete->datos);
	log_info(cpu_log,"Tamaño de pagina de memoria %d",pagesize);
	borrarPaquete(paquete);
}

void desconexionKernel(int socket){
	desconexion=true;
	log_warning(cpu_log,"El kernel se desconecto");
}

void waitKernel(int socketKernel,t_dictionary* diccionarioFunciones){
	while(1){
		t_package* paquete = recibirPaquete(socketKernel,&desconexionKernel);
		if(desconexion){
			borrarPaquete(paquete);
			break;
		}
		procesarPaquete(paquete, socketKernel, diccionarioFunciones, NULL,NULL);

		if(terminoCPU){
			break;
		}
	 }
}

void modificarQuantum(char*data, int socket) {
	quantum = atoi(data);
	log_info(cpu_log, "Quantum=%d", quantum);
}

void modificarQuantumSleep(char*data, int socket) {
	quantumSleep = atoi(data);
	log_info(cpu_log, "QuantumSleep=%d", quantumSleep);
}

void correrPCB(char* pcb, int socket){
	actual_pcb = deserializar_pcb(pcb);
	log_info(cpu_log,"Recibí PCB del PID:%d",actual_pcb->pid);
	if(!signalSIGUSR1)
		ejecutarPrograma();
	if (!proceso_bloqueado) {
		t_pcb_serializado* paqueteSerializado = serializar_pcb(actual_pcb);

		t_retornar_pcb retornar;
		retornar.pcb = actual_pcb;
		retornar.rafagasEjecutadas=rafagas_ejecutadas;
		retornar.desconectar=signalSIGUSR1;

		char* buffer = serializar_retornar_pcb(&retornar,paqueteSerializado);
		empaquetarEnviarMensaje(socketKernel, "RET_PCB", paqueteSerializado->tamanio+sizeof(int32_t)+sizeof(bool), buffer);
		log_info(cpu_log,"Finaliza procesamiento PCB del PID:%d",actual_pcb->pid);

		free(buffer);
		free(paqueteSerializado->contenido_pcb);
		free(paqueteSerializado);
	}
	error_en_ejecucion = 0;
	proceso_bloqueado = 0;
	rafagas_ejecutadas = 0;

	destruir_pcb(actual_pcb);
}

void ejecutarPrograma() {
	t_pedido_solicitar_bytes* pedido = (t_pedido_solicitar_bytes*)malloc(sizeof (t_pedido_solicitar_bytes));
	int fifo = quantum == 0;
	int cicloActual = quantum;
	while(actual_pcb->pc < actual_pcb->cant_instrucciones && (fifo || cicloActual > 0)) {
		pedido->pid = actual_pcb->pid;
		pedido->pagina = actual_pcb->indice_codigo[actual_pcb->pc].pag;
		pedido->offsetPagina = actual_pcb->indice_codigo[actual_pcb->pc].offset;
		pedido->tamanio = actual_pcb->indice_codigo[actual_pcb->pc].size;

		actual_pcb->pc++;
		rafagas_ejecutadas++;

		char* buffer =  serializar_pedido_solicitar_bytes(pedido);
		int longitudMensaje = sizeof(t_pedido_solicitar_bytes);
		if(!empaquetarEnviarMensaje(socketMemoria, "SOLC_BYTES", longitudMensaje, buffer)) {
			perror("Hubo un error procesando el paquete");
			exit(EXIT_FAILURE);
		}
		t_package* paqueteRespuesta = recibirPaquete(socketMemoria, NULL);
		t_respuesta_solicitar_bytes* bufferRespuesta = deserializar_respuesta_solicitar_bytes(paqueteRespuesta->datos);
		ejecutarInstruccion(bufferRespuesta);
		borrarPaquete(paqueteRespuesta);
		free(bufferRespuesta->data);
		free(bufferRespuesta);
		sleep(quantumSleep * 0.001);
		cicloActual--;
		if(error_en_ejecucion || proceso_bloqueado)
			break;
	}
	//Finalizo ok si no hubo un error en la ejecucion y es fifo (ejecuta todas las rafagas) o es RR y llega hasta la ultima instruccion
	if(!error_en_ejecucion && !proceso_bloqueado && (fifo || actual_pcb->pc==actual_pcb->cant_instrucciones) )
		actual_pcb->exit_code=FINALIZAR_OK;
	free(pedido);
}

void ejecutarInstruccion(t_respuesta_solicitar_bytes* respuesta) {
	if (respuesta->codigo != OK_SOLICITAR) {
		perror("Hubo un error al solicitar la página");
		exit(EXIT_FAILURE);
	}
	respuesta->data=realloc(respuesta->data,respuesta->tamanio+1);
	respuesta->data[respuesta->tamanio]='\0';//Sino analizadorLinea rompe
	log_info(cpu_log,"Analizando linea: %s",respuesta->data);
	analizadorLinea(respuesta->data, funcionesParser->funciones_comunes, funcionesParser->funciones_kernel);
}

void mostrarMensaje(char* mensaje, int socket){
	printf("Mensaje recibido: %s \n",mensaje);
}

void signalHandler(int signal){
	signalSIGUSR1=true;

	log_info(cpu_log,"Se recibe signal SIGUSR1 se solicita la desconexión");
	empaquetarEnviarMensaje(socketKernel,"FINISH_CPU",10,"FINISH_CPU");//la data del mensaje es trivial
}

void finishCPU(char* data, int socket){
	log_info(cpu_log,"Se procede a la desconexión por autorizacion del Kernel");
	terminoCPU=true;
}

t_config* cargarConfiguracion(char * nombreArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);

	if (config_has_property(configFile, "PUERTO_KERNEL")) {
		puertoKernel = config_get_int_value(configFile, "PUERTO_KERNEL");
	} else {
		perror("La key PUERTO_KERNEL no existe");
		exit(EXIT_FAILURE);
	}

	if(config_has_property(configFile, "IP_KERNEL") == true){
		ipKernel = config_get_string_value(configFile,"IP_KERNEL");
	}else{
		perror("La key IP_KERNEL no existe");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(configFile, "PUERTO_MEMORIA")) {
		puertoMemoria = config_get_int_value(configFile, "PUERTO_MEMORIA");
	} else {
		perror("La key PUERTO_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}

	if(config_has_property(configFile, "IP_MEMORIA") == true){
		ipMemoria = config_get_string_value(configFile,"IP_MEMORIA");
	}else{
		perror("La key IP_MEMORIA no existe");
		exit(EXIT_FAILURE);
	}

	return configFile;
}

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("Falta parametro: archivo de configuracion");
		exit(EXIT_FAILURE);
	}
	t_config* configFile = cargarConfiguracion(argv[1]);
	quantum=0;//Arranca en 0 porque si es fifo kernel no manda el quantum
	quantumSleep=0;
	error_en_ejecucion = 0;
	proceso_bloqueado = 0;
	rafagas_ejecutadas = 0;

	printf("PUERTO KERNEL: %d\n",puertoKernel);
	printf("IP KERNEL: %s\n",ipKernel);
	printf("PUERTO MEMORIA: %d\n",puertoMemoria);
	printf("IP MEMORIA: %s\n",ipMemoria);

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"CORRER_PCB",&correrPCB);
	dictionary_put(diccionarioFunciones,"FINISH_CPU",&finishCPU);
	dictionary_put(diccionarioFunciones,"NUEVO_QUANTUM",&modificarQuantum);
	dictionary_put(diccionarioFunciones,"NUEVO_QUANTUM_SLEEP",&modificarQuantumSleep);
	// ver comuncicaciones memoria-kernel

	cpu_log = log_create("cpu.log","CPU",0,LOG_LEVEL_TRACE);

	socketMemoria = conectar(ipMemoria,puertoMemoria);
	if(!handshake(socketMemoria,"HCPME","HMECP")){
		log_error(cpu_log,"No se pudo realizar la conexion con la memoria");
		exit(EXIT_FAILURE);
	}

    recibirTamanioPagina(socketMemoria);

	socketKernel = conectar(ipKernel,puertoKernel);
	if(!handshake(socketKernel,"HCPKE","HKECP")){
		log_error(cpu_log,"No se pudo realizar la conexion con el kernel");
		exit(EXIT_FAILURE);
	}

	signal(SIGUSR1, signalHandler);

	funcionesParser = inicializar_primitivas();
	waitKernel(socketKernel, diccionarioFunciones);

	liberarFuncionesAnsisop(funcionesParser);
	dictionary_destroy(diccionarioFunciones);
	config_destroy(configFile);
	log_destroy(cpu_log);

	return EXIT_SUCCESS;
}




