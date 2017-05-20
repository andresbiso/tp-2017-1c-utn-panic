#include "CPU.h"


void recibirTamanioPagina(int socket){
	empaquetarEnviarMensaje(socket,"GET_MARCOS",sizeof("GET_MARCOS"),"GET_MARCOS");
	t_package* paquete = recibirPaquete(socket,NULL);
	pagesize = atoi(paquete->datos);
}

void waitKernel(int socketKernel,t_dictionary* diccionarioFunciones){
	while(1){
		t_package* paquete = recibirPaquete(socketKernel,NULL);
		procesarPaquete(paquete, socketKernel, diccionarioFunciones, NULL);
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
	ejecutarPrograma();
	t_pcb_serializado* paqueteSerializado = serializar_pcb(actual_pcb);
	empaquetarEnviarMensaje(socketKernel, "RET_PCB", paqueteSerializado->tamanio, paqueteSerializado->contenido_pcb);
	free(paqueteSerializado->contenido_pcb);
	free(paqueteSerializado);
	destruir_pcb(actual_pcb);
}

void ejecutarPrograma() {
	t_pedido_solicitar_bytes* pedido = (t_pedido_solicitar_bytes*)malloc(sizeof (t_pedido_solicitar_bytes));
	int instruccionActual = 0;
	int fifo = quantum == 0;
	int cicloActual = quantum;
	while(instruccionActual < actual_pcb->cant_instrucciones && (fifo || cicloActual > 0)) {
		pedido->pid = actual_pcb->pid;
		pedido->pagina = actual_pcb->indice_codigo->pag;
		pedido->offsetPagina = actual_pcb->indice_codigo->offset;
		pedido->tamanio = actual_pcb->indice_codigo->size;
		char* buffer =  serializar_pedido_solicitar_bytes(pedido);
		int longitudMensaje = sizeof(t_pedido_solicitar_bytes);
		if(empaquetarEnviarMensaje(socketMemoria, "SOLC_BYTES", longitudMensaje, buffer)) {
			perror("Hubo un error procesando el paquete");
			exit(EXIT_FAILURE);
		}}
		t_package* paqueteRespuesta = recibirPaquete(socketMemoria, NULL);
		t_respuesta_solicitar_bytes* bufferRespuesta = deserializar_respuesta_solicitar_bytes(paqueteRespuesta->datos);
		ejecutarInstruccion(bufferRespuesta);
		borrarPaquete(paqueteRespuesta);
		free(bufferRespuesta->data);
		free(bufferRespuesta);
		sleep(quantumSleep * 0.001);
		cicloActual--;
	}
	free(pedido);
}

void ejecutarInstruccion(t_respuesta_solicitar_bytes* respuesta) {
	if (respuesta->codigo != OK_SOLICITAR) {
		perror("Hubo un error al solicitar la página");
		exit(EXIT_FAILURE);
	}
	analizadorLinea(respuesta->data, funcionesParser->funciones_comunes, funcionesParser->funciones_kernel);
}

void mostrarMensaje(char* mensaje, int socket){
	printf("Mensaje recibido: %s \n",mensaje);
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

	printf("PUERTO KERNEL: %d\n",puertoKernel);
	printf("IP KERNEL: %s\n",ipKernel);
	printf("PUERTO MEMORIA: %d\n",puertoMemoria);
	printf("IP MEMORIA: %s\n",ipMemoria);

	t_dictionary* diccionarioFunciones = dictionary_create();
	dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
	dictionary_put(diccionarioFunciones,"CORRER_PCB",&correrPCB);
	dictionary_put(diccionarioFunciones,"NUEVO_QUANTUM",&modificarQuantum);
	dictionary_put(diccionarioFunciones,"NUEVO_QUANTUM_SLEEP",&modificarQuantumSleep);
	// ver comuncicaciones memoria-kernel

	cpu_log = log_create("cpu.log","CPU",1,LOG_LEVEL_TRACE);

	socketKernel = conectar(ipKernel,puertoKernel);
	if(!handshake(socketKernel,"HCPKE","HKECP")){
		log_error(cpu_log,"No se pudo realizar la conexion con el kernel");
		exit(EXIT_FAILURE);
	}

	socketMemoria = conectar(ipMemoria,puertoMemoria);
	if(!handshake(socketMemoria,"HCPME","HMECP")){
		log_error(cpu_log,"No se pudo realizar la conexion con la memoria");
		exit(EXIT_FAILURE);
	}

    recibirTamanioPagina(socketMemoria);

	funcionesParser = inicializar_primitivas();
	waitKernel(socketKernel, diccionarioFunciones);

	liberarFuncionesAnsisop(funcionesParser);
	dictionary_destroy(diccionarioFunciones);
	config_destroy(configFile);
	log_destroy(cpu_log);

	return EXIT_SUCCESS;
}




