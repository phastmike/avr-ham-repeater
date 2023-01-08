/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 3; tab-width: 3 -*- */
/* vim: set tabstop=3 softtabstop=3 shiftwidth=3 expandtab :               */
/*
 * main.c
 *
 * ATMEGA328P Repeater controller for CQ0UGMR
 *
 * CT1ENQ
 * José Miguel Fonte
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "io.h"
#include "morse.h"

/* F_CPU
 * 
 * Could define CPU speed with 
 * #define F_CPU 8000000UL
 * for an 8MHz external crystal (XTAL)
 *
 * Or, as we are doing, pass the definition to the compiler
 * So we don't need to edit the code in case we change the 
 * XTAL and just recompile passing -D F_CPU <value>
 */

/* TIME_ID_SEC & TIME_WAIT_ID
 * Time for ID in seconds. For 10 minutes, 60 sec/min * 10 min = 600 
 * Reaching time to id we wait TIME_WAIT_ID seconds to ID.
 * We subtract the wait time to make the "correct" time = 600 - TIME_WAIT_ID.
 */

#define TIME_WAIT_ID    6
#define TIME_ID_SEC     (600 - TIME_WAIT_ID)

/* N_ID_FOR_MORSE
 * Number of ISD Identifications at which the morse ID 
 * will play after the ISD ID (Voice ID). 
 * For 10 minutes, 600 sec. IDs, every hour, morse ID
 * will follow along.
 *
 * Default: 6
 */

#define N_ID_FOR_MORSE  6

/* MORSE_ID_x
 * Set of strings used for morse code regarding this repeater
 */

#define MORSE_WPM       28
#define MORSE_MSG_CALL  "CQ0UGMR"
#define MORSE_MSG_QTH   "IN51UK"
#define MORSE_TOT_INFO  "TOT"
#define MORSE_TOT_END   "K"
#define MORSE_RPT_START "S"
#define MORSE_ID        MORSE_MSG_CALL // MORSE_MSG_CALL " " MORSE_MSG_QTH


/* TIME_TOT_SEC
 * Time Out Timer duration, Our default time is 3 min = 180 sec.
 * Added additional 2 seconds for radios with only 3 minutes TOT
 * So they can timeout before the repeater.
 *
 * Default: 180 + 2
 */

#define TIME_TOT_SEC    182

/* Other definitions */

#define BEEP_RX_OFF_ENABLED               false
#define DEFAULT_BEEP_DURATION_MS          40
#define DEFAULT_TX_OFF_PENALTY_MS         200
#define DEFAULT_TAIL_DURATION_MS          800 
#define DEFAULT_TOT_INHIBIT_DURATION_MS   1500
#define DEFAULT_INHIBIT_TX_DURATION_SEC   5

/* GLOBAL VARIABLES */

volatile unsigned int counter_tot         = 0;
volatile unsigned int counter_id          = 0;
volatile unsigned int counter_wait        = 0;
volatile unsigned int counter_beep        = 0;
volatile unsigned int counter_tail        = 0;
volatile unsigned int counter_tot_inhibit = 0;
volatile unsigned int counter_inhibit_tx  = 0;
volatile bool time_to_tot                 = false;
volatile bool time_to_id                  = false;
volatile bool tot_enabled                 = false;
volatile bool rx_audio_disable            = false;
volatile unsigned int beep_hperiod        = 4;
static bool beep_tot_played               = false;
static bool tail_pending                  = false;
static bool tot_inhibit                   = false;
static unsigned char n_id                 = 0;
static bool tot_play_end                  = false;

/******************************************************************************
 * TIMER ISR's
 *****************************************************************************/

/* TIMER 0 OVERFLOW ISR
 * Runs at 1 us and counts 100 = 100us period.
 * This ISR enables/disables the RX LED and
 * also toggles the 4066 switch (RX AUDIO)
 */

ISR(TIMER0_OVF_vect){
   if (IO_IS_ENABLED(PINB, IO_RPT_RX)) {
      // Started Receiving a signal
      // __/```

      IO_ENABLE(PORTD, IO_LED_RX);

      if (!tot_enabled && !rx_audio_disable) {
         IO_ENABLE(PORTD, IO_RX_UNMUTE);
      }

      if (tot_inhibit) counter_tot_inhibit = 0;

   } else {
      // Stopped receiving a signal
      // ```\__

      IO_DISABLE(PORTD, IO_LED_RX);

      if (!tot_enabled && !rx_audio_disable) {
         IO_DISABLE(PORTD, IO_RX_UNMUTE);
      }
   }

   if (tail_pending) counter_tail++;
   if (tot_inhibit) counter_tot_inhibit++;

   TCNT0 = 256-100;
}

/* TIMER 1 OVERFLOW ISR
 * Runs every 1 sec and updates the counters
 * which use seconds to count.
 */

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

   if (tot_inhibit) counter_inhibit_tx++;

   TCNT1  = 65536 - (F_CPU/1024);
}

/* TIMER 2 OVERFLOW ISR
 * Runs at 1uS and counts 100 = 100us.
 * Being used for audio generation, morse and beeps

    ___/```\___/```\__''__|
   |---|---|---|---|--''--|

 * Timer counts hperiod * 100uS then toggles state
 * This means that the period for a square wave
 * on the output pin is:
 *
 * period (sec) =  2 * hperiod * 100us
 * frequency (Hz) = 1 / ( 2 * hperiod * 100u)
 *
 * Example:
 *
 * hperiod = 4
 * frequency = 1 / (2 * 4 * 100u) = 1 / 800u
 *           = 0.00125 MHz = 1.25 kHz = 1250 Hz
 */


ISR(TIMER2_OVF_vect) {
   counter_beep++;
   if (counter_beep > beep_hperiod) {
      IO_TOGGLE(PORTC, IO_BEEP);
      counter_beep = 0;
   }
   TCNT2 = 256-100;
}

/******************************************************************************
 * DELAY FUNCTIONS - BLOCKING delays based on avr _delay_ms 
 *****************************************************************************/

void delay_sec(unsigned int sec) {
   if (sec > 0 && sec <= 65535) {
      for (int c = 0; c <= sec; c++) {
         _delay_ms(1000);
      }
   }
}

void delay_ms(unsigned int ms) {
   if (ms > 0 && ms <= 65535) {
      for (int c = 0; c <= ms; c++) {
         _delay_ms(1);
      }
   }
}


/*
void change_status(repeater_status_t status) {
   ATOMIC_BLOCK(ATOMIC_FORCEON) {
      repeater_status = status;
   }
}
*/


/******************************************************************************
 * BEEP and BEEPS - Audio functions. 
 *****************************************************************************/

void beep(unsigned char hperiod, unsigned int duration) {
   beep_hperiod = hperiod;
   TCCR2B = (1 << CS21); //Set Prescaler to 8 (bit 11 => 1 MHz) and start timer
   delay_ms(duration);
   TCCR2B = 0;
}

void beep_morse(unsigned int duration) {
   beep(6, duration);
}

void beep_rx_off(void) {
   beep(20, DEFAULT_BEEP_DURATION_MS);
}

void beep_tail_normal(void) {
   beep(4, DEFAULT_BEEP_DURATION_MS * 2);
}

void beep_tail_id(void) {
   beep(4, DEFAULT_BEEP_DURATION_MS);
   delay_ms(40);
   beep(4, DEFAULT_BEEP_DURATION_MS);
}

void beep_timeout(void) {
   beep(3, 25);
   beep(5, 25);
   beep(4, 25);
   beep(1, 25);
   beep(6, 25);
   beep(2, 25);
   beep(8, 25);
}

void beep_on_boot(void) {
   for(int x=128; x > 0; x--) {
      beep(x,5);
   }
}

/******************************************************************************
 * HELPER FUNCTIONS 
 *****************************************************************************/

/* Wait a bit for radios to stabilize
 * And do a boot animation by lighting
 * all leds sequentially and remain light
 * over 2 sec.
 */

void intro_sequence(void) {
   PORTD = 0x0;
   delay_ms(500);
   IO_ENABLE(PORTD, IO_LED_RX);
   delay_ms(500);
   IO_ENABLE(PORTD, IO_LED_TX);
   delay_ms(500);
   IO_ENABLE(PORTD, IO_LED_TOT);
   delay_ms(2500);
   PORTD = 0x0;
}

/* Setup IO ports */

void setup_io(void) {
   /* PORT B as Inputs */
   DDRB = 0x00;

   /* Set pins as outputs */
   DDRC |= _BV(DDC0);
   DDRD |= _BV(DDD5) + _BV(DDD4) + _BV(DDD3) + _BV(DDD2) + _BV(DDD1) + _BV(DDD0);
}

/******************************************************************************
 * APPLICATION ENTRY POINT 
 *****************************************************************************/

int main(void) {

   setup_io();
   intro_sequence();

   /* Morse generator init */
   morse_t *morse = morse_new();
   morse_speed_set(morse, MORSE_WPM);
   morse_beep_delegate_connect(morse, beep_morse);
   morse_delay_delegate_connect(morse, delay_ms);

   /* TIMER 0
    *
    * Set Timer to 100usec. With XTAL 8MHz / 8 = 1MHz.
    * Prescaler set to 8 we get 1MHZ which equals 1usec.
    * Count 100 times and we have 100usec
    * TCCR0B = (1 << CS01) sets the Prescaler to 8 (bit 11)
    * and starts the timer
    */

   TCNT0  = 256-100;
   TIMSK0 = (1 << TOIE0); 
   TCCR0A = 0x00;
   TCCR0B = (1 << CS01);

   /* TIMER 1
    *
    * We will use this timer as a 1 second clock. Prescaler at 1024
    * and count MAX_INT - (8MHz/1024).
    * TCCR1B = (1 << CS10) | (1 << CS12); sets the prescaler to 1024
    * (bit 10 and 12) and starts the timer
    */

   TCNT1  = 65535 - (F_CPU/1024);
   TIMSK1 = (1 << TOIE1); 
   TCCR1A = 0x00;
   TCCR1B = (1 << CS10) | (1 << CS12); //Set Prescaler to 1024 (bit 10 and 12) and start timer

   /* TIMER 2
    *
    * Sets the timer to 1usec as timer 0. But we use it as needed
    * and control the timer with TCCR2B. This timer with the ISR will
    * generate the beeps being used in the repeater.
    * Count 100 and we have 100usec as reference. The ISR will toggle
    * the output pin.
    *
    */

   TCNT2  = 256-100;
   TIMSK2 = (1 << TOIE2); 
   TCCR2A = 0x00;
   TCCR2B = 0;


   /* Turn interrupts on */ 
   sei();

   /* On boot beeping */
   IO_ENABLE(PORTD, IO_LED_TX);
   IO_ENABLE(PORTD, IO_PTT);

   delay_ms(500);
   beep_on_boot();
   delay_ms(500);
   morse_send_msg(morse, MORSE_RPT_START);
   delay_ms(500);
   
   IO_DISABLE(PORTD, IO_LED_TX);
   IO_DISABLE(PORTD, IO_PTT);

   delay_ms(500);

   /* Superloop */

   while(true) {

      if (IO_IS_ENABLED(PINB, PINB5)) {
         IO_ENABLE(PORTD, IO_LED_TX);
         IO_ENABLE(PORTD, IO_PTT);
         if (!tail_pending) {
            counter_tot = 0;
         }

         beep_tot_played = false;

         while (IO_IS_ENABLED(PINB, PINB5) && !tot_enabled) {
            asm("");
            
            if (counter_tot > TIME_TOT_SEC && !beep_tot_played) {
               IO_ENABLE(PORTD, IO_LED_TOT);
               tot_enabled = true;
               delay_ms(100);
               IO_DISABLE(PORTD, IO_RX_UNMUTE);
               beep_timeout();
               delay_ms(100);
               beep_tot_played = true;

               IO_DISABLE(PORTD, IO_LED_TX);
               IO_DISABLE(PORTD, IO_PTT);
            }
         }

         if (tot_enabled) {
            counter_inhibit_tx = 0;
            tot_inhibit = true;
            counter_tot_inhibit = 0;
            tot_play_end = false;

            while (counter_tot_inhibit < (DEFAULT_TOT_INHIBIT_DURATION_MS * 10)) {
               IO_DISABLE(PORTD, IO_LED_TOT);
               delay_ms(50); 
               if(counter_inhibit_tx >= DEFAULT_INHIBIT_TX_DURATION_SEC) {
                  IO_ENABLE(PORTD, IO_LED_TOT);
                  IO_ENABLE(PORTD, IO_LED_TX);
                  IO_ENABLE(PORTD, IO_PTT);
                  delay_ms(200);
                  morse_send_msg(morse, MORSE_TOT_INFO);
                  delay_ms(200);
                  IO_DISABLE(PORTD, IO_LED_TX);
                  IO_DISABLE(PORTD, IO_PTT);
                  counter_inhibit_tx = 0;
                  tot_play_end = true;
               }
               IO_ENABLE(PORTD, IO_LED_TOT);
               delay_ms(50); 
            }

            IO_DISABLE(PORTD, IO_LED_TOT);

            if (tot_play_end) {
               IO_ENABLE(PORTD, IO_LED_TX);
               IO_ENABLE(PORTD, IO_PTT);
               delay_ms(200);
               morse_send_msg(morse, MORSE_TOT_END);
               delay_ms(200);
               IO_DISABLE(PORTD, IO_LED_TX);
               IO_DISABLE(PORTD, IO_PTT);
            }

            tot_inhibit = false;
            tot_enabled = false; 
            tail_pending = false;
         } else {
            // Normal tail ending. Add some time and beep
            tail_pending = true;
            if (BEEP_RX_OFF_ENABLED) {
               delay_ms(200);
               beep_rx_off();
            }
         }

         counter_tail = 0;
         counter_wait = 0;
         counter_tot_inhibit = 0;
      }

      if (counter_tail >= (DEFAULT_TAIL_DURATION_MS * 10) && tail_pending && !IO_IS_ENABLED(PINB, IO_RPT_RX)) {

         rx_audio_disable = true;
         if (time_to_id) { 
            beep_tail_id();
         } else {
            beep_tail_normal();
         }

         IO_DISABLE(PORTD, IO_LED_TX);
         IO_DISABLE(PORTD, IO_PTT);
         delay_ms(DEFAULT_TX_OFF_PENALTY_MS);
         rx_audio_disable = false;
         tail_pending = false;
         counter_tail = 0;
      }

      /**
       * It's time to ID? If so, we can count if it has
       * been free in the last TIME_WAIT_ID seconds
       */

      if (time_to_id && counter_wait > TIME_WAIT_ID) {
         time_to_id = false;
         IO_ENABLE(PORTD, IO_ISD_PLAY);
         IO_ENABLE(PORTD, IO_PTT);
         
         for (int c=0; c<21; c++) {
            IO_ENABLE(PORTD, IO_LED_TX);
            delay_ms(250); 
            IO_DISABLE(PORTD, IO_LED_TX);
            delay_ms(250);
         }

         IO_DISABLE(PORTD, IO_ISD_PLAY);
         n_id++;
         if (n_id >= N_ID_FOR_MORSE) {
            IO_ENABLE(PORTD, IO_LED_TX);
            delay_ms(100);
            morse_send_msg(morse, MORSE_ID);
            delay_ms(100);
            n_id = 0;
            IO_DISABLE(PORTD, IO_LED_TX);
         }
         
         if (!IO_IS_ENABLED(PINB, IO_RPT_RX)) {
            IO_DISABLE(PORTD, IO_LED_TX);
            IO_DISABLE(PORTD, IO_PTT);
            delay_ms(DEFAULT_TX_OFF_PENALTY_MS);
         } else {
            IO_ENABLE(PORTD, IO_LED_TX);
         }
         counter_id = 0;
         counter_wait = 0;
      }
   }
}

