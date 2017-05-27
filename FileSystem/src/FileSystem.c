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
		punto_montaje= config_get_string_value(configFile,"PUNTO_MONTAJE");
	}else {
		perror("La key PUNTO_MONTAJE no existe");
		exit(EXIT_FAILURE);
	}
	return configFile;
}
int validarArchivo(char* ruta, int socket)
{
	return access(ruta, F_OK );
}
void crearArchivo(char* nombre, int socket)
{
	int bloqueVacio = obtenerBloqueVacio();
	t_metadata_archivo nuevoArchivo;
	nuevoArchivo.bloques = malloc(sizeof(int)*10);
	nuevoArchivo.bloques[0] = bloqueVacio;
	nuevoArchivo.tamanio = metadataFS.tamanioBloque;

	char* ruta = concat(rutaArchivos, nombre);

	FILE* archivo = fopen(ruta, "wb");
	fwrite(&nuevoArchivo, sizeof(t_metadata_archivo),1, archivo);
	fclose(archivo);
	free(nuevoArchivo.bloques);
	marcarBloqueOcupado(bloqueVacio);
}
void borrarArchivo(char* nombre, int socket)
{
	char* ruta = concat(rutaArchivos, nombre);
	FILE* archivo = fopen(ruta, "rb");
	t_metadata_archivo archivoABorrar;
	fread(&archivoABorrar, sizeof(t_metadata_archivo), 1, archivo);
	int i = 0;
	while(archivoABorrar.bloques[i]>= 0 && archivoABorrar.bloques[i] != NULL)
	{
		marcarBloqueDesocupado(archivoABorrar.bloques[i]);
		i++;
	}
	remove(ruta);
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
	size_t size;
	t_bitarray* bitmap = bitarray_create_with_mode(bitarray, size, LSB_FIRST);
	guardarArchivoBitmap(&bitmap);
}
void leerArchivoMetadataFS()
{
	char* ruta = concat(punto_montaje, "Metadata/Metadata.bin");
	FILE* archivo = fopen(ruta, "rb");
	fread(&metadataFS, sizeof(t_metadata_fs), 1, archivo);
	fclose(archivo);
}
void cargarConfiguracionAdicional()
{
	rutaBloques = concat(punto_montaje, "Bloques/");
	rutaArchivos = concat(punto_montaje, "Archivos/");
	rutaBitmap = concat(punto_montaje, "Metadata/Bitmap.bin");
	leerArchivoMetadataFS();
}
int obtenerBloqueVacio()
{
	t_bitarray bitmap;
	bitmap.bitarray = malloc(sizeof(int)* metadataFS.cantidadBloques);
	leerArchivoBitmap(&bitmap);
	int i = 0;
	while(bitarray_test_bit(&bitmap, i))
	{
		i++;
	};
	free(bitmap.bitarray);
	return i;
}
void marcarBloqueOcupado(int bloque)
{
	t_bitarray bitmap;
	bitmap.bitarray = malloc(sizeof(int)* metadataFS.cantidadBloques);
	leerArchivoBitmap(&bitmap);
	bitarray_set_bit(&bitmap, bloque);
	guardarArchivoBitmap(&bitmap);
	free(bitmap.bitarray);
}
void marcarBloqueDesocupado(int bloque)
{
	t_bitarray bitmap;
	bitmap.bitarray = malloc(sizeof(int)* metadataFS.cantidadBloques);
	leerArchivoBitmap(&bitmap);
	bitarray_clean_bit(&bitmap, bloque);
	guardarArchivoBitmap(&bitmap);
	free(bitmap.bitarray);
}
void leerArchivoBitmap(t_bitarray bitmap)
{
	FILE* archivoBitmap = fopen(rutaBitmap, "rb");
	fread(&bitmap, sizeof(t_bitarray), 1, archivoBitmap);
	fclose(archivoBitmap);
}
void guardarArchivoBitmap(t_bitarray bitmap)
{
	FILE* archivoBitmap = fopen(rutaBitmap, "wb");
	fwrite(&bitmap, sizeof(t_bitarray), 1, archivoBitmap);
	fclose(archivoBitmap);
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",punto_montaje);

	cargarConfiguracionAdicional();
	crearArchivo("prueba2.bin", 1);

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
