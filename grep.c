#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>

#define MAX_LINE_SIZE 1024
#define MAX_BUFFER_SIZE 8192
#define MAX_NUM_THREADS 4

// Estructura para pasar los argumentos a los hilos
typedef struct {
    char* buffer;
    int buffer_size;
    int thread_id;
    FILE* file;
    regex_t* regex;
    int* found;
    pthread_mutex_t* mutex;
} thread_args_t;

// Función que busca el patrón en una línea del archivo
void search_line(regex_t* regex, char* line, int* found) {
    if (regexec(regex, line, 0, NULL, 0) == 0) {
        *found = 1;
    }
}

// Función que procesa una porción del archivo
void* process_file(void* args) {
    thread_args_t* targs = (thread_args_t*) args;
    int thread_id = targs->thread_id;
    FILE* file = targs->file;
    regex_t* regex = targs->regex;
    int* found = targs->found;
    pthread_mutex_t* mutex = targs->mutex;

    // Reservar memoria para el buffer
    targs->buffer = (char*) malloc(targs->buffer_size);
    if (targs->buffer == NULL) {
        perror("Error al reservar memoria para el buffer");
        exit(EXIT_FAILURE);
    }

    // Inicializar el buffer con ceros
    memset(targs->buffer, 0, targs->buffer_size);

    // Leer el archivo en porciones de MAX_BUFFER_SIZE bytes
    while (!feof(file)) {
        // Leer una porción del archivo
        size_t bytes_read = fread(targs->buffer, 1, targs->buffer_size, file);

        // Actualizar el puntero de lectura del archivo hasta el final de la última línea que se leyó completa
        if (bytes_read > 0) {
            char* last_newline = strrchr(targs->buffer, '\n');
            if (last_newline != NULL) {
                size_t offset = last_newline - targs->buffer + 1;
                fseek(file, -bytes_read + offset, SEEK_CUR);
                bytes_read = offset;
            }
        }
	

        // Procesar cada línea del buffer
        char* line = strtok(targs->buffer, "\n");
        while (line != NULL) {
            // Buscar el patrón en la línea
            int line_found = 0;
            search_line(regex, line, &line_found);

            // Si se encontró el patrón, imprimir la línea y el número de hilo
            if (line_found) {
                pthread_mutex_lock(mutex);
                printf("Thread %d: %s\n", thread_id, line);
                pthread_mutex_unlock(mutex);
                *found = 1;
            }

            // Pasar a la siguiente línea
            line = strtok(NULL, "\n");
        }

        // Limpiar el buffer
        memset(targs->buffer, 0, targs->buffer_size);
    }

    // Liberar la memoria del buffer
    free(targs->buffer);
    targs->buffer = NULL;

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <filename> <regex>\n", argv[0]);
        return 1;
    }

    // Open the file for reading
    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Compile the regular expression
    regex_t regex;
    int ret = regcomp(&regex, argv[2], REG_EXTENDED);
    if (ret != 0) {
        char error_message[MAX_LINE_SIZE];
        regerror(ret, &regex, error_message, sizeof(error_message));
        printf("Error compiling regex: %s\n", error_message);
        fclose(file);
        return 1;
    }

    // Initialize variables for tracking whether the pattern was found and for mutual exclusion
    int found = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Initialize the thread argument structure
    thread_args_t targs[MAX_NUM_THREADS];
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        targs[i].buffer = NULL;
        targs[i].buffer_size = MAX_BUFFER_SIZE;
        targs[i].thread_id = i + 1;
        targs[i].file = file;
        targs[i].regex = &regex;
        targs[i].found = &found;
        targs[i].mutex = &mutex;
    }

    // Create the threads
    pthread_t threads[MAX_NUM_THREADS];
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        int ret = pthread_create(&threads[i], NULL, process_file, &targs[i]);
        if (ret != 0) {
            perror("Error creating thread");
            fclose(file);
            regfree(&regex);
            return 1;
        }
    }

    // Wait for the threads to finish
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&mutex);
    regfree(&regex);
    fclose(file);

}
