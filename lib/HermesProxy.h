/* -*- c++ -*- */
/* 
 * Copyright 2013-2017 Tom McDermott, N5EG
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

// HermesProxy.h
//
// Proxy for Hermes board. Each HermesNB module communicates with
// only one hardware module. Multiple hardware modules need to use
// multiple instantiations of the HermesNB within GNURadio.
// Note: multiple receivers on one Hermes is not implemented. 
//
// Version:  December 15, 2012
//	     July 10, 2013		-- Updates for GRC 3.7
// 	     December 4, 2013		-- Fix bug in free() on termination.
//					-- Add additional parameters to constructor
//	     July 2017			-- Changes supporting up to 8 receivers


#include <gnuradio/io_signature.h>

#ifndef HermesProxy_H
#define HermesProxy_H

#define NUMRXIQBUFS	128		// number of receiver IQ buffers in circular queue.
					// Must be integral power of 2 (2,4,8,16,32,64, etc.)

#define RXBUFSIZE	256		// number of floats in one RxIQBuf, #complexes is half
					// Must be integral power of 2 (2,4,8,16,32,64, etc.)

#define NUMTXBUFS	128		// number of transmit buffers in circular queue
					// Must be integral power of 2

#define TXBUFSIZE	512		// number of bytes in one TxBuf


#define TXINITIALBURST	  4		// Number of Ethernet frames to holdoff before bursting
					// to fill hardware TXFIFO

#define MAXRECEIVERS      8		// Maximum number of receivers defined by protocol specification


typedef float* IQBuf_t;			// IQ buffer type (IQ samples as floats)
typedef unsigned char* RawBuf_t;	// Raw transmit buffer type

enum {  PTTOff,				// PTT disabled
	PTTVox,				// PTT vox mode (examines TxFrame to decide whether to Tx)
	PTTOn };			// PTT force Tx on

class HermesProxy
{

private:

	IQBuf_t RxIQBuf[NUMRXIQBUFS];	// ReceiveIQ buffers
	unsigned RxWriteCounter;	// Which Rx buffer to write to
	unsigned RxReadCounter;		// Which Rx buffer to read from
	unsigned RxWriteFill;		// Fill level of the RxWrite buffer
	bool TxHoldOff;			// Transmit buffer holdoff flag

	RawBuf_t TxBuf[NUMTXBUFS]; 	// Transmit buffers
	unsigned TxWriteCounter;	// Which Tx buffer to write to
	unsigned TxReadCounter;		// Which Tx buffer to read from
	unsigned TxControlCycler;	// Which Tx control register set to send
	unsigned TxFrameIdleCount;	// How long we've gone since sending a TxFrame

	unsigned long LostRxBufCount;	// Lost-buffer counter for packets we actually got
	unsigned long TotalRxBufCount;	// Total buffer count (may roll over)
	unsigned long LostTxBufCount;	//
	unsigned long TotalTxBufCount;	//
	unsigned long CorruptRxCount;	//
	unsigned long LostEthernetRx;	//
	unsigned long CurrentEthSeqNum;	// Diagnostic

	//pthread_mutex_t mutexRPG;	// Rx to Proxy to Gnuradio buffer
	//pthread_mutex_t mutexGPT;	// Gnuradio to Proxy to Tx buffer


public:

	unsigned Receive0Frequency;	// 1st rcvr. Corresponds to out0 in gnuradio
	unsigned Receive1Frequency;	// 2nd rcvr. Corresponds to out1 in gnuradio
	unsigned Receive2Frequency;	// 3rd rcvr. Corresponds to out2 in gnuradio
	unsigned Receive3Frequency;	// 4th rcvr. Corresponds to out3 in gnuradio
	unsigned Receive4Frequency;	// 5th rcvr. Corresponds to out4 in gnuradio
	unsigned Receive5Frequency;	// 6th rcvr. Corresponds to out5 in gnuradio
	unsigned Receive6Frequency;	// 7th rcvr. Corresponds to out6 in gnuradio
	unsigned Receive7Frequency;	// 8th rcvr. Corresponds to out7 in gnuradio

	unsigned TransmitFrequency;
	int NumReceivers;
	int RxSampleRate;

	unsigned char TxDrive;
	unsigned char RxAtten;		// not yet used (requires Hermes firmware V2.0)

	unsigned int ClockSource;	// upper 6-bits of clock control register

	unsigned char AlexRxAnt;	// Select Alex Receive Antenna or from T/R relay
	unsigned char AlexTxAnt;	// Select Alex Tx Antenna
	unsigned char AlexRxHPF;	// Select Alex Receive High Pass Filter
	unsigned char AlexTxLPF;	// Select Alex Transmit Low Pass Filter

	int PTTMode;
	bool RxPreamp;
	bool ADCdither;
	bool ADCrandom;
	bool ADCoverload;
	bool Duplex;

	unsigned char HermesVersion;
	unsigned int AIN1, AIN2, AIN3, AIN4, AIN5, AIN6;  // Analog inputs to Hermes
	unsigned int AlexRevPwr;
	unsigned int SlowCount;
	int Verbose;

	bool TxStop;
	bool PTTOffMutesTx;		// PTT Off mutes the transmitter
	bool PTTOnMutesRx;		// PTT On receiver
	char interface[16];

	char mactarget[18];		// Requested target's MAC address as string
					// "HH:HH:HH:HH:HH:HH" HH is hexadecimal string.
	unsigned int metis_entry;	// Index into Metis_card MAC table


	HermesProxy(int RxFreq0, int RxFreq1, int RxFreq2, int RxFreq3, int RxFreq4,
			 int RxFreq5, int RxFreq6, int RxFreq7, int TxFreq, int RxPre,
			 int PTTModeSel, int PTTTxMute, int PTTRxMute,
			 unsigned char TxDr, int RxSmp, const char* Intfc, 
			 const char * ClkS, int AlexRA, int AlexTA,
			 int AlexHPF, int AlexRPF, int Verbose, int NumRx,
			 const char* MACAddr);	// constructor

	~HermesProxy();			// destructor

	void Stop();			// stop ethernet I/O
	void Start();			// start rx stream

	void SendTxIQ();		// send an IQ buffer to Hermes transmit hardware
	void BuildControlRegs(unsigned, RawBuf_t);	// fill in the 8 byte sync+control registers from RegNum
	int PutTxIQ(const gr_complex *, /*const gr_complex *,*/ int);	// post a transmit TxIQ buffer
	void ScheduleTxFrame(unsigned long);    // Schedule a Tx frame
	RawBuf_t GetNextTxBuf(); // get an empty Tx Buffer

	void UpdateHermes();		// update control registers in Hermes without any Tx data

	void ReceiveRxIQ(unsigned char *); // receive an IQ Ethernet frame from Hermes hardware via metis.cc thread
	IQBuf_t GetRxIQ();		// Gnuradio pickup a received RxIQ buffer if available (next readable Rx buffer)
	IQBuf_t GetNextRxBuf();  	// get an empty output buffer, NULL if no new one available (next writable Rx buffer)
	float Unpack2C(const unsigned char* inptr);  // unpack 2's complement to float
	unsigned int USBRowCount[MAXRECEIVERS];	// Rows (samples per receiver) for one USB frame.

	void PrintRawBuf(RawBuf_t);	// for debugging

	// Not yet implemented
	void ReceiveMicLR();		// receive an LR audio bufer from Hermes hardware

};

#endif  // #ifndef HermesProxy_H

