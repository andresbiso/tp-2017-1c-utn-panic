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

		if (error_en_ejecucion) {
			return -1;
		}

		log_info(cpu_log,"Se solicita definir %c", identificador_variable);

		if(actual_pcb->fin_stack.pag == actual_pcb->cant_pags_totales){
			log_warning(cpu_log,"Stack Overflow: Se ha intentado leer una pagina invalida");
			puts("Stack Overflow: Se ha intentado leer una pagina invalida");
			error_en_ejecucion = true;
			return -1;
		}

		int actual_entrada = actual_pcb->cant_entradas_indice_stack - 1;
		registro_indice_stack* actual_stack = &actual_pcb->indice_stack[actual_entrada];

		int poslogica;

		t_pedido_almacenar_bytes* pedido = (t_pedido_almacenar_bytes*)malloc(sizeof (t_pedido_almacenar_bytes));
		pedido->pid = actual_pcb->pid;

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

		pedido->pagina = actual_pcb->fin_stack.pag;
		pedido->offsetPagina = actual_pcb->fin_stack.offset;

		actual_pcb->fin_stack.offset += 4;

		if(actual_pcb->fin_stack.offset >= pagesize){
			actual_pcb->fin_stack.offset = 0;
			actual_pcb->fin_stack.pag++;
		}

		pedido->tamanio = 4;
		pedido->data = malloc(sizeof(int32_t));
		memset(pedido->data,0,sizeof(int32_t));
		char* buffer =  serializar_pedido_almacenar_bytes(pedido);
		free(pedido->data);
		// Tamanio de la estructura. Data 4 bytes = int32_t
		empaquetarEnviarMensaje(socketMemoria, "ALMC_BYTES", (sizeof(int32_t)*4)+pedido->tamanio, buffer);
		free(buffer);
		free(pedido);

		t_package* paqueteRespuesta = recibirPaquete(socketMemoria, NULL);
		t_respuesta_almacenar_bytes* bufferRespuesta = deserializar_respuesta_almacenar_bytes(paqueteRespuesta->datos);

		if (bufferRespuesta->codigo == OK_ALMACENAR){
			log_info(cpu_log,"Exito al almacenar en el stack");
		} else {
			log_error(cpu_log,"Hubo un error al modificar la pagina");
			error_en_ejecucion = 1;
			actual_pcb->exit_code = -5;
			return -1;
		}

		borrarPaquete(paqueteRespuesta);
		free(bufferRespuesta);

		return poslogica;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable) {

	if (error_en_ejecucion) {
		return -1;
	}

	log_info(cpu_log,"Se solicita obtener la posición de la variable: %c", identificador_variable);

	registro_indice_stack* stack_actual = &actual_pcb->indice_stack[actual_pcb->cant_entradas_indice_stack-1];

	if(isalpha(identificador_variable)){
		log_info(cpu_log,"Identificado %c como una variable",identificador_variable);
		int i;
		for(i=stack_actual->cant_variables-1;i>=0;i--){
			if(stack_actual->variables[i].id == identificador_variable){
				t_puntero poslogica = pos_fisica_a_logica(stack_actual->variables[i].posicionVar);
				log_info(cpu_log,"La posicion logica de la variable %c es %d",identificador_variable,poslogica);
				return poslogica;
			}
		}
		log_info(cpu_log,"La variable %c no se pudo encontrar en el stack",identificador_variable);
	}

	if(isdigit(identificador_variable)){
		int numero_variable = identificador_variable;
		log_info(cpu_log,"Identificado %c como un argumento",identificador_variable);

		if(numero_variable >= stack_actual->cant_argumentos) {
			log_error(cpu_log,"Es %d es un argumento incorrecto, porque solo hay %d argumentos",numero_variable,stack_actual->cant_argumentos);
			error_en_ejecucion = true;
			actual_pcb->exit_code = -5;
			return -1;
		}

		t_puntero poslogica = pos_fisica_a_logica(stack_actual->argumentos[numero_variable]);
		log_info(cpu_log,"La posicion logica del argumento %d es %d",numero_variable,poslogica);
		return poslogica;
	}

	log_error(cpu_log,"No se pudo encontrar la variable %c",identificador_variable);
	error_en_ejecucion = 1;
	actual_pcb->exit_code = -5;

	return -1;
}
t_valor_variable dereferenciar(t_puntero direccion_variable) {

	if (error_en_ejecucion) {
		return -1;
	}

	t_posMemoria posicionFisica = pos_logica_a_fisica(direccion_variable);

	log_debug(cpu_log,
			"Se solicita dereferenciar la dirección\nLogica: %d\nFisica: pag %d, offset %d",
			direccion_variable,
			posicionFisica.pag,
			posicionFisica.offset);

	t_pedido_solicitar_bytes pedido;
	pedido.pid=actual_pcb->pid;
	pedido.pagina=posicionFisica.pag;
	pedido.offsetPagina=posicionFisica.offset;
	pedido.tamanio=4;

	char* buffer = serializar_pedido_solicitar_bytes(&pedido);
	empaquetarEnviarMensaje(socketMemoria,"SOLC_BYTES",sizeof(t_pedido_solicitar_bytes),buffer);
	free(buffer);

	t_package* paquete = recibirPaquete(socketMemoria,NULL);

	t_respuesta_solicitar_bytes* respuesta = deserializar_respuesta_solicitar_bytes(paquete->datos);
	borrarPaquete(paquete);

	t_valor_variable valor;
	memcpy(&valor,respuesta->data,4);

	free(respuesta->data);
	free(respuesta);

	log_info(cpu_log,"Valor dereferenciado = %d",valor);
	return valor;
}
void asignar(t_puntero	direccion_variable,	t_valor_variable valor) {
	if (error_en_ejecucion) {
		return;
	}

	t_posMemoria posicionFisica = pos_logica_a_fisica(direccion_variable);

	t_pedido_almacenar_bytes pedido;
	pedido.pid=actual_pcb->pid;
	pedido.pagina = posicionFisica.pag;
	pedido.offsetPagina = posicionFisica.offset;
	pedido.tamanio = posicionFisica.size;
	pedido.data = malloc(posicionFisica.size);
	memcpy(pedido.data,&valor,posicionFisica.size);

	char *buffer = serializar_pedido_almacenar_bytes(&pedido);
	empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",(sizeof(int32_t)*4)+pedido.tamanio,buffer);
	free(buffer);
	free(pedido.data);

	log_info(cpu_log,
			"Se solicita asignar la dirección logica %d con el valor %d (posicion fisica: pag %d, offset %d)",
			direccion_variable,valor,
			posicionFisica.pag,
			posicionFisica.offset);

	t_package *paquete = recibirPaquete(socketMemoria,NULL);

	t_respuesta_almacenar_bytes* respuesta = deserializar_respuesta_almacenar_bytes(paquete->datos);

	if(respuesta->codigo == OK_ALMACENAR){
		log_info(cpu_log,"Pedido de asignacion correcto");
	}else{
		log_error(cpu_log,"Pedido de asignacion incorrecto");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = -5;
		return;
	}

	free(respuesta);
	borrarPaquete(paquete);
}
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable) {
	if (error_en_ejecucion) {
		return -1;
	}
	//primero serializo
	//empaquetarEnviarMensaje(socketMemoria,"OBT_COMP",,);
	//deserializo respuesta
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
