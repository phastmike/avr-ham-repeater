#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdbool.h>

#include "io.h"
#include "morse.h"

//#define F_CPU           8000000UL
#define MS_DELAY        250
#define TIME_ID_SEC     60    // 10 min.  FIXME: remove wait time
#define TIME_TOT_SEC    10     // 180 sec = 3 min.
#define TIME_WAIT_ID    10 

/*
 * Global variables
 */
volatile bool update = false;
volatile bool repeater_is_receiving = false;
volatile unsigned int counter_tot = 0;
volatile unsigned int counter_id  = 0;
volatile unsigned int counter_wait= 0;
volatile unsigned int counter_beep= 0;
volatile unsigned int counter_tail= 0;
volatile bool time_to_tot = false;
volatile bool time_to_id  = false;
volatile bool tot_enabled = false;
volatile unsigned int beep_freq   = 17; //
static bool beep_tot_played = false;
static bool tail_pending = false;


void beep(unsigned char freq, unsigned int duration);
void delay_ms(unsigned int ms);


/**
 * PIN Change interrupt does not work very well, there are misses
 * which are unacceptable. 
 */
ISR(PCINT0_vect) {
   if (PINB && _BV(PINB5)) {
      // Started Receiving a signal
      // __/```
      IO_ENABLE(PORTD, IO_LED_RX);
      repeater_is_receiving = true;
   } else {
      // Stopped receiving a signal
      // ```\__
      IO_DISABLE(PORTD, IO_LED_RX);
      repeater_is_receiving = false;
   }
}

ISR(TIMER0_OVF_vect){
   if (IO_IS_ENABLED(PINB, PINB5)) {
      // Started Receiving a signal
      // __/```
      IO_ENABLE(PORTD, IO_LED_RX);
      repeater_is_receiving = true;
   } else {
      // Stopped receiving a signal
      // ```\__
      IO_DISABLE(PORTD, IO_LED_RX);
      repeater_is_receiving = false;
   }

   TCNT0 = 255-99;
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

   if (tail_pending) counter_tail++;

   TCNT1  = 65535 - (F_CPU/1024);
}

ISR(TIMER2_OVF_vect) {
   counter_beep++;
   if (counter_beep > beep_freq) {
      IO_TOGGLE(PORTC, IO_BEEP);
      counter_beep = 0;
   }
   TCNT2  = 255-99;
}

void delay_sec(unsigned int sec) {
   // FIXME: sec overflow
   if (sec > 0 && sec <= 65535) {
      for (int c = 0; c <= (sec * 1000); c++) { 
         _delay_ms(1);
      }
   }
}

void delay_ms(unsigned int ms) {
   // FIXME:
   if (ms > 0 && ms <= 65535) {
      for (int c = 0; c <= ms; c++) { 
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
   IO_ENABLE(PORTD, IO_LED_RX);
   delay_sec(1);
   IO_ENABLE(PORTD, IO_LED_TX);
   delay_sec(1);
   IO_ENABLE(PORTD, IO_LED_TOT);
   delay_sec(1);
   PORTD = 0x0;
}

/*
void change_status(repeater_status_t status) {
   ATOMIC_BLOCK(ATOMIC_FORCEON) {
      repeater_status = status;
   }
}
*/

void beep(unsigned char freq, unsigned int duration) {
   beep_freq = freq;
   TCCR2B = (1 << CS21); //Set Prescaler to 8 (bit 11 => 1 MHz) and start timer
   delay_ms(duration);
   TCCR2B = 0;
}

int main(void) {

   /* PORT B as Inputs */
	DDRB = 0x00;

   /* Set pins as outputs */
   DDRC |= _BV(DDC0);
   DDRD |= _BV(DDD5) + _BV(DDD4) + _BV(DDD3) + _BV(DDD2) + _BV(DDD1) + _BV(DDD0);

   intro_sequence();

	/**
	 * Pin Change Interrupt enable on PCINT0 (PB0)
	 */
	//PCICR  = _BV(PCIE0);
	//PCMSK0 |= _BV(PCINT5);

   
   /**
    * Set Timer 0 - 100usec
    * Timer 8MHz/8 = 1MHz
    * Count 100 => 100usec
    */
   TCNT0  = 255-99;
   TIMSK0 = (1 << TOIE0); 
   TCCR0A = 0x00;
   TCCR0B = (1 << CS01); //Set Prescaler to 8 (bit 11) and start timer

   /**
    * Set Timer 1 - 1 Hz | 1 sec.
    * Count MAX_INT - (8MHz/1024)
    */
   TCNT1  = 65535 - (F_CPU/1024);
   TIMSK1 = (1 << TOIE1); 
   TCCR1A = 0x00;
   TCCR1B = (1 << CS10) | (1 << CS12); //Set Prescaler to 1024 (bit 10 and 12) and start timer

   /**
    * Set Timer 2 Test for 1kHz, 500usec on 500us off
    *
    */
   TCNT2  = 255-99;
   TIMSK2 = (1 << TOIE2); 
   TCCR2A = 0x00;
   //TCCR2B = (1 << CS21); //Set Prescaler to 8 (bit 11) and start timer
   TCCR2B = 0;

   morse_t *morse = morse_new();
   morse_beep_delegate_connect(morse, beep);
   morse_delay_delegate_connect(morse, delay_ms);

	// Turn interrupts on.
	sei();

   IO_ENABLE(PORTD, IO_RX_MUTE);


	while(true) {

      if (IO_IS_ENABLED(PINB, PINB5)) {
         IO_ENABLE(PORTD, IO_LED_TX);
         IO_ENABLE(PORTD, IO_PTT);
         counter_tot = 0;

         beep_tot_played = false;

         while (IO_IS_ENABLED(PINB, PINB5)) {
            asm("");
            
            if (counter_tot > TIME_TOT_SEC && !beep_tot_played) {
               IO_ENABLE(PORTD, IO_LED_TOT);
               
               // TIMEOUT
               beep(3, 25);
               beep(5, 25);
               beep(4, 15);
               beep(6, 25);
               beep(2, 25);
               beep(8, 15);

               delay_ms(300);
               beep_tot_played = true;

               IO_DISABLE(PORTD, IO_LED_TX);
               IO_DISABLE(PORTD, IO_PTT);
               tot_enabled = true;
            }
         }


         if (tot_enabled) {
            // TOT Enabled so we don't need the long tail
            tot_enabled = false; 
            tail_pending = false;
            IO_DISABLE(PORTD, IO_LED_TOT);
            // FIXME: Add 1 sec penalti. Figure out the best way...
            //delay_sec(1);
         } else {
            // Normal tail ending. Add some time and beep
            tail_pending = true;
            delay_ms(800);
            beep(16,25);
         }

         counter_tail = 0;
         counter_wait = 0;
      }

      if (counter_tail >= 4 && tail_pending) {
         tail_pending = false;
         counter_tail = 0;

         if (time_to_id) { 
            beep(4, 40);
            delay_ms(40);
            beep(4, 40);
         } else {
            beep(10, 40);
         }

         //morse_send_msg(morse, "R K");
         delay_ms(500);

         IO_DISABLE(PORTD, IO_LED_TX);
         IO_DISABLE(PORTD, IO_PTT);
      }

      /**
       * It's time to ID? If so, we can count if it has
       * been free in the last TIME_WAIT_ID seconds
       */
      if (time_to_id && counter_wait > TIME_WAIT_ID) {
         time_to_id = false;
         IO_DISABLE(PORTD, IO_RX_MUTE);
         IO_ENABLE(PORTD, IO_ISD_PLAY);
         IO_ENABLE(PORTD, IO_PTT);
         for (int c=0; c<21; c++) {
            IO_ENABLE(PORTD, IO_LED_TX);
            _delay_ms(250); 
            IO_DISABLE(PORTD, IO_LED_TX);
            _delay_ms(250);
         }
         IO_DISABLE(PORTD, IO_ISD_PLAY);
         IO_DISABLE(PORTD, IO_PTT);
         IO_ENABLE(PORTD, IO_RX_MUTE);

         counter_id = 0;
         counter_wait = 0;
      }
   }
}

