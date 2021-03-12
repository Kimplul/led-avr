# led-avr
Project repo for a school assignment

I compile this file with `avr-gcc main.c -O2 -mmcu=atmega8515 -o main.elf`

and upload to the device with `avrdude -c usbtiny -p m8515 -U flash:w:main.elf -U lfuse:w:0xc1:m -U hfuse:w:0xd9:m`
