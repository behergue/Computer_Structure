#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "44b.h"
#include "uart.h"
#include "intcontroller.h"

#define BUFLEN 100

// Estructura utilizada para mantener el estado de cada puerto
struct port_stat {
	enum URxTxMode rxmode;       //Modo de recepcion (DIS, POLL, INT, DMA)
	enum URxTxMode txmode;       //Modo de envio     (DIS, POLL, INT, DMA)
	unsigned char ibuf[BUFLEN];  //Buffer de recepcion (usado en modo INT)
	int rP;                      //Puntero de lectura en ibuf (modo INT)
	int wP;                      //Puntero de escritura en ibuf (modo INT)
	char *sendP;                 //Puntero a la cadena de envio (modo INT)
	enum ONOFF echo;             //Marca si el puerto debe hacer eco de los caracteres recibidos
};

static struct port_stat uport[2]; //Array con el estado de los puertos

// COMPLETADO: Declaracion adelantada de las rutinas de tratamiento de
// interrupcion de la uart por linea IRQ (las marca como ISRs)
// Las rutinas son: Uart0_RxInt, Uart0_TxInt, Uart1_RxInt, Uart1_TxInt
void Uart0_RxInt(void) 	__attribute__ ((interrupt ("IRQ")));
void Uart0_TxInt(void) 	__attribute__ ((interrupt ("IRQ")));
void Uart1_RxInt(void) 	__attribute__ ((interrupt ("IRQ")));
void Uart1_TxInt(void) 	__attribute__ ((interrupt ("IRQ")));


void uart_init(void)
{
	int i;

	// Inicializacion de las estructuras de estado de los puertos
	for (i=0; i < 2; i++) {
		uport[i].rxmode = DIS;
		uport[i].txmode = DIS;
		uport[i].rP = 0;
		uport[i].wP = 0;
		uport[i].sendP = NULL;
		uport[i].echo = OFF;
	}

	//COMPLETADO: Registrar adecuadamente las rutinas de tratamiento de
	//interrupcion de la uart
	pISR_UTXD1 = Uart1_TxInt;
	pISR_URXD1 = Uart1_RxInt;
	pISR_UTXD0 = Uart0_TxInt;
	pISR_URXD0 = Uart0_RxInt;

	//COMPLETADO: Configurar las lineas de interrupcion de la uart en modo IRQ
	ic_conf_line(INT_URXD0, IRQ);
	ic_conf_line(INT_URXD1, IRQ);
	ic_conf_line(INT_UTXD0, IRQ);
	ic_conf_line(INT_UTXD1, IRQ);
}
	
/* uart_lconf: Esta funcion configura el modo linea de la uart,
 *       Numero de bits por trama
 *       Numero de bits de parada
 *       Paridad
 *       Modo infrarrojos
 *       Baudios
 * y configura los pines adecuados para que las lineas Rx y Tx de los puertos
 * salgan fuera del chip, hacia los conectores DB9 de la placa
 */
int uart_lconf(enum UART port, struct ulconf *lconf)
{
	unsigned int confvalue = 0; // valor de configuracion del registro ULCON
	int baud;
		
	// COMPLETADO: darle a confvalue el valor adecuado en funcion de la
	// configuracion deseada (parametro lconf)

	// Configuracion de longitud de palabra
	if(lconf->wordlen == FIVE); 		// xxxxx00
	else if(lconf->wordlen == SIX)	    // xxxxx01
		confvalue |= 0x01;
	else if(lconf->wordlen == SEVEN)	// xxxxx10
		confvalue |= 0x02;
	else								// xxxxx11
		confvalue |= 0x03;

	// Configuracion de numero de bits de parada
	if(lconf->stopb == TWO)				// xxxx1xx
		confvalue |= 0x04;

	// Configuracion del modo de paridad
	if(lconf->par == NONE);
	else if(lconf->par == ODD)
		confvalue |= (0x04 << 3);
	else if(lconf->par == EVEN)
		confvalue |= (0x05 << 3);
	else if(lconf->par == FONE)
		confvalue |= (0x06 << 3);
	else if(lconf->par == FZERO)
		confvalue |= (0x07 << 3);

	// Configuracion del modo infrarrojos
	if(lconf->ired == ON)
		confvalue |= (0x01 << 6);

	baud = (int)( 64000000 /(16.0 * lconf->baud) + 0.5) - 1;

	switch (port) {
		case UART0:
			rULCON0 = confvalue;
			rUBRDIV0 = baud;
			// habilitamos la salida fuera del chip de las seÒales RxD0 y TxD0
			rPCONE = (rPCONE & ~(0xF << 2)) | (0x2 << 2) | (0x2 << 4);
			break;

		case UART1:
			rULCON1 = confvalue;
			rUBRDIV1 = baud;
			// habilitamos la salida fuera del chip de las seÒales RxD1 y TxD1
			rPCONC = rPCONC  | (0xF << 24);
			break;

		default:
			return -1;
	}

	uport[port].echo = lconf->echo;

	return 0;
}

/* uart_conf_txmode: funcion que configura el modo de transmision del puerto */
int uart_conf_txmode(enum UART port, enum URxTxMode mode)
{
	int conf = 0; //variable para modo POLL/INT o DMA

	if (mode < 0 || mode > 3)
		return -1;

	if (port < 0 || port > 1)
		return -1;

	switch (mode) {
		case POLL:
		case INT:
			conf = 1;
			break;
		case DMA:
			conf = (port == UART0) ? 2 : 3;
			break;
		default:
			conf = 0;
	}
		

	switch (port) {
		case UART0:
			//COMPLETADO: modo indicado por conf, Tx interrupt por nivel
			rUCON0 &= ~(0x03 << 2);
			rUCON0 |=  (conf << 2);
			rUCON0 |=  (0x01 << 9);
			break;

		case UART1:
			//COMPLETADO: modo indicado por conf, Tx interrupt por nivel
			rUCON1 &= ~(0x03 << 2);
			rUCON1 |=  (conf << 2);
			rUCON1 |=  (0x01 << 9);
			break;
	}

	uport[port].txmode = mode;

	return 0;
}

/* uart_conf_rxmode: funcion que configura el modo de recepcion del puerto
*/
int uart_conf_rxmode(enum UART port, enum URxTxMode mode)
{
	int conf = 0; //variable para modo POLL/INT o DMA

	if (mode < 0 || mode > 3)
		return -1;

	if (port < 0 || port > 1)
		return -1;

	switch (mode) {
		case POLL:
		case INT:
			conf = 1;
			break;
		case DMA:
			conf = (port == UART0) ? 2 : 3;
			break;
		default:
			conf = 0;
	}

	switch (port) {
		case UART0:
			//COMPLETADO: modo indicado por conf, Rx interrupt por pulso

			rUCON0 &= ~(0x03);
			rUCON0 |=  (conf);
			rUCON0 &= ~(0x01 << 8);

			//COMPLETADO: si se el modo es por interrupciones habilitar la linea
			//de interrupcion por recepcion en el puerto 0
			if(mode == INT)
				ic_enable(INT_URXD0);

			break;

		case UART1:
			//COMPLETADO: modo indicado por conf, Rx interrupt por pulso

			rUCON1 &= ~(0x03);
			rUCON1 |=  (conf);
			rUCON1 &= ~(0x01 << 8);

			//COMPLETADO: si se el modo es por interrupciones habilitar la linea
			//de interrupcion por recepcion en el puerto 1
			if(mode == INT)
				ic_enable(INT_URXD1);

			break;
	}

	uport[port].rxmode = mode;

	return 0;
}

/* uart_rx_ready: funcion que realiza un espera activa hasta que el puerto haya
 * recibido un byte
 */
static void uart_rx_ready(enum UART port)
{
	switch (port) {
		case UART0:
			//COMPLETADO: esperar a que la uart0 haya recibido un dato (UTRSTAT0,
			//Receive Buffer Data Ready)
			while(!(rUTRSTAT0 & 0x01));
			break;

		case UART1:
			//COMPLETADO: esperar a que la uart1 haya recibido un dato (UTRSTAT1,
			//Receive Buffer Data Ready)
			while(!(rUTRSTAT1 & 0x01));
			break;
	}
}

/* uart_tx_ready: funcion que realiza un espera activa hasta que se vacie el
 * buffer de transmision del puerto
 */
static void uart_tx_ready(enum UART port)
{
	switch (port) {
		case UART0:
			//COMPLETADO: esperar a que se vacie el buffer de transmision de la
			//uart0 (UTRSTAT0, Transmit Buffer Empty)
			while(!(rUTRSTAT0 & 0x02));
			break;

		case UART1:
			//COMPLETADO: esperar a que se vacie el buffer de transmision de la
			//uart1 (UTRSTAT1, Transmit Buffer Empty)
			while(!(rUTRSTAT1 & 0x02));
			break;
	}
}

/* uart_write: funcion que escribe un byte en el buffer de transmision del
 * puerto
 */
static void uart_write(enum UART port, char c)
{
	if (port == UART0)
		//COMPLETADO: Escribir el caracter c en el puerto 0, usar la macro WrUTXH0
		WrUTXH0(c);
	else
		//COMPLETADO: Escribir el caracter c en el puerto 1, usar la macro WrUTXH1
		WrUTXH1(c);
}
 
/* uart_read: funcion que lee un byte del buffer (registro) de recepcion del
 * puerto, y hace el eco del caracter si el puerto tiene el eco activado
 */
static char uart_read(enum UART port)
{
	char c;

	if (port == UART0)
		//COMPLETADO: Leer un byte del puerto 0, usar la macro RdUTXH0
		c = RdURXH0();
	else
		//COMPLETADO: Leer un byte del puerto 1, usar la macro RdUTXH1
		c = RdURXH1();

	if (uport[port].echo == ON) {
		//COMPLETADO: Esperar a que el puerto este listo para transmitir
		//COMPLETADO: Escribir el caracter leido (c) en el puerto port

		uart_tx_ready(port);
		uart_write(port,c);
	}
	return c;
}

/* uart_readtobuf: funcion invocada por la ISR de recepcion. Su mision es
 * escribir el caracter recibido en el buffer de reccepcion del puerto (campo
 * ibuf de la estructura port_stat correspondiente)
 */
static void uart_readtobuf(enum UART port)
{
	char c;
	struct port_stat *pst = &uport[port];

	/* COMPLETAR:
	 * 1. Leer un byte del puerto y copiarlo en el buffer de reccepcion del
	 *    puerto en la posicion indicada por el puntero de escritura.
	 *
	 * 2. Incrementar el puntero de escritura y si es necesario corregir su
	 *    valor para que esta siempre en el rango 0 - BUFLEN-1 (gestionado de
	 *    forma circular)
	 */

	c = uart_read(port);

	pst->ibuf[pst->wP] = c;
	pst->wP++;

	if(pst->wP == BUFLEN)
		pst->wP = 0;
}

/* uart_readfrombuf: funcion invocada por uart_getch en el caso de que el puerto
 * esta configurado por interrupciones para la recepcion. Su mision es esperar a
 * que al menos haya un byte en el buffer de recepcion, y entonces sacarlo del
 * buffer y devolverlo como byte leido.
 */
static char uart_readfrombuf(enum UART port)
{
	struct port_stat *pst = &uport[port];

	/* COMPLETAR:
	 * 1. Corregir (de forma circular) el valor del puntero de lectura si esta
	 *    fuera del rango 0 - BUFLEN-1.
	 * 2. Esperar a que el buffer de recepcion contenga algun byte.
	 * 3. Extraer el primer byte y devolverlo (el byte se devuelve y el puntero
	 *    de lectura se deja incrementado, con lo que el byte queda fuera del
	 *    buffer)
	 */

	while(pst->rP == pst->wP);

	char c = pst->ibuf[pst->rP];

	pst->rP++;
	if(pst->rP == BUFLEN)
		pst->rP = 0;

	return c;
}

/* ISR de recepcion por el puerto 0 */
void Uart0_RxInt(void)
{
	uart_readtobuf(UART0);
	
	//COMPLETADO: borrar el flag de interrupcion por recepcion en el puerto 0
	ic_cleanflag(INT_URXD0);
}

/* ISR de recepcion por el puerto 1 */
void Uart1_RxInt(void)
{
	uart_readtobuf(UART1);
	
	//COMPLETADO: borrar el flag de interrupcion por recepcion en el puerto 1
	ic_cleanflag(INT_URXD1);
}

/* uart_dotxint: rutina invocada por la ISR de transmision. Su mision es enviar
 * el siguiente byte del buffer de transmision, apuntado por el campo sendP de
 * la estructura port_stat asociada al puerto, y si la transmision ha finalizado
 * desactivar las interrupciones de envio y seÒalizar el final del envio
 * poniendo el puntero sendP a NULL.
 */
static void uart_dotxint(enum UART port)
{
	struct port_stat *pst = &uport[port];

	if (*pst->sendP != '\0' ) {
		if (*pst->sendP == '\n') {
			/* Para que funcione bien con los terminales windows, vamos a hacer
			 * la conversion de \n por \r\n, por tanto enviamos un caracter \r
			 * extra en este caso
			 */
			//COMPLETADO: enviar \r y esperar a que el puerto quede libre para
			//enviar
			uart_write(port, '\r');
			uart_tx_ready(port);
		}
		//COMPLETADO: enviar el car·cter apuntado por sendP e incrementar dicho
		//puntero
		uart_write(port, *pst->sendP);
		pst->sendP++;
	}

	if (*pst->sendP == '\0') {
		//COMPLETADO: si hemos llegado al final de la cadena de caracteres
		// deshabilitamos la linea de interrupcion por transmision del puerto
		// y ponemos el puntero sendP a NULL

		pst->sendP = NULL;

		if(port == UART0)
			ic_disable(INT_UTXD0);
		else
			ic_disable(INT_UTXD1);
	}
}

/* ISR de transmision por el puerto 0 */
void Uart0_TxInt(void)
{
	uart_dotxint(UART0);
	
	//COMPLETADO: borrar el flag de interrupcion por transmision en el puerto 0
	ic_cleanflag(INT_UTXD0);
}

/* ISR de transmision por el puerto 1 */
void Uart1_TxInt(void)
{
	uart_dotxint(UART1);
	
	//COMPLETADO: borrar el flag de interrupcion por transmision en el puerto 1
	ic_cleanflag(INT_UTXD1);
}


/* uart_getch: funcion bloqueante (sincrona) para la recepcion de un byte por el
 * puerto serie
 */
int uart_getch(enum UART port, char *c)
{
	if (port < 0 || port > 1)
		return -1;

	switch (uport[port].rxmode) {
		case POLL:
			// COMPLETADO: Esperar a que el puerto port haya recibido un byte
			// Leer dicho byte y escribirlo en la direcci√≥n apuntada por c
			uart_rx_ready(port);
			*c = uart_read(port);
			break;

		case INT:
			// COMPLETADO: Leer el primer byte del buffer de recepcion del puerto
			// y copiarlo en la direccion apuntada por c
			*c = uart_readfrombuf(port);
			break;

		case DMA:
			//OPCIONAL
			return -1;
			break;

		default:
			return -1;
	}

	return 0;
}

/* uart_sendch: funcion bloqueante (sincrona) para la transmision de un byte por el
 * puerto serie
 */
int uart_sendch(enum UART port, char c)
{
	char localB[2] = {0};

	if (port < 0 || port > 1)
		return -1;

	switch (uport[port].txmode) {
		case POLL:
			/* COMPLETADO:
			 * 1. Esperar a que el puerto esta listo para transmitir un byte
			 * 2. Si el byte es \n enviamos primero \r y volvemos a esperar a
			 *    que esta listo para transmitir
			 * 3. Enviamos el caracter c por el puerto
			 */
			if(c == '\n')
				uart_sendch(port, '\r');

			uart_tx_ready(port);
			uart_write(port, c);

			break;

		case INT:
			localB[0] = c;
			uart_send_str(port, localB);
			break;

		case DMA:
			//OPCIONAL
			return -1;
			break;

		default:
			return -1;
	}

	return 0;
}

/* uart_send_str: funcion bloqueante (sincrona) para la transmision de una
 * cadena de caracteres por el puerto serie
 */
int uart_send_str(enum UART port, char *str)
{
	int line;

	if (port < 0 || port > 1)
		return -1;

	switch (uport[port].txmode) {
		case POLL:
			//COMPLETADO: usar uart_sendch para enviar todos los bytes de la
			//cadena apuntada por str
			while(*str != '\0'){
				uart_sendch(port, *str);
				str++;
			}
			break;

		case INT:
			/* COMPLETAR:
			 * 1. Hacer que el puntero del buffer de envio (campo sendP en la
			 *    estructura port_stat del puerto) apunte al comienzo de la
			 *    cadena str.
			 * 2. Habilitar las interrupciones por transmision en el puerto
			 * 3. Esperar a que se complete el env√≠o (la ISR pondra a NULL el
			 *    puntero de envio sendP)
			 */
			uport[port].sendP = str;

			if(port == UART0)
				ic_enable(INT_UTXD0);
			else
				ic_enable(INT_UTXD1);

			while(uport[port].sendP != NULL);

			break;

		case DMA:
			//TODO
			return -1;
			break;

		default:
			return -1;
	}

	return 0;

}

/* uart_printf: funcion bloqueante (sincrona) para la transmision de una
 * cadena de caracteres con formato por el puerto serie
 */
void uart_printf(enum UART port, char *fmt, ...)
{
    va_list ap;
    char str[256];

    va_start(ap, fmt);
    vsnprintf(str, 256, fmt, ap);
    uart_send_str(port, str);
    va_end(ap);
}

