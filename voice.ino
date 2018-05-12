/** \file
 * SAM ported to the Arduino
 *
 * On the Teensy3 and ESP32 this can use the DAC to output the audio,
 * otherwise a single bit output is used.
 *
 * ATmega32u4 boards like the Teensy 2 or Circuit Playground only have
 * 2 KB of SRAM, which is too low to store all the tables.  Maybe with
 * some serious effort...
 *
 * Forked from https://github.com/vidarh/SAM
 */
#include "sam.h"
#include "reciter.h"

// esp32
#include <driver/dac.h>

void setup()
{
	Serial.begin(115200);
}


static inline char hexdigit(int x)
{
	x &= 0xF;
	if (0 <= x && x <= 9)
		return '0' + x;
	else
		return 'A' + x - 0xA;
}


void loop()
{
	extern char input[];
	unsigned len = 0;

	Serial.print("> ");

	while(1)
	{
		int c = Serial.read();
		if (c == -1)
			continue;
		if (c == '\n')
		{
			input[len++] = '['; // yeah WTF
			input[len++] = '\0';
			break;
		}

		input[len++] = toupper(c);
	}

	TextToPhonemes(input);
	Serial.println(input);

	SAMMain();

	const uint8_t * buf = (const uint8_t *) GetBuffer();
	int samples = GetBufferLength() / 50;  // why 50?

	Serial.print("samples=");
	Serial.println(samples);

	for(int i = 0 ; i < samples ; i++)
	{
		// esp32
		dac_out_voltage(DAC_CHANNEL_1, 255 - buf[i]);
		delayMicroseconds(2*1000000/22000);
	}

	if(0)
	for(int i = 0 ; i < samples ; i++)
	{
		Serial.print(hexdigit(buf[i] >> 4));
		Serial.print(hexdigit(buf[i] >> 0));
		if((i % 16) == 15)
			Serial.println();
		else
			Serial.print(' ');
	}
}


extern "C" {
extern void serial_print(int val, int width);
extern void serial_println();
};

void serial_print(int val, int width)
{
	Serial.printf("%8d", val);
}
void serial_println()
{
	Serial.println();
}
