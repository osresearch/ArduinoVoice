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

	char * buf = GetBuffer();
	int samples = GetBufferLength() / 50;
	Serial.print("samples=");
	Serial.println(samples);

	for(int i = 0 ; i < samples ; i++)
	{
		// esp32
		dac_out_voltage(DAC_CHANNEL_1, buf[i]);
	}
}

