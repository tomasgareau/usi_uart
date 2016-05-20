BIN=echo

MCU=attiny85
AVRDUDEMCU=t85
CPUFREQ=8000000UL

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-g -Os -Wall -mcall-prologues -DF_CPU=${CPUFREQ} -mmcu=${MCU}
SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
PORT=/dev/spidev0.0

${BIN}.hex: ${BIN}.elf
	${OBJCOPY} -O ihex -R .eeprom $< $@

${BIN}.elf: ${OBJS}
	${CC} -o $@ $^

install: ${BIN}.hex
	sudo gpio -g mode 22 out
	sudo gpio -g write 22 0
	sudo avrdude -p ${AVRDUDEMCU} -P ${PORT} -c linuxspi -b 10000 -U flash:w:${BIN}.hex
	sudo gpio -g write 22 1

clean:
	rm -f ${BIN}.elf ${BIN}.hex ${OBJS}
