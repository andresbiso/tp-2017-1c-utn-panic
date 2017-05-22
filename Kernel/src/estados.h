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
#include <panicommons/serializacion.h>
#include <semaphore.h>
#include <pthread.h>

t_queue *colaNew;
t_queue *colaReady;
t_queue *colaExec;
t_queue *colaBlocked;
t_queue *colaExit;
t_list *lista_programas_actuales;
t_list *lista_cpus_conectadas;
t_list *lista_relacion;
t_log* logEstados;

sem_t grado;

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
void desbloquear_pcb(t_pcb* pcb);
void destruir_pcb (t_pcb* pcbADestruir);

#endif /* SRC_ESTADOS_H_ */
