# compile

avr-gcc -g -Os -mmcu=attiny45 main.c -o main.elf
avr-objcopy -j .text -j .data -O ihex main.elf main.hex
sudo avrdude -p t45 -c usbasp -U flash:w:main.hex
