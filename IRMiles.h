/*
 * IRMiles.cpp
 *
 * Created: 09.02.2013 0:03:38
 *  Author: csv
 */ 



#define CLKFUDGE 5      // fudge factor for clock interrupt overhead
//#define CLK 256      // max value for clock (timer 2)
#define CLK 65536      // max value for clock (timer 2)
#define PRESCALE 8      // timer2 clock prescale
#define SYSCLOCK 8000000  // main atmega clock
#define F_CPU 8000000  // main atmega clock
#define CLKSPERUSEC (SYSCLOCK/PRESCALE/1000000)   // timer clocks per microsecond
#define USECPERTICK 50  // microseconds per clock interrupt tick
#define _GAP 5000 // Minimum map between transmissions
#define GAP_TICKS (_GAP/USECPERTICK)

#define SONY_BITS 12
#define ERR 0
#define DECODED 1
#define SONY 2

// IR detector output is active low
#define MARK  0
#define SPACE 1

#define MARK_EXCESS 100

#define TOLERANCE 25  // percent tolerance in measurements
#define LTOL (1.0 - TOLERANCE/100.) 
#define UTOL (1.0 + TOLERANCE/100.) 


#define TICKS_LOW(us) (int) (((us)*LTOL/USECPERTICK))
#define TICKS_HIGH(us) (int) (((us)*UTOL/USECPERTICK + 1))


#define MATCH(measured_ticks, desired_us) ((measured_ticks) >= TICKS_LOW(desired_us) && (measured_ticks) <= TICKS_HIGH(desired_us))
#define MATCH_MARK(measured_ticks, desired_us) MATCH(measured_ticks, (desired_us) + MARK_EXCESS)
#define MATCH_SPACE(measured_ticks, desired_us) MATCH((measured_ticks), (desired_us) - MARK_EXCESS)


#define RAWBUF 76 // Length of raw duration buffer

#define __DELAY_BACKWARD_COMPATIBLE__

// receiver states
#define STATE_IDLE     2
#define STATE_MARK     3
#define STATE_SPACE    4
#define STATE_STOP     5

#define SONY_HDR_MARK	2400
#define SONY_HDR_SPACE	600
#define SONY_ONE_MARK	1200
#define SONY_ZERO_MARK	600

#define RESTART_CODE 0xA90
#define MENU_CODE 0x70

#define TOPBIT 0x80000000

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#ifndef bit_is_set
#define bit_is_set(sfr, bit)   (_SFR_BYTE(sfr) & _BV(bit))
#endif
#ifndef bit_is_clear
#define bit_is_clear(sfr, bit)   (!(_SFR_BYTE(sfr) & _BV(bit)))
#endif


// clock timer reset value
#define INIT_TIMER_COUNT1 (CLK - USECPERTICK*CLKSPERUSEC + CLKFUDGE)
#define RESET_TIMER1 TCNT0 = INIT_TIMER_COUNT1




// information for the interrupt handler
typedef struct {
  uint8_t recvpin;           // pin for IR data from detector
  uint8_t rcvstate;          // state machine
  uint8_t blinkflag;         // TRUE to enable blinking of pin 13 on IR processing
  unsigned int timer;     // state timer, counts 50uS ticks.
  unsigned int rawbuf[RAWBUF]; // raw data
  uint8_t rawlen;         // counter of entries in rawbuf
} 
irparams_t;


// Results returned from the decoder
class decode_results {
public:
  int decode_type; // NEC, SONY, RC5, UNKNOWN
  unsigned long value; // Decoded value
  int bits; // Number of bits in decoded value
  volatile unsigned int *rawbuf; // Raw intervals in .5 us ticks
  int rawlen; // Number of records in rawbuf.
};

#ifdef IROUT
  void enableIROut(int khz);
  void sendSony(unsigned long data, int nbits);
#endif

#ifdef IRIN
  void enableIRIn();
  void resumeIRIn();
  int decode(decode_results *results);
  unsigned long millis();
#endif
void _delay_loop(uint16_t __count);