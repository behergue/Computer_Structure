#include <stdio.h>
#include "44b.h"
#include "button.h"
#include "leds.h"
#include "utils.h"
#include "D8Led.h"
#include "intcontroller.h"
#include "timer.h"
#include "gpio.h"
#include "keyboard.h"

struct RLstat {
	int moving;
	int speed;
	int direction;
	int position;
};

static struct RLstat RL = {
	.moving = 0,
	.speed = 5,
	.direction = 0,
	.position = 0,
};

void timer_ISR(void) __attribute__ ((interrupt ("IRQ")));
void button_ISR(void) __attribute__ ((interrupt ("IRQ")));
//void keyboard_ISR(void) __attribute__ ((interrupt ("IRQ")));

void timer_ISR(void)
{
	//COMPLETAR: tomar el código de avance de posición del led rotante de la práctica anterior
	if (RL.direction == 1) // Hacia derecha en array Segments.
	{
		if (RL.position + 1 < 6)
		{
			RL.position += 1;
		}
		else
		{
			RL.position = 0;
		}
	}
	else
	{
		if (RL.position - 1 >= 0)
		{
			RL.position -= 1;
		}
		else
		{
			RL.position = 5;
		}
	}

	D8Led_segment(RL.position);
}

void button_ISR(void)
{
	unsigned int whicheint = rEXTINTPND;
	unsigned int buttons = (whicheint >> 2) & 0x3;

	//COMPLETAR: usar el código de la primera parte parte de atención a los
	//pulsadores

	if (buttons & ~BUT1) {
		// COMPLETADO: utilizando la interfaz para los leds definida en leds.h
		// hay que apagar ambos leds
		// Tambi�n hay que comutar la direcci�n del movimiento del led rotante
		// representado por el campo direction de la variable RL

		led1_off();
		led2_off();

		if(RL.direction == 0)
		{
			RL.direction = 1;
		}
		else
		{
			RL.direction = 0;
		}
	}

	if (buttons & ~BUT2) {
		// COMPLETADO: utilizando la interfaz para los leds definida en leds.h
		// Incrementar contador de pulsaciones. Si es par, conumtar led1. Si es impar, conmutar el led2.
		// Tambi�n hay que comutar el estado de movimiento del led rotante
		// representado por el campo moving de la variable RL, y en caso de
		// ponerlo en marcha debemos reiniciar el campo iter al valor del campo
		// speed.

		led1_switch();

		led2_switch();


		if (RL.moving == 0)
		{
			RL.moving= 1;
		}
		else
		{
			RL.moving= 0;
		}
	}

	// eliminamos rebotes
	Delay(2000);
	// borramos el flag en extintpnd
	rEXTINTPND &= ~(0x03 << 2);
			// COMPLETADO: debemos borrar las peticiones de interrupción en
		    // EXTINTPND escribiendo un 1 en los flags que queremos borrar (los
			// correspondientes a los pulsadores pulsados)
}

//void keyboard_ISR(void)
//{
//	int key;
//
//	/* Eliminar rebotes de presión */
//	Delay(200);
//	
//	/* Escaneo de tecla */
//	key = kb_scan();
//
//	if (key != -1) {
//		/* Visualizacion en el display */
//		//COMPLETAR: mostrar la tecla en el display utilizando el interfaz
//		//definido en D8Led.h
//
//		switch (key) {
//			case 0:
//				//COMPLETAR: poner en timer0 divisor 1/8 y contador 62500
//				break;
//			case 1:
//				//COMPLETAR: poner en timer0 timer divisor 1/8 y contador 31250
//				break;
//			case 2:
//				//COMPLETAR: poner en timer0 timer divisor 1/8 y contador 15625
//				break;
//			case 3:
//				//COMPLETAR: poner en timer0 timer divisor 1/4 y contador 15625
//				break;
//			default:
//				break;
//		}
//		
//		/* Esperar a que la tecla se suelte, consultando el registro de datos */		
//		while (/*COMPLETAR: true si está pulsada la tecla (leer del registro rPDATG)*/);
//	}
//
//    /* Eliminar rebotes de depresión */
//    Delay(200);
//     
//    /* Borrar interrupciones pendientes */
//	//COMPLETAR
//	//borrar la interrupción por la línea EINT1 en el registro rI_ISPC
//}

int setup(void)
{
	ic_enable(INT_GLOBAL);

	leds_init();
	D8Led_init();
	D8Led_segment(RL.position);

	/* Port G: configuración para generación de interrupciones externas,
	 *         botones y teclado
	 **/

	//COMPLETAR: utilizando el interfaz para el puerto G definido en gpio.h
	//configurar los pines 1, 6 y 7 del puerto G para poder generar interrupciones
	//externas por flanco de bajada por ellos y activar las correspondientes
	//resistencias de pull-up.

	portG_conf(6, EINT);
	portG_conf(7, EINT);
	portG_conf(1, EINT);

	portG_conf_pup(6, ENABLE);
	portG_conf_pup(7, ENABLE);
	portG_conf_pup(1, ENABLE);
	/********************************************************************/

	/* Configuración del timer */
	tmr_set_prescaler(TIMER0, 255);
	tmr_set_divider(8, TIMER0);
	tmr_set_count(TIMER0, 62500, 1);

	if (RL.moving)
		tmr_start(TIMER0);
	/***************************/

	// Registramos las ISRs
	pISR_TIMER0   = timer_ISR; // COMPLETADO: registrar la RTI del timer
	pISR_EINT4567 = button_ISR; // COMPLETADO: registrar la RTI de los botones
	//pISR_EINT1    = keyboard_ISR; // COMPLETADO: registrar la RTI del teclado

	/* Configuración del controlador de interrupciones
	 * Habilitamos la línea IRQ, en modo vectorizado 
	 * Configuramos el timer 0 en modo IRQ y habilitamos esta línea
	 * Configuramos la línea EINT4567 en modo IRQ y la habilitamos
	 * Configuramos la línea EINT1 en modo IRQ y la habilitamos
	 */
	ic_conf_irq(ENABLE, VEC);
	ic_conf_fiq(DISABLE);
	ic_conf_line(INT_TIMER0, IRQ);
	ic_conf_line(INT_EINT4567, IRQ);
	ic_conf_line(INT_EINT1, IRQ);
	ic_enable(INT_TIMER0);
	ic_enable(INT_EINT4567);
	ic_enable(INT_EINT1);

	ic_init();
	//COMPLETAR: utilizando el interfaz definido en intcontroller.h
	//		habilitar la línea IRQ en modo vectorizado
	//		deshabilitar la línea FIQ
	//		configurar la línea INT_TIMER0 en modo IRQ
	//		configurar la línea INT_EINT4567 en modo IRQ
	//		configurar la línea INT_EINT1 en modo IRQ
	//		habilitar la línea INT_TIMER0
	//		habilitar la línea INT_EINT4567
	//		habilitar la línea INT_EINT1
	ic_conf_irq(ENABLE, VEC);
	ic_conf_fiq(DISABLE);
	ic_conf_line(INT_TIMER0, IRQ);
	ic_conf_line(INT_EINT4567, IRQ);
	ic_conf_line(INT_EINT1, IRQ);
	ic_enable(INT_TIMER0);
	ic_enable(INT_EINT4567);
	ic_enable(INT_EINT1);
	/***************************************************/

	Delay(0);
	return 0;
}

int loop(void)
{
	return 0;
}


int main(void)
{
	setup();

	while (1) {
		loop();
	}
}
