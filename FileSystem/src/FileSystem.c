#include "FileSystem.h"

void error(char* data,int socket){
	//Ignore
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
void validarArchivo(char* data, int socket){

	t_pedido_validar_crear_borrar_archivo_fs* pedido = deserializar_pedido_validar_crear_borrar_archivo(data);

	log_info(logFS, "Se validará la existencia del archivo %s", pedido->direccion);
	t_respuesta_validar_archivo respuesta;
	char* ruta = concat(rutaArchivos, pedido->direccion+1);

	int rta = access(ruta, F_OK );
	if (rta != 0){
		log_error(logFS, "No existe el archivo que se solicitó");
		respuesta.codigoRta = NO_EXISTE_ARCHIVO;
	}else{
		log_info(logFS, "Existe el archivo solicitado");
		respuesta.codigoRta = VALIDAR_OK;
	}

	char* buffer = serializar_respuesta_validar_archivo(&respuesta);
	empaquetarEnviarMensaje(socket, "RES_VALIDAR", sizeof(t_respuesta_validar_archivo), buffer);
	free(buffer);
	free(ruta);
	free(pedido->direccion);
	free(pedido);
}
void marcarBloqueOcupado(char* bloque){
	char*rutaBloque=string_new();
	string_append(&rutaBloque,rutaBloques);
	string_append(&rutaBloque,bloque);
	string_append(&rutaBloque,".bin");

	FILE* file = fopen(rutaBloque,"w");
	fclose(file);
	free(rutaBloque);

	log_trace(logFS, "Se marca el bit %s como ocupado", bloque);
	bitarray_set_bit(bitmap, atoi(bloque));
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);
}
void marcarBloqueDesocupado(char* bloque){
	log_trace(logFS, "Se marca el bit %s como desocupado", bloque);
	bitarray_clean_bit(bitmap, atoi(bloque));
	msync(bitmap->bitarray,bitmap->size,MS_SYNC);
}
int obtenerBloqueVacio(){
	int i = 0;
	while(i<=metadataFS->cantidadBloques && bitarray_test_bit(bitmap, i)){
		i++;
	}
	if (i <= metadataFS->cantidadBloques)
		return i;
	else
		return -1;
}
void crearArchivo(char* data, int socket){

	t_pedido_validar_crear_borrar_archivo_fs* pedido = deserializar_pedido_validar_crear_borrar_archivo(data);

	log_info(logFS, "Se intentara crear el archivo %s", pedido->direccion);
	int bloqueVacio = obtenerBloqueVacio();
	t_respuesta_crear_archivo* rta = malloc(sizeof(t_respuesta_crear_archivo));

	if (bloqueVacio >= 0){
		t_metadata_archivo* nuevoArchivo = malloc(sizeof(t_metadata_archivo));
		nuevoArchivo->bloques = malloc(sizeof(int));
		nuevoArchivo->bloques[0] = string_itoa(bloqueVacio);
		nuevoArchivo->tamanio = 0;

		char* ruta = concat(rutaArchivos, pedido->direccion+1);

		bool already_exist = (fopen(ruta,"r")!=NULL);

		char* tmp = strdup(ruta);
		char* dir = dirname(tmp);
		_mkdir(dir);

		if (!already_exist){
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

			log_info(logFS, "El archivo se creo exitosamente");
			marcarBloqueOcupado(nuevoArchivo->bloques[0]);
			rta->codigoRta = CREAR_OK;
		}else{
			log_error(logFS, "Error al crear el archivo");
			if(already_exist)
				log_error(logFS, "El archivo ya existe");
			rta->codigoRta = CREAR_ERROR;
		}
		free(nuevoArchivo->bloques);
		free(nuevoArchivo);
		free(ruta);
	}else{
		log_error(logFS, "No hay bloques libres disponibles");
		rta->codigoRta = NO_HAY_BLOQUES;
	}

	char* buffer = serializar_respuesta_crear_archivo(rta);
	empaquetarEnviarMensaje(socket, "RES_CREAR_ARCH", sizeof(rta), buffer);

	free(buffer);
	free(rta);
	free(pedido->direccion);
	free(pedido);
}
void borrarArchivo(char* data, int socket){
	t_pedido_validar_crear_borrar_archivo_fs* pedido = deserializar_pedido_validar_crear_borrar_archivo(data);

	log_info(logFS, "Se intentara borrar el archivo %s", pedido->direccion);
	t_respuesta_borrar_archivo * rta = malloc(sizeof(t_respuesta_borrar_archivo));
	char* ruta = concat(rutaArchivos, pedido->direccion+1);

	t_config* file = config_create(ruta);
	if (file != NULL){
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
	}else{
		log_error(logFS, "No se pudo borrar el archivo");
		rta->codigoRta = BORRAR_ERROR;
	}
	char* buffer = serializar_respuesta_borrar_archivo(rta);
	empaquetarEnviarMensaje(socket, "RES_BORRAR_ARCH", sizeof(rta), buffer);
	free(buffer);
	free(rta);
	free(pedido->direccion);
	free(pedido);
}
char* leerBloque(char* numero, int tamanio, int offset){
	char* nombreBloque = concat(numero, ".bin");
	char* ruta = concat(rutaBloques, nombreBloque);
	char* bloque = malloc(tamanio);
	FILE* file = fopen(ruta, "rb");
	if (file != NULL){
		fseek(file, offset, SEEK_SET);
		fread(bloque, tamanio, 1, file);
	}else{
		log_error(logFS, "No se pudo leer el bloque numero: %d", numero);
	}
	free(nombreBloque);
	free(ruta);
	fclose(file);

	return bloque;
}
void leerDatosArchivo(char* datos, int socket){
	t_pedido_lectura_datos* pedidoDeLectura = deserializar_pedido_lectura_datos(datos);
	t_respuesta_pedido_lectura rta;

	log_info(logFS,"Se intentara leer en el archivo %s con offset %d y size %d",pedidoDeLectura->ruta,pedidoDeLectura->offset,pedidoDeLectura->tamanio);

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

			log_info(logFS,"Lectura correcta del archivo");

		}else{
			log_info(logFS,"El offset %d es más grande que el archivo de tamanio %d",pedidoDeLectura->offset,archivoALeer.tamanio);
			rta.tamanio=5;
			rta.datos="ERROR";
			rta.codigo=LECTURA_ERROR;
		}

		config_destroy(metadata_file);
	}else{
		log_info(logFS,"El archivo %s no existe",pedidoDeLectura->ruta);
		rta.tamanio=5;
		rta.datos="ERROR";
		rta.codigo=LECTURA_ERROR;
	}

	char* respuestaBuffer = serializar_respuesta_pedido_lectura(&rta);
	empaquetarEnviarMensaje(socket, "RES_LEER_DATOS", (sizeof(int32_t)+sizeof(codigo_respuesta_pedido_lectura)+rta.tamanio), respuestaBuffer);
	free(respuestaBuffer);
	free(buffer);

	free(pedidoDeLectura);
	free(ruta);
}
void escribirBloque(char* bloque, char* buffer, int tamanio, int offset){
	char* nombre = concat(bloque, ".bin");
	char* ruta = concat(rutaBloques, nombre);
	FILE* file = fopen(ruta, "wb");
	if (file != NULL){
		fseek(file, offset, SEEK_SET);
		fwrite(buffer, tamanio, 1, file);
		fflush(file);
	}
}
void eliminarBloque(char* bloque){
	char* nombreBloque = concat(bloque, ".bin");
	char* ruta = concat(rutaBloques, nombreBloque);

	remove(ruta);
	free(nombreBloque);
	free(ruta);
}
bool hayXBloquesLibres(int cantidad){

	int libres = 0;
	while(libres<=metadataFS->cantidadBloques && !bitarray_test_bit(bitmap, libres)){
		libres++;
		if(libres>=cantidad)
			break;
	}

	return libres>=cantidad;
}
void escribirDatosArchivo(char* datos, int socket){
	t_pedido_escritura_datos* pedidoEscritura = deserializar_pedido_escritura_datos(datos);
	t_respuesta_pedido_escritura rta;
	t_metadata_archivo archivoAEscribir;

	log_info(logFS,"Se intentara escribir en el archivo %s con offset %d y size %d",pedidoEscritura->ruta,pedidoEscritura->offset,pedidoEscritura->tamanio);

	char* ruta = concat(rutaArchivos, pedidoEscritura->ruta);
	FILE* file = fopen(ruta, "r");
	if (file != NULL){
		t_config* metadata_file = config_create(ruta);

		archivoAEscribir.bloques=config_get_array_value(metadata_file,"BLOQUES");
		archivoAEscribir.tamanio=config_get_int_value(metadata_file,"TAMANIO");

		int cantBloques = sizeArray(archivoAEscribir.bloques);

		int nroBloque;
		int offset = 0;
		int tamanioAEscribir = pedidoEscritura->tamanio;
		int offsetBloque=pedidoEscritura->offset%metadataFS->tamanioBloque;
		int startBlockIndex=((pedidoEscritura->offset)/metadataFS->tamanioBloque);

		if(pedidoEscritura->offset!=0 && offsetBloque==0){
			startBlockIndex++;//Esta en un extremo de un bloque
		}

		int bloquesNewSize = ((pedidoEscritura->offset+tamanioAEscribir)/metadataFS->tamanioBloque)+1;
		int totalBloques = bloquesNewSize>cantBloques?bloquesNewSize:cantBloques;

		int cantNuevosBloques = totalBloques - cantBloques;
		archivoAEscribir.bloques = realloc(archivoAEscribir.bloques, sizeof(char*) * totalBloques);
		if(hayXBloquesLibres(cantNuevosBloques)){
			for (nroBloque = startBlockIndex; nroBloque < totalBloques; nroBloque++){
				if(nroBloque > (cantBloques-1)){
					int bloqueNuevo = obtenerBloqueVacio();
					archivoAEscribir.bloques[nroBloque] = string_itoa(bloqueNuevo);
					marcarBloqueOcupado(archivoAEscribir.bloques[nroBloque]);
				}
				int tamanio = ((offsetBloque + tamanioAEscribir)>metadataFS->tamanioBloque)?(metadataFS->tamanioBloque-offsetBloque):tamanioAEscribir;
				char* buffer = malloc(tamanio);
				memcpy(buffer, (void*)pedidoEscritura->buffer+offset, tamanio);
				escribirBloque(archivoAEscribir.bloques[nroBloque], buffer, tamanio, offsetBloque);
				free(buffer);

				offset+=tamanio;
				tamanioAEscribir-=tamanio;

				offsetBloque = 0;

				if(tamanioAEscribir==0)
					break;
			}
			int nuevoTamanio = pedidoEscritura->offset + pedidoEscritura->tamanio;
			int tamanioGuardar = (nuevoTamanio > archivoAEscribir.tamanio)?nuevoTamanio:archivoAEscribir.tamanio;

			char* tamanioAGuardar = string_itoa(tamanioGuardar);
			config_set_value(metadata_file, "TAMANIO", tamanioAGuardar);
			free(tamanioAGuardar);

			char* array = string_get_string_from_array(archivoAEscribir.bloques, totalBloques);
			config_set_value(metadata_file, "BLOQUES", array);
			free(array);

			config_save(metadata_file);
			config_destroy(metadata_file);

			rta.codigoRta= ESCRIBIR_OK;
			log_info(logFS,"Se completó la escritura");
		}else{
			log_info(logFS,"No hay espacio en el FS");
			rta.codigoRta = NO_HAY_ESPACIO;
		}
	}else{
		log_info(logFS,"El archivo %s no existe",pedidoEscritura->ruta);
		rta.codigoRta = ESCRIBIR_ERROR;
	}
	fclose(file);
	free(ruta);

	free(pedidoEscritura->ruta);
	free(pedidoEscritura->buffer);
	free(pedidoEscritura);

	char* respuestaBuffer = serializar_respuesta_pedido_escritura(&rta);
	empaquetarEnviarMensaje(socket, "RES_ESCR_DATOS", sizeof(t_respuesta_pedido_escritura), respuestaBuffer);
	free(respuestaBuffer);
}
void leerArchivoMetadataFS(){
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
void validarDirectorio(char* ruta){
	DIR* dir = opendir(ruta);
	if (dir){
		closedir(dir);
	}else if (ENOENT == errno){
		mkdir(ruta, S_IRWXU);
	}
}
void cargarConfiguracionAdicional(){
	rutaBloques = concat(puntoMontaje, "Bloques/");
	rutaArchivos = concat(puntoMontaje, "Archivos/");
	rutaMetadata = concat(puntoMontaje, "Metadata/");
	rutaBitmap = concat(rutaMetadata, "Bitmap.bin");
	validarDirectorio(rutaBloques);
	validarDirectorio(rutaArchivos);
	validarDirectorio(rutaMetadata);
	leerArchivoMetadataFS();
}

void mapearBitmap(){
	archivoBitmap = fopen(rutaBitmap, "r+b");
	int32_t fd = fileno(archivoBitmap);
	struct stat buf;
	fstat(fd,&buf);
	char* bitArrayMap = mmap(0, buf.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	bitmap = bitarray_create_with_mode(bitArrayMap, buf.st_size, MSB_FIRST);
}

void cerrarArchivosYLiberarMemoria(){
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

int main(int argc, char** argv){
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
	dictionary_put(diccionarioFunc, "VALIDAR_ARCH", &validarArchivo);
	dictionary_put(diccionarioFunc, "CREAR_ARCH", &crearArchivo);
	dictionary_put(diccionarioFunc, "BORRAR_ARCH", &borrarArchivo);
	dictionary_put(diccionarioFunc, "LEER_ARCH", &leerDatosArchivo);
	dictionary_put(diccionarioFunc, "ESCRIBIR_ARCH", &escribirDatosArchivo);

	int sock= crearHostMultiConexion(puerto);
	correrServidorMultiConexion(sock,NULL,NULL,NULL,diccionarioFunc,diccionarioHands);

	cerrarArchivosYLiberarMemoria();
	dictionary_destroy(diccionarioFunc);
	dictionary_destroy(diccionarioHands);
	config_destroy(configFile);
	log_destroy(logFS);

	return EXIT_SUCCESS;
}
