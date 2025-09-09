#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mv.h"

uint8_t versionPrograma = 0;
int continuarEjecucion = 1;//para controlar el bucle

void mostrarUso() {
    printf("Uso: mvx [archivo.vmx] [-d] \n");
    printf("Opciones:\n");
    printf("  archivo.vmx   : Archivo de programa a cargar \n");
    printf("  -d            : Mostrar desensamblado \n");
}

int main(int argc, char *argv[]) {
    //Procesamient de argumentos
    int desensamblar = 0;
    const char *archivo_vmx = NULL;
    srand(time(NULL)); // Para la instruccion RND

    printf("Maquina Virtual MV1 - UNMDP - Arquitectura de Computadoras\n");

    for (int i = 1; i < argc; i++) {
        if(archivo_vmx == NULL && strstr(argv[i], ".vmx")){
            archivo_vmx = argv[i];
        }else if(strcmp(argv[i], "-d") == 0){
            desensamblar = 1;
        }
    }

    if (archivo_vmx == NULL) {
        mostrarUso();
        return 1;
    }
    
    //Inicializar memoriay cargar programa
    inicializaMemoria();
    if (cargaPrograma(archivo_vmx) != 0) {
        fprintf(stderr, "Error al cargar el programa\n");
        return 1;
    }

    // Modo desensamblado
    if (desensamblar) {
        muestraDesensamblador(versionPrograma);
        return 0;
    }
    // Ejecutar
    int resultado = ejecutarPrograma();

    printf("Registros: \n");
    for (int i = 0; i < NUM_REGISTROS; i++) {
        printf("%s: %08X  ", NOMBRES_REGISTROS[i], Registros[i]);
    }
    printf("\n");
    printf("Tabla de segmentos: \n");
    for (int i = 0; i < NUM_SEG; i++) {
        printf("Segmento %d - Base: %04X, Tamanio: %04X\n", i, tablaSegmentos[i].base, tablaSegmentos[i].tamanio);
    }
    return resultado;
}
