/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 3; tab-width: 3 -*- */
/* vim: set tabstop=3 softtabstop=3 shiftwidth=3 expandtab :               */
/*
 * morse.h
 *
 * Morse implementation file
 * 
 * Uses Mark VandeWettering K6HX algorithm
 * as referenced by ON7EQ. Nevertheless tnx
 * to author, nice catch.
 *
 * José Miguel Fonte
 */

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "morse.h"

#define DEFAULT_WPM     24
#define WPM_MIN         10
#define WPM_MAX         60
#define DEFAULT_WEIGHT  3.0 /* CW dit to dash weight/ratio. Default as 3.5 */
#define WEIGHT_MIN      2.5
#define WEIGHT_MAX      4.5
#define DOTLEN          (1200/CW_SPEED)
#define DASHLEN         (WEIGHT * DOTLEN)

struct _morse_t {
   unsigned char speed;
   float weight;
   unsigned char length_dot;
   unsigned char length_dash; 

   void (* beep_delegate)(unsigned int duration);
   void (* delay_delegate)(unsigned int duration);
};


struct t_mtab { char c, pat; };

struct t_mtab morsetab[] = {
  	{'.', 106},
	{',', 115},
	{'?', 76},
	{'/', 41},
	{'A', 6},
	{'B', 17},
	{'C', 21},
	{'D', 9},
	{'E', 2},
	{'F', 20},
	{'G', 11},
	{'H', 16},
	{'I', 4},
	{'J', 30},
	{'K', 13},
	{'L', 18},
	{'M', 7},
	{'N', 5},
	{'O', 15},
	{'P', 22},
	{'Q', 27},
	{'R', 10},
	{'S', 8},
	{'T', 3},
	{'U', 12},
	{'V', 24},
	{'W', 14},
	{'X', 25},
	{'Y', 29},
	{'Z', 19},
	{'1', 62},
	{'2', 60},
	{'3', 56},
	{'4', 48},
	{'5', 32},
	{'6', 33},
	{'7', 35},
	{'8', 39},
	{'9', 47},
	{'0', 63}
};

#define N_MORSE   (sizeof(morsetab)/sizeof(morsetab[0]))

/* Private */

static void dash(morse_t *morse) {
   assert(morse != NULL);
   morse->beep_delegate(morse->length_dash);
   morse->delay_delegate(morse->length_dot);
}

static void dit(morse_t *morse) {
   assert(morse != NULL);
   morse->beep_delegate(morse->length_dot);
   morse->delay_delegate(morse->length_dot);
}

static void send(morse_t *morse, char c) {
   assert(morse != NULL);
   
   int i ;
   if (c == ' ') {
      morse->delay_delegate(7 * morse->length_dot);
      return ;
   }
   
   if (c == '+') {
      morse->delay_delegate(4 * morse->length_dot);
      dit(morse);
      dash(morse);
      dit(morse);
      dash(morse);
      dit(morse);
      morse->delay_delegate(4 * morse->length_dot);
      return ;
   }    
    
   for (i=0; i<N_MORSE; i++) {
      if (morsetab[i].c == c) {
         unsigned char p = morsetab[i].pat;
         while (p != 1) {
            if (p & 1)
               dash(morse);
            else
               dit(morse);
            p = p / 2 ;
         }
         morse->delay_delegate(2 * morse->length_dot);
         return ;
      }
   }
}

/* Public */

morse_t * morse_new(void) {
   morse_t *morse = (morse_t *) calloc(1, sizeof(morse_t));
   morse->speed = DEFAULT_WPM;
   morse->weight = DEFAULT_WEIGHT;
   morse->length_dot = (1200/morse->speed);
   morse->length_dash= (morse->weight * morse->length_dot);
   morse->beep_delegate = NULL;
   morse->delay_delegate= NULL;
   return morse;
}

void morse_speed_set(morse_t *morse, unsigned char speed) {
   assert(morse != NULL);
   assert(speed >= WPM_MIN && speed <= WPM_MAX);
   morse->speed = speed;
   morse->length_dot = (1200/morse->speed);
   morse->length_dash= (morse->weight * morse->length_dot);
}

unsigned char morse_speed_get(morse_t *morse) {
   assert(morse != NULL);
   return morse->speed;
}

void morse_weigh_set(morse_t *morse, float weight) {
   assert(morse != NULL);
   assert(weight >= WEIGHT_MIN && weight <= WEIGHT_MAX);
   morse->weight = weight;
   morse->length_dash = (morse->weight * morse->length_dot);
}

float morse_weight_get(morse_t *morse) {
   assert(morse != NULL);
   return morse->weight;
}

unsigned char morse_length_dot(morse_t *morse) {
   assert(morse != NULL);
   return morse->length_dot;
}

unsigned char morse_length_dashed(morse_t *morse) {
   assert(morse != NULL);
   return morse->length_dash;
}

void morse_beep_delegate_connect(morse_t *morse, void (*delegate)(unsigned int duration)) {
   assert(morse != NULL);
   morse->beep_delegate = delegate;
}

void morse_delay_delegate_connect(morse_t *morse, void (*delegate)(unsigned int duration)) {
   assert(morse != NULL);
   morse->delay_delegate = delegate;
}

void morse_send_msg(morse_t *morse, char *str) {
   assert(morse != NULL);
   char *msg = strupr(str);
   while (*msg)
      send(morse, *msg++) ;
}

