#ifndef _IR_MILES_HK_H_
#define _IR_MILES_HK_H_

#define TOPBIT 0x80000000

#define onIRx TCCR2 =0x19; // вкл 36 к√ц на ик диод PB.3
#define offIRx TCCR2 =0b00100001; PORTB &= ~_BV(3);// выкл + PB3 = 0

#define uchar unsigned char
#define uint unsigned int



void sendSonyK(unsigned long data, int nbits);
void InitIR();

#endif 