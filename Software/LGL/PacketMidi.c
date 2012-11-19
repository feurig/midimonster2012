/*-------------------------------------------------------------------------------------PacketMidi.c
 * 
 * Draft of Suspect Devices "PacketMidi" midi library. 
 * I started this because I needed a library that I could embed in client projects
 * Some of the better libraries are gpl only and therefor could not be used where
 * the client wanted control over their end product and code. This library is there
 * for bsd liscensed. 
 *
 * The philosophy of PacketMidi is based on the model presented in the usb spec
 * where all midi events are 4 bytes long, serialization (sic) and deserialialization
 * is handled by the uart interupts and uart output routines. 
 * events are queued and handled in the main loop.
 *
 *  
 * Created by Donald D Davis on 9/9/11.
 *
 * Copyright (c) 2011 Donald Delmar Davis, Suspect Devices
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION 
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "PacketMidi.h"
// should use fancy use if __AVR_ATMEGA168__
#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega168__)
#define MIDI_USES_UART0 1
#endif

#ifndef DEFAULT_MIDI_CHANNEL
#define DEFAULT_MIDI_CHANNEL	0
#define DEFAULT_MIDI_CABLE_NO	0
#endif


volatile uint8_t Channel = DEFAULT_MIDI_CHANNEL;



/*---------------------------------------dispatchMidiEvent(MIDI_EVENT_PACKET_t e)
 * 
 */


void dispatchMidiEvent(MIDI_EVENT_PACKET_t e) {
	
    if (CIN_IS_SYSEX(e.cin)) {
		sysexInterpreter(e);
    } else {
        doLocalMidiEvent(e);
		routeToUart(e);
    }
}


#define MAX_EVENT_QUEUE_LENGTH 16
/*------------------------------------------------------------------------------
 * uart event queue routines. 
 * these are brutally rudimentary at the moment.
 *----------------------------------------------------------------------------*/

MIDI_EVENT_PACKET_t UARTMIDIEventQueue[MAX_EVENT_QUEUE_LENGTH];
uint8_t UARTMIDIEventQueueHead=0;
uint8_t UARTMIDIEventQueueTail=0;
/*------------------------------------------queueMidiEvent(MIDI_EVENT_PACKET_t e)
 * add packet to queue 
 */

void queueMidiEvent(MIDI_EVENT_PACKET_t e) {
	UARTMIDIEventQueue[UARTMIDIEventQueueTail]=e;
	if (++UARTMIDIEventQueueTail>=MAX_EVENT_QUEUE_LENGTH) {
		UARTMIDIEventQueueTail=0;
	} // some bounds checking should be here!!!
}
/*------------------------------------------------------------midiEventInQueue() 
 * 
 */

bool midiEventInQueue() {
	return (UARTMIDIEventQueueHead!=UARTMIDIEventQueueTail);
}

/*------------------------------------------------------------------------------
 * uart event queue routines. 
 *----------------------------------------------------------------------------*/
MIDI_EVENT_PACKET_t nextMidiEvent() {
	// some bounds checking should be here!!!
	MIDI_EVENT_PACKET_t e = UARTMIDIEventQueue[UARTMIDIEventQueueHead];
	if (++UARTMIDIEventQueueHead>=MAX_EVENT_QUEUE_LENGTH) {
		UARTMIDIEventQueueHead=0;
	} 
	return (e);
}

/*------------------------------------------------------------------------------
 * uart setup and handling
 *----------------------------------------------------------------------------*/

/*------------------------------------------------ISR(USARTx_RX_vect, ISR_BLOCK)
 * parse incoming bytes into midi Events. 
 */

#ifndef MIDI_USES_UART0
ISR(USART1_RX_vect, ISR_BLOCK)
{
    uint8_t b = UDR1;
#else 
ISR(USART_RX_vect, ISR_BLOCK)
{
    uint8_t b = UDR0;
#endif
        
	static uint8_t needBytes;
	static uint8_t bytesIndex;
	static bool inSysex;
	static MIDI_EVENT_PACKET_t realTimeEvent;
	static MIDI_EVENT_PACKET_t pendingEvent;
	
	if (inSysex) {
		if (b == MIDIv1_SYSEX_END){
			inSysex=0;
			if (bytesIndex==3) {
				pendingEvent.cin=CIN_SYSEX_ENDS_IN_3;
				pendingEvent.midi2=b;
			} else if (bytesIndex==2) {
				pendingEvent.cin=CIN_SYSEX_ENDS_IN_2;
				pendingEvent.midi1=b;
			} else {
				pendingEvent.cin=CIN_SYSEX_ENDS_IN_1;
				pendingEvent.midi0=b;
			}
			bytesIndex=0;
			needBytes=0;
			queueMidiEvent(pendingEvent);
			
		} else {
			if (bytesIndex==1){
				pendingEvent.midi0=b;
				bytesIndex++;
			} else if (bytesIndex==2) {
				pendingEvent.midi1=b;
				bytesIndex++;
			} else {
				pendingEvent.midi2=b;
				queueMidiEvent(pendingEvent);
				pendingEvent.cin=CIN_SYSEX;
				bytesIndex=1;
			}
			
		}
		return;
	}
	
	if (needBytes) { // if still parsing last command.
		if (bytesIndex==1){
			pendingEvent.midi0=b;
			bytesIndex++;
		} else if (bytesIndex==2) {
			pendingEvent.midi1=b;
			bytesIndex++;
		} else {
			pendingEvent.midi2=b;
		}
		needBytes--;
		if (needBytes < 1) { // if that was the last Byte			
			needBytes=0;
			queueMidiEvent(pendingEvent);
		} 
		return;	
	}
	
	
	if (MIDIv1_IS_REALTIME(b)){
		realTimeEvent.cable=DEFAULT_MIDI_CABLE_NO;
		realTimeEvent.cin=CIN_1BYTE;
		realTimeEvent.midi0=b;
		queueMidiEvent(realTimeEvent);
		return ;
	}
	if (MIDIv1_IS_VOICE(b)){
        
		pendingEvent.cable = DEFAULT_MIDI_CABLE_NO;
		pendingEvent.cin = (b>>4)&0x0f;
		pendingEvent.midi0 = b;
		if((MIDIv1_VOICE_COMMAND(b) == MIDIv1_PROGRAM_CHANGE)
		   || (MIDIv1_VOICE_COMMAND(b) == MIDIv1_CHANNEL_PRESSURE)
		   ) {
			needBytes = 1;
		} else {
			needBytes = 2;
		}
		bytesIndex = 2;
		return;
	}
	if (MIDIv1_IS_SYSCOMMON(b)){
		pendingEvent.cable = DEFAULT_MIDI_CABLE_NO;
		switch (b) {
			case MIDIv1_SYSEX_START :
				inSysex = 1;
				needBytes = 2; 
				bytesIndex = 2;
				pendingEvent.cin = CIN_SYSEX;
				pendingEvent.midi0 = b;
				break;
			case MIDIv1_SONG_POSITION_PTR :
				pendingEvent.cin = CIN_3BYTE_SYS_COMMON;
				pendingEvent.midi0 = b;
				needBytes=2;
				bytesIndex=2;
				break;
			case MIDIv1_TUNE_REQUEST :
				pendingEvent.cin = CIN_1BYTE;
				pendingEvent.midi0 = b;
				queueMidiEvent(pendingEvent);
				break;
			case MIDIv1_MTC_QUARTER_FRAME :
			case MIDIv1_SONG_SELECT :
				pendingEvent.cin = CIN_2BYTE_SYS_COMMON;
				pendingEvent.midi0 = b;
				needBytes = 1;
				bytesIndex = 2;
				break;
                // case MIDIv1_SYSEX_END handled up top
		}
		return;
	}
	
}

/*---------------------------------------------routeToUart(MIDI_EVENT_PACKET_t E)
 * 
 */
void routeToUart(MIDI_EVENT_PACKET_t E) {
	// we will handle these when they are defined in the standard.
	//	if (E.cin<CIN_2BYTE_SYS_COMMON ) { 
	//		return;
	//	}
	sendUartMidiByte(E.midi0); // we always send at least data1
	if ((E.cin==CIN_1BYTE)
		||(E.cin==CIN_SYSEX_ENDS_IN_1)
		) {
		return;
	}
	sendUartMidiByte(E.midi1); // we sometimes send data2
	if ((E.cin==CIN_2BYTE_SYS_COMMON)
		|| (E.cin==CIN_SYSEX_ENDS_IN_2)
		|| (E.cin==CIN_PROGRAM_CHANGE)
		|| (E.cin==CIN_CHANNEL_PRESSURE)
		) {
		return;
	}
    sendUartMidiByte(E.midi2); // the rest of the time we send data3
}

/*---------------------------------------------------sendUartMidiByte(uint8_t b)
 * 
 */
void sendUartMidiByte(uint8_t b) {	
#ifndef MIDI_USES_UART0
	while (!(UCSR1A & (1 << UDRE1)));
	UDR1 = b;	
#else 
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = b;	
#endif
}

/*---------------------------------------------------------------setupMidiUart()
 * 
 */
#define UART_USE_u2x 1

void setupMidiUart()   {                
    uint16_t baud;
#ifndef MIDI_USES_UART0    
#ifdef	UART_USE_u2x 
    UCSR1A = _BV(U2X1);
    baud = (F_CPU / 4 / MIDIv1_BAUD_RATE - 1) / 2;
#else 
    UCSR1A = 0;
    baud = (F_CPU / 8 / MIDIv1_BAUD_RATE - 1) / 2;
#endif
    UBRR1 = baud;
	UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
#else
#ifdef	UART_USE_u2x 
    UCSR0A = _BV(U2X0);
    baud = (F_CPU / 4 / MIDIv1_BAUD_RATE - 1) / 2;
#else 
    UCSR0A = 0;
    baud = (F_CPU / 8 / MIDIv1_BAUD_RATE - 1) / 2;
#endif
    UBRR0 = baud;
	UCSR0B = ((1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0));
#endif
}


