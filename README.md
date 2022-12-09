# Tips,

## Install toolchain
```
>$ sudo apt-get install gcc-avr binutils-avr avr-libc
>$ sudo apt-get install avrdude
```

and avr-gcc

```
Install package 'avr-gcc' to provide command 'avr-gcc'? [N/y] y


 * Waiting in queue... 
The following packages have to be installed:
 avr-gcc-1:11.2.0-1.fc36.x86_64	Cross Compiling GNU GCC targeted at avr
Proceed with changes? [N/y] y
```

## Compile

```
>$ avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o blink_led.o blink_led.c

We create the executable:

>$ avr-gcc -mmcu=atmega328p blink_led.o -o blink_led

And we convert the executable to a binary file:

>$ avr-objcopy -O ihex -R .eeprom blink_led blink_led.hex

Finally, we can upload the binary file:

>$ avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:blink_led.hex

Look at your Arduino Uno! The led is blinking!
```

## Weird things

After upgrading to Fedora 37 the infoic.xml error appeared.
Workaround is to copy the file to the location of project where you run minipro to flash.
That's why you see the file in this folder, with the code. Don't need to include in git.
In Fedora 37, it's residing originally in /usr/share/minipro/

## Links

- https://create.arduino.cc/projecthub/milanistef/introduction-to-bare-metal-programming-in-arduino-uno-f3e2b4
