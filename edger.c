#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include "bmp.h"
#include <math.h>

typedef struct {
    int startHeight;
    int endHeight;
    int width;
    BMP_Image *image;
    BMP_Image *resultImage;
    int Gx[3][3];
    int Gy[3][3];
} ThreadData;

// para aplicar el filtro a la iamgen
void* applySobelFilter(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    printf("edger: aplicando filtro Sobel desde altura %i\n", data->startHeight);

    for (int i = data->startHeight; i < data->endHeight; i++) {
        for (int j = 1; j < data->width - 1; j++) {
            int sumX = 0, sumY = 0;

            for (int k = -1; k <= 1; k++) {
                for (int l = -1; l <= 1; l++) {
                    int index = (i + k) * data->width + (j + l);
                    int gray = (data->image->pixels[index].red + data->image->pixels[index].green + data->image->pixels[index].blue) / 3;
                    sumX += gray * data->Gx[k + 1][l + 1];
                    sumY += gray * data->Gy[k + 1][l + 1];
                }
            }

            int index = i * data->width + j;
            int magnitude = (int)fmin(sqrt(sumX * sumX + sumY * sumY), 255);

            data->resultImage->pixels[index].red = magnitude;
            data->resultImage->pixels[index].green = magnitude;
            data->resultImage->pixels[index].blue = magnitude;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <altura_inicio> <altura_fin> <num_hilos>\n", argv[0]);
        return 1;
    }

    int startHeight = atoi(argv[1]);
    int endHeight = atoi(argv[2]);
    int numThreads = atoi(argv[3]);

    // validamos que los parametros sean correctos sino se imprime el error y se retorna la funcion
    if (startHeight < 0 || endHeight < 0 || startHeight >= endHeight || numThreads <= 0) {
        fprintf(stderr, "Parámetros inválidos\n");
        return 1;
    }

    printf("edger: startHeight: %d\n", startHeight);
    printf("edger: endHeight: %d\n", endHeight);
    printf("edger: numThreads: %d\n", numThreads);

    printf("edger: obteniendo imagen de memoria compartida\n");
    
    // Obtenemos la imagen desde la memoria compartida
    key_t key = ftok("shmfile", 65);
    BMP_Image *shmaddr = getSharedMemoryImage(key);
    BMP_Image* image = shmaddr;
    int width = image->header.width_px;
    int height = image->norm_height;

    if (endHeight > height) {
        fprintf(stderr, "Altura fin excede la altura de la imagen\n");
        return 1;
    }

    printf("edger: preparando hilos\n");

    // Reservar espacio para la imagen procesada
    size_t imageSize = sizeof(BMP_Image) + width * height * sizeof(Pixel);
    BMP_Image* resultImage = (BMP_Image*)malloc(imageSize);
    resultImage->header = image->header;
    resultImage->norm_height = image->norm_height;
    resultImage->bytes_per_pixel = image->bytes_per_pixel;

    // Copiar datos de la imagen original a la imagen resultante
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int index = i * width + j;
            resultImage->pixels[index] = image->pixels[index];
        }
    }

    // Definir los operadores de Sobel en X y Y
    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    pthread_t threads[numThreads];
    ThreadData threadData[numThreads];
    int heightPerThread = (endHeight - startHeight) / numThreads;

    printf("edger: aplicando filtro Sobel\n");
    
    // Creamos los hilos para aplicar el filtro
    for (int i = 0; i < numThreads; i++) {
        threadData[i].startHeight = startHeight + i * heightPerThread;
        threadData[i].endHeight = (i == numThreads - 1) ? endHeight : threadData[i].startHeight + heightPerThread;
        threadData[i].width = width;
        threadData[i].image = image;
        threadData[i].resultImage = resultImage;
        memcpy(threadData[i].Gx, Gx, sizeof(Gx));
        memcpy(threadData[i].Gy, Gy, sizeof(Gy));

        pthread_create(&threads[i], NULL, applySobelFilter, &threadData[i]);
        printf("edger: hilo %d creado\n", i);
    }

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
        printf("edger: hilo %d terminado\n", i);
    }

    // Copiar los resultados en la imagen original
    for (int i = startHeight; i < endHeight; i++) {
        for (int j = 0; j < width; j++) {
            int index = i * width + j;
            image->pixels[index] = resultImage->pixels[index];
        }
    }

    printf("edger: filtro sobel aplicado\n");

    // Liberamops memoria compartida
    free(resultImage);
    shmdt(shmaddr);
    return 0;
}
