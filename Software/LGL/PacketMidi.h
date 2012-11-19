/*-------------------------------------------------------------------------------------PacketMidi.h
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
#ifndef __LETS_PACKET_MIDI_H__
#define __LETS_PACKET_MIDI_H__

#include "MidiSpecs.h"

void setupMidiUart(void);
void routeToUart(MIDI_EVENT_PACKET_t  e);
void dispatchMidiEvent(MIDI_EVENT_PACKET_t  e);
void queueMidiEvent(MIDI_EVENT_PACKET_t  e);
bool midiEventInQueue(void);
MIDI_EVENT_PACKET_t nextMidiEvent(void); 
void sendUartMidiByte(uint8_t b);

// user supplied ....
void doLocalMidiEvent(MIDI_EVENT_PACKET_t  e);
void getLocalMidiEvents(void);
void sysexInterpreter(MIDI_EVENT_PACKET_t  e);

#endif



