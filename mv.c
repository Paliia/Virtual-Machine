#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h> //para tener INT_MAX e INT_MIN
#include "mv.h"

//-------------VARIABLES GLOBALES---------------
// Memoria principal
uint8_t *MemoriaPrincipal;
uint32_t TAMANIO_MEMORIA = 16384; //tamanio en bytes por defecto
// Tabla de Registros
uint32_t Registros[NUM_REGISTROS];
DescriptoresSegmentos tablaSegmentos[NUM_SEG];

//variables del main
extern uint8_t versionPrograma;
extern int continuarEjecucion;//para controlar el bucle

//---------------FUNCION PARA DETECCION DE ERROR---------------
void detectaError(int8_t cod, int32_t er){
    continuarEjecucion=0;
    switch(cod){
        case COD_ERR_DIV: {
            printf("Error, dividendo es cero \n");
            break;
            }
        case COD_ERR_INS: {
            printf("Error, instruccion invalida \n");
            break;
        }
        case COD_ERR_REG:{
            printf("Error, registro invalido \n");
            break;
        }
        case COD_ERR_FIS: {
            printf("Error, direccion fisica invalida: 0x%08X \n",er);
            break;
        }
        case COD_ERR_OPE:{
            printf("Error, operando invalido \n");
            break;
        }
        case COD_ERR_READ:{
            printf("Error, modo de lectura invalida \n");
            break;
        }
        case COD_ERR_WRITE:{
            printf("Error, modo de escritura invalido \n");
            break;
        }
        case COD_ERR_LOG:{
            printf("Error, direccion logica invalida: 0x%08X \n",er);
            break;
        }
        case COD_ERR_MEM_INS:{
            printf("Error, tamanio de memoria insuficiente para guardar el programa. Tamanio memoria: %d, tamanio programa: %d \n",TAMANIO_MEMORIA,er);
            break;
        }
        case COD_ERR_SEGMENT:{
            printf("Error, segmento invalido 0x%08X \n",er);
            break;
        }        
    }
}

//--------------DECLARACIONES DE FUNCIONES PARA VIRTUAL MACHINE------//
void inicializaMemoria(){
    if (MemoriaPrincipal != NULL) {
        free(MemoriaPrincipal);
    }

    MemoriaPrincipal = malloc(TAMANIO_MEMORIA);
    if (!MemoriaPrincipal) {
        exit(EXIT_FAILURE);
    }

    memset(MemoriaPrincipal, 0, TAMANIO_MEMORIA);
}

//---------------- FUNCIONES DE MEMORIA ----------------
uint32_t calculaDireccionFisica(uint32_t direccionLogica) {
    uint16_t segmento = direccionLogica >> 16;
    uint16_t offset = direccionLogica & 0xFFFF;

    if (segmento >= NUM_SEG) {
        detectaError(COD_ERR_LOG, direccionLogica);
        return 0;
    }
    // Verificar que el desplazamiento no exceda el tamanio del segmento
    if (offset >= tablaSegmentos[segmento].tamanio) {
        detectaError(COD_ERR_LOG, direccionLogica);
        return 0;
    }
    uint32_t direccionFisica = tablaSegmentos[segmento].base + offset;

    if (direccionFisica >= TAMANIO_MEMORIA) {
        detectaError(COD_ERR_FIS, direccionFisica);
        return 0;
    }
    return direccionFisica;
}

int32_t leerMemoria(uint32_t direccionFisica, uint8_t tamanio) {
    if(direccionFisica + tamanio > TAMANIO_MEMORIA) {
        detectaError(COD_ERR_FIS, direccionFisica + tamanio);
        return 0;
    }

    int32_t valor = 0;
    for(int i = 0; i < tamanio; i++) {
        valor = (valor << 8) | MemoriaPrincipal[direccionFisica + i];
    }

    // Extensión de signo para tamaños menores a 4 bytes (parte 2)
    //if(tamanio < 4) {
    //    int bits = tamanio * 8;
    //    valor = (valor << (32 - bits)) >> (32 - bits);
    //}
    return valor;
}

void escribirMemoria(uint32_t direccionFisica, int32_t valor, uint8_t tamanio) {
    if(direccionFisica+tamanio > TAMANIO_MEMORIA) {
        detectaError(COD_ERR_FIS, direccionFisica+tamanio);
        return;
    }
     for (int i = 0; i < tamanio; i++) {
        MemoriaPrincipal[direccionFisica + i] = (valor >> (8 * (tamanio - 1 - i))) & 0xFF;
    }

}

void inicializaSegmentosV1(uint16_t tamanioCodigo){
    for(int i=0; i<NUM_SEG; i++){
        tablaSegmentos[i].base = 0;
        tablaSegmentos[i].tamanio = 0;
    }
        //actualiza la tabla de descriptores
        //En esta version, SEG_CS = 0 y SEG_DS = 1 (posiciones del CS y DS)
    tablaSegmentos[SEG_CS].tamanio = tamanioCodigo;

    tablaSegmentos[SEG_DS].base = tamanioCodigo;
    tablaSegmentos[SEG_DS].tamanio = TAMANIO_MEMORIA-tamanioCodigo;
}

void inicializaTablaRegistrosV1(){
    memset(Registros, 0, sizeof(Registros)); // Inicializo registros a 0

    // CS: segmento 0x0000, offset 0x0000
    Registros[POS_CS] = 0x00000000; // o podria ser POS_CS<<16
    // DS: segmento 0x0001, offset 0x0000
    Registros[POS_DS] = 0x00010000; // o podria ser POS_DS<<16
    // IP: comienza en 0
    Registros[POS_IP] = 0x00000000;
}

//-------------------LECTURA DEL ARCHIVO---------------------------------
    //-----------CARGA PROGRAMA PARA VERSION 1----------------------//
int cargaProgramaV1(FILE *arch) {
    VMXHeaderV1 encabezado;

    // Leer header completo
    if (fread(&encabezado, sizeof(VMXHeaderV1), 1, arch) != 1) {
        printf("Error al leer encabezado MV1\n");
        return -1;
    }


    // Verificar identificador y versión
    if (memcmp(encabezado.identificador, "VMX25", 5) != 0 || encabezado.version != 1) {
        printf("Encabezado inválido para MV1\n");
        return -1;
    }

    encabezado.tamanio = convertirBigEndian16(encabezado.tamanio);

    // Verificar que el programa cabe en memoria
    if (encabezado.tamanio == 0 || encabezado.tamanio >= TAMANIO_MEMORIA ) {
        detectaError(COD_ERR_MEM_INS, encabezado.tamanio);
        fclose(arch);
        return -1;
    }

    // Leer código directamente al inicio de la memoria
    size_t bytes_leidos = fread(MemoriaPrincipal, sizeof(uint8_t), encabezado.tamanio, arch);
    if (bytes_leidos != encabezado.tamanio) {
        printf("Error: tamaño del código no coincide\n");
        fclose(arch);
        return -1;
    }

    fclose(arch);

    // Inicializar segmentos y registros
    inicializaSegmentosV1(encabezado.tamanio);
    inicializaTablaRegistrosV1();

    return 0;
}

//----------------CARGA PROGRAMA SEGUN LA VERSION---------------
int cargaPrograma(const char *nombreArchivo) {
    FILE *arch;
    char identificador[6] = {0};

    printf("Intentando abrir archivo: '%s'\n", nombreArchivo);
    arch = fopen(nombreArchivo, "rb");
    if (arch == NULL) {
        printf("Error: no se pudo abrir el archivo\n");
        return -1;
    }

    // Leer identificador y versión
    if (fread(identificador, 1, 6, arch) != 6) {
        printf("Error: no se pudo leer la cabecera\n");
        fclose(arch);
        return -1;
    }

    // Volver al inicio para que las funciones específicas lean desde cero
    fseek(arch, 0, SEEK_SET);

    if (strncmp(identificador, "VMX25", 5) != 0) {
        printf("Error: encabezado desconocido (no es VMX25)\n");
        fclose(arch);
        return -1;
    }

    versionPrograma = (uint8_t)identificador[5];

    if (versionPrograma == 1) {
        printf("Archivo detectado como MV1\n");
        return cargaProgramaV1(arch);
    } else { //en version 2, modificar y agregar parametros
        printf("Versión de archivo no soportada: %d\n", versionPrograma);
        fclose(arch);
        return -1;
    }
}

//-----------------FUNCIONES DE REGISTROS--------------
int verificaRegistro(uint8_t numReg) {
    if (numReg >= NUM_REGISTROS) {
        detectaError(COD_ERR_REG,numReg);
        return -1;
    }
    return 0;
}

int32_t obtenerValorRegistro(uint8_t numReg) {
    int32_t valor = (int32_t)Registros[numReg];
    return valor;
}

void escribirEnRegistro(uint8_t numReg, int32_t valor) {
    //registro verificado en funcion que la invoca
    Registros[numReg] = valor;
}

uint32_t calculaDireccionSalto(uint8_t tipoA, uint32_t operandoA,uint8_t tamA) {
    //los saltos son con respecto al Code Segment
    uint32_t direccionSalto;
    uint32_t offset;

    if(tipoA == OP_REG) {
        uint8_t numReg = operandoA & 0x1F;
        offset = obtenerValorRegistro(numReg); // Obtener el offset que guarda el registro

    } else if(tipoA == OP_INM) {
        offset = operandoA;

    } else if(tipoA == OP_MEM) {
        uint32_t direccionFisica = calculaDireccionFisica(operandoA);
        offset = leerMemoria(direccionFisica, tamA); // Leer el offset de memoria

    } else {
        detectaError(COD_ERR_OPE,tipoA);
        return 0xFFFFFFFF;
    }
    // Verificar si la direccion de salto es valida
    if (offset >= tablaSegmentos[Registros[POS_CS]>>16].tamanio) {
        return 0xFFFFFFFF;
    }
    direccionSalto = (Registros[POS_CS] & 0xFFFF0000) | offset;
    return direccionSalto;
}

void actualizarCC(int32_t resultado) {
    Registros[POS_CC] &= ~(CC_N | CC_Z);
    // Bit N (signo)
    if (resultado < 0) {
        Registros[POS_CC] |= CC_N;
    }

    // Bit Z (cero)
    if (resultado == 0) {
        Registros[POS_CC] |= CC_Z;
    }
}

//---------------------FUNCIONES DE OPERANDOS------------------
void guardaRegistroOP(uint8_t tipo, uint32_t operando, uint8_t tipoOP_AB){
    if(tipoOP_AB == 1){
        Registros[POS_OP1] = (tipo<<24) | operando;
    }else if(tipoOP_AB == 2){
        Registros[POS_OP2] = (tipo<<24) | operando;
    }
}

uint32_t obtenerOperando(uint8_t tipo, unsigned int *ip, uint8_t *tam, uint8_t tipoOP_AB) {
    uint32_t operando = 0;
    uint8_t byte1, byte2, byte3;
    // Verifica si el operando es valido, y mueve el IP
    uint32_t ip_aux=calculaDireccionFisica(*ip);

    *tam=4;
    if(tipo == OP_REG) { // Operando de registro (1 byte)
        byte1 = MemoriaPrincipal[ip_aux];
        (*ip)++; ip_aux++;
        if(verificaRegistro(byte1 & 0x1F == -1)) {
            detectaError(COD_ERR_REG, byte1 & 0x1F);
            return 0;
        }
        operando = byte1;
        guardaRegistroOP(tipo, operando, tipoOP_AB);

    }else if(tipo == OP_INM) { // Operando inmediato (2 bytes)
        byte1 = MemoriaPrincipal[ip_aux];
        byte2 = MemoriaPrincipal[ip_aux + 1];
        (*ip) += 2; ip_aux += 2;
        operando = (byte1 << 8) | byte2; // Ensamblar el valor inmediato de 16 bits
        // Si el valor es negativo, extiendo para signo
        if (operando & 0x8000) {
            operando |= 0xFFFF0000;
        }

        //guardar en OP1 u OP2
        uint32_t operandoOP = byte1 << 8 | byte2;
        guardaRegistroOP(tipo, operandoOP, tipoOP_AB);

    }else if(tipo == OP_MEM) { // Operando de memoria (3 bytes)
        byte1 = MemoriaPrincipal[ip_aux];
        byte2 = MemoriaPrincipal[ip_aux + 1];
        byte3 = MemoriaPrincipal[ip_aux + 2];
        (*ip) += 3; ip_aux += 3;

        //Determinar tamanio de acceso: (para version 2)
        //uint8_t mod=byte3 & 0x03;
        //switch(mod){
        //    case MOD_LONG: *tam=4; break;
        //    case MOD_WORD: *tam=2; break;
        //    case MOD_BYTE: *tam=1; break;
        //}

        uint8_t codReg = byte1 & 0x1F; // Extraer posicion del registro que guarda la memoria
        uint16_t offsetReg = (Registros[codReg]) & 0xFFFF; // Extraer el offset del registro
        uint16_t offset = (byte2 << 8) | byte3; // Ensambla el offset de 16 bits
        // Calcular direccion logica
        uint32_t base = Registros[codReg] >>16; // Extraer el segmento
        uint32_t direccionLogica = (base << 16) | (offset+offsetReg); // Ensambla la direccion logica

        operando = direccionLogica; //Devuelve DIRECCION LOGICA

        //guardar en OP1 u OP2
        uint32_t operandoOP = byte1 << 16 | byte2 << 8 | byte3;
        guardaRegistroOP(tipo, operandoOP, tipoOP_AB);
    }else {
        detectaError(COD_ERR_OPE,tipo);
        return 0;
    }

    return operando;
}

int32_t obtenerValorOperando(uint8_t tipoOp, uint32_t operando, uint8_t tamanio){
    int32_t valor = 0;

    if (tipoOp == OP_REG) {
        uint8_t numReg = operando & 0x1F;
        valor =(int32_t) obtenerValorRegistro(numReg);

    } else if (tipoOp == OP_INM) {
                valor = (int32_t)operando;
    } else if (tipoOp == OP_MEM) {
        uint32_t direccionFisica = calculaDireccionFisica(operando);
        valor = (int32_t) leerMemoria(direccionFisica, tamanio);    

        //guardo en LAR la direccion logica
        Registros[POS_LAR] = operando; 
        // guardo en la parte alta del MAR la cantidad de bytes
        Registros[POS_MAR] = tamanio << 16;
        //guardo en la parte baja del MAR la direccion fisica
        Registros[POS_MAR]= (Registros[POS_MAR] & 0xFFFF0000) | (direccionFisica & 0x0000FFFF);
        //guardo en MBR el valor leido 
        Registros[POS_MBR] = valor; 

    } else {
        detectaError(COD_ERR_OPE, tipoOp);
        return 0;
    }
    return valor;
}

void escribirValorOperando(uint8_t tipoOp, uint32_t operando, int32_t valor, uint8_t tamA){

    if(tipoOp == OP_REG) {
        uint8_t numReg = operando & 0x1F;
        escribirEnRegistro(numReg, valor);
    } else if(tipoOp == OP_MEM) {
        uint32_t direccionFisica = calculaDireccionFisica(operando);
        escribirMemoria(direccionFisica, valor, tamA);
        
        //guardo en LAR la direccion logica
        Registros[POS_LAR] = operando; 
        // guardo en la parte alta del MAR la cantidad de bytes
        Registros[POS_MAR] = tamA << 16;
        //guardo en la parte baja del MAR la direccion fisica
        Registros[POS_MAR]= (Registros[POS_MAR] & 0xFFFF0000) | (direccionFisica & 0x0000FFFF);
        //guardo en MBR el valor leido 
        Registros[POS_MBR] = valor; 
    } else {
        detectaError(COD_ERR_OPE,tipoOp);
    }
}

//----------------FUNCIONES DE INSTRUCCIONES--------------------
void ejecutarMOV(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA,uint8_t tamB) {
    int32_t valor;

    valor = obtenerValorOperando(tipoB, operandoB, tamB);
    escribirValorOperando(tipoA, operandoA, valor, tamA);
    // MOV no afecta el registro CC
}

void ejecutarADD(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;
    // Obtener valor del operando B (fuente)
    valorB = obtenerValorOperando(tipoB, operandoB, tamB);

    // Obtener valor del operando A (destino)
    valorA = obtenerValorOperando(tipoA, operandoA, tamA);

    // Verificar overflow
    if ((valorB > 0 && valorA > INT32_MAX - valorB) || (valorB < 0 && valorA < INT32_MIN - valorB)) {
        detectaError(COD_ERR_OVF,0x0);
        return;
    }
    // Sumar los valores
    resultado = valorA + valorB;

    //Guardo resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarSUB(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;
    //fuente
    valorB = obtenerValorOperando(tipoB, operandoB, tamB);

    //destino
    valorA = obtenerValorOperando(tipoA, operandoA, tamA);

    // Verificar overflow
    if ((valorB > 0 && valorA < INT32_MIN + valorB) || (valorB < 0 && valorA > INT32_MAX + valorB)) {
        detectaError(COD_ERR_OVF,0x00);
        return;
    }

    // Restar los valores
    resultado = valorA - valorB;
    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado, tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarMUL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t resultado, valorA, valorB;
    //Fuente
    valorB = obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = obtenerValorOperando(tipoA, operandoA,tamA);

    // Verificar overflow
    if ((valorB > 0 && valorA > INT32_MAX / valorB) || (valorB < 0 && valorA < INT32_MIN / valorB)) {
        detectaError(COD_ERR_OVF,0x00);
        return;
    }
    // Multiplicar los valores
    resultado = valorA * valorB;
    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    actualizarCC(resultado); // Actualizar el registro de condicion (CC)
}

void ejecutarDIV(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t resultado, resto, valorA, valorB;

    //Fuente
    valorB = obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = obtenerValorOperando(tipoA, operandoA,tamA);

    // Verificar division por cero
    if (valorB == 0) {
        detectaError(COD_ERR_DIV,0x00); // Error en la division
        return;
    }
    // Dividir los valores
    resultado = valorA / valorB;
    resto = valorA % valorB; // Obtener el resto de la division

    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);

    // Guardar el resto en el registro (AC)
    Registros[POS_AC] = resto;
}

void ejecutarCMP(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;
    //SUB no guarda el valor en ningun registro ni memoria, solo actualiza el CC

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA,tamA);

    // Restar los valores
    resultado = valorA - valorB;
    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarSHL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB,tamB);

    // Verificar que el valor B sea un desplazamiento valido
    if (valorB < 0 || valorB > 31) {
        detectaError(COD_ERR_OPE,0x00);
        return;
    }

    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA,tamA);

    //Desplazar a la izquierda el valor A
    resultado = valorA << valorB;

    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarSHR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB, tamB);

    //Verificar que el valor B sea un desplazamiento valido
    if (valorB < 0 || valorB > 31) {
        detectaError(COD_ERR_OPE, 0x0); // Error en el desplazamiento
        return;
    }

    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA, tamA);

    //Desplazar a la derecha el valor A
    resultado = valorA >> valorB;
    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado, tamA);

    //Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

//void ejecutarSAR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {

void ejecutarAND(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA,tamA);

    //Realizar la operacion AND
    resultado = valorA & valorB;
    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarOR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA,tamA);

    //Realizar la operacion OR
    resultado = valorA | valorB;

    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    //Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

void ejecutarXOR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB, resultado;

    //Fuente
    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB,tamB);
    //Destino
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA,tamA);

    //Realizar la operacion XOR
    resultado = valorA ^ valorB;

    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado,tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}


void ejecutarSWAP(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB){
    uint32_t valorA, valorB;
    //Verificar que los tamanios son compatibles
    if(tamA != tamB){
        detectaError(COD_ERR_OPE,0x0);
        return;
    }

    // Obtener valor del operando A
    valorA = obtenerValorOperando(tipoA, operandoA,tamA);
    // Obtener valor del operando B
    valorB = obtenerValorOperando(tipoB, operandoB,tamB);

    // Intercambiar los valores
    escribirValorOperando(tipoA, operandoA, valorB,tamB);
    escribirValorOperando(tipoB, operandoB, valorA,tamA);

    //SWAP no afecta el registro CC
}

void ejecutarLDL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB;

    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB, tamB);
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA, tamA);

    int32_t resultado = (valorB & 0xFFFF) | (valorA & 0xFFFF0000);

    //Guardo el valor
    escribirValorOperando(tipoA, operandoA, resultado, tamA);

}

void ejecutarLDH(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB, uint8_t tamA, uint8_t tamB) {
    int32_t valorA, valorB;

    valorB = (int32_t)obtenerValorOperando(tipoB, operandoB, tamB);
    valorA = (int32_t)obtenerValorOperando(tipoA, operandoA, tamA);

    int32_t resultado = ((valorB & 0xFFFF) <<16 ) | (valorA & 0xFFFF);

    escribirValorOperando(tipoA, operandoA, resultado, tamA);
}

void ejecutarRND(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB, uint8_t tamA, uint8_t tamB) {
    int32_t max;

    //Fuente
    max = (int32_t)obtenerValorOperando(tipoB, operandoB, tamB);

    // Generar un numero aleatorio entre 0 y valorB
    int32_t resultado = rand() % (max + 1);

    //Guardar resultado
    escribirValorOperando(tipoA, operandoA, resultado, tamA);
}

void ejecutarJMP(uint8_t tipoA, uint32_t operandoA, uint8_t tamA) {
    // Salta a la direccion especificada en el operando A
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA, tamA);

    if(direccionSalto != 0xFFFFFFFF) {
        Registros[POS_IP] = direccionSalto; // Actualizar el IP con la direccion de salto
    }
    else return;
}

void ejecutarJZ(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    // Salta a la direccion especificada en el operando A si el CC es cero
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA,tamA);

    if((Registros[POS_CC] & CC_Z) && direccionSalto!= 0xFFFFFFFF) {
        Registros[POS_IP] = direccionSalto;
    }
}

void ejecutarJP(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    // Salta a la direccion especificada en el operando A si el Z=0 y N=0
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA,tamA);

    if(!(Registros[POS_CC] & (CC_N | CC_Z)) && direccionSalto!= 0xFFFFFFFF) {
        Registros[POS_IP] = direccionSalto;
    }else return;
}

void ejecutarJN(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    // Salta a la direccion especificada en el operando A si el CC es negativo
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA,tamA);

    if((Registros[POS_CC] & CC_N) && direccionSalto!= 0xFFFFFFFF) {
        Registros[POS_IP] = direccionSalto;
    }
}

void ejecutarJNZ(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
// Salta a la direccion especificada si el resultado es distinta de cero
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA,tamA);
    uint32_t valorCC = Registros[POS_CC]; // Obtener el valor del registro de condicion
    uint32_t cero= valorCC & CC_Z; // Obtener el valor del registro de condicion

    if(cero==00 && direccionSalto!= 0xFFFFFFFF) {
        Registros[POS_IP] = direccionSalto;
    }
}

void ejecutarJNP(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    // Salta a la direccion especificada si el resultado es <= cero
    uint32_t valorCC = Registros[POS_CC]; // Obtener el valor del registro de condicion
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA,tamA);

    if(((valorCC & CC_N) || (valorCC & CC_Z)) && direccionSalto != 0xFFFFFFFF){
        Registros[POS_IP] = direccionSalto;
    }
}

void ejecutarJNN(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    // Salta a la direccion especificada si el resultado es >= cero
    uint32_t valorCC = Registros[POS_CC]; // Obtener el valor del registro de condicion
    uint32_t direccionSalto = calculaDireccionSalto(tipoA, operandoA, tamA);

    if(!(valorCC & CC_N) && direccionSalto != 0xFFFFFFFF){
        Registros[POS_IP] = direccionSalto;
    }
}

void ejecutarNOT(uint8_t tipoA, uint32_t operandoA, uint8_t tamA){
    int32_t resultado, valor;

    valor = (int32_t)obtenerValorOperando(tipoA, operandoA, tamA);
    resultado = ~valor;

    escribirValorOperando(tipoA, operandoA, resultado, tamA);

    // Actualizar el registro de condicion (CC)
    actualizarCC(resultado);
}

//LLAMADA A SYS
void writeSYS() {
    uint32_t EDX = Registros[POS_EDX];
    uint16_t tamanio = (Registros[POS_ECX] >> 16) & 0xFF; // Tamaño de cada elemento
    uint16_t cantidad = Registros[POS_ECX] & 0xFF;       // Cantidad de elementos
    uint32_t EAX = Registros[POS_EAX];       // Modo de salida

    uint32_t dirFisica = calculaDireccionFisica(EDX);

    // Verificación de parámetros
    if ( tamanio!= 1 && tamanio!= 2 && tamanio!= 4) {
        detectaError(COD_ERR_WRITE, tamanio);
        return;
    }

    if (dirFisica + (tamanio*cantidad) > TAMANIO_MEMORIA) {
        detectaError(COD_ERR_FIS, dirFisica + (tamanio * cantidad));
        return;
    }

    for (int i = 0; i < cantidad; i++) {
        uint32_t dirActual = dirFisica + (i * tamanio);
        printf("[%04X]:", dirActual);

        uint32_t valor = leerMemoria(dirActual, tamanio);

        // Mostrar hexadecimal
        if (EAX & 0x08) {
            printf(" 0x%X", valor);
        }

        // Mostrar octal
        if (EAX & 0x04) {
            printf(" 0o%o", valor);
        }

        // Mostrar caracteres
        if (EAX & 0x02) {
            printf(" ");
            // Leer y mostrar los bytes en orden de memoria
            for (int j = 0; j < tamanio; j++) {
                uint8_t c = MemoriaPrincipal[dirActual+j];
                if (c >= 32 && c <= 126) {
                    printf("%c", c);
                } else {
                    printf(".");
                }
            }
        }

        // Mostrar decimal
        if (EAX & 0x01) {
            printf(" ");
            switch (tamanio) {
                case 1: printf("%d", (int8_t)valor); break;
                case 2: printf("%d", (int16_t)valor); break;
                case 4: printf("%d", (int32_t)valor); break;
            }
        }
        printf("\n");
    }
}

void readSYS() {
    uint32_t EDX = Registros[POS_EDX], dirFisica;
    int32_t valor;
    uint16_t tamanio = Registros[POS_ECX] >> 16 & 0xFF; // Tamaño de cada elemento
    uint16_t cantidad = Registros[POS_ECX] & 0xFF;        // Cantidad de elementos
    uint32_t EAX = Registros[POS_EAX];        // Tipo de entrada
    char binario[MAX];

    dirFisica = calculaDireccionFisica(EDX);
    if (dirFisica + (tamanio * cantidad) > TAMANIO_MEMORIA) {
        detectaError(COD_ERR_READ, EAX);
        return;
    }

    for (int i = 0; i < cantidad; i++) {
        int intentoValido = 0,intento=0;
        uint32_t direccionMemoria = dirFisica + (i * tamanio);
        while(intento<3 && !intentoValido){
            printf("[%04X]: ", direccionMemoria);
            fflush(stdout); // Asegura que se muestre el prompt

            switch (EAX) {
                case 0x10: { // BINARIO
                        if (scanf("%s", binario) == 1) {
                            valor = 0;
                            int j = 0;
                            while (binario[j] == '0' || binario[j] == '1') {
                                valor = (valor << 1) | (binario[j] - '0');
                                j++;
                            }
                            intentoValido=1;
                        }
                    break;
                }
                case 0x01: { // DECIMAL
                        if (scanf("%d", &valor) == 1) {
                            valor = (uint32_t)valor;
                            intentoValido=1;
                        }
                    break;
                }
                case 0x02: { // CARÁCTER
                        valor = getchar();
                        if (valor != '\n') intentoValido=1;
                    break;
                }
                case 0x04: { // OCTAL
                        if (scanf("%o", &valor) == 1) intentoValido=1;;
                    break;
                }
                case 0x08: { // HEX
                        if (scanf("%X", &valor) == 1) intentoValido=1;
                    break;
                }
                default:
                    detectaError(COD_ERR_READ, EAX);
                    return;
            }
            while (getchar() != '\n'); // limpiar buffer de entrada

            if (!intentoValido && intento < 2) {
                    printf("Entrada inválida. Reintente.\n");
            }
            intento++;
        }
        if (!intentoValido) {
            detectaError(COD_ERR_READ, EAX);
            return;
        }

        escribirMemoria(direccionMemoria, valor, tamanio);
    }
}

void ejecutarSYS(uint32_t operandoA){
    switch(operandoA){
        case SYS_READ:{
            readSYS(); // Leer de memoria
            break;
        }
        case SYS_WRITE:{
            writeSYS(); // Escribir en memoria
            break;
        }
        default:{
            detectaError(COD_ERR_OPE, 0x0);
            return;
        }
    }
}

uint16_t convertirBigEndian16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

uint32_t convertirBigEndian32(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

//-------------------FUNCIONES DE EJECUCION-------------------------
int ejecutarInstruccion(){
    uint8_t posCS = Registros[POS_CS] >> 16;
    uint32_t offsetIP = Registros[POS_IP] & 0xFFFF;

    // Verificar si IP está dentro del code segment
    if (offsetIP >= tablaSegmentos[posCS].tamanio) {
        continuarEjecucion = 0;
        return 0;
    }

    uint32_t direccionFisica = calculaDireccionFisica(Registros[POS_IP]); // Calcular la direccion fisica

    if(direccionFisica > TAMANIO_MEMORIA){
        detectaError(COD_ERR_FIS,direccionFisica);
        return 1;
    }

    uint8_t codigo = leerMemoria(direccionFisica, 1);
    uint8_t codOp = codigo & 0x1F;
    uint8_t cantOperandos = (codigo >> 4) & 0x01;
    uint8_t tipoA = OP_NING, tipoB =OP_NING, tamA=0, tamB=0;

    uint32_t operandoA=0, operandoB=0;
    // Decodifico el codigo de operacion y tipo de operandos

    Registros[POS_IP]++; // IP apunta a siguiente instruccion
    Registros[POS_OPC] = codOp;

    if((codOp!= OP_STOP) ){ // agregar &&(codOp!=OP_RET) para version 2
        if(cantOperandos == 0x01){
            tipoB = (codigo >> 6) & 0x03;
            tipoA = (codigo >> 4) & 0x03;
            operandoB = obtenerOperando(tipoB, &Registros[POS_IP], &tamB, 2);
            operandoA = obtenerOperando(tipoA, &Registros[POS_IP], &tamA, 1);
        }else{
            tipoA = (codigo >> 6) & 0x03;
            operandoA = obtenerOperando(tipoA, &Registros[POS_IP], &tamA, 1);
            Registros[POS_OP2] = 0; // No hay operando B
        }
    }else {
        Registros[POS_OP1] = 0; // No hay operando A
        Registros[POS_OP2] = 0; // No hay operando B
    }

    // Ejecuta la instruccion
    switch(codOp){
        case OP_MOV:
            ejecutarMOV(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_ADD:
            ejecutarADD(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SUB:
            ejecutarSUB(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SWAP:
            ejecutarSWAP(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_MUL:
            ejecutarMUL(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_DIV:
            ejecutarDIV(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_CMP:
            ejecutarCMP(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SHL:
            ejecutarSHL(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SHR:
            ejecutarSHR(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SAR: 
            //ejecutarSAR(tipoA, operandoA, tipoB, operandoB, tamA, tamB);  
            break;
        case OP_AND:
            ejecutarAND(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_OR:
            ejecutarOR(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_XOR:
            ejecutarXOR(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_LDL:
            ejecutarLDL(tipoA, operandoA,tipoB, operandoB, tamA, tamB);
            break;
        case OP_LDH:
            ejecutarLDH(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_RND:
            ejecutarRND(tipoA, operandoA, tipoB, operandoB, tamA, tamB);
            break;
        case OP_SYS:
            ejecutarSYS(operandoA);
            break;
        case OP_JMP:
            ejecutarJMP(tipoA, operandoA, tamA);
            break;
        case OP_JZ:
            ejecutarJZ(tipoA, operandoA, tamA);
            break;
        case OP_JP:
            ejecutarJP(tipoA, operandoA, tamA);
            break;
        case OP_JN:
            ejecutarJN(tipoA, operandoA, tamA);
            break;
        case OP_JNZ:
            ejecutarJNZ(tipoA, operandoA, tamA);
            break;
        case OP_JNP:
            ejecutarJNP(tipoA, operandoA, tamA);
            break;
        case OP_JNN:
            ejecutarJNN(tipoA, operandoA, tamA);
            break;
        case OP_NOT:{
            ejecutarNOT(tipoA, operandoA, tamA);
            break;
        }
        case OP_STOP:{
            Registros[POS_IP] = -1;
            continuarEjecucion = 0; // Detener la ejecucion
            break;
        }
        default:{
            detectaError(COD_ERR_INS,codOp); // Error en la instruccion
            return 1;
        }
    }
    return 0;
}

int ejecutarPrograma () {
    while(continuarEjecucion ){
        if(ejecutarInstruccion()!=0)
            return 1;
    }
    return 0;
}

//---------------FUNCIONES PARA DISASSEMBLER---------------
// Funcion auxiliar para determinar tamanio del operando
int operandoSize(uint8_t tipo) {
    switch (tipo) {
        case OP_REG: return 1;
        case OP_INM: return 2;
        case OP_MEM: return 3;
        default: return 0;
    }
}

void decodificarOperando(uint32_t punt, int operandoSize,uint8_t codOp) {
uint8_t numReg, codReg;
uint32_t valor;
int32_t offset;

switch (operandoSize) {
    case 1: { //Registro
        numReg = MemoriaPrincipal[punt] & 0x1F;
        printf("%-s", NOMBRES_REGISTROS[numReg]);
        break;
    }
    case 2: { //Inmediato
        valor = (MemoriaPrincipal[punt] << 8) | MemoriaPrincipal[punt+1];
        if (valor & 0x8000) {
                valor |= 0xFFFF0000;
        }
        if(codOp==OP_JMP || codOp==OP_JN || codOp==OP_JNN || codOp==OP_JNP || codOp==OP_JNZ || codOp==OP_JP || codOp==OP_JZ){
             printf("%04X", valor);
        }else{
            printf("%-4d", (int32_t)valor);
        }
        break;
    }
    case 3: { //Memoria
        offset = (MemoriaPrincipal[punt+1] << 8) | MemoriaPrincipal[punt + 2];
        // Extender el signo para el offset
        if (offset & 0x8000) {
            offset |= 0xFFFF0000;
        }
        codReg = MemoriaPrincipal[punt]& 0x1F;

        //if(versionPrograma==2){ (para version 2)
        //    uint8_t modMem = MemoriaPrincipal[punt+2] & 0x03;
        //    switch(modMem){ 
        //        case MOD_BYTE: printf("b"); break;
        //        case MOD_WORD: printf("w"); break;
        //        case MOD_LONG: printf("l"); break;
        //    }
        //}

        printf("[");
        if (codReg < NUM_REGISTROS && NOMBRES_REGISTROS[codReg][0] != '\0') {
            printf("%s", NOMBRES_REGISTROS[codReg]);
            if (offset != 0) {
                printf("%+d", offset);
            }
        }else{
            printf("%d", offset);
        }
        printf("]");
        break;
    }
    default: printf("?? ");break;
}
}


void disassemblerInstruccion(uint32_t *ip) {
    uint32_t ip0=(*ip)+1, offsetA, offsetB;
    uint8_t codigo = MemoriaPrincipal[*ip];
    uint8_t codOp = codigo & 0x1F;
    uint8_t tipoA = 0, tipoB = 0;
    char bytesStr[32] = ""; //Buffer para los bytes de la instruccion
    int bytesLen = 0;

    if(versionPrograma==1) {// Imprimir direccion
        printf("[%04X] ", *ip);
    }
    //} else if(versionPrograma==2){
    //    printf("[%04X] ", *ip - tablaSegmentos[Registros[POS_CS] >> 16].base);
    //}

    //Bytes de la instruccion
    bytesLen += sprintf(bytesStr + bytesLen, "%02X", codigo);

    // Determinar formato de instruccion
    if(codOp == OP_STOP){ // Instruccion sin operandos // agregar || codOp == OP_RET para version 2
        printf("%-24s | %-6s", bytesStr, MNEMONICOS[codOp]);
        (*ip)++;
    }
    else if (codOp <= OP_NOT) {// Instruccion con 1 operando (modificar para version 2)
        tipoA = (codigo >> 6) & 0x03;
        // Imprimir bytes
        for (int i = 0; i < operandoSize(tipoA); i++) {
            bytesLen += sprintf(bytesStr + bytesLen, " %02X", MemoriaPrincipal[(*ip)+1+i]);
        }
        printf("%-24s | %-6s ", bytesStr, MNEMONICOS[codOp]); // Imprimir operando A
        decodificarOperando(ip0, operandoSize(tipoA),codOp);
        (*ip) = operandoSize(tipoA)+ip0;
    }
    else {// Instruccion con 2 operandos
        tipoB = (codigo >> 6) & 0x03;
        tipoA = (codigo >> 4) & 0x03;

        //Agregar bytes de la instruccion B
        for (int i = 0; i < operandoSize(tipoB); i++) {
            bytesLen += sprintf(bytesStr + bytesLen," %02X", MemoriaPrincipal[(*ip) +1+i]);
        }
        //Agregar bytes de la instruccion A
        for (int i = 0; i < operandoSize(tipoA); i++) {
            bytesLen += sprintf(bytesStr + bytesLen," %02X", MemoriaPrincipal[(*ip) + operandoSize(tipoB) +1+i]);
        }
        printf("%-24s | %-6s", bytesStr, MNEMONICOS[codOp]);

        // Imprimir operandos
        offsetB = ip0;
        offsetA = ip0 + operandoSize(tipoB);
        decodificarOperando(offsetA, operandoSize(tipoA),codOp);
        printf(", ");
        decodificarOperando(offsetB, operandoSize(tipoB),codOp);
        (*ip) = offsetA + operandoSize(tipoA);
    }
    printf("\n");
}

void disassembleProgramaMV1() {
    uint32_t ip = 0;
    uint32_t tamCod = tablaSegmentos[SEG_CS].tamanio;

    printf("Maquina Virtual MV1 - Desensamblado\n");
    printf("Tamanio del codigo: %d bytes\n\n", tamCod);

    while (ip < tamCod) {
        disassemblerInstruccion(&ip);
    }
}

void muestraDesensamblador(uint8_t version){
    switch(version){
        case 1:
            disassembleProgramaMV1();
            break;
        //case 2:
        //    disassembleProgramaMV2();
        //    break;
        default:
            printf("Error: Version de desensamblador no soportada\n");
            break;
    }
}
