/*
 * IRMiles.cpp
 *
 * Created: 09.02.2013 0:03:38
 *  Author: csv
 */ 


//#include "IRMiles.h"

#include "globals.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "IRMiles.h"

//extern volatile 
irparams_t irparams;

#ifdef IROUT
void enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (OC2B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
  // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.

  
  // Disable the Timer2 Interrupt (which is used for receiving IR)
  
  
  
  sbi(DDRB, 2);
  //PORTB &= ~_BV(2);
  cbi(PORTB, 2);
    
  // COM2A = 00: disconnect OC2A
  // COM2B = 00: disconnect OC2B; to send signal set to 10: OC2B non-inverted
  // WGM2 = 101: phase-correct PWM with OCRA as top
  // CS2 = 000: no prescaling
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);

  // The top value for the timer.  The modulation frequency will be SYSCLOCK / 2 / OCR2A.
  OCR1A = SYSCLOCK / 2 / khz / 1000;
  OCR1B = 125; // OCR1A ; // 33% duty cycle

  
  /*
  //  нопка
	DDRD  &= 0b11111011;	// кнопка
	PORTD |= 0b00000100;	// кнопка

	//сбрасываем все биты ISCxx
	//настраиваем на срабатывание INT0 по нижнему уровню
	MCUCR &= ~( (1<<ISC11)|(1<<ISC10)|(1<<ISC01)|(1<<ISC00) );

	//разрешаем внешнее прерывание INT0 
	GICR |= (1<<INT0);
	//выставл€ем флаг глобального разрешени€ прерываний

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	*/


}
#endif

void
_delay_loop(uint16_t __count)
{
	__asm__ volatile (
		"1: sbiw %0,1" "\n\t"
		"brne 1b"
		: "=w" (__count)
		: "0" (__count)
	);
}

#ifdef IROUT
void mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  TCCR1A |= _BV(COM1B1); // Enable pin 3 PWM output
  //_delay_us(time);
  _delay_loop(2*time);
}

/* Leave pin off for time (given in microseconds) */
void space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  TCCR1A &= ~(_BV(COM1B1)); // Disable pin 3 PWM output
  //_delay_us(time);
  _delay_loop(2*time);
}


void sendSony(unsigned long data, int nbits) {
  enableIROut(56);
  cbi(TIMSK,TOIE0); //Disable Timer2 Overflow Interrupt
  
  mark(SONY_HDR_MARK);
  space(SONY_HDR_SPACE);
  data = data << (32 - nbits);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(SONY_ONE_MARK);
      space(SONY_HDR_SPACE);
    } 
    else {
      mark(SONY_ZERO_MARK);
      space(SONY_HDR_SPACE);
    }
    data <<= 1;
  }
  sbi(TIMSK,TOIE0); //Enable Timer2 Overflow Interrupt
}
#endif

#ifdef IRIN
// initialization
void enableIRIn() {
  // setup pulse clock timer interrupt
  //TCCR1A = 0;  // normal mode

  //Prescale /8 (16M/8 = 0.5 microseconds per tick)
  // Therefore, the timer interval can range from 0.5 to 128 microseconds
  // depending on the reset value (255 to 0)
  cbi(TCCR0,CS02);
  sbi(TCCR0,CS01);
  cbi(TCCR0,CS00);

  //Timer2 Overflow Interrupt Enable
  sbi(TIMSK,TOIE0);

  RESET_TIMER1;

  sei();  // enable interrupts

  // initialize state machine variables
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;


  // set pin modes
  cbi(DDRD, 3); // pin 3 read IR data
  //sbi(DDRD, 4); // blink pin 4 when IR in

  
}

volatile unsigned long timer0_millis = 0;
volatile int cnt_timer_tics = 0;

unsigned long millis()
{
	unsigned long m;
	cli();
	m = timer0_millis;
    sei();
	return m;
}



// TIMER2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50 microseconds.
// rawlen counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
// As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts
ISR(TIMER0_OVF_vect)
{
  //sbi(PORTD, 4);
  RESET_TIMER1;
  
  cnt_timer_tics++;
  if (cnt_timer_tics == 20) {
	  cnt_timer_tics = 0;
	  timer0_millis++;
  }	
  uint8_t irdata;
  if (bit_is_set(PIND, PIND3)) {
	irdata = SPACE;
  } else {
	irdata = MARK;
  }	
    
  irparams.timer++; // One more 50us tick
  if (irparams.rawlen >= RAWBUF) {
    // Buffer overflow
    irparams.rcvstate = STATE_STOP;
  }
  switch(irparams.rcvstate) {
  case STATE_IDLE: // In the middle of a gap
    if (irdata == MARK) {
      if (irparams.timer < GAP_TICKS) {
        // Not big enough to be a gap.
        irparams.timer = 0;
      } 
      else {
        // gap just ended, record duration and start recording transmission
        irparams.rawlen = 0;
        irparams.rawbuf[irparams.rawlen++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_MARK;
      }
    }
    break;
  case STATE_MARK: // timing MARK
    if (irdata == SPACE) {   // MARK ended, record time
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_SPACE;
    }
    break;
  case STATE_SPACE: // timing SPACE
    if (irdata == MARK) { // SPACE just ended, record it
      irparams.rawbuf[irparams.rawlen++] = irparams.timer;
      irparams.timer = 0;
      irparams.rcvstate = STATE_MARK;
    } 
    else { // SPACE
      if (irparams.timer > GAP_TICKS) {
        // big SPACE, indicates gap between codes
        // Mark current code as ready for processing
        // Switch to STOP
        // Don't reset timer; keep counting space width
        irparams.rcvstate = STATE_STOP;
      } 
    }
    break;
  case STATE_STOP: // waiting, measuring gap
    if (irdata == MARK) { // reset gap timer
      irparams.timer = 0;
    }
    break;
  }
/*
	if (irdata == MARK) {
		sbi(PORTD, 4);
	} 
	else {
		cbi(PORTD, 4);;  // turn pin 13 LED off
	}
*/
}


long decodeSony(decode_results *results) {
  long data = 0;
  if (irparams.rawlen < 2 * SONY_BITS + 2) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results->rawbuf[offset], SONY_HDR_MARK)) {
    return ERR;
  }
  offset++;

  while (offset + 1 < irparams.rawlen) {
    if (!MATCH_SPACE(results->rawbuf[offset], SONY_HDR_SPACE)) {
      break;
    }
    offset++;
    if (MATCH_MARK(results->rawbuf[offset], SONY_ONE_MARK)) {
      data = (data << 1) | 1;
    } 
    else if (MATCH_MARK(results->rawbuf[offset], SONY_ZERO_MARK)) {
      data <<= 1;
    } 
    else {
      return ERR;
    }
    offset++;
  }

  // Success
  results->bits = (offset - 1) / 2;
  if (results->bits < 12) {
    results->bits = 0;
    return ERR;
  }
  results->value = data;
  results->decode_type = SONY;
  return DECODED;
}


void resumeIRIn() {
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
}

// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
int decode(decode_results *results) {
  results->rawbuf = irparams.rawbuf;
  results->rawlen = irparams.rawlen;
  if (irparams.rcvstate != STATE_STOP) {
    return ERR;
  }

  if (decodeSony(results)) {
    return DECODED;
  }
  // Throw away and start over
  resumeIRIn();
  return ERR;
}
#endif