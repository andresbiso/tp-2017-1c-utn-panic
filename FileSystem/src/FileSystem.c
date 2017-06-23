#include "FileSystem.h"

void error(char** args)
{
	printf("%s",args[0]);
}
void mostrarMensaje(char* mensajes,int socket)
{
 	printf("Mensaje recibido: %s \n",mensajes);
}

char* string_get_string_from_array(char** array,int nroBloques){
	char* result = string_new();

	int i;
	string_append(&result,"[");
	for(i=0;i<nroBloques;i++){
		string_append(&result,array[i]);
		if(i!=(nroBloques-1))
			string_append(&result,",");
	}
	string_append(&result,"]");


	return result;
}

t_config* cargarConfiguracion(char* nombreDelArchivo){
	char* configFilePath =string_new();
	string_append(&configFilePath,nombreDelArchivo);
	t_config* configFile = config_create(configFilePath);
	free(configFilePath);
	if (config_has_property(configFile, "PUERTO")) {
		puerto = config_get_int_value(configFile, "PUERTO");
	} else {
	 	perror("La key PUERTO no existe");
	 	exit(EXIT_FAILURE);
	 }
	if (config_has_property(configFile,"PUNTO_MONTAJE")){
		puntoMontaje= config_get_string_value(configFile,"PUNTO_MONTAJE");
	}else {
		perror("La key PUNTO_MONTAJE no existe");
		exit(EXIT_FAILURE);
	}
	return configFile;
}
void validarArchivo(char* archivo, int socket)
{
	log_info(logFS, "Se validará la existencia del archivo %s", archivo);
	t_respuesta_validar_archivo respuesta;
	char* ruta = concat(rutaArchivos, archivo);

	int rta = access(ruta, F_OK );
	if (rta != 0)
	{
		log_error(logFS, "No existe el archivo que se solicitó");
		respuesta.codigoRta = NO_EXISTE_ARCHIVO;
	}
	else
	{
		log_info(logFS, "Existe el archivo solicitado");
		respuesta.codigoRta = VALIDAR_OK;
	}

	char* buffer = serializar_respuesta_validar_archivo(&respuesta);
	empaquetarEnviarMensaje(socket, "RES_VALIDAR", sizeof(t_respuesta_validar_archivo), buffer);
	free(buffer);
	free(ruta);
}
void marcarBloqueOcupado(char* bloque)
{
	char*rutaBloque=string_new();
	string_append(&rutaBloque,rutaBloques);
	string_append(&rutaBloque,bloque);
	string_append(&rutaBloque,".bin");

	fopen(rutaBloque,"w");
	free(rutaBloque);

	log_trace(logFS, "Se marca el bit %s como ocupado", bloque);
	bitarray_set_bit(bitmap, atoi(bloque));
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);

}
void marcarBloqueDesocupado(char* bloque)
{
	log_trace(logFS, "Se marca el bit %s como desocupado", bloque);
	bitarray_clean_bit(bitmap, atoi(bloque));
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);
}
int obtenerBloqueVacio()
{
	int i = 0;
	while(i<=metadataFS->cantidadBloques && bitarray_test_bit(bitmap, i))
	{
		i++;
	};
	if (i <= metadataFS->cantidadBloques)
		return i;
	else
		return -1;
}
void crearArchivo(char* archivo, int socket)
{
	log_info(logFS, "Se intentará crear el archivo %s", archivo);
	int bloqueVacio = obtenerBloqueVacio();
	t_respuesta_crear_archivo* rta = malloc(sizeof(t_respuesta_crear_archivo));
	if (bloqueVacio >= 0)
	{
		t_metadata_archivo* nuevoArchivo = malloc(sizeof(t_metadata_archivo));
		nuevoArchivo->bloques = malloc(sizeof(int));
		nuevoArchivo->bloques[0] = string_itoa(bloqueVacio);
		nuevoArchivo->tamanio = 0;

		char* ruta = concat(rutaArchivos, archivo);

		bool already_exist = fopen("r",ruta)!=NULL;

		char* tmp = strdup(ruta);
		char* dir = dirname(tmp);
		_mkdir(dir);


		if (!already_exist)
		{
			fopen(ruta,"w");
			t_config* file =config_create(ruta);

			char* tamanio_string = string_itoa(nuevoArchivo->tamanio);
			config_set_value(file,"TAMANIO",tamanio_string);
			free(tamanio_string);

			char* string_bloques = string_get_string_from_array(nuevoArchivo->bloques,1);
			config_set_value(file,"BLOQUES",string_bloques);
			free(string_bloques);

			config_save(file);
			config_destroy(file);

			log_info(logFS, "El archivo se creó exitosamente");
			marcarBloqueOcupado(nuevoArchivo->bloques[0]);
			rta->codigoRta = CREAR_OK;
		}
		else
		{
			log_error(logFS, "Error al crear el archivo");
			if(already_exist)
				log_error(logFS, "El archivo ya existe");
			rta->codigoRta = CREAR_ERROR;
		}
		free(nuevoArchivo->bloques);
		free(nuevoArchivo);
		free(ruta);
	}
	else
	{
		log_error(logFS, "No hay bloques libres disponibles");
		rta->codigoRta = NO_HAY_BLOQUES;
	}
	char* buffer = serializar_respuesta_crear_archivo(rta);
	empaquetarEnviarMensaje(socket, "RES_CREAR_ARCH", sizeof(rta), buffer);
	free(buffer);
	free(rta);
}
void borrarArchivo(char* archivo, int socket)
{
	log_info(logFS, "Se intentará borrar el archivo %s", archivo);
	t_respuesta_borrar_archivo * rta = malloc(sizeof(t_respuesta_borrar_archivo));
	char* ruta = concat(rutaArchivos, archivo);
	t_config* file = config_create(ruta);
	if (file != NULL)
	{
		t_metadata_archivo archivoABorrar;
		archivoABorrar.bloques=config_get_array_value(file,"BLOQUES");
		archivoABorrar.tamanio=config_get_int_value(file,"TAMANIO");
		int cantBloques = sizeArray(archivoABorrar.bloques);

		int i;
		for (i = 0; i < cantBloques; ++i) {
			marcarBloqueDesocupado(archivoABorrar.bloques[i]);
		}
		remove(ruta);
		rta->codigoRta = BORRAR_OK;

		config_destroy(file);
	}
	else
	{
		log_error(logFS, "No se pudo borrar el archivo");
		rta->codigoRta = BORRAR_ERROR;
	}
	char* buffer = serializar_respuesta_borrar_archivo(rta);
	empaquetarEnviarMensaje(socket, "RES_BORRAR_ARCH", sizeof(rta), buffer);
	free(buffer);
	free(rta);
}
char* leerBloque(char* numero, int tamanio, int offset)
{
	char* nombreBloque = concat(numero, ".bin");
	char* ruta = concat(rutaBloques, nombreBloque);
	char* bloque = malloc(tamanio);
	FILE* file = fopen(ruta, "r");
	if (file != NULL)
	{
		fseek(file, offset, SEEK_SET);
		fread(bloque, tamanio, 1, file);
	}
	else
	{
		log_error(logFS, "No se pudo leer el bloque numero: %d", numero);
	}
	free(nombreBloque);
	free(ruta);
	fclose(file);

	return bloque;
}
void leerDatosArchivo(char* datos, int socket)
{
	t_pedido_lectura_datos* pedidoDeLectura = deserializar_pedido_lectura_datos(datos);
	t_respuesta_pedido_lectura rta;

	char* buffer = malloc(pedidoDeLectura->tamanio);

	char* ruta = concat(rutaArchivos, pedidoDeLectura->ruta);
	FILE* file = fopen(ruta, "r");
	if (file != NULL){
		t_config* metadata_file = config_create(ruta);

		t_metadata_archivo archivoALeer;
		archivoALeer.bloques=config_get_array_value(metadata_file,"BLOQUES");
		archivoALeer.tamanio=config_get_int_value(metadata_file,"TAMANIO");
		int cantBloques = sizeArray(archivoALeer.bloques);

		if((pedidoDeLectura->offset < archivoALeer.tamanio) && (pedidoDeLectura->offset+pedidoDeLectura->tamanio)<=archivoALeer.tamanio ){

			int nroBloque;
			int offset = 0;
			int tamanioAleer = pedidoDeLectura->tamanio;
			int offsetBloque=pedidoDeLectura->offset%metadataFS->tamanioBloque;
			int startBlock=((pedidoDeLectura->offset)/metadataFS->tamanioBloque);

			for (nroBloque = startBlock; nroBloque < cantBloques; nroBloque++) {
				 int tamanio = ((offsetBloque + tamanioAleer)>metadataFS->tamanioBloque)?(metadataFS->tamanioBloque-offsetBloque):tamanioAleer;

				 char* bloque = leerBloque(archivoALeer.bloques[nroBloque], tamanio, offsetBloque);

				 memcpy(buffer+offset, bloque, tamanio);
				 offset+=tamanio;
				 tamanioAleer-=tamanio;

				 free(bloque);

				 offsetBloque=0;//a partir del segundo lee desde el comienzo

				 if(tamanioAleer==0)
					 break;
			}

			rta.datos=buffer;
			rta.tamanio=pedidoDeLectura->tamanio;
			rta.codigo=LECTURA_OK;

		}else{
			rta.tamanio=5;
			rta.datos="ERROR";
			rta.codigo=LECTURA_ERROR;
		}

		config_destroy(metadata_file);
	}else{
		rta.tamanio=5;
		rta.datos="ERROR";
		rta.codigo=LECTURA_ERROR;
	}

	char* respuestaBuffer = serializar_respuesta_pedido_lectura(&rta);
	empaquetarEnviarMensaje(socket, "RES_LEER_DATOS", (sizeof(int32_t)+sizeof(codigo_respuesta_pedido_lectura)+rta.tamanio), respuestaBuffer);
	free(buffer);

	free(pedidoDeLectura);
	free(ruta);
}
void leerArchivoMetadataFS()
{
	metadataFS = malloc(sizeof(t_metadata_fs));
	char* ruta = concat(rutaMetadata, "Metadata.bin");
	t_config* metadata = config_create(ruta);

	metadataFS->tamanioBloque=config_get_int_value(metadata,"TAMANIO_BLOQUES");
	metadataFS->magicNumber=string_new();
	string_append(&metadataFS->magicNumber,config_get_string_value(metadata,"MAGIC_NUMBER"));
	metadataFS->cantidadBloques=config_get_int_value(metadata,"CANTIDAD_BLOQUES");

	config_destroy(metadata);
	free(ruta);

}
void validarDirectorio(char* ruta)
{
	DIR* dir = opendir(ruta);
	if (dir)
	{
		closedir(dir);
	}
	else if (ENOENT == errno)
	{
		mkdir(ruta, S_IRWXU);
	}
}
void cargarConfiguracionAdicional()
{
	rutaBloques = concat(puntoMontaje, "Bloques/");
	rutaArchivos = concat(puntoMontaje, "Archivos/");
	rutaMetadata = concat(puntoMontaje, "Metadata/");
	rutaBitmap = concat(rutaMetadata, "Bitmap.bin");
	validarDirectorio(rutaBloques);
	validarDirectorio(rutaArchivos);
	validarDirectorio(rutaMetadata);
	leerArchivoMetadataFS();
}

void mapearBitmap()
{
	archivoBitmap = fopen(rutaBitmap, "r+b");
	int32_t fd = fileno(archivoBitmap);
	struct stat buf;
	fstat(fd,&buf);
	char* bitArrayMap = mmap(0, buf.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	bitmap = bitarray_create_with_mode(bitArrayMap, buf.st_size, MSB_FIRST);
}

void cerrarArchivosYLiberarMemoria()
{
	munmap(bitmap->bitarray, metadataFS->cantidadBloques);
	fclose(archivoBitmap);
	bitarray_destroy(bitmap);

	free(metadataFS->magicNumber);
	free(metadataFS);

	free(rutaBloques);
	free(rutaArchivos);
	free(rutaMetadata);
	free(rutaBitmap);
}

void crearBloqueDePrueba(char* numero)
{
	int tamanio = string_length("Esto es un bloque de prueba")+1;
	char* bloque = malloc(tamanio);
	strcpy(bloque, "Esto es un bloque de prueba");

	char* tmp = concat(numero, ".bin");
	char* nombre = concat(rutaBloques, tmp);
	FILE* file = fopen(nombre, "wb");
	if(file != NULL)
	{
		fwrite(bloque, tamanio, 1, file);
		fclose(file);
	}

	free(bloque);
	free(nombre);
	free(tmp);
}
void crearBloqueDePruebaLleno(char* numero)
{
	int tamanio = string_length("Esto es un bloque de prueba que va a estar lleno. ASDFGHJKLÑQW")+1;
	char* bloque = malloc(tamanio);
	bloque = strdup("Esto es un bloque de prueba que va a estar lleno. ASDFGHJKLÑQWE");

	char* tmp = concat(numero, ".bin");
	char* nombre = concat(rutaBloques, tmp);
	FILE* file = fopen(nombre, "w");
	if(file != NULL)
	{
		fwrite(bloque, tamanio, 1, file);
		fclose(file);
	}

	free(bloque);
	free(nombre);
	free(tmp);
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	logFS = log_create("fs.log", "FILE SYSTEM", 0, LOG_LEVEL_TRACE);

	cargarConfiguracionAdicional();
	mapearBitmap();

	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",puntoMontaje);
	printf("TAMANIO BITMAP: %d\n", bitmap->size);

	t_dictionary* diccionarioFunc= dictionary_create();
	t_dictionary* diccionarioHands= dictionary_create();

	dictionary_put(diccionarioHands,"HKEFS","HFSKE");
	dictionary_put(diccionarioFunc,"ERROR_FUNC",&error);
	dictionary_put(diccionarioFunc,"KEY_PRINT",&mostrarMensaje);
	dictionary_put(diccionarioFunc, "VALIDAR_ARCH", &validarArchivo);
	dictionary_put(diccionarioFunc, "CREAR_ARCH", &crearArchivo);
	dictionary_put(diccionarioFunc, "BORRAR_ARCH", &borrarArchivo);
	dictionary_put(diccionarioFunc, "LEER_ARCH", &leerDatosArchivo);

	int sock= crearHostMultiConexion(puerto);
	correrServidorMultiConexion(sock,NULL,NULL,NULL,diccionarioFunc,diccionarioHands);

	cerrarArchivosYLiberarMemoria();
	dictionary_destroy(diccionarioFunc);
	dictionary_destroy(diccionarioHands);
	config_destroy(configFile);
	log_destroy(logFS);

	return EXIT_SUCCESS;
}
