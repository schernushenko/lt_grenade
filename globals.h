#ifndef GLOBALS_H
#define GLOBALS_H


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define uchar unsigned char
#define uint unsigned int
#define NULL	0
#define FALSE	0
#define TRUE	1


#define SYSCLOCK 8000000  // main atmega clock
#define F_CPU 8000000  // main atmega clock

#define IROUT
#define IRIN

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

//---
//следующий код - для облегчения использования констант.

#define INVERTED	1
#define NORMAL		0

#define INIT_INPUT(_PORT, _PIN)\
do {\
	PORT ## _PORT |= _BV(_PIN);\
	DDR ## _PORT &= ~_BV(_PIN);\
} while(0)

#define INIT_OUTPUT(_PORT, _PIN, _MODE)\
do {\
	if (_MODE) {\
		PORT ## _PORT |= _BV(_PIN);\
	} else {\
		PORT ## _PORT &= ~_BV(_PIN);\
	};\
	DDR ## _PORT |= _BV(_PIN);\
} while(0)

#define DISABLE_OUTPUT(_PORT, _PIN, _MODE)\
do {\
	if (_MODE) {\
		PORT ## _PORT |= _BV(_PIN);\
	} else {\
		PORT ## _PORT &= ~_BV(_PIN);\
	};\
} while(0)

#define ENABLE_OUTPUT(_PORT, _PIN, _MODE)\
do {\
	if (_MODE) {\
		PORT ## _PORT &= ~_BV(_PIN);\
	} else {\
		PORT ## _PORT |= _BV(_PIN);\
	};\
} while(0)

#define INIT_INPUT3ARG(_PORT, _PIN, _MODE) INIT_INPUT(_PORT, _PIN)

#define INIT_BUTTON(_BTN) INIT_INPUT3ARG(_BTN)

#define INIT_LED(_LED) INIT_OUTPUT(_LED)

#define DISABLE_LED(_LED) DISABLE_OUTPUT(_LED)

#define ENABLE_LED(_LED) ENABLE_OUTPUT(_LED)

#define CHECK_INPUT(_PORT, _PIN, _MODE) (_MODE?((PIN ## _PORT & _BV(_PIN)) != 0):((PIN ## _PORT & _BV(_PIN)) == 0))

#define CHECK_BUTTON(_BTN) CHECK_INPUT(_BTN)


//частота несущей ИК выстрела (в герцах, стандартные значения: 36000, 38000, 40000, 56000)
#define CARRIER_FREQ	56000 //56000//36000


#endif
