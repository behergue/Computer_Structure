#include "44b.h"
#include "gpio.h"
#include <math.h>

/* Port B interface implementation */

int portB_conf(int pin, enum port_mode mode)
{
	int ret = 0;
	if (pin < 0 || pin > 10)
	{
		return -1; // indica error
	}

	if (mode == SIGOUT)
	{
		// COMPLETADO: poner en rPCONB el bit indicado por pin a 1 para que por
		// dicho pin en el puerto B salga la seÃ±al correspondiente del
		// controlador de memoria

		unsigned int b = 1; // 0x0000...0001
		b <<= pin;			// 0x000...00100...00

		rPCONB |= b;
	}
	else if (mode == OUTPUT)
	{
		// COMPLETADO: poner en rPCONB el bit indicado por pin a 0 para que dicho
		// pin sea un pin de salida

		unsigned int b = 1; // 0x0000...0001
		b <<= pin;			// 0x000...00100...00

		rPCONB &= (~b);
	}
	else
	{
		ret = -1; // indica error
	}

	return ret;
}

int portB_write(int pin, enum digital val)
{
	if (pin < 0 || pin > 10)
	{
		return -1; // indica error
	}

	if (val < 0 || val > 1)
	{
		return -1; // indica error
	}

	if (val)
	{
		// COMPLETAR: poner en rPDATB el bit indicado por pin a 1

		unsigned int b = 1; // 0x0000...0001
		b <<= pin;			// 0x000...00100...00

		rPDATB |= b;
	}
	else
	{
		// COMPLETAR: poner en rPDATB el bit indicado por pin a 0

		unsigned int b = 1; // 0x0000...0001
		b <<= pin;			// 0x000...00100...00

		rPDATB &= (~b);
	}

	return 0;
}

/* Port G interface implementation */

int portG_conf(int pin, enum port_mode mode)
{
	int pos  = pin*2;

	if (pin < 0 || pin > 7)
		return -1; // indica error

	switch (mode) {
		case INPUT:
		{
			// COMPLETADO: poner en rPCONG 00 a partir de la posiciÃ³n pos para
			// configurar como pin de entrada el pin indicado por el parÃ¡metro pin

			unsigned int bb = 3; // 0x0000...0011
			bb <<= pos; // 0x00...0110...00

			rPCONG &= (~bb);
			break;
		}
		case OUTPUT:
		{
			// COMPLETADO: poner en rPCONG 01 a partir de la posiciÃ³n pos para
			// configurar como pin de salida el pin indicado por el parÃ¡metro pin

			unsigned int b1 = 1; //0x0000...0001
			b1 <<= pos; //....

			unsigned int b2 = 2; //0x0000...0010
			b2 <<= pos;

			rPCONG |= b1;
			rPCONG &= (~b2);
			break;
		}
		case SIGOUT:
		{
			// COMPLETADO: poner en rPCONG 10 a partir de la posiciÃ³n pos para
			// que salga la seÃ±al interna correspondiente por el pin indicado
			// por el parÃ¡metro pin

			unsigned int b1 = 1; //0x0000...0001
			b1 <<= pos; //....

			unsigned int b2 = 2; //0x0000...0010
			b2 <<= pos;

			rPCONG &= (~b1);
			rPCONG |= b2;
			break;
		}
		case EINT:
		{
			// COMPLETAR: poner en rPCONG 11 a partir de la posiciÃ³n pos para
			// habilitar la generaciÃ³n de interrupciones externas por el pin
			// indicado por el parÃ¡metro pin

			unsigned int bb = 3; // 0x0000...0011
			bb <<= pos; // 0x00...0110...00

			rPCONG |= (~bb);
			break;
		}
		default:
			return -1;
	}

	return 0;
}

int portG_eint_trig(int pin, enum trigger trig)
{
	int pos = pin * 4;

	if (pin < 0 || pin > 7)
		return -1;

	switch (trig) {
	case LLOW:
		rEXTINT &= ~(0x07 << pos);
		// COMPLETAR: poner en rEXTINT a partir de la posición pos tres bits
		// a 000, para configurar interrupciones externas por nivel bajo
		break;
	case LHIGH:
		rEXTINT |= (1  << pos);
		rEXTINT &= ~(0x03 << pos+1);
		// COMPLETAR: poner en rEXTINT a partir de la posición pos tres bits
		// a 001, para configurar interrupciones externas por nivel alto
		break;
	case FALLING:
		rEXTINT &= ~(1 << pos);
		rEXTINT |= (1  << pos + 1);
		rEXTINT &= ~(1 << pos + 2);
		// COMPLETAR: poner en rEXTINT a partir de la posición pos tres bits
		// a 010, para configurar interrupciones externas por flanco de
		// bajada
		break;
	case RISING:
		rEXTINT &= ~(0x03 << pos);
		rEXTINT |= (1  << pos + 2);
		// COMPLETAR: poner en rEXTINT a partir de la posición pos tres bits
		// a 100, para configurar interrupciones externas por flanco de
		// subida
		break;
	case EDGE:
		rEXTINT &= ~(1  << pos);
		rEXTINT |= (0x03  << pos + 1);
		// COMPLETAR: poner en rEXTINT a partir de la posición pos tres bits
		// a 110, para configurar interrupciones externas por cualquier
		// flanco
		break;
	default:
		return -1;
	}
	return 0;
}

int portG_write(int pin, enum digital val)
{
	int pos = pin*2;

	if (pin < 0 || pin > 7)
		return -1; // indica error

	if (val < 0 || val > 1)
		return -1; // indica error

	if ((rPCONG & (0x3 << pos)) != (0x1 << pos))
		return -1; // indica error

	if (val)
	{
		// COMPLETADO: poner en rPDATG el bit indicado por pin a 1

		unsigned int b = 1; //0x0000...0001
		b <<= pin;

		rPDATG |= b;
	}
	else
	{
		// COMPLETADO: poner en rPDATG el bit indicado por pin a 0

		unsigned int b = 1; //0x0000...0001
		b <<= pin;

		rPDATG &= (~b);
	}

	return 0;
}

int portG_read(int pin, enum digital* val)
{
	int pos = pin*2;

	if (pin < 0 || pin > 7)
		return -1; // indica error

	if (rPCONG & (0x3 << pos))
		return -1; // indica error

	int b = 1;
	b <<= pin;

	if ( (rPDATG & b) == b /*COMPLETADO: true si estÃ¡ a 1 en rPDATG el pin indicado por el parÃ¡metro pin*/)
	{
		*val = HIGH;
	}
	else
	{
		*val = LOW;
	}

	return 0;
}

int portG_conf_pup(int pin, enum enable st)
{
	if (pin < 0 || pin > 7)
		return -1; // indica error

	if (st != ENABLE && st != DISABLE)
		return -1; // indica error

	if (st == ENABLE)
	{
		// COMPLETADO: poner el pin de rPUPG indicado por el parametro pin al valor adecuado,
		// para activar la resistencia de pull-up

		unsigned int b = 1; //0x0000...0001
		b <<= pin;

		rPUPG &= ~(b) ;

	}
	else
	{
		// COMPLETADO: poner el pin de rPUPG indicado por el parametro pin al valor adecuado,
		// para desactivar la resistencia de pull-up
		unsigned int b = 1; //0x0000...0001
		b <<= pin;

		rPUPG |= b;
	}

	return 0;
}

