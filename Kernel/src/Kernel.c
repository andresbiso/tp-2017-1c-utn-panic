#include "Kernel.h"

typedef struct {
    int socketEscucha;
    void (*nuevaConexion) (int);
    void (*desconexion) (int);
    void (*afterHandshake) (int);
    t_dictionary* funciones;
    t_dictionary* handshakes;
}threadParams;

typedef struct {
	int32_t socket;
	char* codigo;
} __attribute__((__packed__)) threadPrograma;

int socketFS;
int socketCPU;
int socketcpuConectadas;
int tamanio_pag_memoria;

sem_t grado;

//consola

void finalizarProgramaConsola(char*data,int socket){
	int32_t* pid = malloc(sizeof(int32_t));
	*pid=atoi(data);

	log_info(logNucleo,"Consola pide terminar el proceso con el PID:%d",pid);

	pthread_t hilo;//Se hace con un hilo por si se pausa la planificacion
	pthread_create(&hilo,NULL,(void*)&finalizarProceso,pid);
}

//consola

//inotify

void configChange(){
	t_config* archivo = config_create(configFileName);
	if(config_has_property(archivo,"QUANTUM_SLEEP")){
		QuantumSleep = config_get_int_value(archivo, "QUANTUM_SLEEP");
		log_info(logNucleo,"Nuevo QUANTUM_SLEEP: %d",QuantumSleep);
	}else{
		log_warning(logNucleo,"La Configuracion QUANTUM_SLEEP no se encuentra");
	}
	config_destroy(archivo);
}

void inotifyWatch(void*args){
	char cwd[1024];
	getcwd(cwd,sizeof(cwd));
	watchFile(cwd,configFileName,&configChange);
}

//inotify

//General

void addForFinishIfNotContains(int32_t* pid){
	bool matchPID(void* elemt){
		return (*((int32_t*) elemt))==(*pid);
	}

	pthread_mutex_lock(&listForFinishMutex);
	if(list_find(listForFinish,matchPID)==NULL){
		list_add(listForFinish,pid);
	}else{
		free(pid);
	}
	pthread_mutex_unlock(&listForFinishMutex);
}

void retornarPCB(char* data,int socket){
	t_pcb* pcb = deserializar_pcb(data);

	log_info(logNucleo,"El socket cpu %d retorno el PID %d",socket,pcb->pid);

	t_pcb* pcbOld = sacarDe_colaExec(pcb->pid);
	destruir_pcb(pcbOld);

	cpu_change_running(socket,false);

	if(processIsForFinish(pcb->pid) && pcb->exit_code>0){
		pcb->exit_code=FINALIZAR_BY_CONSOLE;
	}

	if(pcb->exit_code<=0){//termino el programa

		log_info(logNucleo,"Programa finalizado desde el socket:%d con el PID:%d",socket,pcb->pid);
		moverA_colaExit(pcb);
		t_respuesta_finalizar_programa* respuesta = finalizarProcesoMemoria(pcb->pid);

		if(respuesta->codigo==OK_FINALIZAR){
			char* message = string_from_format("Proceso finalizado con exitCode: %d\0",pcb->exit_code);

			pthread_mutex_lock(&mutexProgramasActuales);
			t_consola* consola = matchear_consola_por_pid(pcb->pid);
			enviarMensajeConsola(message,"END_PRGM",pcb->pid,consola->socket,1,0);
			free(consola);
			pthread_mutex_unlock(&mutexProgramasActuales);

			free(message);
		}
	}else{
		moverA_colaReady(pcb);
		program_change_running(pcb->pid,false);
	}

	enviar_a_cpu();

}

void finalizarProceso(void* pidArg){
	int32_t* pid = (int32_t*)pidArg;

	log_info(logNucleo,"A punto de finalizar el proceso con el PID:%d",*pid);

	pthread_mutex_lock(&mutexProgramasActuales);
	t_consola* consola = matchear_consola_por_pid(*pid);

	if(consola != NULL){
		if(consola->corriendo==false){
			//Puede estar bloqueado o en ready
			t_pcb* pcb;

			pcb = sacarDe_colaReady(*pid);
			if(pcb!=NULL){//Esta en ready
				log_info(logNucleo,"Finalizando proceso con PID:%d, que estaba en READY",*pid);

				t_respuesta_finalizar_programa* respuesta = finalizarProcesoMemoria(*pid);
				pcb->exit_code=FINALIZAR_BY_CONSOLE;
				if(respuesta->codigo==OK_FINALIZAR){
					char* message=string_from_format("Proceso finalizado con exitCode: %d\0",pcb->exit_code);
					enviarMensajeConsola(message,"LOG_MESSAGE",pcb->pid,consola->socket,1,0);
					free(message);
				}
				free(consola);
				moverA_colaExit(pcb);

				free(respuesta);
			}else{//Esta bloqueado
				addForFinishIfNotContains(pid);
			}
		}else{
			addForFinishIfNotContains(pid);
		}
	}
	pthread_mutex_unlock(&mutexProgramasActuales);
}

void printMessage(char* data, int socket){
	t_aviso_consola* aviso = deserializar_aviso_consola(data);

	log_info(logNucleo,"Se recibio un MENSAJE:%s para el PID:%d de la CONSOLA:%d",aviso->mensaje,aviso->idPrograma,socket);

	pthread_mutex_lock(&mutexProgramasActuales);
	t_consola* consola =matchear_consola_por_pid(aviso->idPrograma);

	if(consola !=NULL){
		char*buffer = serializar_aviso_consola(aviso);
		empaquetarEnviarMensaje(consola->socket,"LOG_MESSAGE",aviso->tamaniomensaje+(sizeof(int32_t)*4),buffer);
		free(buffer);
	}else{
		log_warning(logNucleo,"No se encontró la consola para el PID:%d",aviso->idPrograma);
	}
	pthread_mutex_unlock(&mutexProgramasActuales);

	free(aviso->mensaje);
	free(aviso);
}

//General

//consola Kernel

void end(int size, char** functionAndParams){
	if(size != 2){
		printf("El comando end debe recibir el PID\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	int32_t* pid = malloc(sizeof(int32_t));
	*pid=atoi(functionAndParams[1]);

	pthread_mutex_lock(&mutexProgramasActuales);
	t_consola* consola = matchear_consola_por_pid(*pid);
	pthread_mutex_unlock(&mutexProgramasActuales);

	if(consola==NULL){
		printf("PID no encontrado\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	pthread_t hilo;//Se hace con un hilo por si se pausa la planificacion
	pthread_create(&hilo,NULL,(void*)&finalizarProceso,pid);

	freeElementsArray(functionAndParams,size);
}

void bajarGrado(void* nuevoGrado){
	int oldValue = GradoMultiprog;
	GradoMultiprog = *(int*)nuevoGrado;
	int i;
	for(i=0;i<(abs((*(int*)nuevoGrado)-oldValue));i++){
		if((*(int*)nuevoGrado)>oldValue)
			sem_post(&grado);
		else
			sem_wait(&grado);
	}
	free(nuevoGrado);
	log_info(logNucleo,"Nuevo grado de multiprogramacion %d",GradoMultiprog);
}

void changeMultiprogramacion(int size, char** functionAndParams){
	if(size != 2){
		printf("El comando multiprog debe recibir el numero nuevo\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	int* nuevoMultiprog = malloc(sizeof(int));
	*nuevoMultiprog=atoi(functionAndParams[1]);

	if((*nuevoMultiprog) <0){
		printf("El grado de multiprogramacion no puede ser menor a 0\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	pthread_t hilo;
	pthread_create(&hilo,NULL,(void*)bajarGrado,(void*)nuevoMultiprog);

	freeElementsArray(functionAndParams,size);
}

void showProcess(int size, char** functionAndParams){
	if(size>2){
		printf("El comando showProcess no puede recibir mas de 1 parametro\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	void logPID(void *process) {
		log_info(logNucleo,"PID: %d",((t_pcb*)process)->pid);
	}
	if(size==2){
		if(string_equals_ignore_case("new",functionAndParams[1])){
			log_info(logNucleo,"-------- Procesos en NEW    --------");
			pthread_mutex_lock(&colaNewMutex);
			list_iterate(colaNew->elements,logPID);
			pthread_mutex_unlock(&colaNewMutex);
		}
		if(string_equals_ignore_case("ready",functionAndParams[1])){
			log_info(logNucleo,"-------- Procesos en READY  --------");
			pthread_mutex_lock(&colaReadyMutex);
			list_iterate(colaReady->elements,logPID);
			pthread_mutex_unlock(&colaReadyMutex);
		}
		if(string_equals_ignore_case("exec",functionAndParams[1])){
			log_info(logNucleo,"-------- Procesos en EXEC   --------");
			pthread_mutex_lock(&colaExecMutex);
			list_iterate(colaExec->elements,logPID);
			pthread_mutex_unlock(&colaExecMutex);
		}
		if(string_equals_ignore_case("block",functionAndParams[1])){
			log_info(logNucleo,"-------- Procesos en BLOCKED--------");
			pthread_mutex_lock(&colaBlockedMutex);
			list_iterate(colaBlocked->elements,logPID);
			pthread_mutex_unlock(&colaBlockedMutex);
		}
		if(string_equals_ignore_case("exit",functionAndParams[1])){
			log_info(logNucleo,"-------- Procesos en EXIT 	--------");
			pthread_mutex_lock(&colaExitMutex);
			list_iterate(colaExit->elements,logPID);
			pthread_mutex_unlock(&colaExitMutex);
		}
	}else{
		log_info(logNucleo,"-------- Procesos en NEW    --------");
		pthread_mutex_lock(&colaNewMutex);
		list_iterate(colaNew->elements,logPID);
		pthread_mutex_unlock(&colaNewMutex);
		log_info(logNucleo,"-------- Procesos en READY  --------");
		pthread_mutex_lock(&colaReadyMutex);
		list_iterate(colaReady->elements,logPID);
		pthread_mutex_unlock(&colaReadyMutex);
		log_info(logNucleo,"-------- Procesos en EXEC   --------");
		pthread_mutex_lock(&colaExecMutex);
		list_iterate(colaExec->elements,logPID);
		pthread_mutex_unlock(&colaExecMutex);
		log_info(logNucleo,"-------- Procesos en BLOCKED--------");
		pthread_mutex_lock(&colaBlockedMutex);
		list_iterate(colaBlocked->elements,logPID);
		pthread_mutex_unlock(&colaBlockedMutex);
		log_info(logNucleo,"-------- Procesos en EXIT 	--------");
		pthread_mutex_lock(&colaExitMutex);
		list_iterate(colaExit->elements,logPID);
		pthread_mutex_unlock(&colaExitMutex);
	}

	freeElementsArray(functionAndParams,size);
}

void stop(int size, char** functionAndParams){
	if(size>1){
		printf("El comando stop no puede recibir parametros\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}

	if(!isStopped)
		isStopped=true;

	freeElementsArray(functionAndParams,size);
}

void restart(int size, char** functionAndParams){
	if(size>1){
		printf("El comando restart no puede recibir parametros\n\r");
		freeElementsArray(functionAndParams,size);
		return;
	}
	if(isStopped){
		isStopped=false;
		sem_post(&stopped);
	}

	freeElementsArray(functionAndParams,size);
}



void consolaCreate(void*args){
	t_dictionary* commands = dictionary_create();
	dictionary_put(commands,"multiprog",&changeMultiprogramacion);
	dictionary_put(commands,"showProcess",&showProcess);
	dictionary_put(commands,"end",&end);
	dictionary_put(commands,"stop",&stop);
	dictionary_put(commands,"restart",&restart);
	waitCommand(commands);
	dictionary_destroy(commands);
}

//consola

void recibirTamanioPagina(int socket){
	t_package* paquete = recibirPaquete(socket,NULL);
	tamanio_pag_memoria = atoi(paquete->datos);
	borrarPaquete(paquete);
}

int obtenerEIncrementarPID()
{
	int		pid;

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

	nuevo_pcb = armar_nuevo_pcb(params->codigo);

	cargar_programa(socket,nuevo_pcb->pid);

	moverA_colaNew(nuevo_pcb);

	enviarMensajeConsola("Nuevo Proceso creado\0","NEW_PID",nuevo_pcb->pid,socket,0,0);

	sem_getvalue(&grado,&gradoActual);
	if(gradoActual <= 0){
		enviarMensajeConsola("Espera por Multiprogramacion\0","LOG_MESSAGE",nuevo_pcb->pid,socket,0,0);
	}

	sem_wait(&grado);

	inicializar_programa(nuevo_pcb);

	respuesta_inicializar_programa(socket, socketMemoria, params->codigo);

	free(params->codigo);
	free(params);
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
		posicion_nueva_instruccion.size = metadata->instrucciones_serializado[i].offset;

		posicion_nueva_instruccion.pag = (metadata->instrucciones_serializado[i].start+posicion_nueva_instruccion.size)/tamanio_pag_memoria;

		if (posicion_nueva_instruccion.pag !=0 && (metadata->instrucciones_serializado[i].start < (posicion_nueva_instruccion.pag*tamanio_pag_memoria)) ){
			posicion_nueva_instruccion.offset = 0;
		}else if (posicion_nueva_instruccion.pag !=0){
			posicion_nueva_instruccion.offset = (metadata->instrucciones_serializado[i].start%(posicion_nueva_instruccion.pag*tamanio_pag_memoria));
			if((posicion_nueva_instruccion.offset)<(nvopcb->indice_codigo[i-1].offset+nvopcb->indice_codigo[i-1].size))
				posicion_nueva_instruccion.offset=(nvopcb->indice_codigo[i-1].size+nvopcb->indice_codigo[i-1].offset);
		}else
			posicion_nueva_instruccion.offset = metadata->instrucciones_serializado[i].start;
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

	nvopcb->exit_code=1;

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

bool almacenarBytes(t_pcb* pcb,int socketMemoria,char* data){
	int i;
	t_pedido_almacenar_bytes pedido;

	int offset=0;

	int paginasCodigo=pcb->cant_pags_totales-StackSize;

	int j;
	int nextInstruction=0;
	for(i=0;i<paginasCodigo;i++){
		pedido.pid = pcb->pid;
		pedido.pagina = i;

		for(j=nextInstruction;j<pcb->cant_instrucciones;j++){
			if(pcb->indice_codigo[j].pag ==i  && ((j+1>=(pcb->cant_instrucciones))|| pcb->indice_codigo[j+1].pag !=i)){
				pedido.tamanio=pcb->indice_codigo[j].size+pcb->indice_codigo[j].offset;
				nextInstruction=j+1;
				break;
			}
		}

		pedido.offsetPagina = 0;
		pedido.data = malloc(pedido.tamanio);
		memcpy(pedido.data,data+offset,pedido.tamanio);

		offset+=pedido.tamanio;

		char* buffer = serializar_pedido_almacenar_bytes(&pedido);
		empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",sizeof(int32_t)*4+pedido.tamanio,buffer);
		free(buffer);
		free(pedido.data);

		t_package* paquete = recibirPaqueteMemoria();

		t_respuesta_almacenar_bytes* respuesta = deserializar_respuesta_almacenar_bytes(paquete->datos);

		switch (respuesta->codigo) {
			case OK_ALMACENAR:
				log_info(logNucleo,"Almacenamiento correcto en pagina PID:%d PAG:%d TAMANIO:%d OFFSET:%d",pedido.pid,pedido.pagina,pedido.tamanio,pedido.offsetPagina);
				break;
			case PAGINA_ALM_OVERFLOW:
				log_info(logNucleo,"Overflow al almacenar el PID: %d",respuesta->pid);
				return false;
				break;
			case PAGINA_ALM_NOT_FOUND:
				log_info(logNucleo,"No se encontro pagina para el PID: %d",respuesta->pid);
				return false;
				break;
		}

		borrarPaquete(paquete);
		free(respuesta);
	}
	return true;
}

void respuesta_inicializar_programa(int socket, int socketMemoria, char* codigo){

	t_package* paquete = recibirPaqueteMemoria();
	t_respuesta_inicializar* respuesta = deserializar_respuesta_inicializar(paquete->datos);

	t_pcb* pcb_respuesta = sacarDe_colaNew(respuesta->idPrograma);
	switch (respuesta->codigoRespuesta) {
		case OK_INICIALIZAR:
			if(almacenarBytes(pcb_respuesta,socketMemoria,codigo)){
				log_info(logNucleo,"Inicializacion correcta del PID: %d",respuesta->idPrograma);
				moverA_colaReady(pcb_respuesta);
				log_debug(logNucleo, "Movi a la cola Ready el PCB con PID: %d",respuesta->idPrograma);

				log_info(logNucleo,"Se envia mensaje a la consola del socket %d que pudo poner en Ready su PCB", socket);
				enviarMensajeConsola("Proceso inicializado\0","LOG_MESSAGE",respuesta->idPrograma,socket,0,0);

				log_info(logNucleo,"Se envia a CPU el PCB inicializado");
				enviar_a_cpu();
			}else{
				pcb_respuesta->exit_code=FINALIZAR_SIN_RECURSOS;

				char* message = string_from_format("Proceso finalizado con exitCode: %d\0",pcb_respuesta->exit_code);
				enviarMensajeConsola(message,"END_PRGM",pcb_respuesta->pid,socket,1,0);
				free(message);

				moverA_colaExit(pcb_respuesta);
				log_debug(logNucleo, "Movi a la cola Exit el PCB con PID: %d",respuesta->idPrograma);
			}
			break;
		case SIN_ESPACIO_INICIALIZAR:
			pcb_respuesta->exit_code=FINALIZAR_SIN_RECURSOS;
			char* message = string_from_format("Proceso finalizado con exitCode: %d\0",pcb_respuesta->exit_code);
			enviarMensajeConsola(message,"END_PRGM",pcb_respuesta->pid,socket,1,0);
			free(message);

			log_warning(logNucleo,"Inicializacion incorrecta del PID %d",respuesta->idPrograma);
			moverA_colaExit(pcb_respuesta);
			log_debug(logNucleo, "Movi a la cola Exit el PCB con PID: %d",respuesta->idPrograma);

			log_info(logNucleo,"Se envia mensaje a la consola del socket %d que no se pudo poner en Ready su PCB", socket);
			break;
		default:
			log_warning(logNucleo,"Llego un codigo de operacion invalido");
			break;
	}

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
	pthread_mutex_lock(&mutexCPUConectadas);
	list_add(lista_cpus_conectadas,cpu_nuevo);
	pthread_mutex_unlock(&mutexCPUConectadas);
	log_debug(logNucleo,"se agrego a la lista el cpu con socket %d", socket);
}

void cargar_programa(int32_t socket, int pid){
	t_consola *programa_nuevo;
	programa_nuevo=malloc(sizeof(t_consola));
	programa_nuevo->socket=socket;
	programa_nuevo->corriendo=false;
	programa_nuevo->pid=pid;
	pthread_mutex_lock(&mutexProgramasActuales);
	list_add(lista_programas_actuales,programa_nuevo);
	pthread_mutex_unlock(&mutexProgramasActuales);
	log_debug(logNucleo,"se agrego a la lista el programa con socket %d, pid: %d", socket, programa_nuevo->pid);
}

void elminar_consola_por_pid(int pid){

	bool matchconsola(void *consola) {
						return ((t_consola*)consola)->pid == pid;
					}
	pthread_mutex_lock(&mutexProgramasActuales);
	list_remove_and_destroy_by_condition(lista_programas_actuales,matchconsola,free);
	pthread_mutex_unlock(&mutexProgramasActuales);
}

void eliminar_cpu_por_socket(int socketcpu){

	bool matchSocket_Cpu(void *cpu) {
						return ((t_cpu*)cpu)->socket == socketcpu;
					}

	pthread_mutex_lock(&mutexCPUConectadas);
	list_remove_and_destroy_by_condition(lista_cpus_conectadas,matchSocket_Cpu,free);
	pthread_mutex_unlock(&mutexCPUConectadas);
}

bool esta_libre(void * unaCpu){
	return !(((t_cpu*)unaCpu)->corriendo);
}

void enviar_a_cpu(){
	pthread_mutex_lock(&mutexCPUConectadas);
	t_cpu *cpu_libre = list_find(lista_cpus_conectadas,esta_libre);
	if(!cpu_libre){
		pthread_mutex_unlock(&mutexCPUConectadas);
		return;
	}
	log_info(logNucleo, "la CPU del socket %d esta libre y lista para ejecutar", cpu_libre->socket);

	t_pcb *pcb_ready=sacarCualquieraDeReady();
	if(!pcb_ready){
		pthread_mutex_unlock(&mutexCPUConectadas);
		return;
	}
	log_info(logEstados, "El PCB con el pid %d salio de la cola Ready y esta listo para ser enviado al CPU del socket %d", pcb_ready->pid, cpu_libre->socket);

	log_info(logNucleo, "Se levanto el pcb con id %d", pcb_ready->pid);

	bool matchPID(void *programa) {
		return ((t_consola*)programa)->pid == pcb_ready->pid;
	}

	pthread_mutex_lock(&mutexProgramasActuales);
	t_consola *programa = list_find(lista_programas_actuales, matchPID);

	if(!programa){
		pthread_mutex_unlock(&mutexCPUConectadas);
		pthread_mutex_unlock(&mutexProgramasActuales);
		return;
	}

	log_debug(logNucleo,"Corriendo el PCB (PID %d) del programa (socket %d) en el CPU (socket %d)",pcb_ready->pid,programa->socket,cpu_libre->socket);

	moverA_colaExec(pcb_ready);
	log_debug(logNucleo, "Movi a la cola Exec el pcb con pid: %d", pcb_ready->pid);

	programa->corriendo=true;
	cpu_libre->corriendo=true;
	pthread_mutex_unlock(&mutexCPUConectadas);
	pthread_mutex_unlock(&mutexProgramasActuales);

	log_info(logNucleo, "Pude relacionar cpu con programa");

	char* quantum = string_itoa(Quantum);
	char* quantumsleep = string_itoa(QuantumSleep);
	empaquetarEnviarMensaje(cpu_libre->socket,"NUEVO_QUANTUM_SLEEP",strlen(quantumsleep),quantumsleep);
	empaquetarEnviarMensaje(cpu_libre->socket,"NUEVO_QUANTUM",strlen(quantum),quantum);
	free(quantum);
	free(quantumsleep);

	t_pcb_serializado* serializado = serializar_pcb(pcb_ready);
	log_trace(logNucleo,"Se serializo un pcb");

	empaquetarEnviarMensaje(cpu_libre->socket,"CORRER_PCB",serializado->tamanio,serializado->contenido_pcb);

	log_info(logNucleo,"Se envio un pcb a correr en la cpu %d",cpu_libre->socket);

	free(serializado->contenido_pcb);
	free(serializado);
}

void mostrarMensaje(char* mensaje,int socket){
	printf("Mensaje recibido: %s \n",mensaje);
}

void correrServidor(void* arg){
	threadParams* params = arg;
	correrServidorMultiConexion(params->socketEscucha,params->nuevaConexion,params->desconexion,params->afterHandshake,params->funciones,params->handshakes);
}

int main(int argc, char** argv) {
	pthread_t thread_consola, thread_cpu;

	configFileName=argv[1];
	t_config* configFile= cargarConfiguracion(configFileName);

	pthread_t hiloInotify;
	pthread_create(&hiloInotify,NULL,(void*)&inotifyWatch,NULL);

	pthread_mutex_init(&relacionMutex,NULL);
	pthread_mutex_init(&colaNewMutex,NULL);
	pthread_mutex_init(&colaReadyMutex,NULL);
	pthread_mutex_init(&colaBlockedMutex,NULL);
	pthread_mutex_init(&colaExecMutex,NULL);
	pthread_mutex_init(&colaExitMutex,NULL);
	pthread_mutex_init(&stoppedMutex,NULL);
	pthread_mutex_init(&listForFinishMutex,NULL);
	pthread_mutex_init(&mutexCPUConectadas,NULL);
	pthread_mutex_init(&mutexProgramasActuales,NULL);
	pthread_mutex_init(&mutexMemoria,NULL);
	sem_init(&stopped,0,0);

	isStopped=false;

    t_dictionary* diccionarioFunciones = dictionary_create();
    dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);
    dictionary_put(diccionarioFunciones,"NUEVO_PROG",&nuevoPrograma);
    dictionary_put(diccionarioFunciones,"END_PROG",&finalizarProgramaConsola);
    dictionary_put(diccionarioFunciones,"RET_PCB",&retornarPCB);
    dictionary_put(diccionarioFunciones,"PRINT_MESSAGE",&printMessage);
    dictionary_put(diccionarioFunciones,"GET_VAR_COMP",&getVariableCompartida);
    dictionary_put(diccionarioFunciones,"SET_VAR_COMP",&setVariableCompartida);
    dictionary_put(diccionarioFunciones,"WAIT",&wait);
    dictionary_put(diccionarioFunciones,"SIGNAL",&signal);

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
    parametrosConsola.afterHandshake = NULL;

    threadParams parametrosCpu;
    parametrosCpu.socketEscucha = socketCPU;
    parametrosCpu.nuevaConexion = NULL;
    parametrosCpu.desconexion = NULL;
    parametrosCpu.handshakes = diccionarioHandshakes;
    parametrosCpu.funciones = diccionarioFunciones;
    parametrosCpu.afterHandshake = &nuevaConexionCPU;

    crear_semaforos();
    cargar_varCompartidas();
    crear_colas();

    listForFinish=list_create();

    logNucleo = log_create("logNucleo.log", "nucleo.c", false, LOG_LEVEL_TRACE);
    logEstados = log_create("logEstados.log", "estados.c", false, LOG_LEVEL_TRACE);

    sem_init(&grado, 0, GradoMultiprog);

    ultimoPID = 1;

    if ((socketMemoria = conectar(IpMemoria,PuertoMemoria)) == -1)
    	exit(EXIT_FAILURE);
    if(handshake(socketMemoria,"HKEME","HMEKE")){
    		puts("Se pudo realizar handshake");
    		empaquetarEnviarMensaje(socketMemoria,"GET_MARCOS",strlen("GET_MARCOS\0"),"GET_MARCOS");
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

    pthread_t hiloConsola;
    pthread_create(&hiloConsola,NULL,(void*)&consolaCreate,NULL);
    pthread_join(hiloConsola,NULL);

    dictionary_destroy(diccionarioFunciones);
    dictionary_destroy(diccionarioHandshakes);

    dictionary_destroy_and_destroy_elements(semaforos,free);
    dictionary_destroy_and_destroy_elements(variablesCompartidas,free);

    destruir_colas();

    config_destroy(configFile);
    log_destroy(logNucleo);
    log_destroy(logEstados);
    list_destroy_and_destroy_elements(listForFinish,free);

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
