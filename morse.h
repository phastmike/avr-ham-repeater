/* -*- Mode: Vala; indent-tabs-mode: nil; c-basic-offset: 3; tab-width: 3 -*- */
/* vim: set tabstop=3 softtabstop=3 shiftwidth=3 expandtab :                  */
/*
 * morse.h
 *
 * Morse Header file
 *
 * Uses Mark VandeWettering K6HX algorithm
 * As no other reference other than ON7EQ
 * Nevertheless tnx to author, nice catch
 * 
 * José Miguel Fonte
 */

#ifndef _MORSE_H_
#define _MORSE_H_

typedef struct _morse_t morse_t;

morse_t *                        morse_new(void);
unsigned char                    morse_speed_get(morse_t * morse);
void                             morse_speed_set(morse_t * morse,unsigned char speed);
void                             morse_weigh_set(morse_t * morse,float weight);
float                            morse_weight_get(morse_t * morse);
unsigned char                    morse_length_dashed(morse_t * morse);
unsigned char                    morse_length_dot(morse_t * morse);
void                             morse_send_msg(morse_t * morse,char * str);

/* delegates | callbacks */

void morse_beep_delegate_connect(morse_t *morse, void (*delegate)(unsigned int duration));
void morse_delay_delegate_connect(morse_t *morse, void (*delegate)(unsigned int duration));


#endif /* _MORSE_H_ */
