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
bool armed;
bool triggered;
bool findMe;

#define STATE_STARTED       1
#define STATE_ARMED         2
#define STATE_TRIGGERED     4
#define STATE_FIND_ME       8
#define STATE_SLEEP         16

int state = STATE_STARTED;

unsigned long tsBtnPressed = 0;

static struct pt ptIRReceive, ptWaitForTrigger, ptExplode, ptFinder;

void piezo(bool allowed) {
	if (allowed) {
		PORTD |= 0b00100000;	// пьезо вкл
	} else {
		PORTD &= 0b11011111;	// пьезо выкл
	}
}


void redInd(bool allowed) {
	if (allowed) {
		PORTD |= 0b01000000;	// красный индикатор
	} else {
		PORTD &= 0b10111111;	// красный индикатор
	}
}


void setup() {

	DDRD  |= 0b01000000;	// красный индикатор 
	PORTD &= 0b10111111;	// красный индикатор

	DDRD  |= 0b00100000;	// пьезо пищалка
	PORTD &= 0b11011111;	// пьезо пищалка

	DDRD  |= 0b00010000;	// включение мк
	PORTD &= 0b11101111;	// включение мк


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

	armed = false;
	triggered = false;
	findMe = true;
	
	PT_INIT(&ptIRReceive);
	PT_INIT(&ptWaitForTrigger);
	PT_INIT(&ptExplode);
	PT_INIT(&ptFinder);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  
} 


//функция обработчик внешнего прерывания INT0
ISR( INT0_vect )
{
	if ( (PIND & 0b00000100) > 0 ) {
		tsBtnPressed = millis();
	} else {
		tsBtnPressed = 0;
	}
	
	findMe = false;
}


static int protothreadIrReceive(struct pt *pt) {
  PT_BEGIN(pt);
  while(1) { 
	PT_WAIT_UNTIL(pt, decode(&results) );
	if (results.bits == 24 && results.value == 0b100000110000010111101000) {
		armed = true;
		findMe = false;
  	    redInd(false);
		piezo(false);
	}
	  
	resumeIRIn();	
  }
  PT_END(pt);
}

static int protothreadWaitForTrigger(struct pt *pt) {
  PT_BEGIN(pt);
  while(1) {
	PT_WAIT_UNTIL(pt, armed );
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	triggered = true;
	armed = false;
	
  }
  PT_END(pt);

}

static int protothreadExplode(struct pt *pt) {
    static unsigned long ts = 0;
	PT_BEGIN(pt);
	while(1) {
		PT_WAIT_UNTIL(pt, triggered);    
		ts = millis(); 
	
		// countdown before explode
		int tDelay = 100;
		while (millis() - ts < 3000) {
			redInd(true);
			piezo(true);
			_delay_ms(tDelay);
			redInd(false);
			piezo(false);
			_delay_ms(tDelay);
			if (tDelay > 10) tDelay -= 10;	
		}

		// explode		
		unsigned long data;
		data = 0b100000110000000011101000;
		for (int i = 1; i < 4; i++) {
			redInd(true);
			piezo(true);
			sendSony(data, 24);
			_delay_ms(200);
			redInd(false);
			piezo(false);
			_delay_ms(100);
		}
		triggered = false;
		findMe = true;
	}
	PT_END(pt);
}

static int protothreadFinder(struct pt *pt) {
    static unsigned long tsFind = 0;
	
	PT_BEGIN(pt);
	while(1) {
	  PT_WAIT_UNTIL(pt, findMe);
		
	  tsFind = millis();
	  redInd(true);
	  piezo(true);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500 || !findMe);
		
	  tsFind = millis();
	  redInd(false);
	  piezo(false);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500 || !findMe);
		
	}
	PT_END(pt);	
}


int main(void)
{

	setup(); 

	while(1) {
		protothreadIrReceive(&ptIRReceive);
		protothreadWaitForTrigger(&ptWaitForTrigger);
		protothreadExplode(&ptExplode);
		protothreadFinder(&ptFinder);

	}
	
	

}