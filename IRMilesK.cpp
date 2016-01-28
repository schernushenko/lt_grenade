
#define F_CPU 8000000UL    // 16 MHz
#define __AVR_ATmega8__

#include <avr/io.h>
#include <util/delay.h>
#include "IRMilesK.h"



void SendStart() {
	onIRx;
	_delay_us(2400);
	offIRx;
	_delay_us(600);
};

void Send1() {
	onIRx;
	_delay_us(1200);
	offIRx;
	_delay_us(600);
};

void Send0() {
	onIRx;
	_delay_us(600);
	offIRx;
	_delay_us(600);
};

void SendBits (uchar data, uchar mask) {
	while (mask) {
		if (data&mask) {
			Send1();
			} else {
			Send0();
		};
		mask>>=1;
	};
};


void sendSonyK(unsigned long data, int nbits) {

	SendStart();
	data = data << (32 - nbits);
	for (int i = 0; i < nbits; i++) {
		if (data & TOPBIT) {
			Send1();
		}
		else {
			Send0();
		}
		data <<= 1;
	}
	offIRx;
}

void InitIR () {
	
	PORTB	= 0b00000000; //0x00;
	DDRB	= 0b00001110; //0x0A; // было 0b00001010


	ASSR=0x00;
	TCCR2=0x19;   // ¬ключить IRx
	TCCR2=0x00;   // ¬џключить + P¬3 = 0
	TCNT2=0x00;

	
	// Fout =  варц/(2*(N+1)) => N =  варц/(2*Fout) - 1;
	// соотв. при  варц=8000 √ц, значени€ N:
	// 36 гц, N = 110
	// 40 гц, N = 99
	// 56 гц, N = 70
	OCR2 = 99;
	
	
	TIMSK = (1<<TOIE0); // включить прерывание по переполнению таймера 0
	TCCR0 = (1<<CS02)|(0<<CS01)|(0<<CS00); //предделитель = 256, при 8ћ√ц - тик = 31250√ц
	SFIOR |= (1<<PSR10); // обнул€ем предделитель
	//TCNT0 - значение счетчика


	TIMSK = (1<<TOIE0); // включить прерывание по переполнению таймера 0
	//	TIMSK=0x00;  // Timer(s)/Counter(s) Interrupt(s) initialization
	WDTCR=0x17;  // запретить WatchDog :  WDCE=1, WDE=0, Time=111(2s)

	// Analog Comparator  Off
	// Analog Comparator Input Capture by Timer/Counter 1: Off
	ACSR=0x80;
	SFIOR=0x02;	
	
};


