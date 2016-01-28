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

#define PIN_INT0 PD2

#define STATE_SWITCH_ON		1			// blink, zummer, sleep
#define STATE_SLEEP			2			// sleep
#define STATE_WAKE_UP		4			// wake up, blink, zummer
#define STATE_RESPAWN		8			// wait ir respown 
#define STATE_TRIGGER		16			// sleep wait for trigger
#define STATE_EXPLODE		32			// boom, blink, zoomer
#define STATE_FIND_ME		64			// blink, zoomer, wait RESP-IR or long press button or long for sleep

volatile int state = 0;

volatile unsigned long cntInt0Called = 0;
volatile unsigned long oldCntInt0Called = 0;
volatile unsigned long tsInt0Iddle = 0;

static struct pt ptIRReceive, ptWaitForTrigger, ptExplode, ptFinder;

void piezo(bool allowed) {
	if (allowed) {
		PORTD |= 0b00100000;	// ����� ���
	} else {
		PORTD &= 0b11011111;	// ����� ����
	}
}


void redInd(bool allowed) {
	if (allowed) {
		PORTD |= 0b01000000;	// ������� ���������
	} else {
		PORTD &= 0b10111111;	// ������� ���������
	}
}

void greenInd(bool allowed) {
	if (allowed) {
		PORTD |= 0b10000000;	// ������� ���������
		} else {
		PORTD &= 0b01111111;	// ������� ���������
	}
}


static int protothreadIrReceive(struct pt *pt) {
  PT_BEGIN(pt);
  while(1) { 
	PT_WAIT_UNTIL(pt, decode(&results) );
	if (state == STATE_WAKE_UP && results.bits == 24 && results.value == 0b100000110000010111101000) {
		state = STATE_RESPAWN;
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
	PT_WAIT_UNTIL(pt, (state == STATE_SWITCH_ON) || (state == STATE_RESPAWN));
	
	if ( cntInt0Called == 0 ) {
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
	}
	
	redInd(true);	
	if (state == STATE_SWITCH_ON && cntInt0Called > 0 /*millis() - cntInt0Called > 2000*/) {
		state = STATE_WAKE_UP;
		
		
	}	
	if (state == STATE_RESPAWN) {
		state = STATE_EXPLODE;
	}	
	
  }
  PT_END(pt);

}

static int protothreadExplode(struct pt *pt) {
    static unsigned long ts = 0;
	PT_BEGIN(pt);
	while(1) {
		PT_WAIT_UNTIL(pt, state == STATE_EXPLODE);   
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
		state = STATE_FIND_ME;
	}
	PT_END(pt);
}

static int protothreadFinder(struct pt *pt) {
    static unsigned long tsFind = 0;
	
	PT_BEGIN(pt);
	while(1) {
	  PT_WAIT_UNTIL(pt, state == STATE_FIND_ME);
		
	  tsFind = millis();
	  redInd(true);
	  piezo(true);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500 || state != STATE_FIND_ME);
		
	  tsFind = millis();
	  redInd(false);
	  piezo(false);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500 || state != STATE_FIND_ME);
		
	}
	PT_END(pt);	
}


// INT0
ISR( INT0_vect )
{

//	greenInd(true);
	_delay_ms(5);
//	greenInd(false);
	_delay_ms(5);

	GICR &= ~(1<<INT0);
	if (cntInt0Called == 300)	{

		cntInt0Called = 0;	
		if (state == STATE_SWITCH_ON) {
			state = 0; 
		} else {
			state = STATE_SWITCH_ON;
		}
		
		for (int i=0; i<5; i++) {
			greenInd(true);
			_delay_ms(100);
			greenInd(false);
			_delay_ms(100);
		}		
		
	} else {
		cntInt0Called++;	
	}
	GICR |= (1<<INT0);
	
}



void mainCycle() {

	cntInt0Called = 0;
    while(1) {
		if (oldCntInt0Called != cntInt0Called) {
			oldCntInt0Called = cntInt0Called;
			tsInt0Iddle = millis();
		}
		
		if ( millis() - tsInt0Iddle > 100 ) {
			cntInt0Called = 0;
		}
		
		if ( (state & STATE_SWITCH_ON) > 0 ) {
			
			protothreadIrReceive(&ptIRReceive);
			protothreadExplode(&ptExplode);
			protothreadFinder(&ptFinder);
			
		} else {
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
			sei();
		}
    }

}

void setup() {

	DDRD  |= 0b10000000;	// Green ind
	PORTD &= 0b01111111;	// Green ind
	
	DDRD  |= 0b01000000;	// Red ind
	PORTD &= 0b10111111;	// Red ind

	DDRD  |= 0b00100000;	// piezo
	PORTD &= 0b11011111;	// piezo

	//настраиваем вывод на вход
	DDRD &= ~(1<<PIN_INT0);
	//включаем подтягивающий резистор
	PORTD |= (1<<PIN_INT0);
	//разрешаем внешнее прерывание на int0
	GICR |= (1<<INT0);
	//настраиваем условие прерывания
	
	// for power_down
	uint8_t x = MCUCR & (~( (1<<ISC01)|(1<<ISC00) ));
	MCUCR = x;
	
	enableIROut(56);
	enableIRIn();

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sei();

	PT_INIT(&ptIRReceive);
	PT_INIT(&ptWaitForTrigger);
	PT_INIT(&ptExplode);
	PT_INIT(&ptFinder);

}


int main(void)
{

	setup(); 
	
	mainCycle();
	
}
