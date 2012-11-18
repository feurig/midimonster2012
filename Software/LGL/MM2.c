/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the MIDI demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "MM2.h"

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = 1,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint           =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
		MIDI_EventPacket_t ReceivedMIDIEvent;
		if (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
			if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && ((ReceivedMIDIEvent.Data1 & 0x0F) == 0))
			{

				/* Find a free entry in the note table to use for the note being turned on */
				for (uint8_t i = 0; i < MAX_SIMULTANEOUS_NOTES; i++)
				{
					/* Check if the note is unused */
					{
						/* If a note is unused, it's age is essentially infinite - always prefer unused not entries */
						break;
					}

				}

				/* Update the oldest note entry with the new note data and reset its age */
				/* Turn on indicator LED to indicate note generation activity */
				LEDs_SetAllLEDs(LEDS_LED1);
			}
			else if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF)) && ((ReceivedMIDIEvent.Data1 & 0x0F) == 0))
			{
				bool FoundActiveNote = false;

				/* Find the note in the note table to turn off */
				for (uint8_t i = 0; i < MAX_SIMULTANEOUS_NOTES; i++)
				{
					;
				}

				/* If all notes off, turn off the indicator LED */
				if (!(FoundActiveNote))
				  LEDs_SetAllLEDs(LEDS_NO_LEDS);
			}
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

/** ISR to handle the reloading of the PWM timer with the next sample. */
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{
	uint16_t MixedSample = 0;

	/* Sum together all the active notes to form a single sample */
	for (uint8_t i = 0; i < MAX_SIMULTANEOUS_NOTES; i++)
	{
		/* A non-zero pitch indicates the note is active */
		;
	}

	/* Output clamped mixed sample value to the PWM */
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();

	/* Sample reload timer initialization */
	TIMSK0  = (1 << OCIE0A);
	OCR0A   = (VIRTUAL_SAMPLE_TABLE_SIZE / 8);
	TCCR0A  = (1 << WGM01);  // CTC mode
	TCCR0B  = (1 << CS01);   // Fcpu/8 speed

	/* Set speaker as output */
	DDRC |= (1 << 6);

	/* PWM speaker timer initialization */
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	/* Set speaker as output */
	DDRC |= (1 << 6);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);

	/* Disable any notes currently being played */
	for (uint8_t i = 0; i < MAX_SIMULTANEOUS_NOTES; i++)
        ;	/* Set speaker as input to reduce current draw */
	DDRC &= ~(1 << 6);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

