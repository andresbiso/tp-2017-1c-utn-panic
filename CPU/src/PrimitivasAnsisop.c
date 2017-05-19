#include "PrimitivasAnsisop.h"

t_puntero pos_fisica_a_logica(t_posMemoria posfisica){
	return posfisica.pag*pagesize+posfisica.offset;
}

t_posMemoria pos_logica_a_fisica(t_puntero poslogica){
	t_posMemoria respuesta;
	respuesta.pag = poslogica/pagesize;
	respuesta.offset = poslogica % pagesize;
	respuesta.size = 4;
	return respuesta;
}

// AnSISOP_funciones
t_puntero definirVariable(t_nombre_variable identificador_variable) {

		if(stack_overflow) {
			return -1;
		}

		log_info(cpu_log,"Se solicita definir %c", identificador_variable);

		if(actual_pcb->fin_stack.pag == actual_pcb->cant_pags_totales){
			log_warning(cpu_log,"Stack Overflow: Se ha intentado leer una pagina invalida");
			puts("Stack Overflow: Se ha intentado leer una pagina invalida");
			stack_overflow = true;
			return -1;
		}

		int actual_entrada = actual_pcb->cant_entradas_indice_stack - 1;
		registro_indice_stack* actual_stack = &actual_pcb->indice_stack[actual_entrada];

		int poslogica;

		if(isalpha(identificador_variable)) {
			log_debug(cpu_log,"%c es una variable",identificador_variable);

			actual_stack->cant_variables++;
			int cant_variables = actual_stack->cant_variables;

			actual_stack->variables=realloc(actual_stack->variables,cant_variables*sizeof(t_variable));

			t_variable nueva_variable;
			nueva_variable.id = identificador_variable;
			nueva_variable.posicionVar = actual_pcb->fin_stack;

			actual_stack->variables[cant_variables-1] = nueva_variable;

			poslogica = pos_fisica_a_logica(nueva_variable.posicionVar);
			log_info(cpu_log,"Se definio la variable %c\n Posicion fisica: Pagina %d, offset %d\n Posicion logica: %d",
					identificador_variable,
					nueva_variable.posicionVar.pag,
					nueva_variable.posicionVar.offset,
					poslogica);
		}

		if(isdigit(identificador_variable)){
			log_debug(cpu_log,"%c es un argumento",identificador_variable);

			actual_stack->cant_argumentos++;
			int cant_argumentos = actual_stack->cant_argumentos;

			actual_stack->argumentos=realloc(actual_stack->argumentos, cant_argumentos*sizeof(t_posMemoria));

			actual_stack->argumentos[cant_argumentos-1] = actual_pcb->fin_stack;

			poslogica = pos_fisica_a_logica(actual_pcb->fin_stack);
			log_info(cpu_log,
					"Se definio el argumento %c\n Posicion fisica: Pagina %d, offset %d\n Posicion logica: %d",
					identificador_variable,
					actual_pcb->fin_stack.pag,
					actual_pcb->fin_stack.offset,
					poslogica);
		}

		actual_pcb->fin_stack.offset += 4;

		if(actual_pcb->fin_stack.offset >= pagesize){
			actual_pcb->fin_stack.offset = 0;
			actual_pcb->fin_stack.pag++;
		}

		t_pedido_almacenar_bytes* pedido = (t_pedido_almacenar_bytes*)malloc(sizeof (t_pedido_almacenar_bytes));
		pedido->pid = actual_pcb->pid;
		pedido->pagina = actual_pcb->indice_codigo->pag;
		pedido->offsetPagina = actual_pcb->indice_codigo->offset;
		pedido->tamanio = actual_pcb->indice_codigo->size;
		char* buffer =  serializar_pedido_almacenar_bytes(pedido);
		int longitudMensaje = sizeof(t_pedido_almacenar_bytes);
		if(empaquetarEnviarMensaje(socketMemoria, "ALMC_BYTES", longitudMensaje, buffer)) {
			perror("Hubo un error procesando el paquete");
			exit(EXIT_FAILURE);
		}
		t_package* paqueteRespuesta = recibirPaquete(socketMemoria, NULL);
		t_respuesta_almacenar_bytes* bufferRespuesta = deserializar_respuesta_almacenar_bytes(paqueteRespuesta->datos);

		if (bufferRespuesta->codigo == OK_ALMACENAR){
			puts("Pagina modificada con exito");
		} else {
			perror("Hubo un error al modificar la pagina");
			exit(EXIT_FAILURE);
		}

		free(pedido);
		free(buffer);
		borrarPaquete(paqueteRespuesta);
		free(bufferRespuesta);

		return poslogica;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {
	t_puntero a;
	return a;
}
t_valor_variable dereferenciar(t_puntero direccion_variable) {
	t_valor_variable a;
	return a;
}
void asignar(t_puntero	direccion_variable,	t_valor_variable valor) {

}
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable) {
	t_valor_variable a;
	return a;
}
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor) {
	t_valor_variable a;
	return a;
}
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta) {

}
void llamarSinRetorno(t_nombre_etiqueta etiqueta) {

}
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {

}
void finalizar(void) {

}
void retornar(t_valor_variable retorno) {

}

// AnSISOP_kernel
void waitAnsisop(t_nombre_semaforo identificador_semaforo) {

}
void signalAnsisop(t_nombre_semaforo identificador_semaforo) {

}
t_puntero reservar(t_valor_variable espacio) {
	t_puntero a;
	return a;
}
void liberar(t_puntero puntero) {

}
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags) {
	t_descriptor_archivo a;
	return a;
}
void borrar(t_descriptor_archivo direccion) {

}
void cerrar(t_descriptor_archivo descriptor_archivo) {

}
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion) {

}
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio) {

}
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio) {

}

FuncionesAnsisop* inicializar_primitivas() {
	FuncionesAnsisop* funciones_ansisop = (FuncionesAnsisop*)malloc(sizeof(FuncionesAnsisop));
	AnSISOP_funciones* funciones_comunes = (AnSISOP_funciones*)malloc(sizeof(AnSISOP_funciones));
	AnSISOP_kernel* funciones_kernel = (AnSISOP_kernel*)malloc(sizeof(AnSISOP_kernel));

	funciones_comunes->AnSISOP_definirVariable = &definirVariable;
	funciones_comunes->AnSISOP_obtenerPosicionVariable = &obtenerPosicionVariable;
	funciones_comunes->AnSISOP_dereferenciar =  &dereferenciar;
	funciones_comunes->AnSISOP_asignar = &asignar;
	funciones_comunes->AnSISOP_obtenerValorCompartida = &obtenerValorCompartida;
	funciones_comunes->AnSISOP_asignarValorCompartida = &asignarValorCompartida;
	funciones_comunes->AnSISOP_irAlLabel = &irAlLabel;
	funciones_comunes->AnSISOP_llamarSinRetorno = &llamarSinRetorno;
	funciones_comunes->AnSISOP_llamarConRetorno = &llamarConRetorno;
	funciones_comunes->AnSISOP_finalizar = &finalizar;
	funciones_comunes->AnSISOP_retornar = &retornar;


	funciones_kernel->AnSISOP_wait = &waitAnsisop;
	funciones_kernel->AnSISOP_signal = &signalAnsisop;
	funciones_kernel->AnSISOP_reservar = &reservar;
	funciones_kernel->AnSISOP_liberar = &liberar;
	funciones_kernel->AnSISOP_abrir = &abrir;
	funciones_kernel->AnSISOP_borrar = &borrar;
	funciones_kernel->AnSISOP_cerrar = &cerrar;
	funciones_kernel->AnSISOP_moverCursor = &moverCursor;
	funciones_kernel->AnSISOP_escribir = &escribir;
	funciones_kernel->AnSISOP_leer = &leer;

	funciones_ansisop->funciones_comunes = funciones_comunes;
	funciones_ansisop->funciones_kernel = funciones_kernel;

	return funciones_ansisop;
 }

void liberarFuncionesAnsisop(FuncionesAnsisop* funciones_ansisop) {
	free(funciones_ansisop->funciones_comunes);
	free(funciones_ansisop->funciones_kernel);
	free(funciones_ansisop);
}
