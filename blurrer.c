#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

// Esta estructura nos sirve para almacenar los valores de cada hilo
typedef struct {
    int startHeight;  // Altura inicial de la región a procesar
    int endHeight;    // Altura final de la región a procesar
    int width;        // Ancho total de la imagen de la imagen
    Pixel *originalPixels; 
    BMP_Image *image;
    int kernel[3][3]; // Matriz del filtro Gaussiano
    int kernelSum;  
} ThreadData;

void* applyGaussianBlur(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    printf("blurrer: aplicando filtro Gaussiano desde %i\n", data->startHeight);
    int kernelSize = 3;

    // Recorre la porción de la imagen que ha sido asignada a este hilo
    for (int y = data->startHeight; y < data->endHeight; y++) {
        for (int x = 1; x < data->width - 1; x++) {
            int red = 0, green = 0, blue = 0;

            for (int ky = 0; ky < kernelSize; ky++) {
                for (int kx = 0; kx < kernelSize; kx++) {
                    int pos = (y + ky - 1) * data->width + (x + kx - 1);
                    red += data->originalPixels[pos].red * data->kernel[ky][kx];
                    green += data->originalPixels[pos].green * data->kernel[ky][kx];
                    blue += data->originalPixels[pos].blue * data->kernel[ky][kx];
                }
            }

            // guiardamos el resultado en la imagen de salida
            int pos = y * data->width + x;
            data->image->pixels[pos].red = red / data->kernelSum;
            data->image->pixels[pos].green = green / data->kernelSum;
            data->image->pixels[pos].blue = blue / data->kernelSum;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <altura_inicio> <altura_fin> <num_hilos>\n", argv[0]);
        return 1;
    }

    // Obtenemos los valores de entrada
    int startHeight = atoi(argv[1]);
    int endHeight = atoi(argv[2]);
    int numThreads = atoi(argv[3]);

    if (startHeight < 0 || endHeight < 0 || startHeight >= endHeight || numThreads <= 0) {
        fprintf(stderr, "Parámetros inválidos\n");
        return 1;
    }

    printf("blurer: startHeight: %d\n", startHeight);
    printf("blurer: endHeight: %d\n", endHeight);
    printf("blurer: numThreads: %d\n", numThreads);

    // Accedemos a la imagen que fue asignada a memeoria compartida
    printf("blurer: accediendo a imagen en memoria compartida\n");
    key_t key = ftok("shmfile", 65);
    BMP_Image *shmaddr = getSharedMemoryImage(key);
    BMP_Image* image = shmaddr;
    int width = image->header.width_px;
    int height = image->norm_height;

    if (endHeight > height) {
        fprintf(stderr, "Error: altura final excede la imagen\n");
        return 1;
    }
    printf("blurer: configurando hilos\n");
    Pixel* originalPixels = (Pixel*)malloc(width * height * sizeof(Pixel));
    if (!originalPixels) {
        fprintf(stderr, "Error en la asignación de memoria\n");
        return 1;
    }

    for (int i = 0; i < width * height; i++) {
        originalPixels[i] = image->pixels[i];
    }

    int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    int kernelSum = 16;

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    int segmentHeight = (endHeight - startHeight) / numThreads;

    // Creamos los hilos necesarios de acuerdo al input del usuario
    for (int i = 0; i < numThreads; i++) {
        threadData[i].startHeight = startHeight + i * segmentHeight;
        threadData[i].endHeight = (i == numThreads - 1) ? endHeight : threadData[i].startHeight + segmentHeight;
        threadData[i].width = width;
        threadData[i].originalPixels = originalPixels;
        threadData[i].image = image;
        threadData[i].kernelSum = kernelSum;
        memcpy(threadData[i].kernel, kernel, sizeof(kernel));

        // Capturamos el error en el caso de que falle
        if (pthread_create(&threads[i], NULL, applyGaussianBlur, &threadData[i]) != 0) {
            fprintf(stderr, "Error al crear el hilo %d\n", i);
            free(originalPixels);
            return 1;
        }
        printf("blurer: hilo %d inicializado\n", i);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Liberar memoria y quitamos la imagen de la memoria compartida
    free(originalPixels);
    shmdt(shmaddr);
    return 0;
}