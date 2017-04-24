#include "PrimitivasAnsisop.h"

t_puntero pos_fisica_a_logica(t_posMemoria posfisica){
	return posfisica.page*PAGESIZE+posfisica.offset;
}

t_posMemoria pos_logica_a_fisica(t_puntero poslogica){
	t_posMemoria respuesta;
	respuesta.page = poslogica/PAGESIZE;
	respuesta.offset = poslogica % PAGESIZE;
	respuesta.size = 4; //todas las variables pesan 4 Bytes
	return respuesta;
}

// AnSISOP_funciones
t_puntero definirVariable(t_nombre_variable identificador_variable) {
	t_puntero a;
	return a;
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
void imprimirValor(t_valor_variable valor_mostrar) {

}
void imprimirLiteral(char* texto) {

}
void entradaSalida(t_nombre_dispositivo, int tiempo) {

}

// AnSISOP_kernel
void waitAnsisop(t_nombre_semaforo identificador_semaforo) {

}
void signalAnsisop(t_nombre_semaforo identificador_semaforo) {

}
t_puntero alocar(t_valor_variable espacio) {
	t_puntero a;
	return a;
}
void liberar(t_puntero puntero) {

}

FuncionesAnsisop inicializar_primitivas() {
	FuncionesAnsisop funciones_ansisop;
	funciones_ansisop.funciones_comunes = (AnSISOP_funciones) {
		.AnSISOP_definirVariable = definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar =  dereferenciar,
		.AnSISOP_asignar = asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel = irAlLabel,
		.AnSISOP_llamarSinRetorno = llamarSinRetorno,
		.AnSISOP_llamarConRetorno = llamarConRetorno,
		.AnSISOP_finalizar = finalizar,
		.AnSISOP_retornar = retornar,
		.AnSISOP_imprimirValor = imprimirValor,
		.AnSISOP_imprimirLiteral = imprimirLiteral,
		.AnSISOP_entradaSalida = entradaSalida
	};

	funciones_ansisop.funciones_kernel =(AnSISOP_kernel) {
		.AnSISOP_wait = waitAnsisop,
		.AnSISOP_signal = signalAnsisop,
		.AnSISOP_alocar = alocar,
		.AnSISOP_liberar = liberar
	};

	return funciones_ansisop;
 }
