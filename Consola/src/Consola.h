#ifndef CONSOLA_H_
#define CONSOLA_H_


#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/string.h>
#include <panicommons/panisocket.h>
#include <panicommons/paniconsole.h>
#include <stdio.h>
#include <stdlib.h>

/* Global Variables*/
char* IpKernel;
int PuertoKernel;

/*Function prototypes*/
t_config* cargarConfiguracion(char * nombreArchivo);


#endif /* CONSOLA_H_ */




