DIR_OUTPUT=output/
FILE_BINARY=${DIR_OUTPUT}main
FILE_OBJECT=${DIR_OUTPUT}main.o ${DIR_OUTPUT}morse.o
FILE_HEX=${FILE_BINARY}.hex

FILE_FUSES=fuses.cfg

MCU_CLOCK_1MHZ=1000000UL
MCU_CLOCK_8MHZ=8000000UL
MCU_CLOCK_16MHZ=16000000UL

MCU_CLOCK=${MCU_CLOCK_8MHZ}


# AVR GCC12 needs --param=min-pagesize=0 to silence array subscript 0 is outside bounds of volatile uint8_t[0] warning 
CFLAGS = -Os -mcall-prologues -g3 -std=gnu99 -Wall -Werror -Wundef --param=min-pagesize=0

all:
	avr-gcc ${CFLAGS} -DF_CPU=${MCU_CLOCK} -mmcu=atmega328p -c -o ${DIR_OUTPUT}morse.o morse.c
	avr-gcc ${CFLAGS} -DF_CPU=${MCU_CLOCK} -mmcu=atmega328p -c -o ${DIR_OUTPUT}main.o main.c
	avr-gcc -mmcu=atmega328p ${FILE_OBJECT} -o ${FILE_BINARY}
	avr-objcopy -O ihex -R .eeprom ${FILE_BINARY} ${FILE_HEX}

flash: all 
	minipro -w ${FILE_HEX} -c code -p ATMEGA328P@DIP28

fuse:
	minipro -w ${FILE_FUSES} -c config -p ATMEGA328P@DIP28 -e

ff: all flash fuse

leds:
	minipro -w examples/turn_on_all_leds.hex -c code -p ATMEGA328P@DIP28
	minipro -w ${FILE_FUSES} -c config -p ATMEGA328P@DIP28 -e

repeater:
	minipro -w examples/cq0ugmr_1_1.hex -c code -p ATMEGA328P@DIP28
	minipro -w ${FILE_FUSES} -c config -p ATMEGA328P@DIP28 -e

clean:
	rm -f ${FILE_BINARY}
	rm -f ${FILE_OBJECT}
	rm -f ${FILE_HEX}
