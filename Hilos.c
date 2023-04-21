#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void *thread_function(void *arg)
{
  char *string;
    size_t bufsize = 32; // Tamaño
    size_t len;

    FILE *archivo;
    archivo = fopen("Archivo.txt", "r");

    if (archivo == NULL) {
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    string = (char *)malloc(bufsize * sizeof(char)); 

    if (string == NULL) {
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    // Leer el string dinámicamente desde el archivo
    if (fgets(string, bufsize, archivo) != NULL) {
        len = strlen(string);
        while (string[len-1] != '\n') { // Si el string no está completo, asignar más memoria
            bufsize *= 2;
            string = (char*)realloc(string, bufsize);
            if (string == NULL) {
                perror("Error al asignar memoria");
                exit(EXIT_FAILURE);
            }
            if (fgets(string + len, bufsize - len, archivo) == NULL)
                break;
            len = strlen(string);
        }
    }

    printf("Contenido del archivo: %s", string);

    fclose(archivo);
    return 0;  

}



int main(void)
{
    int rc;
  pthread_t mythread;
  rc = pthread_create(&mythread, NULL, thread_function, NULL);
  if (rc) {
    printf("Error al crear el hilo: %d\n", rc);
    return 1;
  }

  printf("Hola desde el hilo principal!\n");

  rc = pthread_join(mythread, NULL);
  if (rc) {
    printf("Error al esperar al hilo: %d\n", rc);
    return 1;
  }

  return 0;
}
