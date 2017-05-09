#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <parser/metadata_program.h>
#include "Kernel.h"
#include "estados.h"
#include <semaphore.h>

typedef struct {
    int socketEscucha;
    void (*nuevaConexion) (int);
    void (*desconexion) (int);
    t_dictionary* funciones;
    t_dictionary* handshakes;
}threadParams;

typedef struct {
	int32_t socket;
	char* codigo;
} __attribute__((__packed__)) threadPrograma;

int socketMemoria;
int socketFS;
int socketCPU;
int socketcpuConectadas;
int tamanio_pag_memoria;

sem_t grado;

//inotify

void configChange(){
	t_config* archivo = config_create(configFileName);
	if(config_has_property(archivo,"QUANTUM_SLEEP")){
		QuantumSleep = config_get_int_value(archivo, "QUANTUM_SLEEP");
		log_info(logNucleo,"Nuevo QUANTUM_SLEEP: %d",QuantumSleep);
	}else
		log_warning(logNucleo,"La Configuracion QUANTUM_SLEEP no se encuentra");

	config_destroy(archivo);
}

void inotifyWatch(void*path){
	char cwd[1024]; // Variable donde voy a guardar el path absoluto hasta el /Debug
	getcwd(cwd,sizeof(cwd));
	watchFile(cwd,configFileName,&configChange);
}

//inotify

void recibirTamanioPagina(int socket){
	t_package* paquete = recibirPaquete(socket,NULL);
	tamanio_pag_memoria = atoi(paquete->datos);
}

int obtenerEIncrementarPID()
{
	int		pid;
	;

	pthread_mutex_lock(&mutexPID);
	pid = ultimoPID;
	ultimoPID++;
	pthread_mutex_unlock(&mutexPID);

	return pid;
}

void cargar_varCompartidas(){
	char** varCompartArray = SharedVars;
	variablesCompartidas = dictionary_create();
	int i;

	for(i=0;varCompartArray[i]!=NULL; i++){
		int32_t *valor = malloc(sizeof(int32_t));
		*valor = 0;

		dictionary_put(variablesCompartidas,varCompartArray[i],valor);
	}
}

void crear_semaforos(){
	semaforos = dictionary_create();
	int i;
	char* sem_id = SemIds[0];

	for(i=0;sem_id;i++){
		int valor_inicial = atoi(SemInit[i]);

		t_semaforo *semaforo =malloc(sizeof(t_semaforo));
		semaforo->valor = valor_inicial;
		semaforo->cola = queue_create();

		dictionary_put(semaforos,sem_id,semaforo);
		sem_id = SemIds[i+1];
	}
}

void nuevoPrograma(char* codigo, int socket){
	pthread_t thread_nuevoprograma;

	threadPrograma* parametrosPrograma = malloc(sizeof(threadPrograma));
	parametrosPrograma->socket = socket;
	int32_t tamanio = strlen(codigo);
	parametrosPrograma->codigo = malloc(tamanio+1);
	memcpy(parametrosPrograma->codigo,codigo,tamanio);
	parametrosPrograma->codigo[tamanio]='\0';

	if (pthread_create(&thread_nuevoprograma, NULL, (void*)programa, (void*)parametrosPrograma)){
	    perror("Error el crear el thread programa.");
	    exit(EXIT_FAILURE);
	}

}

void programa(void* arg){
	threadPrograma* params = arg;
	int socket = params->socket;
	int gradoActual;
	t_pcb* nuevo_pcb;

	sem_getvalue(&grado,&gradoActual);

	nuevo_pcb = armar_nuevo_pcb(params->codigo);

	if(gradoActual <= 0){
		t_aviso_consola aviso_consola;
		aviso_consola.mensaje = "Rechazo por Multiprogramacion\0";
		aviso_consola.tamanomensaje = strlen(aviso_consola.mensaje);
		aviso_consola.idPrograma = nuevo_pcb->pid;

		char *pedido_serializado = serializar_aviso_consola(&aviso_consola);

		empaquetarEnviarMensaje(socket,"LOG_MESSAGE",aviso_consola.tamanomensaje+(sizeof(int32_t)*2),pedido_serializado);
		free(pedido_serializado);
	}

	free(params->codigo);
	free(params);

	moverA_colaNew(nuevo_pcb);

	sem_wait(&grado);

	inicializar_programa(nuevo_pcb);

	respuesta_inicializar_programa(socket, socketMemoria);
}

t_pcb* armar_nuevo_pcb(char* codigo){
	log_debug(logNucleo,"Armando pcb para programa original:\n %s \nTamano: %d bytes",codigo,strlen(codigo));
	t_pcb* nvopcb = malloc(sizeof(t_pcb));
	t_metadata_program* metadata;
	int i;

	metadata = metadata_desde_literal(codigo);

	nvopcb->pid = obtenerEIncrementarPID();
    nvopcb->pc = metadata->instruccion_inicio;

	nvopcb->cant_instrucciones = metadata->instrucciones_size;

	int tamano_instrucciones = sizeof(t_posMemoria)*(nvopcb->cant_instrucciones);

	nvopcb->indice_codigo=malloc(tamano_instrucciones);

	for(i=0;i<(metadata->instrucciones_size);i++){
		t_posMemoria posicion_nueva_instruccion;
		posicion_nueva_instruccion.pag = metadata->instrucciones_serializado[i].start/tamanio_pag_memoria;
		posicion_nueva_instruccion.offset = metadata->instrucciones_serializado[i].start%tamanio_pag_memoria;
		posicion_nueva_instruccion.size = metadata->instrucciones_serializado[i].offset;
		nvopcb->indice_codigo[i] = posicion_nueva_instruccion;

		log_trace(logNucleo,"Instruccion %d pag %d offset %d size %d",i,posicion_nueva_instruccion.pag,posicion_nueva_instruccion.offset,posicion_nueva_instruccion.size);
		log_trace(logNucleo,"%.*s",posicion_nueva_instruccion.size,codigo+metadata->instrucciones_serializado[i].start);
	}

	int result_pag = divAndRoundUp(strlen(codigo), tamanio_pag_memoria);
	nvopcb->cant_pags_totales=(result_pag + StackSize);
	log_debug(logNucleo,"Cant paginas codigo: %d",result_pag);
	log_debug(logNucleo,"Cant paginas stack: %d",StackSize);
	log_debug(logNucleo,"Cant paginas totales: %d",nvopcb->cant_pags_totales);

	nvopcb->tamano_etiquetas=metadata->etiquetas_size;
	nvopcb->indice_etiquetas=malloc(nvopcb->tamano_etiquetas);
	memcpy(nvopcb->indice_etiquetas,metadata->etiquetas,nvopcb->tamano_etiquetas);

	nvopcb->cant_entradas_indice_stack=1;
	nvopcb->indice_stack= calloc(1,sizeof(registro_indice_stack));
	nvopcb->indice_stack[0].cant_variables = 0;
	nvopcb->indice_stack[0].cant_argumentos=0;
	nvopcb->indice_stack[0].argumentos = NULL;
	nvopcb->indice_stack[0].variables = NULL;

	nvopcb->fin_stack.pag=result_pag;
	nvopcb->fin_stack.offset=0;
	nvopcb->fin_stack.size=4;

	metadata_destruir(metadata);
	return nvopcb;
}

void inicializar_programa(t_pcb* nuevo_pcb){
	t_pedido_inicializar pedido_inicializar;

	pedido_inicializar.pagRequeridas = nuevo_pcb->cant_pags_totales;
	pedido_inicializar.idPrograma = nuevo_pcb->pid;

	char *pedido_serializado = serializar_pedido_inicializar(&pedido_inicializar);

	empaquetarEnviarMensaje(socketMemoria,"INIT_PROGM",sizeof(t_pedido_inicializar),pedido_serializado);
	log_debug(logNucleo,"Pedido inicializar enviado. PID: %d, Paginas: %d",pedido_inicializar.idPrograma,pedido_inicializar.pagRequeridas);

	free(pedido_serializado);
}

void respuesta_inicializar_programa(int socket, int socketMemoria){

	t_package* paquete = recibirPaquete(socketMemoria,NULL);

	t_respuesta_inicializar* respuesta = deserializar_respuesta_inicializar(paquete->datos);

	pthread_mutex_lock(&mutexKernel);

	cargar_programa(socket,respuesta->idPrograma);

	t_pcb* pcb_respuesta = sacarDe_colaNew(respuesta->idPrograma);

	switch (respuesta->codigoRespuesta) {
		case OK_INICIALIZAR:
			log_info(logNucleo,"Inicializacion correcta del PID: %d",respuesta->idPrograma);
			moverA_colaReady(pcb_respuesta);
			log_debug(logNucleo, "Movi a la cola Ready el PCB con PID: %d",respuesta->idPrograma);

			log_info(logNucleo,"Se envia mensaje a la consola del socket %d que pudo poner en Ready su PCB", socket);
			t_aviso_consola aviso_ok;
			aviso_ok.mensaje = "Proceso inicializado\0";
			aviso_ok.tamanomensaje = strlen(aviso_ok.mensaje);
			aviso_ok.idPrograma = respuesta->idPrograma;

			char* pedido_ok = serializar_aviso_consola(&aviso_ok);

			empaquetarEnviarMensaje(socket,"LOG_MESSAGE",aviso_ok.tamanomensaje+(sizeof(int32_t)*2),pedido_ok);
			free(pedido_ok);

			log_info(logNucleo,"Se envia a CPU el PCB inicializado");
			enviar_a_cpu();
			break;
		case SIN_ESPACIO_INICIALIZAR:
			log_warning(logNucleo,"Inicializacion incorrecta del PID %d",respuesta->idPrograma);
			moverA_colaExit(pcb_respuesta);
			log_debug(logNucleo, "Movi a la cola Exit el PCB con PID: %d",respuesta->idPrograma);

			log_info(logNucleo,"Se envia mensaje a la consola del socket %d que no se pudo poner en Ready su PCB", socket);
			t_aviso_consola aviso_no_ok;
			aviso_no_ok.mensaje = "Sin espacio en Memoria\0";
			aviso_no_ok.tamanomensaje = strlen(aviso_no_ok.mensaje);
			aviso_no_ok.idPrograma = respuesta->idPrograma;

			char* pedido_no_ok = serializar_aviso_consola(&aviso_no_ok);

			empaquetarEnviarMensaje(socket,"LOG_MESSAGE",aviso_no_ok.tamanomensaje+(sizeof(int32_t)*2),pedido_no_ok);
			free(pedido_no_ok);
			break;
		default:
			log_warning(logNucleo,"LLego un codigo de operacion invalido");
			break;
	}

	pthread_mutex_unlock(&mutexKernel);
	borrarPaquete(paquete);
}

void nuevaConexionCPU(int sock){
	cargarCPU(sock);
	enviar_a_cpu();
}

void cargarCPU(int32_t socket){
	log_trace(logNucleo,"Cargando nueva CPU con socket %d",socket);
	t_cpu* cpu_nuevo;
	cpu_nuevo=malloc(sizeof(t_cpu));
	cpu_nuevo->socket=socket;
	cpu_nuevo->corriendo=false;
	list_add(lista_cpus_conectadas,cpu_nuevo);
	log_debug(logNucleo,"se agrego a la lista el cpu con socket %d", socket);
}

void cargar_programa(int32_t socket, int pid){
	t_consola *programa_nuevo;
	programa_nuevo=malloc(sizeof(t_consola));
	programa_nuevo->socket=socket;
	programa_nuevo->corriendo=false;
	programa_nuevo->pid=pid;
	list_add(lista_programas_actuales,programa_nuevo);
	log_debug(logNucleo,"se agrego a la lista el programa con socket %d, pid: %d", socket, programa_nuevo->pid);
}

void relacionar_cpu_programa(t_cpu *cpu, t_consola *programa, t_pcb *pcb){
	t_relacion *nueva_relacion;
	nueva_relacion=malloc(sizeof(t_relacion));
	cpu->corriendo=true;
	programa->corriendo=true;
	nueva_relacion->cpu=cpu;
	nueva_relacion->programa=programa;
	list_add(lista_relacion,nueva_relacion);
	log_info(logNucleo,"Se agrego la relacion entre cpu del socket: %d y el programa del socket: %d",cpu->socket, programa->socket);
	moverA_colaExec(pcb);
	log_debug(logNucleo, "Movi a la cola Exec el pcb con pid: %d", pcb->pid);
}

void elminar_consola_por_pid(int pid){

	bool matchconsola(void *consola) {
						return ((t_consola*)consola)->pid == pid;
					}

	free(list_remove_by_condition(lista_programas_actuales,matchconsola));
}

void liberar_consola(t_relacion *rel){
	rel->cpu->corriendo= false;
	rel->programa->corriendo= false;

	if(rel->programa->socket==-1){ //esta consola ya se murio
		log_warning(logNucleo,"Aprovecho para eliminar del sistema la consola cerrada del PID %d",rel->programa->pid);
		moverA_colaExit(sacarDe_colaExec(rel->programa->pid));
		//enviar(FINALIZA_PROGRAMA,sizeof(int32_t),&(rel->programa->pid),socket_umc);
		elminar_consola_por_pid(rel->programa->pid);
	}
}

void liberar_una_relacion(t_pcb *pcb_devuelto){

	bool matchPID(void *relacion) {
		return ((t_relacion*)relacion)->programa->pid == pcb_devuelto->pid;
	}

	t_relacion *rel = list_remove_by_condition(lista_relacion,matchPID);

	liberar_consola(rel);

	free(rel);
}

void liberar_una_relacion_porsocket_cpu(int socketcpu){

	bool matchsocketcpu(void *relacion) {
		return ((t_relacion*)relacion)->cpu->socket == socketcpu;
	}

	t_relacion *rel = list_remove_by_condition(lista_relacion,matchsocketcpu);

	liberar_consola(rel);

	free(rel);
}

void eliminar_cpu_por_socket(int socketcpu){

	bool matchSocket_Cpu(void *cpu) {
						return ((t_cpu*)cpu)->socket == socketcpu;
					}

	t_cpu* cpu = list_remove_by_condition(lista_cpus_conectadas,matchSocket_Cpu);
	free(cpu);
}

t_consola* matchear_consola_por_pid(int pid){

	bool matchPID_Consola(void *consola) {
						return ((t_consola*)consola)->pid == pid;
					}

	t_consola * programa_terminado=list_find(lista_programas_actuales, matchPID_Consola);
	return programa_terminado;
}

t_relacion* matchear_relacion_por_socketcpu(int socket){

	bool matchsocketcpurelacion(void *relacion) {
						return ((t_relacion*)relacion)->cpu->socket == socket;
					}

	return list_find(lista_relacion, matchsocketcpurelacion);
}

t_relacion* matchear_relacion_por_socketconsola(int socket){

	bool matchsocketconsolarelacion(void *relacion) {
						return ((t_relacion*)relacion)->programa->socket == socket;
					}

	return list_find(lista_relacion, matchsocketconsolarelacion);
}

void elminar_consola_por_socket(int socket){

	bool matchconsola(void *consola) {
						return ((t_consola*)consola)->socket == socket;
					}

	free(list_remove_by_condition(lista_programas_actuales,matchconsola));
}

bool esta_libre(void * unaCpu){
	return !(((t_cpu*)unaCpu)->corriendo);
}

void enviar_a_cpu(){
	t_cpu *cpu_libre = list_find(lista_cpus_conectadas,esta_libre);
	if(!cpu_libre){
		return;
	}
	log_info(logNucleo, "la CPU del socket %d esta libre y lista para ejecutar", cpu_libre->socket);

	t_pcb *pcb_ready;
	pcb_ready = queue_pop(colaReady);
	if(!pcb_ready){
		return;
	}
	log_info(logEstados, "El PCB con el pid %d salio de la cola Ready y esta listo"
			" para ser enviado al CPU del socket %d", pcb_ready->pid, cpu_libre->socket);
	log_info(logNucleo, "Se levanto el pcb con id %d", pcb_ready->pid);

	bool matchPID(void *programa) {
		return ((t_consola*)programa)->pid == pcb_ready->pid;
	}

	t_consola *programa = list_find(lista_programas_actuales, matchPID);
	if(!programa){
		return;
	}

	log_debug(logNucleo,"Corriendo el PCB (PID %d) del programa (socket %d)en el CPU (socket %d)",pcb_ready->pid,programa->socket,cpu_libre->socket);
	relacionar_cpu_programa(cpu_libre,programa,pcb_ready);
	log_info(logNucleo, "Pude relacionar cpu con programa");

	char* quantum = string_itoa(Quantum);
	char* quantumsleep = string_itoa(QuantumSleep);
	empaquetarEnviarMensaje(cpu_libre->socket,"NUEVO_QUANTUM",strlen(quantum),quantum);
	empaquetarEnviarMensaje(cpu_libre->socket,"NUEVO_QUANTUM_SLEEP",strlen(quantumsleep),quantumsleep);
	free(quantum);
	free(quantumsleep);

	t_pcb_serializado* serializado = serializar_pcb(pcb_ready);
	log_trace(logNucleo,"Se serializo un pcb");
	empaquetarEnviarMensaje(cpu_libre->socket,"CORRER_PCB",serializado->tamanio,serializado->contenido_pcb);
	log_info(logNucleo,"Se envio un pcb a correr en la cpu %d",cpu_libre->socket);
	free(serializado->contenido_pcb);
}

void mostrarMensaje(char* mensaje,int socket){
	printf("Mensaje recibido: %s \n",mensaje);
}

void correrServidor(void* arg){
	threadParams* params = arg;
	correrServidorMultiConexion(params->socketEscucha,params->nuevaConexion,params->desconexion,params->funciones,params->handshakes);
}

int main(int argc, char** argv) {
	pthread_t thread_consola, thread_cpu;

	configFileName=argv[1];
	t_config* configFile= cargarConfiguracion(configFileName);

	pthread_t hiloInotify;
	pthread_create(&hiloInotify,NULL,inotifyWatch,configFile->path);

    t_dictionary* diccionarioFunciones = dictionary_create();
    dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
    dictionary_put(diccionarioFunciones,"NUEVO_PROG",&nuevoPrograma);
    dictionary_put(diccionarioFunciones,"RECB_MARCOS",&recibirTamanioPagina);
    dictionary_put(diccionarioFunciones,"RES_INICIALIZAR",&respuesta_inicializar_programa);

    t_dictionary* diccionarioHandshakes = dictionary_create();
    dictionary_put(diccionarioHandshakes,"HCPKE","HKECP");
    dictionary_put(diccionarioHandshakes,"HCSKE","HKECS");

    int socketConsola = crearHostMultiConexion(PuertoConsola);
    socketCPU = crearHostMultiConexion(PuertoCpu);

    threadParams parametrosConsola;
    parametrosConsola.socketEscucha = socketConsola;
    parametrosConsola.nuevaConexion = NULL;
    parametrosConsola.desconexion = NULL;
    parametrosConsola.handshakes = diccionarioHandshakes;
    parametrosConsola.funciones = diccionarioFunciones;

    threadParams parametrosCpu;
    parametrosCpu.socketEscucha = socketCPU;
    parametrosCpu.nuevaConexion = &nuevaConexionCPU;
    parametrosCpu.desconexion = NULL;
    parametrosCpu.handshakes = diccionarioHandshakes;
    parametrosCpu.funciones = diccionarioFunciones;

    crear_semaforos();
    cargar_varCompartidas();
    crear_colas();

    logNucleo = log_create("logNucleo.log", "nucleo.c", false, LOG_LEVEL_TRACE);
    logEstados = log_create("logEstados.log", "estados.c", false, LOG_LEVEL_TRACE);

    sem_init(&grado, 0, GradoMultiprog);

    ultimoPID = 1;

    if ((socketMemoria = conectar(IpMemoria,PuertoMemoria)) == -1)
    	exit(EXIT_FAILURE);
    if(handshake(socketMemoria,"HKEME","HMEKE")){
    		puts("Se pudo realizar handshake");
    		empaquetarEnviarMensaje(socketMemoria,"GET_MARCOS",sizeof("GET_MARCOS"),"GET_MARCOS");
    		recibirTamanioPagina(socketMemoria);
    }
    else{
    	puts("No se pudo realizar handshake");
    	exit(EXIT_FAILURE);
    	}

    if ((socketFS = conectar(IpFS,PuertoFS)) == -1)
        exit(EXIT_FAILURE);
    if(handshake(socketFS,"HKEFS","HFSKE")){
     		puts("Se pudo realizar handshake");
	}else
		puts("No se pudo realizar handshake");

    if (pthread_create(&thread_consola, NULL, (void*)correrServidor, &parametrosCpu)){
    		        perror("Error el crear el thread consola.");
    		        exit(EXIT_FAILURE);
    		}

    if (pthread_create(&thread_cpu, NULL, (void*)correrServidor, &parametrosConsola)){
      		        perror("Error el crear el thread consola.");
      		        exit(EXIT_FAILURE);
      		}

    pthread_join(thread_consola, NULL);

    pthread_join(thread_cpu, NULL);

    dictionary_destroy(diccionarioFunciones);
    dictionary_destroy(diccionarioHandshakes);

    dictionary_destroy_and_destroy_elements(semaforos,free);
    dictionary_destroy_and_destroy_elements(variablesCompartidas,free);

    destruir_colas();

    config_destroy(configFile);
    log_destroy(logNucleo);
    log_destroy(logEstados);

	return EXIT_SUCCESS;
}

t_config* cargarConfiguracion(char* archivo){
	t_config* archivo_cnf;

		archivo_cnf = config_create(archivo);

		if(config_has_property(archivo_cnf, "PUERTO_CONSOLA") == true)
			PuertoConsola = config_get_int_value(archivo_cnf, "PUERTO_CONSOLA");
		else{
			printf("ERROR archivo config sin puerto Consola\n");
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
			IpMemoria = config_get_string_value(archivo_cnf,"IP_MEMORIA");
		}
		else{
			printf("ERROR archivo config sin IP Memoria\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "IP_FS") == true){
			IpFS = config_get_string_value(archivo_cnf,"IP_FS");
		}
		else{
			printf("ERROR archivo config sin IP FILESYSTEM\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "ALGORITMO") == true){
			char *aux = config_get_string_value(archivo_cnf,"ALGORITMO");
			if (strcmp(aux, "FIFO") == 0)
				Modo = FIFO;
			else
				Modo = RR;
		}
		else{
			printf("ERROR archivo config sin Algoritmo\n");
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

		if(config_has_property(archivo_cnf, "SEM_IDS") == true)
			SemIds = config_get_array_value(archivo_cnf, "SEM_IDS");
		else{
			printf("ERROR archivo config sin ID de Semaforos\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "SEM_INIT") == true)
			SemInit = config_get_array_value(archivo_cnf, "SEM_INIT");
		else{
			printf("ERROR archivo config sin Inicializacion de Semaforos\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "SHARED_VARS") == true)
			SharedVars = config_get_array_value(archivo_cnf, "SHARED_VARS");
		else{
			printf("ERROR archivo config sin Variables Compartidas\n");
			exit(EXIT_FAILURE);
			}

		if(config_has_property(archivo_cnf, "STACK_SIZE") == true)
			StackSize = config_get_int_value(archivo_cnf, "STACK_SIZE");
		else{
			printf("ERROR archivo config sin Stack Size\n");
			exit(EXIT_FAILURE);
			}

		return archivo_cnf;
}
