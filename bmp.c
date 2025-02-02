#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

// Incluimos el archivo con las
// funciones para imprimir errores
#include "bmp.h"

void printError(int error) {
    switch (error) {
        case ARGUMENT_ERROR:
            printf("Argument Error\n");
            break;
        case FILE_ERROR:
            printf("File Error\n");
            break;
        case MEMORY_ERROR:
            printf("Memory Error\n");
            break;
        case VALID_ERROR:
            printf("BMP file not valid!\n");
            break;
        default:
            printf("Unknown Error\n");
            break;
    }
}

BMP_Image* createBMPImage() {
    BMP_Image* newImage = (BMP_Image*) malloc(sizeof(BMP_Image));
    if (newImage == NULL) {
        printError(MEMORY_ERROR);
        return NULL;
    }
    return newImage;
}

BMP_Image* readImage(FILE *srcFile) {
    if (srcFile == NULL) {
        printError(FILE_ERROR);
        return NULL;
    }

    BMP_Image tempImage;
    if (fread(&(tempImage.header), sizeof(BMP_Header), 1, srcFile) != 1) {
        printError(FILE_ERROR);
        return NULL;
    }

    if (checkBMPValid(&(tempImage.header)) == FALSE) {
        printError(VALID_ERROR);
        return NULL;
    }

    tempImage.norm_height = abs(tempImage.header.height_px);
    tempImage.bytes_per_pixel = tempImage.header.bits_per_pixel / 8;

    int dataSize = tempImage.header.width_px * tempImage.norm_height * tempImage.bytes_per_pixel;
    BMP_Image* dataImage = (BMP_Image*) malloc(sizeof(BMP_Image) + dataSize);
    if (dataImage == NULL) {
        printError(MEMORY_ERROR);
        return NULL;
    }

    memcpy(dataImage, &tempImage, sizeof(BMP_Image));
    fseek(srcFile, tempImage.header.offset, SEEK_SET);
    fread(dataImage->pixels, dataSize, 1, srcFile);

    return dataImage;
}

void writeImage(char* destFileName, BMP_Image* dataImage) {
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        return;
    }
    fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile);
    int dataSize = dataImage->header.width_px * dataImage->norm_height * dataImage->bytes_per_pixel;
    fwrite(dataImage->pixels, dataSize, 1, destFile);

    fclose(destFile);
}

void freeImage(BMP_Image* image) {
    free(image);
}

int checkBMPValid(BMP_Header* header) {
    if (header->type != 0x4D42) {
        return FALSE;
    }
    if (header->bits_per_pixel != 24) {
        return FALSE;
    }
    if (header->compression != 0) {
        return FALSE;
    }
    if (header->planes != 1) {
        return FALSE;
    }
    return TRUE;
}

void printBMPHeader(BMP_Header* header) {
    printf("BMP Header Information:\n");
    printf("Tipo de archivo: %x\n", header->type);
    printf("Tamaño del archivo: %u bytes\n", header->size);
    printf("Ancho de la imagen: %d px\n", header->width_px);
    printf("Alto de la imagen: %d px\n", header->height_px);
    printf("Bits por pixel: %u\n", header->bits_per_pixel);
    printf("Compresión: %u\n", header->compression);
}

void printBMPImage(BMP_Image* image) {
    printBMPHeader(&(image->header));
    printf("Altura normalizada: %d\n", image->norm_height);
    printf("Bytes por píxel: %d\n", image->bytes_per_pixel);
}

// Para obtener acceso a una imagen almacenada en memoria compartida
BMP_Image* getSharedMemoryImage(key_t key) {
    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("getSharedMemoryImage: Error al obtener la memoria compartida (shmget)");
        exit(1);
    }

    BMP_Image* shmaddr = (BMP_Image*)shmat(shmid, (void*)0, 0);
    if (shmaddr == (BMP_Image*)(-1)) {
        perror("getSharedMemoryImage: Error al adjuntar la memoria compartida (shmat)");
        exit(1);
    }

    return shmaddr; // Se va a retoirnar la direccion de la imagen en memoria compartida
}

// Usamos una funcion para liberar el espacio de memoria compartida
void liberarMemoriaCompartida(key_t key) {
    int shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("liberarMemoriaCompartida: Error al obtener la memoria compartida (shmget)");
        exit(1);
    }
    // Eliminamos el segmento de memoria que esta siendo compartida para liberar recursos
    if (shmctl(shmid, IPC_RMID, NULL) < 0) {
        perror("liberarMemoriaCompartida: Error al liberar la memoria compartida (shmctl)");
        exit(1);
    }
}
