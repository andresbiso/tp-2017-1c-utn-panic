#include "FileSystem.h"

void error(char** args)
{
	printf("%s",args[0]);
}
void mostrarMensaje(char* mensajes,int socket)
{
 	printf("Mensaje recibido: %s \n",mensajes);
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
	bitarray_set_bit(bitmap, bloque);
}
void marcarBloqueDesocupado(int bloque)
{
	bitarray_clean_bit(bitmap, bloque);
}
int obtenerBloqueVacio()
{
	int i = 0;
	while(bitarray_test_bit(bitmap, i))
	{
		i++;
	};
	if (i <= metadataFS.cantidadBloques)
		return i;
	else
		return -1;
}
void crearArchivo(char* archivo, int socket)
{
	log_info(logFS, "Se intentará crear el archivo %s", archivo);
	int bloqueVacio = obtenerBloqueVacio();
	t_respuesta_crear_archivo* rta;
	if (bloqueVacio >= 0)
	{
		t_metadata_archivo* nuevoArchivo;
		nuevoArchivo->bloques = malloc(sizeof(int));
		nuevoArchivo->bloques[0] = bloqueVacio;
		nuevoArchivo->tamanio = metadataFS.tamanioBloque;

		char* ruta = concat(rutaArchivos, archivo);

		FILE* file = fopen(ruta, "wb");
		if (file != NULL)
		{
			fwrite(&nuevoArchivo, sizeof(t_metadata_archivo),1, file);
			fclose(file);
			log_info(logFS, "El archivo se creó exitosamente");
			free(nuevoArchivo->bloques);
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
	empaquetarEnviarMensaje(socket, "RES_CREAR_ARCH", sizeof(t_respuesta_crear_archivo), buffer);
	free(buffer);
}
void borrarArchivo(char* nombre, int socket)
{
	log_info(logFS, "Se intentará borrar el archivo %s", nombre);
	char* ruta = concat(rutaArchivos, nombre);
	FILE* archivo = fopen(ruta, "rb");
	if (archivo != NULL)
	{
		t_metadata_archivo* archivoABorrar;
		fread(&archivoABorrar, sizeof(t_metadata_archivo), 1, archivo);
		int i = 0;
		while(archivoABorrar->bloques[i]>= 0 && archivoABorrar->bloques[i] != NULL)
		{
			marcarBloqueDesocupado(archivoABorrar->bloques[i]);
			i++;
		}
		fclose(archivo);
		remove(ruta);
	}
	else
	{
		log_error(logFS, "No se pudo borrar el archivo");
	}
}
void leerBloque(int bloque)
{
	char* ruta = concat(rutaBloques, bloque);
	FILE* file = fopen(ruta, "rb");

	fclose(file);
}
t_metadata_archivo leerMetadataArchivo(char* nombre)
{
	t_metadata_archivo archivoLeido;
	char* ruta = concat(rutaArchivos, nombre);
	FILE* archivo = fopen(ruta, "rb");
	fread(&archivoLeido.tamanio, sizeof(int32_t), 1, archivo);
	fclose(archivo);
	free(archivoLeido.bloques);

	return archivoLeido;
}
void leerDatosArchivo(char* nombre, int socket)
{
	t_metadata_archivo metadataArchivo= leerMetadataArchivo(nombre);
	int i = 0;
	while (metadataArchivo.bloques[i] >= 0 && metadataArchivo.bloques[i] != NULL)
	{
		leerBloque(metadataArchivo.bloques[i]);
		i++;
	}
}
void leerArchivoMetadataFS()
{
	char* ruta = concat(puntoMontaje, "Metadata/Metadata.bin");
	FILE* archivo = fopen(ruta, "rb");
	fread(&metadataFS, sizeof(t_metadata_fs) + sizeof("SADICA"), 1, archivo);
	fclose(archivo);
}
void cargarConfiguracionAdicional()
{
	rutaBloques = concat(puntoMontaje, "Bloques/");
	rutaArchivos = concat(puntoMontaje, "Archivos/");
	rutaBitmap = concat(puntoMontaje, "Metadata/Bitmap.bin");
	leerArchivoMetadataFS();
}
void mapearBitmap()
{
	archivoBitmap = fopen(rutaBitmap, "rb");
	char* bitmapArray = malloc(metadataFS.cantidadBloques*sizeof(int));
	mmap(bitmapArray, metadataFS.cantidadBloques*sizeof(int), PROT_WRITE, MAP_SHARED, fileno(archivoBitmap), 0);
	bitmap = bitarray_create_with_mode(bitmapArray, metadataFS.cantidadBloques * sizeof(int), LSB_FIRST);
}
void cerrarArchivosYLiberarMemoria()
{
	munmap(bitmap->bitarray, metadataFS.cantidadBloques*sizeof(int));
	fclose(archivoBitmap);
	bitarray_destroy(bitmap);
}
void leerArchivoBitmap(t_bitarray bitmap)
{
	FILE* archivoBitmap = fopen(rutaBitmap, "rb");
	fread(&bitmap, sizeof(t_bitarray), 1, archivoBitmap);
	fclose(archivoBitmap);
}
void crearMetadataFS()
{
	t_metadata_fs asd;
	asd.cantidadBloques = 5192;
	asd.tamanioBloque = 64;
	asd.magicNumber = malloc(sizeof("SADICA"));
	asd.magicNumber = "SADICA";
	char* ruta = concat(puntoMontaje, "Metadata/Metadata.bin");
	FILE* file = fopen(ruta, "wb");
	fwrite(&asd, sizeof(t_metadata_fs) + sizeof("SADICA"), 1, file);
	fclose(file);
	free(asd.magicNumber);
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",puntoMontaje);
	logFS = log_create("fs.log", "FILE SYSTEM", 0, LOG_LEVEL_TRACE);

	crearMetadataFS();
	cargarConfiguracionAdicional();
	mapearBitmap();

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

	return EXIT_SUCCESS;
}
