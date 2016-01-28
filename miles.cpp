#include "globals.h"

//#include "config.h"
#include "miles.h"

uchar previous_ir_time;
//struct IRPackage current_ir;
uchar ir_bits_recieved=0;
uchar ir_recieve_mask=0b10000000;

// �������� send_ir_xxx �� ������2. ����� ������ ��������� � ��������� � ������� delay.h
#define WAIT_USING_TIMER2


#ifdef WAIT_USING_TIMER2

	#define _us1(X) (CARRIER_FREQ/1000+1)*2*X/1000
	#define _us0(X) X/31.25

	//��� ����� ������ ���� ��������� ������ �� 56���, �� ��������� �������� ������� uchar
	//��� ����� ������������ �����
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

	//������� �����. �������� �������� ��-�� ����������. ������� ���� (�� �������1) ����� ������.
	#include <util/delay.h>
	#define _us1(X) X
	#define _us0(X) X
	#define wait_us(X) _delay_us(X)

#endif



//��������� ��� ��������
//WGM21 = CTC
//COM20 = Toggle OC2 on Compare Match
//CS20	= prescaler clk/1
#define enable_fire_pwm(ticks)\
do {\
	TCCR2 = (1<<COM20)|(1<<WGM21)|(1<<CS20);\
	TCNT2 = 0;\
	wait_us(_us1(ticks));\
} while (0)

//������������� ��� ��������
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
	INIT_OUTPUT (B,3,NORMAL); // �� ������� (OC2)
	
	// ������0 - �������� ������ ��� ������� ������ ��������� (TCNT0 - �������� ��������)
	TCCR0 = (1<<CS02)|(0<<CS01)|(0<<CS00); //������������ = 256, ��� 8��� - ��� = 31250��
	SFIOR |= (1<<PSR10);	// �������� ������������
	TIMSK = (1<<TOIE0);		// �������� ���������� �� ������������ ������� 0
	
	//����� ������� �������0 - 31250��, ������ ���������������, �������������� ������������
	//��������� ������ 0,008192���. � ����� ��������� � ����� �������� ���� ������ �� ���������
	//��������, ���������������� � �������. �.�. ���� �� �����, ���� �� �������� ���� ��������� (�-� main)
	//����������� �� ����� ������������ � 8��.
	GICR = (1<<INT0);	// ��������� ���0
	MCUCR = (0<<ISC01)|(1<<ISC00);	// ���������� �� ��������� ����������� �������

	//�������������� ������� ��������� �������2 - ������������ ��� ������������ ������� ��
	// Fout = �����/(2*(N+1)) => N = �����/(2*Fout) - 1;
	// � ���������� ������� ��������� �� ������
	OCR2 = F_CPU/CARRIER_FREQ/2-1;

	sei();	
}