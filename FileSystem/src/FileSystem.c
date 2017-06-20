#include "FileSystem.h"

void error(char** args)
{
	printf("%s",args[0]);
}
void mostrarMensaje(char* mensajes,int socket)
{
 	printf("Mensaje recibido: %s \n",mensajes);
}

void printBitmap() {
	int i;
	for (i = 0; i < 5192; ++i) {
		printf("el bit %d tiene el valor %d\n\r", i, bitarray_test_bit(bitmap,i));
	}
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
}
void marcarBloqueOcupado(int bloque)
{
	log_trace(logFS, "Se marca el bit %d como ocupado", bloque);
	bitarray_set_bit(bitmap, bloque);
	printBitmap();
}
void marcarBloqueDesocupado(int bloque)
{
	log_trace(logFS, "Se marca el bit %d como desocupado", bloque);
	bitarray_clean_bit(bitmap, bloque);
	printBitmap();
}
int obtenerBloqueVacio()
{
	int i = 0;
	while(bitarray_test_bit(bitmap, i))
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
		nuevoArchivo->bloques[0] = bloqueVacio;
		nuevoArchivo->tamanio = metadataFS->tamanioBloque;

		char* ruta = concat(rutaArchivos, archivo);

		char* tmp = strdup(ruta);
		char* dir = dirname(tmp);
		_mkdir(dir);

		FILE* file = fopen(ruta, "wb");
		if (file != NULL)
		{
			fwrite(&nuevoArchivo->tamanio, sizeof(int32_t), 1, file);
			fseek(file, sizeof(int32_t), SEEK_SET);
			fwrite(&nuevoArchivo->bloques, sizeof(int), 1, file);
			fclose(file);
			log_info(logFS, "El archivo se creó exitosamente");
			marcarBloqueOcupado(bloqueVacio);
			rta->codigoRta = CREAR_OK;
		}
		else
		{
			log_error(logFS, "Error al crear el archivo");
			rta->codigoRta = CREAR_ERROR;
		}

		free(nuevoArchivo);
	}
	else
	{
		log_error(logFS, "No hay bloques libres disponibles");
		rta->codigoRta = NO_HAY_BLOQUES;
	}
	char* buffer = serializar_respuesta_crear_archivo(&rta);
	empaquetarEnviarMensaje(socket, "RES_CREAR_ARCH", sizeof(rta), buffer);
	free(buffer);
	free(rta);
}
void borrarArchivo(char* archivo, int socket)
{
	log_info(logFS, "Se intentará borrar el archivo %s", archivo);
	t_respuesta_borrar_archivo * rta = malloc(sizeof(t_respuesta_borrar_archivo));
	char* ruta = concat(rutaArchivos, archivo);
	FILE* file = fopen(ruta, "rb");
	if (file != NULL)
	{
		t_metadata_archivo* archivoABorrar;
		fread(&archivoABorrar->tamanio, sizeof(int32_t), 1, file);
		int cantBloques = archivoABorrar->tamanio / metadataFS->tamanioBloque;
		fseek(file, sizeof(int32_t), SEEK_SET);
		fread(&archivoABorrar->bloques, cantBloques, 1, file);

		int i;
		for (i = 0; i < (cantBloques-1); ++i) {
			marcarBloqueDesocupado(archivoABorrar->bloques[i]);
		}
		fclose(file);
		remove(ruta);
		free(archivoABorrar);
		rta->codigoRta = BORRAR_OK;
	}
	else
	{
		log_error(logFS, "No se pudo borrar el archivo");
		rta->codigoRta = BORRAR_ERROR;
	}
	char* buffer = serializar_respuesta_borrar_archivo(&rta);
	empaquetarEnviarMensaje(socket, "RES_BORRAR_ARCH", sizeof(rta), buffer);
	free(buffer);
	free(rta);
}
t_bloque* leerBloque(int numero)
{
	char* nombreBloque = concat((char*)numero, ".bin");
	char* ruta = concat(rutaBloques, nombreBloque);
	t_bloque* bloque = malloc(sizeof(t_bloque));
	FILE* file = fopen(ruta, "rb");
	if (file != NULL)
	{
		fread(&(bloque->tamanio), sizeof(int32_t), 1, file);
		bloque->datos = malloc(bloque->tamanio);
		fseek(file, bloque->tamanio, SEEK_SET);
		fread(&(bloque->datos), bloque->tamanio, 1, file);
	}
	else
	{
		log_error(logFS, "No se pudo leer el bloque numero: %d", numero);
	}
	fclose(file);
	return bloque;
}
void leerDatosArchivo(t_pedido_lectura_datos* pedidoDeLectura, int socket)
{
	//t_pedido_lectura_datos* pedidoDeLectura = deserializar_pedido_lectura_datos(datos);
	t_respuesta_pedido_lectura* rta = malloc(sizeof(t_respuesta_pedido_lectura));
	char* buffer;
	int offset = 0;

	char* ruta = concat(rutaArchivos, pedidoDeLectura->ruta);
	FILE* file = fopen(ruta, "rb");
	if (file != NULL)
	{
		t_metadata_archivo* archivoALeer;
		fread(&(archivoALeer->tamanio), sizeof(int32_t), 1, file);
		int cantBloques = archivoALeer->tamanio / metadataFS->tamanioBloque;
		fseek(file, sizeof(int32_t), SEEK_SET);
		fread(&(archivoALeer->bloques), cantBloques*sizeof(int), 1, file);

		int i;
		int offset = 0;
		for (i = 0; i < cantBloques; ++i) {
			 t_bloque* bloque = leerBloque(archivoALeer->bloques[i]);
			 memcpy(buffer+offset, &(bloque->datos), bloque->tamanio);
			 offset+=bloque->tamanio;
		}
	}
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
	char* bitmapArray = malloc(metadataFS->cantidadBloques);
	mmap(bitmapArray, metadataFS->cantidadBloques*sizeof(int), PROT_WRITE, MAP_SHARED, fileno(archivoBitmap), 0);
	bitmap = bitarray_create_with_mode(bitmapArray, metadataFS->cantidadBloques, MSB_FIRST);
	free(bitmapArray);

	printBitmap();
}

void cerrarArchivosYLiberarMemoria()
{
	munmap(bitmap->bitarray, metadataFS->cantidadBloques*sizeof(int));
	fclose(archivoBitmap);
	bitarray_destroy(bitmap);
}

void crearBloque(int numero)
{
	t_bloque* bloque = malloc(sizeof(t_bloque));
	bloque->datos = malloc(strlen("Esto es un bloque de prueba")+1);
	bloque->datos = "Esto es un bloque de prueba\0";
	bloque->tamanio = strlen(bloque->datos)+1;

	char* numeroBloque = string_itoa(numero);
	char* tmp = concat(numeroBloque, ".bin");
	free(numeroBloque);
	char* nombre = concat(rutaBloques, tmp);
	FILE* file = fopen(nombre, "wb");

	fwrite(&(bloque->tamanio), sizeof(int32_t), 1, file);
	fseek(file, sizeof(int32_t), SEEK_SET);
	fwrite(bloque->datos, bloque->tamanio, 1, file);
	fclose(file);
	free(bloque);
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",puntoMontaje);
	logFS = log_create("fs.log", "FILE SYSTEM", 0, LOG_LEVEL_TRACE);

	cargarConfiguracionAdicional();
	mapearBitmap();

	//crearArchivo("asd2.bin", 1);
	t_pedido_lectura_datos* asd = malloc(sizeof(t_pedido_lectura_datos));
	asd->ruta = malloc(strlen("prueba.bin")+1);
	asd->offset = 0;
	asd->ruta = "prueba.bin";
	asd->tamanio = 0;
	crearBloque(2);
	leerDatosArchivo(asd,2);

	printf("TAMANIO BITMAP: %d\n", bitmap->size);

	cerrarArchivosYLiberarMemoria();

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

	dictionary_destroy(diccionarioFunc);
	dictionary_destroy(diccionarioHands);
	config_destroy(configFile);

	return EXIT_SUCCESS;
}
