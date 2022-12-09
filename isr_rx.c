#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdbool.h>

//#define F_CPU 8000000UL
#define MS_DELAY  250
#define TOT_SEC   10     // 180 sec = 3 min.

typedef enum _rx_status_t {
   RPT_STBY,
   RPT_START,
   RPT_RTX,
   RPT_STOP,
   RPT_TOT,
   RPT_ID,
   RX_N
} rx_status_t;

/*
 * Global variables
 */
volatile bool update = false;
volatile rx_status_t rx_status = RPT_STBY; 
volatile unsigned int tot = 0;

/**
 * set update on a high edge
 */
ISR(PCINT0_vect) {
   if (PINB && _BV(PINB5)) {
      // Started Receiving a signal
      // __/```
      PORTD |= _BV(PORTD2);
      if (rx_status == RPT_STBY)
         rx_status = RPT_START;
   } else {
      // Stopped receiving a signal
      // ```\__
      PORTD &= ~_BV(PORTD2);
      rx_status = RPT_STOP;
   }
}

ISR(TIMER1_OVF_vect) {
   //PORTD ^= _BV(PORTB4);
   if (rx_status == RPT_RTX) {
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

int main(void) {

   /* PORT B as Inputs */
	DDRB = 0x00;

   /* Set pins as outputs */
   DDRD |= _BV(DDD4) + _BV(DDD3) + _BV(DDD2) + _BV(DDD0);

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
      switch(rx_status) {
         case RPT_START:
            PORTD |= _BV(PORTD3);
            ATOMIC_BLOCK(ATOMIC_FORCEON) {
               rx_status = RPT_RTX;
            }
            break;
         case RPT_RTX:
            if (tot >= TOT_SEC) {
               ATOMIC_BLOCK(ATOMIC_FORCEON) {
                  tot = 0;
                  PORTD &= ~_BV(PORTD3);
                  PORTD |= _BV(PORTB4);
                  rx_status = RPT_TOT;   
               }
            }
            break;
         case RPT_STOP:
            for(int i=0;i<175;i++)
               _delay_ms(10);
            //Mutex ?
            ATOMIC_BLOCK(ATOMIC_FORCEON) {
               if (rx_status == RPT_STOP) {
                  PORTD &= ~_BV(PORTD3);
                  rx_status = RPT_STBY;
               }
            }
            break;
         case RPT_TOT:
            _delay_ms(15);
            break;
         default:
            break;
      }
      _delay_ms(5);
	}
}

