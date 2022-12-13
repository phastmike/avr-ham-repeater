#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdbool.h>

#include "io.h"

//#define F_CPU           8000000UL
#define MS_DELAY        250
#define TIME_ID_SEC     30
#define TIME_TOT_SEC    10     // 180 sec = 3 min.
#define TIME_WAIT_ID    6

typedef enum _repeater_status_t {
   RPT_STBY,
   RPT_TX_START,
   RPT_RTX,
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
volatile unsigned int counter_tot = 0;
volatile unsigned int counter_id  = 0;
volatile unsigned int counter_wait= 0;
volatile bool time_to_tot = false;
volatile bool time_to_id  = false;
volatile bool tot_enabled = false;

/**
 * set update on a high edge
 */
ISR(PCINT0_vect) {
   if (PINB && _BV(PINB5)) {
      // Started Receiving a signal
      // __/```
      IO_ENABLE(IO_LED_RX);
      repeater_is_receiving = true;
   } else {
      // Stopped receiving a signal
      // ```\__
      IO_DISABLE(IO_LED_RX);
      repeater_is_receiving = false;
   }
}

ISR(TIMER0_OVF_vect){
   if (IOB_IS_ENABLED(PINB5)) {
   //if (PINB && _BV(PINB5)) {
      // Started Receiving a signal
      // __/```
      IO_ENABLE(IO_LED_RX);
      repeater_is_receiving = true;
   } else {
      // Stopped receiving a signal
      // ```\__
      IO_DISABLE(IO_LED_RX);
      repeater_is_receiving = false;
   }

   TCNT0 = 255-100;
}

ISR(TIMER1_OVF_vect) {
   if (counter_id <= TIME_ID_SEC) {
      counter_id++;
      if (counter_id > TIME_ID_SEC) {
         time_to_id = true;
      }
   }

   if (counter_tot <= TIME_TOT_SEC) {
      counter_tot++;
   }

   if (time_to_id && counter_wait <= TIME_WAIT_ID) {
      counter_wait++;
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
   DDRD |= _BV(DDD5) + _BV(DDD4) + _BV(DDD3) + _BV(DDD2) + _BV(DDD1) + _BV(DDD0);

   intro_sequence();

	/**
	 * Pin Change Interrupt enable on PCINT0 (PB0)
	 */
	//PCICR  = _BV(PCIE0);
	//PCMSK0 |= _BV(PCINT5);

   
   /**
    * Set Timer 0 - 1 Hz | 1 sec.
    */
   TCNT0  = 255-99;
   TIMSK0 = (1 << TOIE0); 
   TCCR0A = 0x00;
   TCCR0B = (1 << CS11); //Set Prescaler to 8 (bit 11) and start timer

   /**
    * Set Timer 1 - 1 Hz | 1 sec.
    */
   TCNT1  = 65535 - (F_CPU/1024);
   TIMSK1 = (1 << TOIE1); 
   TCCR1A = 0x00;
   TCCR1B = (1 << CS10) | (1 << CS12); //Set Prescaler to 1024 (bit 10 and 12) and start timer


	// Turn interrupts on.
	sei();

	while(true) {

loop_start:

      /*
      while (!IOB_IS_ENABLED(PINB5)) {
         asm("");
      }
      */

      if (IOB_IS_ENABLED(PINB5)) {
         IO_ENABLE(IO_LED_TX);
         IO_ENABLE(IO_PTT);
         counter_tot = 0;

         while (IOB_IS_ENABLED(PINB5)) {
            asm("");
            if (counter_tot > TIME_TOT_SEC) {
               IO_DISABLE(IO_LED_TX);
               IO_DISABLE(IO_PTT);
               tot_enabled = true;
               IO_ENABLE(IO_LED_TOT);
            }
         }

         if (tot_enabled) {
            tot_enabled = false; 
            IO_DISABLE(IO_LED_TOT);
            // TOT Enabled so we don't need the long tail
         } else {
            // Normal tail ending. Add some time and beep
            delay_sec(1);
            IO_DISABLE(IO_LED_TX);
         }

         counter_wait = 0;
      }

      /**
       * It's time to ID? We cant count if it has
       * been free in the last TIME_WAIT_ID seconds
       */
      if (time_to_id && counter_wait > TIME_WAIT_ID) {
         time_to_id = false;
         for (int c=0; c<8; c++) {
            IO_ENABLE(IO_LED_TX);
            _delay_ms(250); 
            IO_DISABLE(IO_LED_TX);
            _delay_ms(250);
         }

         counter_id = 0;
         counter_wait = 0;
      }
   }
}

