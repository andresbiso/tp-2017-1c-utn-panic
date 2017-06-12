/*
 * estados.h
 *
 *  Created on: 30/4/2017
 *      Author: utnso
 */

#ifndef SRC_ESTADOS_H_
#define SRC_ESTADOS_H_

#include <commons/collections/queue.h>
#include <commons/log.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <commons/config.h>
#include <panicommons/serializacion.h>
#include <semaphore.h>
#include <pthread.h>
#include <commons/collections/dictionary.h>
#include <parser/metadata_program.h>
#include <panicommons/paniconsole.h>
#include <panicommons/panisocket.h>

typedef struct{
	int socket;
	bool corriendo;
}t_cpu;

typedef struct{
	int pid;
	int socket;
	bool corriendo;
}t_consola;

t_queue *colaNew;
t_queue *colaReady;
t_queue *colaExec;
t_queue *colaBlocked;
t_queue *colaExit;
t_list *lista_programas_actuales;
t_list *lista_cpus_conectadas;
t_list *lista_relacion;
t_log* logEstados;
bool isStopped;

sem_t grado;

t_log *logNucleo;
t_dictionary *variablesCompartidas;
pthread_mutex_t relacionMutex;
pthread_mutex_t colaNewMutex;
pthread_mutex_t colaReadyMutex;
pthread_mutex_t colaBlockedMutex;
pthread_mutex_t colaExecMutex;
pthread_mutex_t colaExitMutex;
pthread_mutex_t stoppedMutex;
pthread_mutex_t listForFinishMutex;
pthread_mutex_t mutexCPUConectadas;
pthread_mutex_t mutexProgramasActuales;
pthread_mutex_t mutexMemoria;
sem_t stopped;

int socketFS;
int socketMemoria;
int tamanio_pag_memoria;

t_list* listForFinish;

void crear_colas();
void destruir_colas();

t_pcb *sacar_pcb_por_pid(t_list *listaAct, uint32_t pidBuscado);

void moverA_colaNew(t_pcb *pcb);
void moverA_colaExit(t_pcb *pcb);
void moverA_colaBlocked(t_pcb *pcb);
void moverA_colaExec(t_pcb *pcb);
void moverA_colaReady(t_pcb *pcb);

t_pcb *sacarDe_colaNew(uint32_t pid);
t_pcb *sacarDe_colaReady(uint32_t pid);
t_pcb *sacarDe_colaExec(uint32_t pid);
t_pcb *sacarDe_colaBlocked(uint32_t pid);
t_pcb* sacarCualquieraDeReady();

void bloquear_pcb(t_pcb* pid);
void desbloquear_pcb(int32_t pid);
void destruir_pcb (t_pcb* pcbADestruir);

bool processIsForFinish(int32_t pid);
void cpu_change_running(int32_t socket, bool newState);
t_consola* matchear_consola_por_pid(int pid);
void eliminarConsolaPorPID(int32_t pid);
void program_change_running(int32_t pid, bool newState);
void enviarMensajeConsola(char*mensaje,char*key,int32_t pid,int32_t socket,int32_t terminoProceso,int32_t mostrarPorPantalla);
t_respuesta_finalizar_programa* finalizarProcesoMemoria(int32_t pid);
t_package* recibirPaqueteMemoria();
void enviar_a_cpu();

#endif /* SRC_ESTADOS_H_ */
