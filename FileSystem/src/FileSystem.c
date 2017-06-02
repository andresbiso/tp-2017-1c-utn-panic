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
		free(&nuevoArchivo);
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
void leerDatosArchivo(t_pedido_datos_fs datosFs, int socket)
{
	t_metadata_archivo metadataArchivo;
	leerMetadataArchivo(datosFs.ruta, &metadataArchivo);
	int i = 0;
	while (metadataArchivo.bloques[i] >= 0 && metadataArchivo.bloques[i] != NULL)
	{
		leerBloque(metadataArchivo.bloques[i]);
		i++;
	}
}
void leerMetadataArchivo(char* nombre)
{
	t_metadata_archivo archivoLeido;
	archivoLeido.bloques = malloc(sizeof(int)*10);
	char* ruta = concat(rutaArchivos, nombre);
	FILE* archivo = fopen(ruta, "rb");
	fread(&archivoLeido, sizeof(t_metadata_archivo), 1, archivo);
	fclose(archivo);
	free(archivoLeido.bloques);
}
void crearBitMap()
{
	char* bitarray;
	t_bitarray* bitmap2 = bitarray_create_with_mode(bitarray, metadataFS.cantidadBloques * sizeof(int), LSB_FIRST);
	guardarArchivoBitmap(&bitmap2);
	printf("TAMANIO BITMAP: %d\n", bitmap2->size);
	bitarray_destroy(bitmap2);
}
void leerArchivoMetadataFS()
{
	char* ruta = concat(puntoMontaje, "Metadata/Metadata.bin");
	FILE* archivo = fopen(ruta, "rb");
	//mmap(&metadataFS, sizeof(t_metadata_fs), PROT_READ, MAP_SHARED, archivo, 0);
	fread(&metadataFS, sizeof(t_metadata_fs), 1, archivo);
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
	mmap(&bitmap, sizeof(metadataFS.cantidadBloques*sizeof(int)), PROT_READ, MAP_SHARED, archivoBitmap, 0);
}
void cerrarArchivosYLiberarMemoria()
{
	munmap(&bitmap, sizeof(t_bitarray));
	fclose(archivoBitmap);
	free(bitmap->bitarray);
	bitarray_destroy(&bitmap);
}
int obtenerBloqueVacio()
{
	int i = 0;
	while(bitarray_test_bit(&bitmap, i))
	{
		i++;
	};
	if (i <= metadataFS.cantidadBloques)
		return i;
	else
		return -1;
}
void marcarBloqueOcupado(int bloque)
{
	bitarray_set_bit(&bitmap, bloque);
}
void marcarBloqueDesocupado(int bloque)
{
	bitarray_clean_bit(&bitmap, bloque);
}
void leerArchivoBitmap(t_bitarray bitmap)
{
	FILE* archivoBitmap = fopen(rutaBitmap, "rb");
	fread(&bitmap, sizeof(t_bitarray), 1, archivoBitmap);
	fclose(archivoBitmap);
}
void guardarArchivoBitmap(char* bitmap)
{
	FILE* archivoBitmap = fopen(rutaBitmap, "wb");
	fwrite(&bitmap, metadataFS.cantidadBloques*sizeof(int), 1, archivoBitmap);
	fclose(archivoBitmap);
}
void crearMetadataFS()
{
	t_metadata_fs metadata;
	metadata.cantidadBloques = 5192;
	metadata.magicNumber = "SADICA";
	metadata.tamanioBloque = 64;
	char* ruta = concat(puntoMontaje, "Metadata/Metadata.bin");
	FILE * archivo = fopen(ruta, "wb");
	fwrite(&metadata, sizeof(t_metadata_fs), 1, archivo);
	fclose(archivo);
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",puntoMontaje);
	logFS = log_create("fs.log", "FILE SYSTEM", 0, LOG_LEVEL_TRACE);

	cargarConfiguracionAdicional();
	crearBitMap();
	mapearBitmap();

	printf("TAMANIO BITMAP: %d\n", bitmap->size);

	//crearArchivo("prueba.bin", 1);

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
