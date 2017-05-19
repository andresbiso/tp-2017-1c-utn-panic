/*
 * estados.c
 *
 *  Created on: 30/4/2017
 *      Author: utnso
 */

#include "estados.h"
#include <semaphore.h>
#include <pthread.h>

pthread_mutex_t colaNewMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t colaReadyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t colaBlockedMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t colaExecMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t colaExitMutex = PTHREAD_MUTEX_INITIALIZER;

void crear_colas()
{
	colaNew = queue_create();
	colaReady = queue_create();
	colaExec = queue_create();
	colaBlocked = queue_create();
	colaExit = queue_create();

	lista_cpus_conectadas = list_create();
	lista_programas_actuales = list_create();
	lista_relacion = list_create();
}

void destruir_colas()
{
	queue_destroy_and_destroy_elements(colaNew,free);
	queue_destroy_and_destroy_elements(colaReady,free);
	queue_destroy_and_destroy_elements(colaExec,free);
	queue_destroy_and_destroy_elements(colaBlocked,free);
	queue_destroy_and_destroy_elements(colaExit,free);

	list_destroy_and_destroy_elements(lista_programas_actuales, free);
	list_destroy_and_destroy_elements(lista_cpus_conectadas, free);
}

t_pcb *sacar_pcb_por_pid(t_list *listaAct, uint32_t pidBuscado)
{
	bool matchPID(void *pcb) {
		return ((t_pcb*)pcb)->pid == pidBuscado;
	}

	return list_remove_by_condition(listaAct, matchPID);
}

void moverA_colaNew(t_pcb *pcb)
{
	if(!pcb)
		return;

	pthread_mutex_lock(&colaNewMutex);
	queue_push(colaNew, pcb);
	pthread_mutex_unlock(&colaNewMutex);

	log_debug(logEstados, "El PCB: %d paso a la cola New",pcb->pid);
}

void moverA_colaExit(t_pcb *pcb)
{
	if(!pcb) return;

	pthread_mutex_lock(&colaExitMutex);
	queue_push(colaExit, pcb);
	pthread_mutex_unlock(&colaExitMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Exit",pcb->pid);
}

void moverA_colaBlocked(t_pcb *pcb)
{
	if(!pcb)
		return;

	pthread_mutex_lock(&colaBlockedMutex);
	queue_push(colaBlocked, pcb);
	pthread_mutex_unlock(&colaBlockedMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Blocked",pcb->pid);
}

void moverA_colaExec(t_pcb *pcb)
{
	if(!pcb)
		return;

	pthread_mutex_lock(&colaExecMutex);
	queue_push(colaExec, pcb);
	pthread_mutex_unlock(&colaExecMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Exec",pcb->pid);
}

void moverA_colaReady(t_pcb *pcb)
{
	if(!pcb)
		return;

	pthread_mutex_lock(&colaReadyMutex);
	queue_push(colaReady, pcb);
	pthread_mutex_unlock(&colaReadyMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Ready",pcb->pid);
}

t_pcb *sacarDe_colaNew(uint32_t pid)
{
	pthread_mutex_lock(&colaReadyMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaNew->elements, pid);
	if(pcb)
		log_debug(logEstados, "El PCB: %d salio de la cola New",pid);
	pthread_mutex_unlock(&colaReadyMutex);
	return pcb;
}

t_pcb *sacarDe_colaReady(uint32_t pid)
{
	pthread_mutex_lock(&colaReadyMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaReady->elements, pid);
	if(pcb)
		log_debug(logEstados, "El PCB: %d salio de la cola Ready",pid);
	pthread_mutex_unlock(&colaReadyMutex);
	return pcb;
}

t_pcb *sacarDe_colaExec(uint32_t pid)
{
	pthread_mutex_lock(&colaExecMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaExec->elements, pid);
	if(pcb)
		log_debug(logEstados, "Sacando PCB: %d de la cola Exec",pid);
	pthread_mutex_unlock(&colaExecMutex);
	return pcb;
}

t_pcb *sacarDe_colaBlocked(uint32_t pid)
{
	pthread_mutex_lock(&colaBlockedMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaBlocked->elements, pid);
	if(pcb)
		log_debug(logEstados, "Sacando PCB: %d de la cola Blocked",pid);
	pthread_mutex_unlock(&colaBlockedMutex);
	return pcb;
}

void bloquear_pcb(t_pcb* pcbabloquear){
	if(!pcbabloquear) return;

	t_pcb* pcbviejo = sacarDe_colaExec(pcbabloquear->pid);
	if(pcbviejo){
		printf("Bloqueo a PID = %d\n",pcbabloquear->pid);

		t_pcb* aux = pcbviejo;
		pcbviejo = pcbabloquear;
		destruir_pcb(aux);

		moverA_colaBlocked(pcbviejo);
	}
}

void desbloquear_pcb(t_pcb* pcb){
	if(!pcb)
		return;

	t_pcb* pcbsacado = sacarDe_colaBlocked(pcb->pid);

	if(pcbsacado)
		moverA_colaReady(pcbsacado);
}
