#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

int main(int argc, char *argv[]) {
    // verfiicamos que se haya pasado un archivo como argumento
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nombre_del_archivo>\n", argv[0]);
        exit(1);
    }

    printf("Publicador: Abriendo el archivo de imagen\n");
    char *nombreArchivo = argv[1];
    key_t clave = ftok("shmfile", 65);  // genermoa una clave única para la memoria compartida
    FILE *archivo = fopen(nombreArchivo, "rb"); 
    if (archivo == NULL) {
        perror("No se pudo abrir el archivo");
        exit(1);
    }

    // Leer la imagen BMP desde el archivo
    BMP_Image* imagenTemporal = readImage(archivo);
    if (imagenTemporal == NULL) {
        fclose(archivo);
        exit(1);
    }
    fclose(archivo);  // Cerrar el archivo para que no quede abierto

    printf("Publicador: Reservando espacio en memoria compartida\n");
    int tamañoImagen = imagenTemporal->header.size;  // Obtener el tamaño de la imagen
    int idMemoriaCompartida = shmget(clave, tamañoImagen, 0666 | IPC_CREAT);  // Crear la memoria compartida
    if (idMemoriaCompartida < 0) {
        perror("Error al crear el segmento de memoria compartida");
        freeImage(imagenTemporal);
        exit(1);
    }

    // Adjuntar el segmento de memoria compartida al espacio de direcciones
    BMP_Image* direccionMemoriaCompartida = (BMP_Image*)shmat(idMemoriaCompartida, (void*)0, 0);
    if (direccionMemoriaCompartida == (BMP_Image*)(-1)) {
        perror("No se pudo adjuntar el segmento de memoria compartida");
        freeImage(imagenTemporal);
        exit(1);
    }

    printf("Publicador: Publicando la imagen en memoria compartida\n");
    memcpy(direccionMemoriaCompartida, imagenTemporal, tamañoImagen);  // Copiar la imagen a la memoria compartida
    freeImage(imagenTemporal);  // Liberar la memoria de la imagen temporal

    printf("Publicador: Imagen publicada exitosamente en memoria compartida\n");
    return 0;
}
