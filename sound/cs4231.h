/**
sudo make - * @file	cs4231.h
 * @brief	Interface of the CS4231
 */

#pragma once

#include "sound.h"
#include "io/dmac.h"

enum {
//
	CS4231_BUFFERS	= (1 << 9),
	CS4231_BUFMASK	= (CS4231_BUFFERS - 1)
};

typedef struct {
	UINT8	adc_l;				// 0
	UINT8	adc_r;				// 1
	UINT8	aux1_l;				// 2
	UINT8	aux1_r;				// 3
	UINT8	aux2_l;				// 4
	UINT8	aux2_r;				// 5
	UINT8	dac_l;				// 6
	UINT8	dac_r;				// 7
	UINT8	datafmt;			// 8
	UINT8	iface;				// 9
	UINT8	pinctrl;			// a
	UINT8	errorstatus;		//b
	UINT8	mode_id;		//c
	UINT8	loopctrl;		//d
	UINT8	playcount[2];		//e-f
	UINT8	featurefunc[2];		//10-11
	UINT8	line_l;		//12
	UINT8	line_r;		//13
	UINT8	timer[2];			//14-15
	UINT8	reserved1;		//16
	UINT8	reserved2;		//17
	UINT8	featurestatus;		//18
	UINT8	chipid;		//19
	UINT8	monoinput;		//1a
	UINT8	reserved3;		//1b
	UINT8	cap_datafmt;		//1c
	UINT8	reserved4;		//1d
	UINT8	cap_basecount[2];		//1e-1f
} CS4231REG;

typedef struct {
	UINT		bufsize;
	UINT		bufdatas;
	UINT		bufpos;
	UINT32		pos12;
	UINT32		step12;

	UINT8		enable;
	UINT8		portctrl;
	UINT8		dmairq;
	UINT8		dmach;
	UINT32		port[16];
	UINT8		adrs;
	UINT8		index;
	UINT8		intflag;
	UINT8		outenable;
	UINT8		extfunc;
	UINT8		extindex;

	CS4231REG	reg;
	UINT8		buffer[CS4231_BUFFERS];
} _CS4231, *CS4231;

typedef struct {
	UINT	rate;
} CS4231CFG;


#ifdef __cplusplus
extern "C"
{
#endif

void cs4231_dma(NEVENTITEM item);
REG8 DMACCALL cs4231dmafunc(REG8 func);
void cs4231_datasend(REG8 dat);

void cs4231_initialize(UINT rate);
void cs4231_setvol(UINT vol);

void cs4231_reset(void);
void cs4231_update(void);
void cs4231_control(UINT index, REG8 dat);

void SOUNDCALL cs4231_getpcm(CS4231 cs, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif
