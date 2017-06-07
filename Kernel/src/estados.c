/*
 * estados.c
 *
 *  Created on: 30/4/2017
 *      Author: utnso
 */

#include "estados.h"

void enviarMensajeConsola(char*mensaje,char*key,int32_t pid,int32_t socket,int32_t terminoProceso,int32_t mostrarPorPantalla){
	t_aviso_consola aviso_consola;
	aviso_consola.mensaje = mensaje;
	aviso_consola.tamaniomensaje = strlen(aviso_consola.mensaje);
	aviso_consola.idPrograma = pid;
	aviso_consola.terminoProceso = terminoProceso;
	aviso_consola.mostrarPorPantalla = mostrarPorPantalla;

	char *pedido_serializado = serializar_aviso_consola(&aviso_consola);

	empaquetarEnviarMensaje(socket,key,aviso_consola.tamaniomensaje+(sizeof(int32_t)*4),pedido_serializado);
	free(pedido_serializado);
}

void crear_colas(){
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

void checkStopped(){
	pthread_mutex_lock(&stoppedMutex);
	if(isStopped)
		sem_wait(&stopped);
	pthread_mutex_unlock(&stoppedMutex);
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

	checkStopped();

	pthread_mutex_lock(&colaNewMutex);
	queue_push(colaNew, pcb);
	pthread_mutex_unlock(&colaNewMutex);

	log_debug(logEstados, "El PCB: %d paso a la cola New",pcb->pid);
}

void moverA_colaExit(t_pcb *pcb)
{
	if(!pcb) return;

	checkStopped();

	pthread_mutex_lock(&colaExitMutex);
	queue_push(colaExit, pcb);
	pthread_mutex_unlock(&colaExitMutex);
	sem_post(&grado);
	log_debug(logEstados, "El PCB: %d paso a la cola Exit",pcb->pid);
}

void moverA_colaBlocked(t_pcb *pcb)
{
	if(!pcb)
		return;

	checkStopped();

	pthread_mutex_lock(&colaBlockedMutex);
	queue_push(colaBlocked, pcb);
	pthread_mutex_unlock(&colaBlockedMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Blocked",pcb->pid);
}

void moverA_colaExec(t_pcb *pcb)
{
	if(!pcb)
		return;

	checkStopped();

	pthread_mutex_lock(&colaExecMutex);
	queue_push(colaExec, pcb);
	pthread_mutex_unlock(&colaExecMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Exec",pcb->pid);
}

void moverA_colaReady(t_pcb *pcb){
	if(!pcb)
		return;

	checkStopped();

	pthread_mutex_lock(&colaReadyMutex);
	queue_push(colaReady, pcb);
	pthread_mutex_unlock(&colaReadyMutex);
	log_debug(logEstados, "El PCB: %d paso a la cola Ready",pcb->pid);

	if(processIsForFinish(pcb->pid)){
		pcb->exit_code=FINALIZAR_BY_CONSOLE;
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
	}


}

t_pcb* sacarCualquieraDeReady(){
	t_pcb*pcb = NULL;

	checkStopped();

	pthread_mutex_lock(&colaReadyMutex);
	pcb = queue_pop(colaReady);
	pthread_mutex_unlock(&colaReadyMutex);

	return pcb;
}

t_pcb *sacarDe_colaNew(uint32_t pid){

	checkStopped();

	pthread_mutex_lock(&colaNewMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaNew->elements, pid);
	if(pcb)
		log_debug(logEstados, "El PCB: %d salio de la cola New",pid);
	pthread_mutex_unlock(&colaNewMutex);
	return pcb;
}

t_pcb *sacarDe_colaReady(uint32_t pid){

	checkStopped();

	pthread_mutex_lock(&colaReadyMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaReady->elements, pid);
	if(pcb)
		log_debug(logEstados, "El PCB: %d salio de la cola Ready",pid);
	pthread_mutex_unlock(&colaReadyMutex);
	return pcb;
}

t_pcb *sacarDe_colaExec(uint32_t pid){

	checkStopped();

	pthread_mutex_lock(&colaExecMutex);
	t_pcb *pcb = sacar_pcb_por_pid(colaExec->elements, pid);
	if(pcb)
		log_debug(logEstados, "Sacando PCB: %d de la cola Exec",pid);
	pthread_mutex_unlock(&colaExecMutex);
	return pcb;
}

t_pcb *sacarDe_colaBlocked(uint32_t pid){

	checkStopped();

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
		log_info(logNucleo,"Bloqueo a PID = %d",pcbabloquear->pid);

		t_pcb* aux = pcbviejo;
		pcbviejo = pcbabloquear;
		destruir_pcb(aux);

		moverA_colaBlocked(pcbviejo);
	}
}

void desbloquear_pcb(int32_t pid){
	t_pcb* pcbsacado = sacarDe_colaBlocked(pid);

	if(pcbsacado)
		moverA_colaReady(pcbsacado);
}

bool processIsForFinish(int32_t pid){

	bool matchPID(void* elemt){
		return (*((int32_t*) elemt))==pid;
	}

	pthread_mutex_lock(&listForFinishMutex);
	if(list_find(listForFinish,matchPID) != NULL){
		list_remove_and_destroy_by_condition(listForFinish,matchPID,free);
		pthread_mutex_unlock(&listForFinishMutex);
		return true;
	}
	pthread_mutex_unlock(&listForFinishMutex);

	return false;
}

void cpu_change_running(int32_t socket, bool newState){
	bool matchSocket(void*elem){
		return ((t_cpu*)elem)->socket==socket;
	}

	pthread_mutex_lock(&mutexCPUConectadas);
	t_cpu* cpu = list_find(lista_cpus_conectadas,matchSocket);
	cpu->corriendo=newState;
	pthread_mutex_unlock(&mutexCPUConectadas);
}


t_consola* matchear_consola_por_pid(int pid){

	bool matchPID_Consola(void *consola) {
						return ((t_consola*)consola)->pid == pid;
					}
	return list_find(lista_programas_actuales, matchPID_Consola);
}

void program_change_running(int32_t pid, bool newState){

	pthread_mutex_lock(&mutexProgramasActuales);
	t_consola* consola = matchear_consola_por_pid(pid);
	consola->corriendo=newState;
	pthread_mutex_unlock(&mutexProgramasActuales);
}

t_respuesta_finalizar_programa* finalizarProcesoMemoria(int32_t pid){
	t_pedido_finalizar_programa pedido;
	pedido.pid = pid;

	char* buffer = serializar_pedido_finalizar_programa(&pedido);
	empaquetarEnviarMensaje(socketMemoria,"FINZ_PROGM",sizeof(t_pedido_finalizar_programa),buffer);
	free(buffer);

	t_package* paquete = recibirPaqueteMemoria();
	t_respuesta_finalizar_programa* respuesta = deserializar_respuesta_finalizar_programa(paquete->datos);
	borrarPaquete(paquete);

	return respuesta;
}

t_package* recibirPaqueteMemoria(){
	pthread_mutex_lock(&mutexMemoria);
	t_package* paquete = recibirPaquete(socketMemoria,NULL);
	pthread_mutex_unlock(&mutexMemoria);
	return paquete;
}


