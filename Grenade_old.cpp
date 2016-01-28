/*
 * Grenade.cpp
 *
 * Created: 30.11.2013 0:05:50
 *  Author: csv
 */ 

#include <avr/io.h>

#include "globals.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <math.h> 
#include "IRMiles.h"
#include "pt.h"
#include <util/delay.h>

decode_results results;
bool bntPressed;

static struct pt pt1

void setup() {

	DDRD  |= 0b01000000;	// красный индикатор 
	PORTD &= 0b10111111;	// красный индикатор

	DDRD  |= 0b00100000;	// пьезо пищалка
	PORTD &= 0b11011111;	// пьезо пищалка


	DDRD  &= 0b11111011;	// кнопка
	PORTD |= 0b00000100;	// кнопка
  
	enableIROut(56);	
	enableIRIn();

	//сбрасываем все биты ISCxx
	//настраиваем на срабатывание INT0 по нижнему уровню
	MCUCR &= ~( (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00) );

	//разрешаем внешнее прерывание INT0
	GICR |= (1<<INT0);
	//выставляем флаг глобального разрешения прерываний

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
} 

void piezo(bool allowed) {
	if (allowed) {
		PORTD &= 0b11011111;	// пьезо выкл 
	} else {
		PORTD |= 0b00100000;	// пьезо вкл
	}
}


void redInd(bool allowed) {
  if (allowed) {
    PORTD |= 0b01000000;	// красный индикатор	
  } else {
    PORTD &= 0b10111111;	// красный индикатор	
  }
}	

void explosion() {
/*
    for (int i = 1; i < 10; i++) {
		_delay_ms(150);
		redInd(true);
		_delay_ms(150);
		redInd(false);
	}
*/	
	int time = 0;
	int delta = 100;
	while (time < 3000) {
		time += delta;
		redInd(true);		
		_delay_ms(delta);
		time += delta;
		redInd(false);		
		_delay_ms(delta);
		delta -= 10;
	}

/*
    int time = 0;
	int delay = 200;
	int dt = 15;
    while (time < 3500) {
		_delay_ms(delay);
		redInd(true);
		_delay_ms(delay);
		redInd(false);
		time += delay*2;
		if (delay-dt > 0) delay -= dt; 
	}
*/
/**
 *  unsigned long data = 0b10000011 00000101 11101000; 
 *                          0x83       0x05    0xE8               New Game (Immediate)
 *
 *
 *  unsigned long data = 0b10000011 00000110 11101000; 
 *                          0x83       0x06    0xE8               Full ammo
 *
 *  unsigned long data = 0b10000011 00001101 11101000; 
 *                          0x83       0x0D    0xE8               Full Health
 *
 *  unsigned long data = 0b10001010 00000001 11101000; 
 *                          0x8A       0x01    0xE8               Ammo Box ID (0-15)
 *
 *  unsigned long data = 0b10001011 00000001 11101000; 
 *                          0x8B       0x01    0xE8               Med Box (0-15)
 *
 *  unsigned long data = 0b10000011 00000000 11101000; 
 *                          0x83       0x00    0xE8               Admin Kill
 *
 *  unsigned long data = 0b10000011 00001011 11101000; 
 *                          0x83       0x00    0xE8               Explode player
 **/		
    
    unsigned long data;
    data = 0b100000110000000011101000;
	
	for (int i = 1; i < 4; i++) {
		redInd(true);		
		_delay_ms(200);
		redInd(false);		
		sendSony(data, 24);
		_delay_ms(100);
	}		
	
	
/*
    while (1) {
		redInd(true);
		_delay_ms(700);
		redInd(false);
		_delay_ms(300);
	}
*/
  
}


//функция обработчик внешнего прерывания INT0
ISR( INT0_vect )
{
	bntPressed = true;
}



int main(void)
{

	setup(); 
	
	while(1) {

		resumeIRIn();
		
		while(1) {
			if (decode(&results)) {
				if (results.bits == 24 && results.value == 0b100000110000010111101000) {
					break;
				} else {
					continue;
				}
			}
		
		}
		

		PORTD &= 0b11011111;	// пьезо выкл 

		sleep_enable();
		sei();
		sleep_cpu();
		//cli();
		sleep_disable();

		explosion();
		
		PORTD |= 0b00100000;	// пьезо вкл
		
	}
	
	

}