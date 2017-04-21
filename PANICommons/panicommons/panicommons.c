/*
 * panicommons.c
 *
 *  Created on: 20/4/2017
 *      Author: utnso
 */

#include "panicommons.h"

//Ojo con esta funcion, solo sirve si el char** viene del string_split
int sizeArray(char** array){
	int size = 0;
	while(array[size] != NULL)
		size++;
	return size;
}

//Funcion para liberar arrays (por ejemplo los parametros que reciben las funciones de los clientes de sockets o las consolas)
void freeElementsArray(char** array, int size){
	int i;
	for(i=0;i<size;i++)
		free(array[i]);
}
