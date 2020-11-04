/*-------------------------------------------------------------------
**
**  Fichero:
**    intcontroller.c  3/3/2016
**
**    Estructura de Computadores
**    Dpto. de Arquitectura de Computadores y Automática
**    Facultad de Informática. Universidad Complutense de Madrid
**
**  Propósito:
**    Contiene las implementación del módulo intcontroller
**
**-----------------------------------------------------------------*/

/*--- ficheros de cabecera ---*/
#include "44b.h"
#include "intcontroller.h"

void ic_init(void)
{
	/* Configuración por defector del controlador de interrupciones:
	 *    Lineas IRQ y FIQ no habilitadas
	 *    Linea IRQ en modo no vectorizado
	 *    Todo por la línea IRQ
	 *    Todas las interrupciones enmascaradas
	 **/
	rINTMOD = 0x0; // Configura las linas como de tipo IRQ
	rINTCON = 0x7; // IRQ y FIQ enmascaradas, IRQ en modo no vectorizado
	rINTMSK = ~(0x0); // Enmascara todas las lineas
}

int ic_conf_irq(enum enable st, enum int_vec vec)
{
	int conf = rINTCON;

	if (st != ENABLE && st != DISABLE)
		return -1;

	if (vec == VEC)
	{
		// COMPLETADO: 
		// Poner la linea IRQ en modo vectorizado
		unsigned int b = 1;
		b <<= 2;

		conf &= ~b;
	}
	else
	{
		//COMPLETADO: 
		// Poner la linea IRQ en modo no vectorizado
		unsigned int b = 1;
		b <<= 2;

		conf |= b;
	}

	if (st == ENABLE)
	{
		// COMPLETADO: 
		// Habilitar la linea IRQ
		unsigned int b = 1;
		b <<= 1;

		conf &= ~b;
	}
	else
	{
		// COMPLETADO: 
		// Deshabilitar la linea IRQ
		unsigned int b = 1;
		b <<= 1;

		conf |= b;
	}

	rINTCON = conf;
	return 0;
}

int ic_conf_fiq(enum enable st)
{
	int ret = 0;

	if (st == ENABLE)
	{
		// COMPLETADO: 
		// Habilitar la linea FIQ
		unsigned int b = 1;

		ret &= ~b;
	}
	else if (st == DISABLE)
	{
		// COMPLETADO: 
		// Deshabilitar la linea FIQ
		unsigned int b = 1;

		ret |= b;
	}
	else
	{
		ret = -1;
	}

	return ret;
}

int ic_conf_line(enum int_line line, enum int_mode mode)
{
	unsigned int bit = INT_BIT(line);

	if (line < 0 || line > 26)
		return -1;

	if (mode != IRQ && mode != FIQ)
		return -1;

	if (mode == IRQ)
	{
		// COMPLETADO: poner la linea line en modo IRQ
		unsigned int b = 1;
		b <<= bit;
		
		rINTMOD &= ~b;
	}
	else
	{
		// COMPLETADO: poner la linea line en modo FIQ
		unsigned int b = 1;
		b <<= bit;
		
		rINTMOD |= b;
	}

	return 0;
}

int ic_enable(enum int_line line)
{
	if (line < 0 || line > 26)
		return -1;

	// COMPLETADO: habilitar las interrupciones por la linea line
	unsigned int b = 1;
	b <<= (int) line;
	
	rINTMSK &= ~b;

	return 0;
}

int ic_disable(enum int_line line)
{
	if (line < 0 || line > 26)
		return -1;

	// COMPLETADO: enmascarar las interrupciones por la linea line
	unsigned int b = 1;
	b <<= (int) line;
	
	rINTMSK |= b;
	
	return 0;
}

int ic_cleanflag(enum int_line line)
{
	int bit;

	if (line < 0 || line > 26)
		return -1;

	bit = INT_BIT(line);

	if (rINTMOD & bit)
	{
		// COMPLETADO
		// Borrar el flag de interrupcion correspondiente a la linea line
		// con la linea configurada por FIQ
		unsigned int b = 1;
		b <<= bit;

		rI_ISPC |= b;
	}
	else
	{
		//COMPLETAR: borrar el flag de interrupcion correspondiente a la linea line
		//con la linea configurada por IRQ
		unsigned int b = 1;
		b <<= bit;

		rF_ISPC |= b;
	}

	return 0;
}



