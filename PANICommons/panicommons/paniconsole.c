/*
 * paniconsole.c
 *
 *  Created on: 20/4/2017
 *      Author: utnso
 */

#include "paniconsole.h"

void showCommands(t_dictionary* dicCommands){

	printf("%s","Comandos disponibles:\n\r");
	void showCommand(char* key, void* value){
		printf("%s\n\r",key);
	}

	dictionary_iterator(dicCommands,showCommand);
}


void doCommand(char* command,t_dictionary* dicCommands){
	char* commandWithNoSpace = string_substring_until(command,string_length(command)-1);
	char** functionAndParams = string_split(commandWithNoSpace," ");
	free(commandWithNoSpace);
	int size = sizeArray(functionAndParams);

	if(functionAndParams[0] != NULL && dictionary_get(dicCommands,functionAndParams[0]) != NULL){
		void (*funcion) (int,char**) = dictionary_get(dicCommands,functionAndParams[0]);
		funcion(size,functionAndParams);
	}else if (functionAndParams[0] != NULL && string_equals_ignore_case(functionAndParams[0],"help")){
		showCommands(dicCommands);
		freeElementsArray(functionAndParams,size);
	}else{
		printf("%s","Comando desconocido\n\r");
		freeElementsArray(functionAndParams,size);
	}
	free(functionAndParams);
}

void waitCommand(t_dictionary* dicCommands){
	while(1) {
		char* input = (char *)malloc(255 * sizeof(char));
		fgets(input,255,stdin);
		doCommand(input,dicCommands);
		if (dictionary_has_key(dicCommands,"theEnd")){
			free(input);
			break;
		}
		free(input);
	}
}
