#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "Kernel.h"

typedef struct {
    int socketEscucha;
    void (*nuevaConexion) (int);
    void (*desconexion) (int);
    t_dictionary* funciones;
    t_dictionary* handshakes;
}threadParams;

int socketMemoria;
int socketFS;
int socketCPU;
int socketcpuConectadas;

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

void nuevaConexionCPU(int sock){
	socketcpuConectadas = sock;
	t_cpu* cpu_nuevo;
	cpu_nuevo=malloc(sizeof(t_cpu));
	cpu_nuevo->socket=sock;
	cpu_nuevo->corriendo=false;
	list_add(lista_cpus_conectadas,cpu_nuevo);
}

void mostrarMensaje(char* mensaje,int socket){
	printf("Mensaje recibido: %s \n",mensaje);
	empaquetarEnviarMensaje(socketMemoria,"KEY_PRINT",1,mensaje);
	empaquetarEnviarMensaje(socketcpuConectadas,"KEY_PRINT",1,mensaje);
	empaquetarEnviarMensaje(socketFS,"KEY_PRINT",1,mensaje);
}

void correrServidor(void* arg){
	threadParams* params = arg;
	correrServidorMultiConexion(params->socketEscucha,params->nuevaConexion,params->desconexion,params->funciones,params->handshakes);
}

int main(int argc, char** argv) {
	pthread_t thread_consola, thread_cpu;

	lista_cpus_conectadas = list_create();

	t_config* configFile= cargarConfiguracion(argv[1]);

    t_dictionary* diccionarioFunciones = dictionary_create();
    dictionary_put(diccionarioFunciones,"KEY_PRINT",&mostrarMensaje);
    dictionary_put(diccionarioFunciones,"ERROR_FUNC",&mostrarMensaje);

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

    if ((socketMemoria = conectar(IpMemoria,PuertoMemoria)) == -1)
    	exit(EXIT_FAILURE);
    if(handshake(socketMemoria,"HKEME","HMEKE")){
    		empaquetarEnviarMensaje(socketMemoria,"KEY_PRINT",1,"hola");
    		puts("Se pudo realizar handshake");
    }
    else{
    	puts("No se pudo realizar handshake");
    	exit(EXIT_FAILURE);
    	}

    if ((socketFS = conectar(IpFS,PuertoFS)) == -1)
        exit(EXIT_FAILURE);
    if(handshake(socketFS,"HKEFS","HFSKE")){
     		empaquetarEnviarMensaje(socketFS,"KEY_PRINT",1,"hola");
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

    list_destroy_and_destroy_elements(lista_cpus_conectadas, free);

    config_destroy(configFile);

	return EXIT_SUCCESS;
}

t_config* cargarConfiguracion(char* archivo){
	t_config* archivo_cnf;
	modo_planificacion Modo;

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
