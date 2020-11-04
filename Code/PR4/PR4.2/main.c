#include <stdio.h>
#include "44b.h"
#include "leds.h"
#include "utils.h"
#include "D8Led.h"
#include "intcontroller.h"
#include "timer.h"
#include "gpio.h"
#include "keyboard.h"
#include "uart.h"

#define N 4 //Tamaño del buffer tmrbuffer
#define M 128 //Tamaño del buffer readlineBuf que se pasa como parametro a la rutina readline

/* Variables para la gestion de la ISR del teclado
 *
 * Keybuffer: puntero que apunta al buffer en el que la ISR del teclado debe
 *            almacenar las teclas pulsadas
 * keyCount: variable en el que la ISR del teclado almacena el numero de teclas pulsadas
 */
volatile static char *keyBuffer = NULL;
volatile static int keyCount = 0;

/* Variables para la gestion de la ISR del timer
 *
 * tmrbuffer: puntero que apunta al buffer que contendra los digitos que la ISR del
 *            timer debe mostrar en el display de 8 segmentos
 * tmrBuffSize: usado por printD8Led para indicar el tamaño del buffer a mostrar
 */
volatile static char *tmrBuffer = NULL;
volatile static int tmrBuffSize = 0;

//Variables globales para la gestion del juego
static char passwd[N];  //Buffer para guardar la clave inicial
static char guess[N];   //Buffer para guardar la segunda clave
char readlineBuf[M];    //Buffer para guardar la linea leida del puerto serie

//Configuracion de la uart
struct ulconf uconf = {
	.ired = OFF,
	.par  = ONE,
	.wordlen = EIGHT,
	.echo = ON,
	.baud    = 115200,
};

enum state {
	INIT = 0,     //Init:       Inicio del juego
	SPWD = 1,     //Show Pwd:   Mostrar password
	DOGUESS = 2,  //Do guess:   Adivinar contraseña
	SGUESS = 3,   //Show guess: Mostrar el intento
	GOVER = 4     //Game Over:  Mostrar el resultado
};
enum state gstate; //estado/fase del juego

//COMPLETADO: Declaracion adelantada de las ISRs de timer y teclado (las marca como ISRs)
void timer_ISR(void) __attribute__ ((interrupt ("IRQ")));
void keyboard_ISR(void) __attribute__ ((interrupt ("IRQ")));

// Funcion que va guardando las teclas pulsadas
static void push_buffer(char *buffer, int key)
{
	int i;
	for (i=0; i < N-1; i++)
		buffer[i] = buffer[i+1];
	buffer[N-1] = (char) key;
}

void timer_ISR(void)
{
	static int pos = 0; //contador para llevar la cuenta del digito del buffer que toca mostrar

    //COMPLETADO: Visualizar el digito en la posicion pos del buffer tmrBuffer en el display
	D8Led_digit(tmrBuffer[pos]);

	//  COMPLETADO
	//	Si es el ultimo digito:
	//      Poner pos a cero,
	//      Parar timerh
	//      Dar tmrBuffer valor NULL
	if(pos == tmrBuffSize-1){
		pos = 0;
		tmr_stop(TIMER0);
		tmrBuffer = NULL;
	}
	// Si no, se apunta al siguiente dígito a visualizar (pos)
	else{
		pos++;
	}

	// COMPLETADO: Finalizar correctamente la ISR
	ic_cleanflag(INT_TIMER0);
}

void printD8Led(char *buffer, int size)
{
	//Esta rutina prepara el buffer que debe usar timer_ISR (tmrBuffer)
	tmrBuffer = buffer;
	tmrBuffSize = size;

	//COMPLETADO: Arrancar el TIMER0
	tmr_start(TIMER0);

	//COMPLETADO: Esperar a que timer_ISR termine (tmrBuffer)
	while(tmrBuffer != NULL);
}

void keyboard_ISR(void)
{
	int key;

	/* Eliminar rebotes de presion */
	Delay(200);

	/* Escaneo de tecla */
	// COMPLETADO
	key = kb_scan();

	if (key != -1) {
		//COMPLETADO:
		//Si la tecla pulsada es F deshabilitar interrupciones por teclado y
		//poner keyBuffer a NULL
		if(key == 15){
			ic_disable(INT_EINT1);
			keyBuffer = NULL;
		}
		else{
			// Si la tecla no es F guardamos la tecla pulsada en el buffer apuntado
			// por keybuffer mediante la llamada a la rutina push_buffer
			push_buffer(keyBuffer, key);
			// Actualizamos la cuenta del número de teclas pulsadas
			keyCount++;
		}
		/* Esperar a que la tecla se suelte, consultando el registro de datos rPDATG */
		while (!(rPDATG & 0x02));
	}

	/* Eliminar rebotes de depresion */
	Delay(200);

	//COMPLETADO: Finalizar correctamente la ISR
	ic_cleanflag(INT_EINT1);
}

int read_kbd(char *buffer)
{
	//Esta rutina prepara el buffer en el que keyboard_ISR almacenara las teclas
	//pulsadas (keyBuffer) y pone a 0 el contador de teclas pulsadas
	keyBuffer = buffer;
	keyCount = 0;

	//COMPLETADO: Habilitar interrupciones por teclado
	ic_enable(INT_EINT1);

	//COMPLETADO: Esperar a que keyboard_ISR indique que se ha terminado de
	//introducir la clave (keyBuffer)
	while(keyBuffer != NULL);

	//COMPLETADO: Devolver numero de teclas pulsadas
	return keyCount;
}

int readline(char *buffer, int size)
{
	int count = 0; //cuenta del numero de bytes leidos
	char c;        //variable para almacenar el caracter leido

	if (size == 0)
		return 0;

	// COMPLETADO: Leer caracteres de la uart0 y copiarlos al buffer
	// hasta que llenemos el buffer (size) o el caracter leido sea
	// un retorno de carro '\r'
	// Los caracteres se leen de uno en uno, utilizando la interfaz
	// del modulo uart, definida en el fichero uart.h

	uart_getch(UART0, &c);

	while(count < size && c != '\r')
	{
		if(c != '\n')
		{	buffer[count] = c;
			count++;}

		uart_getch(UART0, &c);
	}

	return count;
}

static int show_result()
{
	int error = 0;
	int i = 0;
	char buffer[2] = {0};

	// COMPLETADO: poner error a 1 si las contraseñas son distintas
	while(i < N && error != 1){
		if(passwd[i] != guess[i]) error = 1;
		i++;
	}

	//COMPLETADO
	// Hay que visualizar el resultado durante 2s.
	// Si se ha acertado tenemos que mostrar una A y si no una E
	// Como en printD8Led haremos que la ISR del timer muestre un buffer con dos
	// caracteres A o dos caracteres E (eso durará 2s)
	if(error == 0){ // si ha acertado
		buffer[0] = 10;
		buffer[1] = 10;
		uart_printf(UART0, "\nCorrecto\n");
	}
	else if(error == 1){ // si ha fallado
		buffer[0] = 14;
		buffer[1] = 14;
		uart_printf(UART0, "\nError\n");
	}
	printD8Led(buffer, 2);


	// MODIFICADO el codigo de la parte1 para que ademas de mostrar A o E en el
	// display de 8 segmentos se envie por el puerto serie uart0 la cadena "\nCorrecto\n"
	// o "\nError\n" utilizando el interfaz del puerto serie definido en uart.h

	// COMPLETADO: esperar a que la ISR del timer indique que se ha terminado
	while(tmrBuffer != NULL);

	// COMPLETADO: Devolver el valor de error para indicar si se ha acertado o no
	return error;
}

int setup(void)
{
	D8Led_init();

	/* COMPLETADO: Configuración del timer0 para interrumpir cada segundo */
	tmr_set_prescaler(0, 255);
	//tmr_set_divider(0, D1_8);
	//tmr_set_count(TIMER0, 31250, 1);
	tmr_set_divider(0, D1_4);
	tmr_set_count(TIMER0, 62500, 1);
	tmr_update(TIMER0);
	//tmr_start(TIMER0);// No hijo mío!!
	tmr_set_mode(TIMER0, RELOAD);

	/********************************************************************/

	// COMPLETADO: Registramos las ISRs
	pISR_TIMER0 = timer_ISR;
	pISR_EINT1 = keyboard_ISR;

	/* Configuración del controlador de interrupciones*/
	ic_init();
	 /* Habilitamos la línea IRQ, en modo vectorizado y registramos una ISR para
		 * la línea IRQ
		 * Configuramos el timer 0 en modo IRQ y habilitamos esta línea
		 * Configuramos la línea EINT1 en modo IRQ y la habilitamos
		 */
	ic_enable(INT_GLOBAL);//Para desenmascarar toidas las lineas
	ic_conf_irq(ENABLE, VEC);
	ic_conf_fiq(DISABLE);
	ic_conf_line(INT_TIMER0, IRQ);
	ic_conf_line(INT_EINT1, IRQ);
	ic_enable(INT_TIMER0);
	ic_enable(INT_EINT1);

	/***************************************************/

	/***************************************************/
	//COMPLETAR: Configuracion de la uart0

		/* Hay que:
		 * 1. inicializar el modulo
		 * 2. Configurar el modo linea de la uart0 usando la variable global uconf
		 * 3. Configurar el modo de recepcion (POLL o INT) de la uart0
		 * 4. Configurar el modo de transmision (POLL o INT) de la uart0
		 */

	uart_init();
	uart_lconf(UART0, &uconf);
	uart_conf_rxmode(UART0, INT);
	uart_conf_txmode(UART0, INT);

	/***************************************************/

	Delay(0);

	/* Inicio del juego */
	gstate = INIT;
	D8Led_digit(12);

	return 0;
}

static char ascii2digit(char c)
{
	char d = -1;

	if ((c >= '0') && (c <= '9'))
		d = c - '0';
	else if ((c >= 'a') && (c <= 'f'))
		d = c - 'a' + 10;
	else if ((c >= 'A') && (c <= 'F'))
		d = c - 'A' + 10;

	return d;
}


int loop(void)
{
	int count; //numero de teclas pulsadas
	int error;

	//Maquina de estados

	switch (gstate) {
		case INIT:
			do {
				//COMPLETADO:
    			//Visualizar una C en el display
     			//Llamar a la rutina read_kbd para guardar los 4 dígitos en el buffer passwd
     			//Esta rutina devuelve el número de teclas pulsadas.
				//Dibujar una E en el display si el número de teclas pulsadas es menor que 4
				D8Led_digit(12);
				count = read_kbd(passwd);
				if(count < N){
					D8Led_digit(14);
					Delay(5000);
				}

			} while (count < N);
			//COMPLETADO: Pasar al estado siguiente
			gstate = SPWD;

			break;

		case SPWD:

			// COMPLETADO:
			// Visualizar en el display los 4 dígitos del buffer passwd, para
			// ello llamar a la rutina printD8Led
			Delay(10000);
			printD8Led(passwd, N);

			//COMPLETADO: Pasar al estado siguiente
			gstate = DOGUESS;
			break;

		case DOGUESS:
			Delay(10000);
			do {
				//COMPLETAR:
				/*
				 * 1. Mandar por el puerto serie uart0 la cadena "Introduzca passwd: "
				 *    usando el interfaz definido en uart.h
				 *
				 * 2. Mostrar una F en el display
				 *
				 * 3. Llamar a la rutina readline para leer una linea del puerto
				 *    serie en el buffer readlineBuf, almacenando en count el
				 *    valor devuelto (numero de caracteres leidos)
				 *
				 * 4. Si el �ltimo caracter leido es un '\r' decrementamos count
				 *    para no tenerlo en cuenta
				 *
				 * 5. Si count es menor de 4 la clave no es valida, mostramos
				 *    una E (digito 14) en el display de 8 segmentos y esperamos
				 *    1 segundo con Delay.
				 */

				uart_send_str(UART0, "Introduzca passwd: ");
				D8Led_digit(15);
				count = readline(readlineBuf, M);

				if(count < N)
				{
					D8Led_digit(14);
					Delay(5000);
				}

			} while (count < N);

			/* COMPLETADO: debemos copiar los 4 ultimos caracteres de readline en
			 * el buffer guess, haciendo la conversion de ascii-hexadecimal a valor
			 * decimal. Para ello podemos utilizar la funcion ascii2digit
			 * definida mas arriba.
			 */
			int i;
			for(i = 1; i <= N; i++)
				guess[N-i] = ascii2digit(readlineBuf[count-i]);


			//COMPLETADO: Pasar al estado siguiente
			gstate = SGUESS;

			break;

		case SGUESS:
			//COMPLETADO:
			//Visualizar en el display los 4 dígitos del buffer guess,
			//para ello llamar a la rutina printD8Led
			Delay(10000);
			printD8Led(guess, N);
			//COMPLETADO: Pasar al estado siguiente
			gstate = GOVER;

			break;

		case GOVER:
			//COMPLETADO:
			//Mostrar el mensaje de acierto o error con show_result()
			Delay(10000);
			int resultado = show_result();
			//Si he acertado el estado siguiente es INIT sino DOGUESS
			if(resultado == 0) gstate = INIT;
			else gstate = DOGUESS;

			break;
	}
	return 0;
}

int main(void)
{
	setup();

	while (1) {
		loop();
	}
}
