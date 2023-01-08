/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 3; tab-width: 3 -*- */
/* vim: set tabstop=3 softtabstop=3 shiftwidth=3 expandtab :               */
/*
 * io.h
 *
 * IO Header file
 * 
 *
 * José Miguel Fonte
 */

#ifndef _IO_H_
#define _IO_H_

#include <avr/io.h>

/* IO_RPT_RX
 * PIN B5, pin 19, as input for Receiver COR
 */

#define IO_RPT_RX    PINB5

/* IO_BEEP
 * PIN C0, pin 23, as output for audio beep and morse
 */

#define IO_BEEP      PORTC0

/* IO_PTT
 * PIN D0, pin 2, as output for PTT control
 */

#define IO_PTT       PORTD0

/* IO_RX_UNMUTE
 * PIN D1, pin 3, as output to control receiver
 * audio mute. A digital zero (0) mutes the receiver.
 * A digital one (1) unmutes the receiver.
 */

#define IO_RX_UNMUTE PORTD1

/* IO_LED_x
 * Uses PIN D2, D3 and D4 (pin 4, 5 and 6) as output
 * for leds, used as RX, TX and TOT indicators.
 * TX led flashing equals ID.
 */

#define IO_LED_RX    PORTD2
#define IO_LED_TX    PORTD3
#define IO_LED_TOT   PORTD4

/* IO_ISD_PLAY
 * Uses PIN D5, pin 11, as output for ISD play control.
 */

#define IO_ISD_PLAY  PORTD5

/* IO HELPER FUNCTIONS
 * These macros just beautify the code to me.
 * Otherwise redundant
 */

#define IO_ENABLE(port, out)     (port |= _BV(out))
#define IO_DISABLE(port, out)    (port &= ~_BV(out))
#define IO_TOGGLE(port, out)     (port ^= _BV(out))
#define IO_IS_ENABLED(port, in)  (port && _BV(in))

#endif /* _IO_H_ */
