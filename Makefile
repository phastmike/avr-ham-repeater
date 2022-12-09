FILE_BINARY=isr_rx
FILE_SOURCE=${FILE_BINARY}.c
FILE_OBJECT=${FILE_BINARY}.o
FILE_HEX=${FILE_BINARY}.hex

MCU_CLOCK_1MHZ=1000000UL
MCU_CLOCK_8MHZ=8000000UL
MCU_CLOCK_16MHZ=16000000UL

MCU_CLOCK=${MCU_CLOCK_8MHZ}

FILE_FUSES=fuses.cfg

all:
	avr-gcc -Os -DF_CPU=${MCU_CLOCK} -mmcu=atmega328p -c -o ${FILE_OBJECT} ${FILE_SOURCE}
	avr-gcc -mmcu=atmega328p ${FILE_OBJECT} -o ${FILE_BINARY}
	avr-objcopy -O ihex -R .eeprom ${FILE_BINARY} ${FILE_HEX}

flash:
	minipro -w ${FILE_HEX} -c code -p ATMEGA328P@DIP28

fuse:
	minipro -p ATMEGA328P@DIP28 -c config -w ${FILE_FUSES} -e

clean:
	rm -f ${FILE_BINARY}
	rm -f ${FILE_OBJECT}
	rm -f ${FILE_HEX}
