#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "bmp.h"

int main() {
    key_t key = ftok("shmfile", 65);  // Crear una clave única
    
    BMP_Image *shmaddr = getSharedMemoryImage(key);

    BMP_Image* image = shmaddr;
        int width = image->header.width_px;
        int height = image->norm_height;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int index = i * width + j;
                image->pixels[index].red = 255 - image->pixels[index].red;
                image->pixels[index].green = 255 - image->pixels[index].green;
                image->pixels[index].blue = 255 - image->pixels[index].blue;
            }
        }

    shmdt(shmaddr);  // Desadjuntar la memoria compartida
    return 0;
}