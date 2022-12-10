#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdbool.h>

//#define F_CPU 8000000UL
#define MS_DELAY  250
#define TOT_SEC   10     // 180 sec = 3 min.

#define MUTE_RX   PORTD1
#define LED_RX    PORTD2
#define LED_TX    PORTD3
#define LED_TOT   PORTD4


typedef enum _repeater_status_t {
   RPT_STBY,
   RPT_RX_START,
   RPT_TX_START,
   RPT_RTX,
   RPT_RX_STOP,
   RPT_TX_STOP,
   RPT_TOT_START,
   RPT_TOT,
   RPT_TOT_STOP,
   RPT_ID_START,
   RPT_ID_STOP,
   RX_N
} repeater_status_t;

/*
 * Global variables
 */
volatile bool update = false;
volatile bool repeater_is_receiving = false;
volatile repeater_status_t repeater_status = RPT_STBY; 
volatile unsigned int tot = 0;

/**
 * set update on a high edge
 */
ISR(PCINT0_vect) {
   if (PINB && _BV(PINB5)) {
      // Started Receiving a signal
      // __/```
      PORTD |= _BV(LED_RX);
      repeater_is_receiving = true;
      //repeater_status = RPT_RX_START;
   } else {
      // Stopped receiving a signal
      // ```\__
      PORTD &= ~_BV(LED_RX);
      repeater_is_receiving = false;
      //repeater_status = RPT_RX_STOP;
   }
}

ISR(TIMER1_OVF_vect) {
   if (repeater_status == RPT_RTX) {
      tot++;
   }
   TCNT1  = 65535 - (F_CPU/1024);
}

void delay_sec(unsigned int sec) {
   // FIXME: 
   if (sec > 0 && sec <= 65535) {
      for (int c = 0; c <= (sec * 1000); c++) { 
         _delay_ms(1);
      }
   }
}

void intro_sequence(void) {
   // Wait a bit for signals to stabilize
   // And do a boot animation by lighting
   // all leds sequentially.
   PORTD = 0x0;
   delay_sec(1);
   PORTD |= _BV(2);
   delay_sec(1);
   PORTD |= _BV(3);
   delay_sec(1);
   PORTD |= _BV(4);
   delay_sec(1);
   PORTD = 0x0;
}

void change_status(repeater_status_t status) {
   ATOMIC_BLOCK(ATOMIC_FORCEON) {
      repeater_status = status;
   }
}

int main(void) {

   /* PORT B as Inputs */
	DDRB = 0x00;

   /* Set pins as outputs */
   DDRD |= _BV(DDD4) + _BV(DDD3) + _BV(DDD2) + _BV(DDD0);

   intro_sequence();

	/**
	 * Pin Change Interrupt enable on PCINT0 (PB0)
	 */
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT5);

   /**
    * Set Timer 1 - 1 Hz | 1 sec.
    */
   TCNT1  = 65535 - (F_CPU/1024);
   TIMSK1 = (1 << TOIE1); 
   TCCR1A = 0x00;
   TCCR1B = (1 << CS10) | (1 << CS12); //Set Prescaler to 1024 (bit 10 and 12) and start timer


	// Turn interrupts on.
	sei();

	for(;;) {
      switch(repeater_status) {
         case RPT_RX_START:
            change_status(RPT_TX_START);
            break;
         case RPT_TX_START:
            PORTD |= _BV(LED_TX);
            change_status(RPT_RTX);
            break;
         case RPT_RTX:
            if (tot >= TOT_SEC) {
               change_status(RPT_TOT_START);
            }
            break;
         case RPT_RX_STOP:
            change_status(RPT_TX_STOP);
            break;
         case RPT_TOT_START:
            PORTD &= ~_BV(LED_TX);
            PORTD |= _BV(LED_TOT);
            tot = 0;
            change_status(RPT_TOT);
            break;
         case RPT_TOT:
            break;
         case RPT_TX_STOP:
            for(int i=0;i<100;i++)
               _delay_ms(10);
            PORTD &= ~_BV(LED_TX);
            PORTD &= ~_BV(LED_TOT);
            change_status(RPT_STBY);
            break;
         default:
            break;
      }
	}
}

