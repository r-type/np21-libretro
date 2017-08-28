/**
 * @file	compiler.h
 * @brief	include file for standard system include files,
 *			or project specific include files that are used frequently,
 *			but are changed infrequently
 */

#pragma once

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdarg.h>
#include <string.h>

#include "libretro.h"
#include "boolean.h"
#include "features/features_cpu.h"
#include "sdlremap/sdl.h"
#include "sdlremap/sdl_keycode.h"

#define GetKeyState(a) 0
#define VK_PAUSE

#ifdef _WIN32
#define G_DIR_SEPARATOR '\\'
#else
#define G_DIR_SEPARATOR '/'
#endif

#define SDL_arraysize(array)   (sizeof(array)/sizeof(array[0]))

#ifdef WIIU
#define SDL_Delay(a)
#define	sigjmp_buf		jmp_buf
#define	sigsetjmp(env, mask)	setjmp(env)
#define	siglongjmp(env, val)	longjmp(env, val)
#else
#define SDL_Delay(a) usleep((a)*1000)
#endif

#ifndef MSB_FIRST
#define	BYTESEX_LITTLE
#else
#define	BYTESEX_BIG
#endif

#define	OSLANG_UTF8
#define	OSLINEBREAK_CRLF
#define RESOURCE_US

#ifdef __WIN32__
#define	sigjmp_buf				jmp_buf
#define	sigsetjmp(env, mask)	setjmp(env)
#define	siglongjmp(env, val)	longjmp(env, val)
#include "windef.h"
#else
typedef char	TCHAR;
typedef	unsigned int	DWORD;
#endif

typedef	signed int			SINT;
typedef	unsigned int		UINT;
typedef	signed char			SINT8;
typedef	unsigned char		UINT8;
typedef	signed short		SINT16;
typedef	unsigned short		UINT16;
typedef	signed int			SINT32;
typedef	signed int			INT32;
typedef	unsigned int		UINT32;
typedef	int64_t		SINT64;
typedef	uint64_t		UINT64;

typedef  int32_t*    INTPTR;

#define	msgbox(title, msg) menumbox(msg, title, 0x0000 | 0x0040)

#define	BRESULT				UINT
#define	OEMCHAR				char
#define	OEMTEXT(string)		string
#define	OEMSPRINTF			sprintf
#define	OEMSTRLEN			strlen

#define SIZE_VGA
#if !defined(SIZE_VGA)
#define	RGB16		UINT32
#define	SIZE_QVGA
#endif

#if defined(SUPPORT_LARGE_HDD)
typedef INT64	FILEPOS;
typedef INT64	FILELEN;
#define	NHD_MAXSIZE		8000
#define	NHD_MAXSIZE2	32000
#else
typedef long	FILEPOS;
typedef long	FILELEN;
#define	NHD_MAXSIZE		2000
#define	NHD_MAXSIZE2	2000
#endif

#if  !defined(__WIN32__)
typedef	signed char		CHAR;
typedef	unsigned short int	WORD;
typedef	unsigned int	DWORD;
#endif
typedef	unsigned char	BYTE;

#if !defined(OBJC_BOOL_DEFINED) && !defined(__WIN32__)
typedef signed char BOOL;
#endif

#define _tcscat strcat
#define _tcsstr strstr
#define	_tcsrchr strrchr
#define	_tcscpy strcpy
#define	_tcsnicmp strncasecmp
#define	_tcsicmp  strcasecmp
#define _T(x)  x
#define _countof(a)  (sizeof(a)/sizeof(a[0]))

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	MAX_PATH
#define	MAX_PATH	4096
#endif

#ifndef	max
#define	max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef	min
#define	min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef	ZeroMemory
#define	ZeroMemory(d,n)		memset((d), 0, (n))
#endif
#ifndef	CopyMemory
#define	CopyMemory(d,s,n)	memcpy((d), (s), (n))
#endif
#ifndef	FillMemory
#define	FillMemory(a, b, c)	memset((a), (c), (b))
#endif

#include "common.h"
#include "milstr.h"
#include "_memory.h"
#include "rect.h"
#include "lstarray.h"
#include "trace.h"

#define  TRACE
#define	GetTickCount() (cpu_features_get_time_usec() / 1000)
#define	GETTICK() (cpu_features_get_time_usec() / 1000)//millisecond timer
//#define	GETTICK()			GetTicks()
#define	__ASSERT(s)
#define	SPRINTF				sprintf
#define	STRLEN				strlen

#define	SUPPORT_UTF8

#define	SUPPORT_16BPP
#define	MEMOPTIMIZE		2

//#define SOUND_CRITICAL
#define	SOUNDRESERVE	100

#define	SUPPORT_OPM

#define	SUPPORT_CRT15KHZ
#define	SUPPORT_HOSTDRV
#define	SUPPORT_SWSEEKSND
#define	SUPPORT_SASI
#define	SUPPORT_SCSI

#define	SUPPORT_CRT31KHZ
#define	SUPPORT_SWSEEKSND
#define  SUPPORT_PC9821

#if defined(SUPPORT_PC9821)
#define	CPUCORE_IA32
#define  USE_FPU
#define	IA32_PAGING_EACHSIZE
#define	SUPPORT_PC9801_119
#endif
#define	SUPPORT_PC9861K
#define	SUPPORT_SOFTKBD		0
#define  SUPPORT_S98

//#define SUPPORT_EXTERNALCHIP
//tosee
#define	SUPPORT_RESUME
#define	SUPPORT_STATSAVE	10
#define	SUPPORT_ROMEO

#define	SUPPORT_NORMALDISP

#define SUPPORT_ARC

#define	SCREEN_BPP		16

#define	VERMOUTH_LIB

#define	SUPPORT_EUC
#define	SUPPORT_KEYDISP

#define	SUPPORT_KAI_IMAGES
#define	SUPPORT_IDEIO

//outdated things to ignore
#define	FASTCALL
#define	CPUCALL
#define	MEMCALL
#define	DMACCALL
#define	IOOUTCALL
#define	IOINPCALL
#define	SOUNDCALL
#define	VRAMCALL
#define	SCRNCALL
#define	VERMOUTHCL
