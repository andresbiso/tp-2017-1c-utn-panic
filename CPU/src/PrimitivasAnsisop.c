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
			actual_pcb->exit_code=FINALIZAR_STACK_OVERFLOW;
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
		int numero_variable = identificador_variable-'0';
		log_info(cpu_log,"Identificado %c como un argumento",identificador_variable);

		if(numero_variable > stack_actual->cant_argumentos) {
			log_error(cpu_log,"Es %d es un argumento incorrecto, porque solo hay %d argumentos",numero_variable,stack_actual->cant_argumentos);
			error_en_ejecucion = true;
			actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
			return -1;
		}

		t_puntero poslogica = pos_fisica_a_logica(stack_actual->argumentos[numero_variable]);
		log_info(cpu_log,"La posicion logica del argumento %d es %d",numero_variable,poslogica);
		return poslogica;
	}

	log_error(cpu_log,"No se pudo encontrar la variable %c",identificador_variable);
	error_en_ejecucion = 1;
	actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;

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
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		return;
	}

	free(respuesta);
	borrarPaquete(paquete);
}
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable) {
	if (error_en_ejecucion) {
		return -1;
	}

	t_pedido_obtener_variable_compartida pedido;
	pedido.pid=actual_pcb->pid;
	pedido.tamanio=strlen(variable);
	pedido.nombre_variable_compartida=malloc(strlen(variable));
	memcpy(pedido.nombre_variable_compartida,variable,strlen(variable));

	char *buffer = serializar_pedido_obtener_variable_compartida(&pedido);
	empaquetarEnviarMensaje(socketKernel,"GET_VAR_COMP",(sizeof(int32_t)*2)+pedido.tamanio,buffer);
	free(buffer);
	free(pedido.nombre_variable_compartida);

	log_info(cpu_log,
			"Se solicita valor variable compartida %s",
			variable);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_obtener_variable_compartida* respuesta = deserializar_respuesta_obtener_variable_compartida(paquete->datos);

	if(respuesta->codigo == OK_VARIABLE){
		log_info(cpu_log,"Se ha obtenido el valor de la variable compartida correctamente");
	}else{
		log_error(cpu_log,"Error al intentar obtener valor variable compartida");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		return -1;
	}

	return respuesta->valor_variable_compartida;
}
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor) {
	if (error_en_ejecucion) {
		return -1;
	}

	t_pedido_asignar_variable_compartida pedido;
	pedido.pid=actual_pcb->pid;
	pedido.tamanio=strlen(variable);
	pedido.nombre_variable_compartida=malloc(strlen(variable));
	pedido.valor_variable_compartida=valor;
	memcpy(pedido.nombre_variable_compartida,variable,strlen(variable));

	char *buffer = serializar_pedido_asignar_variable_compartida(&pedido);
	empaquetarEnviarMensaje(socketKernel,"SET_VAR_COMP",(sizeof(int32_t)*3)+pedido.tamanio,buffer);
	free(buffer);
	free(pedido.nombre_variable_compartida);

	log_info(cpu_log,
			"Se solicita valor variable compartida %s",
			variable);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_asignar_variable_compartida* respuesta = deserializar_respuesta_asignar_variable_compartida(paquete->datos);

	if(respuesta->codigo == OK_ASIGNAR_VARIABLE){
		log_info(cpu_log,"Se ha obtenido el valor de la variable compartida correctamente");
	}else{
		log_error(cpu_log,"Error al intentar obtener valor variable compartida");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		return -1;
	}

	return valor;
}
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta) {
	if (error_en_ejecucion) {
		return;
	}

	int tamanioLabel = strlen(t_nombre_etiqueta);

	if(t_nombre_etiqueta[tamanioLabel-1]=='\n'){
		t_nombre_etiqueta[tamanioLabel-1]='\0';
	}

	t_puntero_instruccion puntero_instruccion = metadata_buscar_etiqueta(t_nombre_etiqueta, actual_pcb->indice_etiquetas, actual_pcb->tamano_etiquetas);
	if (puntero_instruccion == -1) {
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ERROR_SIN_DEFINICION;
		log_info(cpu_log,"Error al intentar encontrar el label: %s", t_nombre_etiqueta);
		return;
	}
	actual_pcb->pc = puntero_instruccion;
	log_info(cpu_log,"Yendo al label: %s", t_nombre_etiqueta);
	return;
}
void llamarSinRetorno(t_nombre_etiqueta etiqueta) {
	if (error_en_ejecucion) {
			return;
	}

	actual_pcb->cant_entradas_indice_stack++;
	int tamanio_stack = actual_pcb->cant_entradas_indice_stack;
	actual_pcb->indice_stack = realloc(actual_pcb->indice_stack,sizeof(registro_indice_stack)*tamanio_stack);

	// tamanio_stack - 1 = a nuevo registro de indice agregado
	registro_indice_stack* stack_actual = &actual_pcb->indice_stack[tamanio_stack-1];
	stack_actual->cant_argumentos=0;
	stack_actual->cant_variables=0;
	stack_actual->argumentos = NULL;
	stack_actual->variables = NULL;
	stack_actual->pos_retorno = actual_pcb->pc;
	stack_actual->pos_var_retorno.pag = -1;
	stack_actual->pos_var_retorno.offset = -1;
	stack_actual->pos_var_retorno.size = -1;
	if (tamanio_stack > 1) {
		stack_actual->posicion = actual_pcb->indice_stack[tamanio_stack-2].posicion + 1;
	} else {
		stack_actual->posicion = 0;
	}

	log_info(cpu_log,"Llamando a funcion: %s sin retorno", etiqueta);

	irAlLabel(etiqueta);
}
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
	if (error_en_ejecucion) {
			return;
	}

	actual_pcb->cant_entradas_indice_stack++;
	int tamanio_stack = actual_pcb->cant_entradas_indice_stack;
	actual_pcb->indice_stack = realloc(actual_pcb->indice_stack,sizeof(registro_indice_stack)*tamanio_stack);

	// tamanio_stack - 1 = a nuevo registro de indice agregado
	registro_indice_stack* stack_actual = &actual_pcb->indice_stack[tamanio_stack-1];
	stack_actual->cant_argumentos=0;
	stack_actual->cant_variables=0;
	stack_actual->argumentos = NULL;
	stack_actual->variables = NULL;
	stack_actual->pos_retorno = actual_pcb->pc;
	stack_actual->pos_var_retorno = pos_logica_a_fisica(donde_retornar);
	if (tamanio_stack > 1) {
		stack_actual->posicion = actual_pcb->indice_stack[tamanio_stack-2].posicion + 1;
	} else {
		stack_actual->posicion = 0;
	}

	log_info(cpu_log,"Llamando a funcion: %s con retorno a: %d", etiqueta, donde_retornar);

	irAlLabel(etiqueta);
}
void finalizar(void) {
	if (error_en_ejecucion) {
			return;
	}

	int tamanio_stack = actual_pcb->cant_entradas_indice_stack;
	registro_indice_stack* stack_actual = &actual_pcb->indice_stack[tamanio_stack-1];
	if ((tamanio_stack-1) != 0) {
		actual_pcb->pc = stack_actual->pos_retorno;
		if (stack_actual->pos_var_retorno.pag != -1) {
			int posicion_logica = pos_fisica_a_logica(stack_actual->pos_var_retorno);
			int valor_variable = dereferenciar(posicion_logica);
			retornar(valor_variable);
			log_info(cpu_log,"Finalizo funcion con retorno");
			log_info(cpu_log,"Regreso a intruccion: %d", actual_pcb->pc);
			return;
		}
		actual_pcb->cant_entradas_indice_stack--;
		tamanio_stack = actual_pcb->cant_entradas_indice_stack;
		actual_pcb->indice_stack = realloc(actual_pcb->indice_stack,sizeof(registro_indice_stack)*tamanio_stack);
		log_info(cpu_log,"Finalizo funcion sin retorno");
		log_info(cpu_log,"Regreso a intruccion: %d", actual_pcb->pc);
		return;
	}
	actual_pcb->cant_entradas_indice_stack--;
	tamanio_stack = actual_pcb->cant_entradas_indice_stack;
	actual_pcb->indice_stack = realloc(actual_pcb->indice_stack,sizeof(registro_indice_stack)*tamanio_stack);
	log_info(cpu_log,"Finalizo ejecucion programa");
	return;
}
void retornar(t_valor_variable retorno) {
	log_info(cpu_log,"Valor retorno de la funcion: %d", retorno);

	int tamanio_stack = actual_pcb->cant_entradas_indice_stack;
	registro_indice_stack* stack_actual = &actual_pcb->indice_stack[tamanio_stack-1];

	t_posMemoria pos_retorno = stack_actual->pos_var_retorno;

	if(pos_retorno.pag != -1){
		t_puntero pos_retorno_logica = pos_fisica_a_logica(pos_retorno);

		asignar(pos_retorno_logica,retorno);

		actual_pcb->cant_entradas_indice_stack--;
		tamanio_stack = actual_pcb->cant_entradas_indice_stack;
		actual_pcb->indice_stack = realloc(actual_pcb->indice_stack,sizeof(registro_indice_stack)*tamanio_stack);
	}
	return;
}

// AnSISOP_kernel
void waitAnsisop(t_nombre_semaforo identificador_semaforo) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_wait pedido;
	pedido.pcb=actual_pcb;
	pedido.tamanio=strlen(identificador_semaforo);
	pedido.rafagas=rafagas_ejecutadas;
	pedido.semId=malloc(strlen(identificador_semaforo));
	memcpy(pedido.semId,identificador_semaforo,strlen(identificador_semaforo));

	char *buffer = serializar_pedido_wait(&pedido);
	empaquetarEnviarMensaje(socketKernel,"WAIT",(sizeof(int32_t)*2)+pedido.tamanio+tamanio_pcb(pedido.pcb),buffer);
	free(buffer);
	free(pedido.semId);

	log_info(cpu_log,
			"Se envia wait al semaforo %s",
			identificador_semaforo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_wait* respuesta = deserializar_respuesta_wait(paquete->datos);

	switch(respuesta->respuesta) {
	case WAIT_OK:
		log_info(cpu_log,"El semaforo %s recibio el wait", identificador_semaforo);
		break;
	case WAIT_BLOCKED:
		log_error(cpu_log,"El semaforo %s bloqueo el proceso", identificador_semaforo);
		proceso_bloqueado = 1;
		break;
	case WAIT_NOT_EXIST:
		log_error(cpu_log,"Error: El semaforo %s no existe", identificador_semaforo);
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		break;
	}

	return;
}
void signalAnsisop(t_nombre_semaforo identificador_semaforo) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_signal pedido;
	pedido.pid=actual_pcb->pid;
	pedido.tamanio=strlen(identificador_semaforo);
	pedido.semId=malloc(strlen(identificador_semaforo));
	memcpy(pedido.semId,identificador_semaforo,strlen(identificador_semaforo));

	char *buffer = serializar_pedido_signal(&pedido);
	empaquetarEnviarMensaje(socketKernel,"SIGNAL",(sizeof(int32_t)*2)+pedido.tamanio,buffer);
	free(buffer);
	free(pedido.semId);

	log_info(cpu_log,
			"Se envia signal al semaforo %s",
			identificador_semaforo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_signal* respuesta = deserializar_respuesta_signal(paquete->datos);

	switch(respuesta->respuesta) {
	case SIGNAL_OK:
		log_info(cpu_log,"El semaforo %s recibio el signal", identificador_semaforo);
		break;
	case SIGNAL_NOT_EXIST:
		log_error(cpu_log,"Error: El semaforo %s no existe", identificador_semaforo);
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		break;
	}

	return;
}
t_puntero reservar(t_valor_variable espacio) {
	if (error_en_ejecucion) {
		return -1;
	}

	t_pedido_reservar pedido;
	pedido.pid = actual_pcb->pid;
	pedido.bytes = espacio;
	pedido.paginasTotales = actual_pcb->cant_pags_totales;

	char *buffer = serializar_pedido_reservar(&pedido);
	empaquetarEnviarMensaje(socketKernel,"RESERVAR",sizeof(int32_t)*3,buffer);
	free(buffer);

	log_info(cpu_log,
			"Se necesitan reservar %d bytes en memoria",
			espacio);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_reservar* respuesta = deserializar_respuesta_reservar(paquete->datos);

	switch(respuesta->codigo) {
	case RESERVAR_OK:
		log_info(cpu_log,"Se reservaron %d bytes", espacio);
		break;
	case RESERVAR_OVERFLOW:
		log_error(cpu_log,"Error: PageOverflow", espacio);
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_PAGE_OVERFLOW;
		break;
	case RESERVAR_SIN_ESPACIO:
		log_error(cpu_log,"Error: Sin espacio disponible", espacio);
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_SIN_MEMORIA;
		break;
	}

	return respuesta->puntero;
}
void liberar(t_puntero puntero) {
	if (error_en_ejecucion) {
		return;
	}

	t_posMemoria posFisica = pos_logica_a_fisica(puntero);
	t_pedido_liberar pedido;
	pedido.pid = actual_pcb->pid;
	pedido.pagina = posFisica.pag;
	pedido.offset = posFisica.offset;

	char *buffer = serializar_pedido_liberar(&pedido);
	empaquetarEnviarMensaje(socketKernel,"LIBERAR",sizeof(int32_t)*3,buffer);
	free(buffer);

	log_info(cpu_log,
			"Se necesita liberar el espacio asociado a el puntero %d en memoria. Page: %d Offset: %d",
			puntero, posFisica.pag, posFisica.offset);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_liberar* respuesta = deserializar_respuesta_liberar(paquete->datos);

	switch(respuesta->codigo) {
	case LIBERAR_OK:
		log_info(cpu_log,"Se libero el espacio");
		break;
	case LIBERAR_ERROR:
		log_error(cpu_log,"Error: Al tratar de liberar memoria");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		break;
	}

	return;
}
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags) {
	if (error_en_ejecucion) {
		return -1;
	}

	t_pedido_abrir_archivo pedido;
	pedido.pid=actual_pcb->pid;
	pedido.flags = malloc(sizeof(bool)*3);
	memcpy(pedido.flags,&flags,sizeof(bool)*3);
	pedido.tamanio = strlen(direccion);
	pedido.direccion = malloc(strlen(direccion));;
	memcpy(pedido.direccion,direccion,strlen(direccion));

	char *buffer = serializar_pedido_abrir_archivo(&pedido);
	empaquetarEnviarMensaje(socketKernel,"ABRIR_ARCH",(sizeof(int32_t)*2)+(sizeof(bool)*3)+pedido.tamanio,buffer);
	free(buffer);
	free(pedido.flags);
	free(pedido.direccion);

	log_info(cpu_log,
			"Se solicita abrir archivo en la direccion %s con los permisos lectura: %d, escritura: %d, creacion: %d",
			direccion, flags.lectura, flags.escritura, flags.creacion);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_abrir_archivo* respuesta = deserializar_respuesta_abrir_archivo(paquete->datos);

	borrarPaquete(paquete);

	if(respuesta->codigo == ABRIR_OK){
		log_info(cpu_log,"Se ha abierto el archivo correctamente");
	}else{
		log_error(cpu_log,"Error al intentar abrir el archivo");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ARCHIVO_NO_EXISTE;
		free(respuesta);
		return -1;
	}

	int32_t fd = respuesta->fd;
	free(respuesta);

	return fd;
}
void borrar(t_descriptor_archivo descriptor_archivo) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_borrar_archivo pedido;
	pedido.pid=actual_pcb->pid;
	pedido.fd = descriptor_archivo;

	char *buffer = serializar_pedido_borrar_archivo(&pedido);
	empaquetarEnviarMensaje(socketKernel,"BORRAR_ARCH",(sizeof(int32_t)*2),buffer);
	free(buffer);

	log_info(cpu_log,
			"Se solicita borrar archivo con FD: %d",
			descriptor_archivo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_borrar_archivo* respuesta = deserializar_respuesta_borrar_archivo(paquete->datos);

	borrarPaquete(paquete);

	if(respuesta->codigoRta == BORRAR_OK){
		log_info(cpu_log,"Se ha borrado el archivo correctamente");
	}else if (respuesta->codigoRta == BORRAR_ERROR){
		log_error(cpu_log,"Error al intentar borrar el archivo");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ARCHIVO_NO_EXISTE;
		free(respuesta);
		return;
	} else {
		log_error(cpu_log,"Error: El archivo se encuentra bloqueado");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ERROR_SIN_DEFINICION;
		free(respuesta);
		return;
	}

	free(respuesta);
	return;
}
void cerrar(t_descriptor_archivo descriptor_archivo) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_cerrar_archivo pedido;
	pedido.pid=actual_pcb->pid;
	pedido.fd = descriptor_archivo;

	char *buffer = serializar_pedido_cerrar_archivo(&pedido);
	empaquetarEnviarMensaje(socketKernel,"CERRAR_ARCH",(sizeof(int32_t)*2),buffer);
	free(buffer);

	log_info(cpu_log,
			"Se solicita cerrar archivo con FD: %d",
			descriptor_archivo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_cerrar_archivo* respuesta = deserializar_respuesta_cerrar_archivo(paquete->datos);

	borrarPaquete(paquete);

	if(respuesta->codigoRta == CERRAR_OK){
		log_info(cpu_log,"Se ha cerrado el archivo correctamente");
	}else{
		log_error(cpu_log,"Error al intentar cerrar el archivo");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ARCHIVO_NO_EXISTE;
		free(respuesta);
		return;
	}

	free(respuesta);
	return;
}
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_mover_cursor pedido;
	pedido.pid=actual_pcb->pid;
	pedido.fd = descriptor_archivo;
	pedido.posicion = posicion;

	char *buffer = serializar_pedido_mover_cursor(&pedido);
	empaquetarEnviarMensaje(socketKernel,"MOVER_CURSOR",(sizeof(int32_t)*3),buffer);
	free(buffer);

	log_info(cpu_log,
			"Se solicita mover cursor del archivo con FD: %d a posicion: %d",
			descriptor_archivo, posicion);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_mover_cursor* respuesta = deserializar_respuesta_mover_cursor(paquete->datos);

	borrarPaquete(paquete);

	if(respuesta->codigo == MOVER_OK){
		log_info(cpu_log,"Se ha movido el cursor a la posicion: %d correctamente", posicion);
	}else{
		log_error(cpu_log,"Error al intentar mover el cursor");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ARCHIVO_NO_EXISTE;
		free(respuesta);
		return;
	}

	free(respuesta);
	return;
}
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio) {
	if (error_en_ejecucion) {
		return;
	}

	if (descriptor_archivo == 1) {
		t_aviso_consola pedido_consola;
		pedido_consola.idPrograma=actual_pcb->pid;
		pedido_consola.mensaje = malloc(tamanio);
		pedido_consola.tamaniomensaje = tamanio;
		pedido_consola.terminoProceso=0;
		pedido_consola.mostrarPorPantalla=1;
		memcpy(pedido_consola.mensaje, informacion, tamanio);
		char *buffer_consola = serializar_aviso_consola(&pedido_consola);
		empaquetarEnviarMensaje(socketKernel,"PRINT_MESSAGE",(sizeof(int32_t)*4)+tamanio,buffer_consola);
		free(buffer_consola);
		free(pedido_consola.mensaje);
		log_info(cpu_log,
		"Se solicito la escitura en archivo cuyo descriptor es: %d",
		descriptor_archivo);
		return;
	}

	t_pedido_escribir pedido;
	pedido.pid =actual_pcb->pid;
	pedido.fd = descriptor_archivo;
	pedido.informacion = malloc(tamanio);
	pedido.tamanio = tamanio;
	memcpy(pedido.informacion, informacion, tamanio);

	char *buffer = serializar_pedido_escribir_archivo(&pedido);
	empaquetarEnviarMensaje(socketKernel,"ESCRIBIR_ARCH",(sizeof(int32_t)*3)+tamanio,buffer);
	free(buffer);
	free(pedido.informacion);

	log_info(cpu_log,
			"Se solicito escribir en archivo cuyo FD es: %d",
			descriptor_archivo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_escribir* respuesta = deserializar_respuesta_escribir_archivo(paquete->datos);

	borrarPaquete(paquete);

	switch(respuesta->codigo) {
	case ESCRIBIR_OK:
		log_info(cpu_log,"Se ha escrito en el archivo cuyo FD es: %d correctamente", descriptor_archivo);
		break;
	case ESCRITURA_ERROR:
		log_error(cpu_log,"Error al intentar escribir el archivo");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ESCRIBIR_ARCHIVO_SIN_PERMISOS;
		break;
	case ESCRITURA_SIN_ESPACIO:
		log_error(cpu_log,"Error: el archivo se encuentra sin espacio");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ERROR_SIN_DEFINICION;
		break;
	case ESCRITURA_BLOCKED:
		log_error(cpu_log,"Error: el archivo se encuentra bloqueado");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ERROR_SIN_DEFINICION;
		break;
	default:
		break;
	}

	free(respuesta);
	return;
}
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio) {
	if (error_en_ejecucion) {
		return;
	}

	t_pedido_leer pedido;
	pedido.pid =actual_pcb->pid;
	pedido.descriptor_archivo = descriptor_archivo;
	pedido.tamanio = tamanio;

	char *buffer = serializar_pedido_leer_archivo(&pedido);
	empaquetarEnviarMensaje(socketKernel,"LEER_ARCH",(sizeof(int32_t)*3),buffer);
	free(buffer);

	log_info(cpu_log,
			"Se solicito leer el archivo cuyo FD es: %d",
			descriptor_archivo);

	t_package *paquete = recibirPaquete(socketKernel,NULL);

	t_respuesta_leer* respuesta = deserializar_respuesta_leer_archivo(paquete->datos);

	borrarPaquete(paquete);

	switch(respuesta->codigo) {
	case LEER_OK:
		log_info(cpu_log,"Se ha leido en el archivo cuyo FD es: %d correctamente", descriptor_archivo);
		t_pedido_almacenar_bytes pedido_memoria;
		t_posMemoria posFisica = pos_logica_a_fisica(informacion);
		pedido_memoria.pid = actual_pcb->pid;
		pedido_memoria.pagina = posFisica.pag;
		pedido_memoria.offsetPagina = posFisica.offset;
		pedido_memoria.data = malloc(respuesta->tamanio);
		pedido_memoria.tamanio =respuesta->tamanio;
		memcpy(pedido_memoria.data, respuesta->informacion, respuesta->tamanio);


		char *buffer_memoria = serializar_pedido_almacenar_bytes(&pedido_memoria);
		empaquetarEnviarMensaje(socketMemoria,"ALMC_BYTES",(sizeof(int32_t)*4)+respuesta->tamanio,buffer_memoria);
		free(buffer_memoria);
		free(pedido_memoria.data);

		log_info(cpu_log,
				"Se envio informacion a almacenar en memoria");

		t_package *paquete_memoria = recibirPaquete(socketMemoria,NULL);

		t_respuesta_almacenar_bytes* respuesta_memoria = deserializar_respuesta_almacenar_bytes(paquete_memoria->datos);

		borrarPaquete(paquete_memoria);

		if (respuesta_memoria->codigo == OK_ALMACENAR) {
			log_info(cpu_log,"Se almaceno la informacion correctamente");
		} else if (respuesta_memoria->codigo == PAGINA_ALM_NOT_FOUND) {
			log_info(cpu_log,"Error: no se encontro la pagina: %d", posFisica.pag);
			error_en_ejecucion = 1;
			actual_pcb->exit_code = FINALIZAR_EXCEPCION_MEMORIA;
		} else {
			log_info(cpu_log,"Error: PageOverflow");
			error_en_ejecucion = 1;
			actual_pcb->exit_code = FINALIZAR_PAGE_OVERFLOW;
		}
		break;
	case LEER_NO_EXISTE:
		log_error(cpu_log,"Error al intentar leer el archivo");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_ARCHIVO_NO_EXISTE;
		break;
	case LEER_BLOCKED:
		log_error(cpu_log,"Error: el archivo se encuentra bloqueado");
		error_en_ejecucion = 1;
		actual_pcb->exit_code = FINALIZAR_LEER_ARCHIVO_SIN_PERMISOS;
		break;
	default:
		break;
	}

	free(respuesta);
	return;
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
