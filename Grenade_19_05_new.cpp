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

#define STATE_SWITCH_ON		1			// blink, zummer, sleep
#define STATE_SLEEP			2			// sleep
#define STATE_WAKE_UP		4			// wake up, blink, zummer
#define STATE_RESPAWN		8			// wait ir respown 
#define STATE_TRIGGER		16			// sleep wait for trigger
#define STATE_EXPLODE		32			// boom, blink, zoomer
#define STATE_FIND_ME		64			// blink, zoomer, wait RESP-IR or long press button or long for sleep

int state = STATE_SWITCH_ON;

unsigned long tsBtnPressed = 0;

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


void setup() {

	DDRD  |= 0b01000000;	// ������� ��������� 
	PORTD &= 0b10111111;	// ������� ���������

	DDRD  |= 0b00100000;	// ����� �������
	PORTD &= 0b11011111;	// ����� �������

	DDRD  |= 0b00010000;	// ��������� ��
	PORTD &= 0b11101111;	// ��������� ��


	DDRD  &= 0b11111011;	// ������
	PORTD |= 0b00000100;	// ������
  
	enableIROut(56);	
	enableIRIn();

	//���������� ��� ���� ISCxx
	//����������� �� ������������ INT0 �� ������� ������
	MCUCR &= ~( (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00) );

	//��������� ������� ���������� INT0
	GICR |= (1<<INT0);
	//���������� ���� ����������� ���������� ����������

	PT_INIT(&ptIRReceive);
	PT_INIT(&ptWaitForTrigger);
	PT_INIT(&ptExplode);
	PT_INIT(&ptFinder);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  
} 


//������� ���������� �������� ���������� INT0
ISR( INT0_vect )
{
	if ( (PIND & 0b00000100) > 0 ) {
		tsBtnPressed = millis();
	} else {
		tsBtnPressed = 0;
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
	if (tsBtnPressed == 0) {
	  sleep_enable();
	  sei();
	  sleep_cpu();
	  sleep_disable();
	} else {
	  if (state == STATE_SWITCH_ON && millis() - tsBtnPressed > 2000) {
		  state = STATE_WAKE_UP;
	  }	
	  if (state == STATE_RESPAWN) {
		  state = STATE_EXPLODE;
	  }	
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
