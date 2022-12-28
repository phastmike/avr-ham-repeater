/* -*- Mode: Vala; indent-tabs-mode: nil; c-basic-offset: 3; tab-width: 3 -*- */
/* vim: set tabstop=3 softtabstop=3 shiftwidth=3 expandtab :                  */
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

#define IO_RPT_RX    PINB5

#define IO_BEEP      PORTC0

#define IO_PTT       PORTD0
#define IO_RX_MUTE   PORTD1
#define IO_LED_RX    PORTD2
#define IO_LED_TX    PORTD3
#define IO_LED_TOT   PORTD4
#define IO_ISD_PLAY  PORTD5

#define IO_ENABLE(port, out)     (port |= _BV(out))
#define IO_DISABLE(port, out)    (port &= ~_BV(out))
#define IO_TOGGLE(port, out)     (port ^= _BV(out))
#define IO_IS_ENABLED(port, in)  (port && _BV(in))

#endif /* _IO_H_ */
