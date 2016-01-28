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
#include "miles.h"
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

static struct pt ptIRReceive, ptWaitForRespown, ptExplode, ptFinder;

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

void stepUp(bool allowed) {
	if (allowed) {
		PORTD |= 0b10000000;	// ������� ���������
		} else {
		PORTD &= 0b01111111;	// ������� ���������
	}
}

void dzin(int count) {
	for (int i=0; i<count; i++) {
		redInd(true);
		piezo(true);
		_delay_ms(50);
		redInd(false);
		piezo(false);
		_delay_ms(20);
	}
}

static int protothreadWaitForRespown(struct pt *pt) {
	PT_BEGIN(pt);
	static unsigned long tsWtResp = 0;
	while(1) {
		PT_WAIT_UNTIL(pt, ((state & STATE_SWITCH_ON) > 0) && ((state & (STATE_RESPAWN | STATE_FIND_ME)) == 0));
	    

		tsWtResp = millis();
		redInd(true);
		piezo(true);
		PT_WAIT_UNTIL(pt, millis() - tsWtResp > 100);
	    
		tsWtResp = millis();
		redInd(false);
		piezo(false);
		PT_WAIT_UNTIL(pt, millis() - tsWtResp > 100);
		
		
		tsWtResp = millis();
		PT_WAIT_UNTIL(pt, millis() - tsWtResp > 2000);
	    
		
	}
	PT_END(pt);

}


static int protothreadIrReceive(struct pt *pt) {
  PT_BEGIN(pt);
  while(1) { 
	PT_WAIT_UNTIL(pt, decode(&results) );
	if ((state & STATE_SWITCH_ON) > 0 && results.bits == 24 && results.value == 0b100000110000010111101000) {
		dzin(2);
		state |= STATE_RESPAWN;
		state &= ~STATE_FIND_ME;
  	    redInd(false);
		piezo(false);
		
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		sei();
		
	}
	  
	resumeIRIn();	
  }
  PT_END(pt);
}


static int protothreadExplode(struct pt *pt) {
    static unsigned long ts = 0;
	PT_BEGIN(pt);
	while(1) {
		PT_WAIT_UNTIL(pt, (state & STATE_EXPLODE) > 0);   
		ts = millis(); 
		//stepUp(true);
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
		
		init_ir_out();
		for (int i = 1; i < 4; i++) {
			redInd(true);
			piezo(true);
			//sendSony(data, 24);
			send_ir_package(data, 24);
			_delay_ms(200);
			redInd(false);
			piezo(false);
			_delay_ms(100);
		}
		//stepUp(false);
		state = STATE_FIND_ME | STATE_SWITCH_ON;
	}
	PT_END(pt);
}

static int protothreadFinder(struct pt *pt) {
    static unsigned long tsFind = 0;
	
	PT_BEGIN(pt);
	while(1) {
	  PT_WAIT_UNTIL(pt, (state & STATE_FIND_ME) > 0);
		
	  tsFind = millis();
	  redInd(true);
	  piezo(true);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500);
		
	  tsFind = millis();
	  redInd(false);
	  piezo(false);
	  PT_WAIT_UNTIL(pt, millis() - tsFind > 500);
		
	}
	PT_END(pt);	
}


// INT0
ISR( INT0_vect )
{

	if ((state & STATE_RESPAWN) > 0) {
		state &= ~STATE_RESPAWN;
		state |= STATE_EXPLODE;
	}
	
	if ((state & STATE_FIND_ME) > 0) {
		state = STATE_SWITCH_ON;
	}

//	greenInd(true);
	_delay_ms(5);
//	greenInd(false);
	_delay_ms(5);

	GICR &= ~(1<<INT0);
	if (cntInt0Called == 300)	{

		cntInt0Called = 0;	
		if ((state & STATE_SWITCH_ON) > 0 ) {
			//state &= ~(STATE_SWITCH_ON);
			state = 0;
		} else {
			state = STATE_SWITCH_ON;
		}

		dzin(3);		
		
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
			
			protothreadWaitForRespown(&ptWaitForRespown);
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

	DDRD  |= 0b10000000;	// stepUp
	PORTD &= 0b01111111;	// stepUp
	
	DDRD  |= 0b01000000;	// red ind
	PORTD &= 0b10111111;	// red ind

	DDRD  |= 0b00100000;	// piezo
	PORTD &= 0b11011111;	// piezo
	
	stepUp(false);
	redInd(false);
	
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
	//init_ir_out();
	enableIRIn();
	

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sei();

	PT_INIT(&ptWaitForRespown);
	PT_INIT(&ptIRReceive);
	PT_INIT(&ptExplode);
	PT_INIT(&ptFinder);

}



void testIR() {
		unsigned long data;
		data = 0b100000110000000011101000;
				
		while (true) {
			_delay_ms(3000);
			redInd(true);
			piezo(true);
			_delay_ms(200);
			redInd(false);
			piezo(false);
			sendSony(data, 24);
			//send_ir_package(data, 24);
		}

/*
while (true) {
	redInd(true);
	//_delay_ms(100);
	sendSony(data, 24);
	redInd(false);
	_delay_ms(100);
}
*/
}


int main(void)
{

	setup(); 
	
	//mainCycle();
	testIR();
	
}
