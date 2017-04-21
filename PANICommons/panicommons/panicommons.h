/*
 * panicommons.h
 *
 *  Created on: 20/4/2017
 *      Author: utnso
 */

#ifndef PANICOMMONS_PANICOMMONS_H_
#define PANICOMMONS_PANICOMMONS_H_

#include <stdio.h>
#include <stdlib.h>

int sizeArray(char** array);//Ojo con esta funcion, solo sirve si el char** viene del string_split
void freeElementsArray(char**, int);

#endif /* PANICOMMONS_PANICOMMONS_H_ */
