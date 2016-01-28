#include "globals.h"

//#include "config.h"
#include "miles.h"

uchar previous_ir_time;
//struct IRPackage current_ir;
uchar ir_bits_recieved=0;
uchar ir_recieve_mask=0b10000000;

// повесить send_ir_xxx на таймер2. более точные интервалы в сравнении с обычной delay.h
#define WAIT_USING_TIMER2


#ifdef WAIT_USING_TIMER2

	#define _us1(X) (CARRIER_FREQ/1000+1)*2*X/1000
	#define _us0(X) X/31.25

	//два байта только ради интервала старта на 56КГц, на остальных частотах хватает uchar
	//код сразу станоновится проще
	volatile uint delay_ticks_left=0;

	ISR(TIMER2_COMP_vect) {
		if (delay_ticks_left != 0) {
			delay_ticks_left--;
		};
	};
	#define wait_us(ticks)\
	do {\
		delay_ticks_left = ticks;\
		TIMSK |= (1<<OCIE2);\
		while (delay_ticks_left != 0);\
		TIMSK &= ~(1<<OCIE2);\
	} while (0)

#else

	//обычная пауза. точность страдает из-за прерываний. вариант выше (на таймере1) более точный.
	#include <util/delay.h>
	#define _us1(X) X
	#define _us0(X) X
	#define wait_us(X) _delay_us(X)

#endif



//запускаем ШИМ стрельбы
//WGM21 = CTC
//COM20 = Toggle OC2 on Compare Match
//CS20	= prescaler clk/1
#define enable_fire_pwm(ticks)\
do {\
	TCCR2 = (1<<COM20)|(1<<WGM21)|(1<<CS20);\
	TCNT2 = 0;\
	wait_us(_us1(ticks));\
} while (0)

//останавливаем ШИМ стрельбы
#define disable_fire_pwm(ticks)\
do {\
	TCNT2 = 0;\
	TCCR2 = (1<<CS20);\
	DISABLE_OUTPUT(B,3,NORMAL);\
	wait_us(_us0(ticks));\
} while(0)

void send_ir_start() {
	enable_fire_pwm		(2400);
	disable_fire_pwm	(600);
};

void send_ir_1() {
	enable_fire_pwm		(1200);
	disable_fire_pwm	(600);
};

void send_ir_0() {
	enable_fire_pwm		(600);
	disable_fire_pwm	(600);
};

void send_ir_bits (const uchar data, uchar mask) {
  while (mask) {
    if (data&mask) {
      send_ir_1();
    } else {
      send_ir_0();
    };
    mask>>=1;
  };
};

void send_ir_fire (const uchar id, const uchar team, const uchar damage) {
	send_ir_start();
	send_ir_0();
	send_ir_bits(id,		0b01000000);
	send_ir_bits(team,	0b00000010);
	send_ir_bits(damage,0b00001000);
};

void send_ir_command (const uchar byte1, const uchar byte2, const uchar byte3) {
	send_ir_start();
	send_ir_bits(byte1,	0b10000000);
	send_ir_bits(byte2,	0b10000000);
	send_ir_bits(byte3,	0b10000000);
};

void send_ir_package(unsigned long data, int nbits) {
	send_ir_start();
	data = data << (32 - nbits);
	for (int i = 0; i < nbits; i++) {
		if (data & TOPBIT) {
			send_ir_1();
		}
		else {
			send_ir_0();
		}
		data <<= 1;
	}
}

void init_ir_out() {
	INIT_OUTPUT (B,3,NORMAL); // ИК выстрел (OC2)
	
	// Таймер0 - основной таймер для обсчёта логики программы (TCNT0 - значение счетчика)
	TCCR0 = (1<<CS02)|(0<<CS01)|(0<<CS00); //предделитель = 256, при 8МГц - тик = 31250Гц
	SFIOR |= (1<<PSR10);	// обнуляем предделитель
	TIMSK = (1<<TOIE0);		// включить прерывание по переполнению таймера 0
	
	//общая частота таймера0 - 31250Гц, таймер восьмиразрядный, соответственно переполнение
	//возникает каждые 0,008192сек. с такой точностью и будет работать наша логика на обработку
	//лампочек, скорострельности и прочего. т.е. было бы круто, если бы основной цикл программы (ф-я main)
	//выполнялась за время сопостовимое с 8мс.
	GICR = (1<<INT0);	// разрешаем инт0
	MCUCR = (0<<ISC01)|(1<<ISC00);	// прерывание на изменение логического сигнала

	//инициализируем регистр сравнения таймера2 - используется для формирования несущей ИК
	// Fout = Кварц/(2*(N+1)) => N = Кварц/(2*Fout) - 1;
	// в дальнейшем регистр сравнения не меняем
	OCR2 = F_CPU/CARRIER_FREQ/2-1;

	sei();	
}