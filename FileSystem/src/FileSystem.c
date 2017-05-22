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
void crearArchivo(char* ruta, int socket)
{
	FILE *fp;
	fp=fopen("test.bin", "wb");
	char hola[10]="ABCDEFGHIJ";
	fwrite(hola, sizeof(hola[0]), sizeof(hola)/sizeof(hola[0]), fp);

	//int bloqueVacio = obtenerBloqueVacio();
//	strcat(ruta, "asd.txt");
//	FILE * file = fopen(ruta,"a");
//	fprintf(file, "TAMANIO=");
//	fclose(file);

	//TODO: asignar 1 bloque de datos.
}
void borrarArchivo(char* ruta, int socket)
{
	remove(ruta);

	//TODO: marcar bloques libres dentro del bitmap.
}
void leerBloque(int bloque)
{
	char* ruta = strcat(rutaBloques, bloque);
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
void leerMetadataArchivo(char* ruta, t_metadata_archivo metadataArchivo)
{
	FILE* archivo = fopen(ruta, "rb");

	fclose(archivo);
}
void crearBitMap()
{
	char* bitarray;
	size_t size;
	t_bitarray* bitmap = bitarray_create_with_mode(bitarray, size, LSB_FIRST);
}
void leerArchivoMetadataFS()
{
	FILE* metadata = fopen("mnt/SADICA_FS/Metadata/Metadata.bin", "rb");



	fclose(metadata);
}
void cargarConfiguracionAdicional()
{
	rutaBloques= concat(punto_montaje, "bloques/");
	leerArchivoMetadataFS();
}
int main(int argc, char** argv)
{
	t_config* configFile = cargarConfiguracion(argv[1]);
	printf("PUERTO: %d\n",puerto);
	printf("PUNTO_MONTAJE: %s\n",punto_montaje);
	cargarConfiguracionAdicional();

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
