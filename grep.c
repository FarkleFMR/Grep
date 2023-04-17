#include <stdio.h>
#include <regex.h>

int main() {
    char *string = "hello world";
    regex_t regex;
    int reti;

    reti = regcomp(&regex, "h...o|", 0);
    if (reti) {
        fprintf(stderr, "No se pudo compilar la expresión regular");
        return 1;
    }

    reti = regexec(&regex, string, 0, NULL, 0);
    if (!reti) {
        printf("Se encontró la cadena de texto: %s\n", string);
    } else if (reti == REG_NOMATCH) {
        printf("No se encontró la cadena de texto\n");
    } else {
        regerror(reti, &regex, NULL, 0);
        fprintf(stderr, "Error al ejecutar la expresión regular\n");
        return 1;
    }

    regfree(&regex);
    return 0;
}

