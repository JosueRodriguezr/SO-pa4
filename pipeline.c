#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bmp.h"

// la funcion que nos permitira realizar todo el proceso de filtros en un pipeline
void process_image(char *inputfile, char *outputfile, char *num_threads) {
    key_t key = ftok("shmfile", 65);  // genramos una clave unica para la memoria compartida
    pid_t pid_publisher, pid_inverter, pid_blurrer;

    // Ejecutar el proceso "publisher" para cargar la imagen en memoria compartida
    // ./publisher
    printf("Ejecutando publisher\n");
    pid_publisher = fork();
    if (pid_publisher < 0) {
        perror("Error al crear proceso hijo");
        exit(1);
    } else if (pid_publisher == 0) {
        char *args[] = {"./publisher", inputfile, NULL};
        execvp(args[0], args);
        perror("Error en execvp de publisher");
        exit(1);
    } else {
        wait(NULL);  // Esperar a que termine publisher
    }

    // Obtener acceso a la imagen en la memoria compartida
    BMP_Image *shmaddr = getSharedMemoryImage(key);
    int height = shmaddr->norm_height;
    int halfheight = shmaddr->norm_height / 2;
    char height_str[12]; // Almacenar altura como string
    char halfheight_str[12]; // Almacenar mitad de la altura como string
    sprintf(height_str, "%d", height);
    sprintf(halfheight_str, "%d", halfheight);

    // Ejecutar el proceso "blurrer" para aplicar un desenfoque
    // ./blurrer
    printf("Ejecutando blurrer\n");
    pid_inverter = fork();
    if (pid_inverter < 0) {
        perror("Error al crear proceso hijo");
        exit(1);
    } else if (pid_inverter == 0) {
        char *args[] = {"./blurrer", halfheight_str, height_str, num_threads, NULL};
        execvp(args[0], args);
        perror("Error en execvp de blurrer");
        exit(1);
    }

    // Ejecutar el proceso "edger" para detectar bordes en la imagen
    // ./edger
    printf("Ejecutando edger\n");
    pid_blurrer = fork();
    if (pid_blurrer < 0) {
        perror("Error al crear proceso hijo");
        exit(1);
    } else if (pid_blurrer == 0) {
        char *args[] = {"./edger", "1", halfheight_str, num_threads, NULL};
        execvp(args[0], args);
        perror("Error en execvp de edger");
        exit(1);
    }

    // Esperar a que finalicen los procesos blurrer y edger
    waitpid(pid_inverter, NULL, 0);
    waitpid(pid_blurrer, NULL, 0);

    // Guardar la imagen procesada en el archivo de salida
    printf("Escribiendo imagen en %s \n", outputfile);
    writeImage(outputfile, shmaddr);

    // Liberar la memoria compartida
    shmdt(shmaddr);
    liberarMemoriaCompartida(key);
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida> <num_hilos> [recurrente < -r >]\n", argv[0]);
        exit(1);
    }

    char *inputfile = argv[1];
    char *outputfile = argv[2];
    char *num_threads = argv[3];
    int recurrent_mode = (argc == 5 && strcmp(argv[4], "-r") == 0);

    do {
        printf("Archivo de entrada: %s\n", inputfile);
        printf("Archivo de salida: %s\n", outputfile);
        printf("Número de hilos: %s\n", num_threads);
        process_image(inputfile, outputfile, num_threads);

        if (recurrent_mode) {
            printf("Ingrese nuevo archivo de entrada: ");
            scanf("%s", inputfile);
            printf("Ingrese nuevo archivo de salida: ");
            scanf("%s", outputfile);
            printf("Ingrese número de hilos: ");
            scanf("%s", num_threads);
        }
    } while (recurrent_mode);

    return 0;
}
