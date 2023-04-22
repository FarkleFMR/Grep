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
} data_thread;

// Función que busca el patrón en una línea del archivo
void buscar_linea(regex_t* regex, char* line, int* found) {
    if (regexec(regex, line, 0, NULL, 0) == 0) {
        *found = 1;
    }
}

void procesar_linea(char* buffer,regex_t* regex,pthread_mutex_t* mutex,int thread_id,int* found){
// Procesar cada línea del buffer
	
   char* line = strtok(buffer, "\n");
   while (line != NULL) {
      // Buscar el patrón en la línea
      int line_found = 0;
      buscar_linea(regex, line, &line_found);

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
}

// Función que procesa una porción del archivo
void* procesar_archivo(void* args) {
    data_thread* data = (data_thread*) args;
    int thread_id = data->thread_id;
    FILE* file = data->file;
    regex_t* regex = data->regex;
    int* found = data->found;
    pthread_mutex_t* mutex = data->mutex;

    // Reservar memoria para el buffer
    data->buffer = (char*) malloc(data->buffer_size);
    if (data->buffer == NULL) {
        perror("Error al reservar memoria para el buffer");
        exit(EXIT_FAILURE);
    }

    // Inicializar el buffer con ceros
    memset(data->buffer, 0, data->buffer_size);

    // Leer el archivo en porciones de MAX_BUFFER_SIZE bytes
    while (!feof(file)) {
        // Leer una porción del archivo
        size_t bytes_read = fread(data->buffer, 1, data->buffer_size, file);

        // Actualizar el puntero de lectura del archivo hasta el final de la última línea que se leyó completa
        if (bytes_read > 0) {
            char* last_line = strrchr(data->buffer, '\n');
            if (last_line != NULL) {
                size_t offset = last_line - data->buffer + 1;
                fseek(file, -bytes_read + offset, SEEK_CUR);
                bytes_read = offset;
            }
        }
	
        procesar_linea(data->buffer,regex,mutex,thread_id,found);

        // Limpiar el buffer
        memset(data->buffer, 0, data->buffer_size);
    }

    // Liberar la memoria del buffer
    free(data->buffer);
    data->buffer = NULL;

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Error formato: %s <regex> <filename>\n", argv[0]);
        return 1;
    }

    //Abrir el archivo
    FILE* file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("Error abriendo el archivo");
        return 1;
    }

    // Compilar la expresion regular
    regex_t regex;
    int ret = regcomp(&regex, argv[1], REG_EXTENDED);
    if (ret != 0) {
        char error_message[MAX_LINE_SIZE];
        regerror(ret, &regex, error_message, sizeof(error_message));
        printf("Error compilando regex: %s\n", error_message);
        fclose(file);
        return 1;
    }

    // Inicializar las variables
    int found = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Iniciar los datos de los hilos
    data_thread data[MAX_NUM_THREADS];
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        data[i].buffer = NULL;
        data[i].buffer_size = MAX_BUFFER_SIZE;
        data[i].thread_id = i + 1;
        data[i].file = file;
        data[i].regex = &regex;
        data[i].found = &found;
        data[i].mutex = &mutex;
    }

    // Crear los hilos
    pthread_t threads[MAX_NUM_THREADS];
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        int ret = pthread_create(&threads[i], NULL, procesar_archivo, &data[i]);
        if (ret != 0) {
            perror("Error creando threads");
            fclose(file);
            regfree(&regex);
            return 1;
        }
    }

    // Esperar a que los otros hilos finalicen
    for (int i = 0; i < MAX_NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    
    pthread_mutex_destroy(&mutex);
    regfree(&regex);
    fclose(file);

}
