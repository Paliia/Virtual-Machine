#ifndef MV_H_INCLUDED
#define MV_H_INCLUDED
#include <stdint.h>
#include <stdio.h>

//------------------CONSTANTES Y ESTRUCTURAS----------
// Cantidad de registros
#define NUM_REGISTROS 32
//Tamanio de la tabla de descriptores de segmentos
#define NUM_SEG 8

//Indices de los segmentos (solo para version 1 tal vez)
#define SEG_CS 0
#define SEG_DS 1

//Indices de los registros
#define POS_LAR 0
#define POS_MAR 1
#define POS_MBR 2
#define POS_IP 3
#define POS_OPC 4
#define POS_OP1 5
#define POS_OP2 6
#define POS_SP 7
#define POS_BP 8
//reservados del 7 al 9
#define POS_EAX 10
#define POS_EBX 11
#define POS_ECX 12
#define POS_EDX 13
#define POS_EEX 14
#define POS_EFX 15
#define POS_AC 16
#define POS_CC 17
//reservados del 18 al 25
#define POS_CS 26
#define POS_DS 27
#define POS_ES 28
#define POS_SS 29
#define POS_KS 30
#define POS_PS 31
//reservados del 28 al 31

// Bits del registro CC
#define CC_N 0x80000000  // Bit de signo (Negative)
#define CC_Z 0x40000000  // Bit de cero (Zero)

//Codigos de operacion
#define OP_MOV 0x10
#define OP_ADD 0x11
#define OP_SUB 0x12
#define OP_MUL 0x13
#define OP_DIV 0x14
#define OP_CMP 0x15
#define OP_SHL 0x16
#define OP_SHR 0x17 
#define OP_SAR 0x18
#define OP_AND 0x19
#define OP_OR 0x1A
#define OP_XOR 0x1B
#define OP_SWAP 0x1C
#define OP_LDL 0x1D
#define OP_LDH 0x1E
#define OP_RND 0x1F 

#define OP_SYS 0x00
#define OP_JMP 0x01
#define OP_JZ 0x02
#define OP_JP 0x03
#define OP_JN 0x04
#define OP_JNZ 0x05
#define OP_JNP 0x06
#define OP_JNN 0x07
#define OP_NOT 0x08

#define OP_STOP 0x0F

//Tipo de operandos
#define OP_NING 0b00
#define OP_REG 0b01
#define OP_INM 0b10
#define OP_MEM 0b11

//Llamadas al sistema
#define SYS_READ 0x01
#define SYS_WRITE 0x02

//Codigos de ERROR
#define COD_ERR_INS 0
#define COD_ERR_DIV 1
#define COD_ERR_FIS 2
#define COD_ERR_REG 3
#define COD_ERR_OPE 4
#define COD_ERR_READ 5
#define COD_ERR_WRITE 6
#define COD_ERR_OVF 7
#define COD_ERR_LOG 8
#define COD_ERR_MEM_INS 9
#define COD_ERR_SEGMENT 12

//maxima cant de bits para SYS read
#define MAX 32

//---Estructuras para VM---//
typedef struct{
    uint16_t base;
    uint16_t tamanio;
} DescriptoresSegmentos;

typedef struct{ //HEADER VERSION 1
  char identificador[5]; // "VMX25"
  uint8_t version;    // 1
  uint16_t tamanio;
} VMXHeaderV1;

//-------------DECLARO VARIABLES GLOBALES QUE ESTAN EN OTRO ARCHIVO---------------
//-------------EXTERN---------------
// Memoria principal
extern uint8_t *MemoriaPrincipal;
extern uint32_t TAMANIO_MEMORIA;
// Tabla de Registros
extern uint32_t Registros[NUM_REGISTROS];
extern DescriptoresSegmentos tablaSegmentos[NUM_SEG];

//-------------FUNCION PARA DETECCION DE ERROR---------------
void detectaError(int8_t cod, int32_t er);

//-------------DECLARACIONES DE FUNCIONES PARA VIRTUAL MACHINE---------------
void inicializaMemoria();

//-------------FUNCIONES DE MEMORIA---------------
uint32_t calculaDireccionFisica(uint32_t direccionLogica);
int32_t leerMemoria(uint32_t direccionFisica, uint8_t tamanio);
void escribirMemoria(uint32_t direccionFisica, int32_t valor, uint8_t tamanio);

//-------------INICIALIZACIONES PARA VERSION 1---------------
void inicializaSegmentosV1(uint16_t tamanioCodigo);
void inicializaTablaRegistrosV1();

//-------------LECTURA DEL ARCHIVO---------------
//-------------CARGA PROGRAMA PARA VERSION 1---------------
int cargaProgramaV1(FILE *arch);

//-------------CARGA PROGRAMA SEGUN LA VERSION---------------
int cargaPrograma(const char *nombreArchivo);

//-------------FUNCIONES DE REGISTROS---------------
int verificaRegistro(uint8_t numReg);
int32_t obtenerValorRegistro(uint8_t numReg);
void escribirEnRegistro(uint8_t numReg, int32_t valor);
uint32_t calculaDireccionSalto(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void actualizarCC(int32_t resultado);

//-------------FUNCIONES DE OPERANDOS---------------
void guardaRegistroOP(uint8_t tipo, uint32_t operando, uint8_t tipoOP_AB);
uint32_t obtenerOperando(uint8_t tipo, unsigned int *ip, uint8_t *tam, uint8_t tipoOP);
int32_t obtenerValorOperando(uint8_t tipoOp, uint32_t operando, uint8_t tamanio);
void escribirValorOperando(uint8_t tipoOp, uint32_t operando, int32_t valor,uint8_t tamA);

//-------------FUNCIONES DE INSTRUCCIONES---------------
void ejecutarMOV(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA,uint8_t tamB);;
void ejecutarADD(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarSUB(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarMUL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarDIV(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarCMP(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarSHL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarSHR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarSAR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarAND(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarOR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarXOR(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarSWAP(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarLDL(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarLDH(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarRND(uint8_t tipoA, uint32_t operandoA, uint8_t tipoB, uint32_t operandoB,uint8_t tamA, uint8_t tamB);
void ejecutarJMP(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJZ(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJP(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJN(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJNZ(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJNP(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarJNN(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);
void ejecutarNOT(uint8_t tipoA, uint32_t operandoA,uint8_t tamA);

//-------------LLAMADA A SYS---------------
void writeSYS();
void readSYS();

uint16_t convertirBigEndian16(uint16_t val);
uint32_t convertirBigEndian32(uint32_t val);
int ejecutarInstruccion();
void mostrarMenu(char *op);
void ejecutarSYS(uint32_t operandoA);

//-------------FUNCIONES DE EJECUCION---------------
int ejecutarPrograma ();

//-------------FUNCIONES PARA DISASSEMBLER---------------
// Tabla de mnemonicos para las instrucciones
static const char* MNEMONICOS[] = {
    [OP_SYS] = "SYS",    [OP_JMP] = "JMP",    [OP_JZ] = "JZ",     [OP_JP] = "JP",
    [OP_JN] = "JN",      [OP_JNZ] = "JNZ",    [OP_JNP] = "JNP",   [OP_JNN] = "JNN",
    [OP_NOT] = "NOT",    [OP_STOP] = "STOP",  [OP_MOV] = "MOV",   [OP_ADD] = "ADD",
    [OP_SUB] = "SUB",    [OP_MUL] = "MUL",    [OP_DIV] = "DIV",   [OP_CMP] = "CMP",    
    [OP_SHL] = "SHL",    [OP_SHR] = "SHR",    [OP_SAR] = "SAR",   [OP_AND] = "AND",
    [OP_OR] = "OR",      [OP_XOR] = "XOR",    [OP_SWAP] = "SWAP", [OP_LDL] = "LDL",   
    [OP_LDH] = "LDH",    [OP_RND] = "RND"
};

//-------------Nombres de los registros---------------
static const char* NOMBRES_REGISTROS[] = { "LAR", "MAR", "MBR", "IP", "OPC", "OP1", "OP2", "-", "-", "-", 
    "EAX", "EBX", "ECX", "EDX", "EEX", "EFX", "AC", "CC", "-", "-", "-", "-", "-", "-", "-", "-",
    "CS", "DS", "-", "-", "-", "-"
};

// Funcion auxiliar para determinar tamanio del operando
int operandoSize(uint8_t tipo);

void decodificarOperando(uint32_t punt, int operandoSize,uint8_t codOp);

void disassemblerInstruccion(uint32_t *ip);

void disassembleProgramaMV1();

void muestraDesensamblador(uint8_t version);

#endif // MV_H_INCLUDED