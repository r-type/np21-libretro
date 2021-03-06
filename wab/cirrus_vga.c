
/*
 * QEMU Cirrus CLGD 54xx VGA Emulator.
 *
 * Copyright (c) 2004 Fabrice Bellard
 * Copyright (c) 2004 Makoto Suzuki (suzu)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/*
 * Reference: Finn Thogersons' VGADOC4b
 *   available at http://home.worldonline.dk/~finth/
 */

#include	"compiler.h"

#if defined(SUPPORT_CL_GD5430)

#include	"pccore.h"
#include	"wab.h"
#include	"cirrus_vga_extern.h"
#include	"cirrus_vga.h"
#include	"vga_int.h"
#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"soundmng.h"

//const uint8_t sr_mask[8] = {0};
//const uint8_t gr_mask[16] = {0};
/* force some bits to zero */
const uint8_t sr_mask[8] = {
    (uint8_t)~0xfc,
    (uint8_t)~0xc2,
    (uint8_t)~0x00, // np21w ver0.86 rev29  (uint8_t)~0xf0,
    (uint8_t)~0xc0,
    (uint8_t)~0xf1,
    (uint8_t)~0xff,
    (uint8_t)~0xff,
    (uint8_t)~0x00,
};

const uint8_t gr_mask[16] = {
    (uint8_t)~0xf0, /* 0x00 */
    (uint8_t)~0xf0, /* 0x01 */
    (uint8_t)~0xf0, /* 0x02 */
    (uint8_t)~0xe0, /* 0x03 */
    (uint8_t)~0xfc, /* 0x04 */
    (uint8_t)~0x84, /* 0x05 */
    (uint8_t)~0xf0, /* 0x06 */
    (uint8_t)~0xf0, /* 0x07 */
    (uint8_t)~0x00, /* 0x08 */
    (uint8_t)~0xff, /* 0x09 */
    (uint8_t)~0xff, /* 0x0a */
    (uint8_t)~0xff, /* 0x0b */
    (uint8_t)~0xff, /* 0x0c */
    (uint8_t)~0xff, /* 0x0d */
    (uint8_t)~0xff, /* 0x0e */
    (uint8_t)~0xff, /* 0x0f */
};

REG8 cirrusvga_regindexA2 = 0;
REG8 cirrusvga_regindex = 0;
//REG8 cirrusvga_iomap = 0x0F;
//REG8 cirrusvga_mmioenable = 0;

NP2CLVGA	np2clvga = {0};
//NP2CLVGA2	np2clvga = {0};
void *cirrusvga_opaque = NULL;
UINT8	cirrusvga_statsavebuf[CIRRUS_VRAM_SIZE + 1024 * 1024];

int g_cirrus_linear_map_enabled = 0;
CPUWriteMemoryFunc *g_cirrus_linear_write[3] = {0};

uint8_t* vramptr;
uint8_t* cursorptr;

DisplayState ds = {0};

BITMAPINFO *ga_bmpInfo;
BITMAPINFO *ga_bmpInfo_cursor;
HBITMAP		ga_hbmp_cursor;
HDC			ga_hdc_cursor;
HPALETTE	ga_hpal = NULL;
BITMAPINFO	bmpInfo = {0};
//UINT16		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;

//UINT32 np2clvga.VRAMWindowAddr = (0x0F<<24);
//UINT32 np2clvga.VRAMWindowAddr2 = (0xf20000);

static HCURSOR ga_hFakeCursor = NULL; // ハードウェアカーソル（仮）

static void cpu_register_physical_memory(target_phys_addr_t start_addr, ram_addr_t size, ram_addr_t phys_offset){
	//cpu_register_physical_memory_offset(start_addr, size, phys_offset, 0);

}

void np2vga_ds_dpy_update(struct DisplayState *s, int x, int y, int w, int h)
{
}
void np2vga_ds_dpy_resize(struct DisplayState *s)
{
}
void np2vga_ds_dpy_setdata(struct DisplayState *s)
{
}
void np2vga_ds_dpy_refresh(struct DisplayState *s)
{
}
void np2vga_ds_dpy_copy(struct DisplayState *s, int src_x, int src_y,
                    int dst_x, int dst_y, int w, int h)
{
}
void np2vga_ds_dpy_fill(struct DisplayState *s, int x, int y,
                    int w, int h, uint32_t_ c)
{
}
void np2vga_ds_dpy_text_cursor(struct DisplayState *s, int x, int y)
{
}


DisplaySurface np2vga_ds_surface = {0};
DisplayChangeListener np2vga_ds_listeners = {0, 0, np2vga_ds_dpy_update, np2vga_ds_dpy_resize, 
											  np2vga_ds_dpy_setdata, np2vga_ds_dpy_refresh, 
											  np2vga_ds_dpy_copy, np2vga_ds_dpy_fill, 
											  np2vga_ds_dpy_text_cursor, NULL};

void np2vga_ds_mouse_set(int x, int y, int on){

}
void np2vga_ds_cursor_define(int width, int height, int bpp, int hot_x, int hot_y,
                          uint8_t *image, uint8_t *mask){

}

DisplayState *graphic_console_init(vga_hw_update_ptr update,
                                   vga_hw_invalidate_ptr invalidate,
                                   vga_hw_screen_dump_ptr screen_dump,
                                   vga_hw_text_update_ptr text_update,
								   void *opaque)
{
	ds.opaque = opaque;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = 1024;
	bmpInfo.bmiHeader.biHeight = 512;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	np2vga_ds_surface.width = 640;
	np2vga_ds_surface.height = 480;
	np2vga_ds_surface.pf.bits_per_pixel = 32;
	np2vga_ds_surface.pf.bytes_per_pixel = 4;

	return &ds;
}

/***************************************
 *
 *  definitions
 *
 ***************************************/

#define qemu_MIN(a,b) ((a) < (b) ? (a) : (b))

// ID
#define CIRRUS_ID_CLGD5422  (0x23<<2)
#define CIRRUS_ID_CLGD5426  (0x24<<2)
#define CIRRUS_ID_CLGD5424  (0x25<<2)
#define CIRRUS_ID_CLGD5428  (0x26<<2)
#define CIRRUS_ID_CLGD5430  (0x28<<2)
#define CIRRUS_ID_CLGD5434  (0x2A<<2)
#define CIRRUS_ID_CLGD5436  (0x2B<<2)
#define CIRRUS_ID_CLGD5440  (0x2C<<2)
#define CIRRUS_ID_CLGD5446  (0x2E<<2)

// sequencer 0x07
#define CIRRUS_SR7_BPP_VGA            0x00
#define CIRRUS_SR7_BPP_SVGA           0x01
#define CIRRUS_SR7_BPP_MASK           0x0e
#define CIRRUS_SR7_BPP_8              0x00
#define CIRRUS_SR7_BPP_16_DOUBLEVCLK  0x02
#define CIRRUS_SR7_BPP_24             0x04
#define CIRRUS_SR7_BPP_16             0x06
#define CIRRUS_SR7_BPP_32             0x08
#define CIRRUS_SR7_ISAADDR_MASK       0xe0

// sequencer 0x0f
#define CIRRUS_MEMSIZE_512k        0x08
#define CIRRUS_MEMSIZE_1M          0x10
#define CIRRUS_MEMSIZE_2M          0x18
#define CIRRUS_MEMFLAGS_BANKSWITCH 0x80	// bank switching is enabled.

// sequencer 0x12
#define CIRRUS_CURSOR_SHOW         0x01
#define CIRRUS_CURSOR_HIDDENPEL    0x02
#define CIRRUS_CURSOR_LARGE        0x04	// 64x64 if set, 32x32 if clear

// sequencer 0x17
#define CIRRUS_BUSTYPE_VLBFAST   0x10
#define CIRRUS_BUSTYPE_PCI       0x20
#define CIRRUS_BUSTYPE_VLBSLOW   0x30
#define CIRRUS_BUSTYPE_ISA       0x38
#define CIRRUS_MMIO_ENABLE       0x04
#define CIRRUS_MMIO_USE_PCIADDR  0x40	// 0xb8000 if cleared.
#define CIRRUS_MEMSIZEEXT_DOUBLE 0x80

// control 0x0b
#define CIRRUS_BANKING_DUAL             0x01
#define CIRRUS_BANKING_GRANULARITY_16K  0x20	// set:16k, clear:4k

// control 0x30
#define CIRRUS_BLTMODE_BACKWARDS        0x01
#define CIRRUS_BLTMODE_MEMSYSDEST       0x02
#define CIRRUS_BLTMODE_MEMSYSSRC        0x04
#define CIRRUS_BLTMODE_TRANSPARENTCOMP  0x08
#define CIRRUS_BLTMODE_PATTERNCOPY      0x40
#define CIRRUS_BLTMODE_COLOREXPAND      0x80
#define CIRRUS_BLTMODE_PIXELWIDTHMASK   0x30
#define CIRRUS_BLTMODE_PIXELWIDTH8      0x00
#define CIRRUS_BLTMODE_PIXELWIDTH16     0x10
#define CIRRUS_BLTMODE_PIXELWIDTH24     0x20
#define CIRRUS_BLTMODE_PIXELWIDTH32     0x30

// control 0x31
#define CIRRUS_BLT_BUSY                 0x01
#define CIRRUS_BLT_START                0x02
#define CIRRUS_BLT_RESET                0x04
#define CIRRUS_BLT_FIFOUSED             0x10
#define CIRRUS_BLT_AUTOSTART            0x80

// control 0x32
#define CIRRUS_ROP_0                    0x00
#define CIRRUS_ROP_SRC_AND_DST          0x05
#define CIRRUS_ROP_NOP                  0x06
#define CIRRUS_ROP_SRC_AND_NOTDST       0x09
#define CIRRUS_ROP_NOTDST               0x0b
#define CIRRUS_ROP_SRC                  0x0d
#define CIRRUS_ROP_1                    0x0e
#define CIRRUS_ROP_NOTSRC_AND_DST       0x50
#define CIRRUS_ROP_SRC_XOR_DST          0x59
#define CIRRUS_ROP_SRC_OR_DST           0x6d
#define CIRRUS_ROP_NOTSRC_OR_NOTDST     0x90
#define CIRRUS_ROP_SRC_NOTXOR_DST       0x95
#define CIRRUS_ROP_SRC_OR_NOTDST        0xad
#define CIRRUS_ROP_NOTSRC               0xd0
#define CIRRUS_ROP_NOTSRC_OR_DST        0xd6
#define CIRRUS_ROP_NOTSRC_AND_NOTDST    0xda

#define CIRRUS_ROP_NOP_INDEX 2
#define CIRRUS_ROP_SRC_INDEX 5

// control 0x33
#define CIRRUS_BLTMODEEXT_SOLIDFILL        0x04
#define CIRRUS_BLTMODEEXT_COLOREXPINV      0x02
#define CIRRUS_BLTMODEEXT_DWORDGRANULARITY 0x01

// memory-mapped IO
#define CIRRUS_MMIO_BLTBGCOLOR        0x00	// dword
#define CIRRUS_MMIO_BLTFGCOLOR        0x04	// dword
#define CIRRUS_MMIO_BLTWIDTH          0x08	// word
#define CIRRUS_MMIO_BLTHEIGHT         0x0a	// word
#define CIRRUS_MMIO_BLTDESTPITCH      0x0c	// word
#define CIRRUS_MMIO_BLTSRCPITCH       0x0e	// word
#define CIRRUS_MMIO_BLTDESTADDR       0x10	// dword
#define CIRRUS_MMIO_BLTSRCADDR        0x14	// dword
#define CIRRUS_MMIO_BLTWRITEMASK      0x17	// byte
#define CIRRUS_MMIO_BLTMODE           0x18	// byte
#define CIRRUS_MMIO_BLTROP            0x1a	// byte
#define CIRRUS_MMIO_BLTMODEEXT        0x1b	// byte
#define CIRRUS_MMIO_BLTTRANSPARENTCOLOR 0x1c	// word?
#define CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK 0x20	// word?
#define CIRRUS_MMIO_LINEARDRAW_START_X 0x24	// word
#define CIRRUS_MMIO_LINEARDRAW_START_Y 0x26	// word
#define CIRRUS_MMIO_LINEARDRAW_END_X  0x28	// word
#define CIRRUS_MMIO_LINEARDRAW_END_Y  0x2a	// word
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_INC 0x2c	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_ROLLOVER 0x2d	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_MASK 0x2e	// byte
#define CIRRUS_MMIO_LINEARDRAW_LINESTYLE_ACCUM 0x2f	// byte
#define CIRRUS_MMIO_BRESENHAM_K1      0x30	// word
#define CIRRUS_MMIO_BRESENHAM_K3      0x32	// word
#define CIRRUS_MMIO_BRESENHAM_ERROR   0x34	// word
#define CIRRUS_MMIO_BRESENHAM_DELTA_MAJOR 0x36	// word
#define CIRRUS_MMIO_BRESENHAM_DIRECTION 0x38	// byte
#define CIRRUS_MMIO_LINEDRAW_MODE     0x39	// byte
#define CIRRUS_MMIO_BLTSTATUS         0x40	// byte

// PCI 0x02: device
#define PCI_DEVICE_CLGD5462           0x00d0
#define PCI_DEVICE_CLGD5465           0x00d6

// PCI 0x04: command(word), 0x06(word): status
#define PCI_COMMAND_IOACCESS                0x0001
#define PCI_COMMAND_MEMACCESS               0x0002
#define PCI_COMMAND_BUSMASTER               0x0004
#define PCI_COMMAND_SPECIALCYCLE            0x0008
#define PCI_COMMAND_MEMWRITEINVALID         0x0010
#define PCI_COMMAND_PALETTESNOOPING         0x0020
#define PCI_COMMAND_PARITYDETECTION         0x0040
#define PCI_COMMAND_ADDRESSDATASTEPPING     0x0080
#define PCI_COMMAND_SERR                    0x0100
#define PCI_COMMAND_BACKTOBACKTRANS         0x0200
// PCI 0x08, 0xff000000 (0x09-0x0b:class,0x08:rev)
#define PCI_CLASS_BASE_DISPLAY        0x03
// PCI 0x08, 0x00ff0000
#define PCI_CLASS_SUB_VGA             0x00
// PCI 0x0c, 0x00ff0000 (0x0c:cacheline,0x0d:latency,0x0e:headertype,0x0f:Built-in self test)
#define PCI_CLASS_HEADERTYPE_00h  0x00
// 0x10-0x3f (headertype 00h)
// PCI 0x10,0x14,0x18,0x1c,0x20,0x24: base address mapping registers
//   0x10: MEMBASE, 0x14: IOBASE(hard-coded in XFree86 3.x)
#define PCI_MAP_MEM                 0x0
#define PCI_MAP_IO                  0x1
#define PCI_MAP_MEM_ADDR_MASK       (~0xf)
#define PCI_MAP_IO_ADDR_MASK        (~0x3)
#define PCI_MAP_MEMFLAGS_32BIT      0x0
#define PCI_MAP_MEMFLAGS_32BIT_1M   0x1
#define PCI_MAP_MEMFLAGS_64BIT      0x4
#define PCI_MAP_MEMFLAGS_CACHEABLE  0x8
// PCI 0x28: cardbus CIS pointer
// PCI 0x2c: subsystem vendor id, 0x2e: subsystem id
// PCI 0x30: expansion ROM base address
#define PCI_ROMBIOS_ENABLED         0x1
// PCI 0x34: 0xffffff00=reserved, 0x000000ff=capabilities pointer
// PCI 0x38: reserved
// PCI 0x3c: 0x3c=int-line, 0x3d=int-pin, 0x3e=min-gnt, 0x3f=maax-lat

#define CIRRUS_PNPMMIO_SIZE         0x1000


/* I/O and memory hook */
#define CIRRUS_HOOK_NOT_HANDLED 0
#define CIRRUS_HOOK_HANDLED 1

#define ABS(a) ((signed)(a) > 0 ? a : -a)

#define BLTUNSAFE(s) \
    ( \
        ( /* check dst is within bounds */ \
            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_dstpitch) \
                + ((s)->cirrus_blt_dstaddr & (s)->cirrus_addr_mask) > \
                    CIRRUS_VRAM_SIZE*2 \
        ) || \
        ( /* check src is within bounds */ \
            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_srcpitch) \
                + ((s)->cirrus_blt_srcaddr & (s)->cirrus_addr_mask) > \
                    CIRRUS_VRAM_SIZE*2 \
        ) \
    )
//#define BLTUNSAFE(s) \
//    ( \
//        ( /* check dst is within bounds */ \
//            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_dstpitch) \
//                + ((s)->cirrus_blt_dstaddr & (s)->cirrus_addr_mask) > \
//                    (s)->vram_size \
//        ) || \
//        ( /* check src is within bounds */ \
//            (s)->cirrus_blt_height * ABS((s)->cirrus_blt_srcpitch) \
//                + ((s)->cirrus_blt_srcaddr & (s)->cirrus_addr_mask) > \
//                    (s)->vram_size \
//        ) \
//    )

struct CirrusVGAState;
typedef void (*cirrus_bitblt_rop_t) (struct CirrusVGAState *s,
                                     uint8_t * dst, const uint8_t * src,
				     int dstpitch, int srcpitch,
				     int bltwidth, int bltheight);
typedef void (*cirrus_fill_t)(struct CirrusVGAState *s,
                              uint8_t *dst, int dst_pitch, int width, int height);

typedef struct CirrusVGAState {
    VGA_STATE_COMMON

    int cirrus_linear_io_addr;
    int cirrus_linear_bitblt_io_addr;
    int cirrus_mmio_io_addr;
    uint32_t_ cirrus_addr_mask;
    uint32_t_ linear_mmio_mask;
    uint8_t cirrus_shadow_gr0;
    uint8_t cirrus_shadow_gr1;
    uint8_t cirrus_hidden_dac_lockindex;
    uint8_t cirrus_hidden_dac_data;
    uint32_t_ cirrus_bank_base[2];
    uint32_t_ cirrus_bank_limit[2];
    uint8_t cirrus_hidden_palette[48];
    uint32_t_ hw_cursor_x;
    uint32_t_ hw_cursor_y;
    int cirrus_blt_pixelwidth;
    int cirrus_blt_width;
    int cirrus_blt_height;
    int cirrus_blt_dstpitch;
    int cirrus_blt_srcpitch;
    uint32_t_ cirrus_blt_fgcol;
    uint32_t_ cirrus_blt_bgcol;
    uint32_t_ cirrus_blt_dstaddr;
    uint32_t_ cirrus_blt_srcaddr;
    uint8_t cirrus_blt_mode;
    uint8_t cirrus_blt_modeext;
    cirrus_bitblt_rop_t cirrus_rop;
#define CIRRUS_BLTBUFSIZE (2048 * 4) /* one line width */
    uint8_t cirrus_bltbuf[CIRRUS_BLTBUFSIZE];
    uint8_t *cirrus_srcptr;
    uint8_t *cirrus_srcptr_end;
    uint32_t_ cirrus_srccounter;
    /* hwcursor display state */
    int last_hw_cursor_size;
    int last_hw_cursor_x;
    int last_hw_cursor_y;
    int last_hw_cursor_y_start;
    int last_hw_cursor_y_end;
    int real_vram_size; /* XXX: suppress that */
    //CPUWriteMemoryFunc **cirrus_linear_write;
    int device_id;
    int bustype;
} CirrusVGAState;
/*
typedef struct PCICirrusVGAState {
    PCIDevice dev;
    CirrusVGAState cirrus_vga;
} PCICirrusVGAState;
*/

CirrusVGAState *cirrusvga = NULL;

static uint8_t vga_dumb_retrace(VGAState *s)
{
    return s->st01 ^ (ST01_V_RETRACE | ST01_DISP_ENABLE);
}

static uint8_t rop_to_index[256];

typedef unsigned int rgb_to_pixel_dup_func(unsigned int r, unsigned int g, unsigned b);
#define NB_DEPTHS 7

static unsigned int rgb_to_pixel8(unsigned int r, unsigned int g,
                                         unsigned int b)
{
    return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
}

static unsigned int rgb_to_pixel15(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
}

static unsigned int rgb_to_pixel15bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
}

static unsigned int rgb_to_pixel16(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static unsigned int rgb_to_pixel16bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3);
}

static unsigned int rgb_to_pixel24(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return (r << 16) | (g << 8) | b;
}

static unsigned int rgb_to_pixel24bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return (b << 16) | (g << 8) | r;
}

static unsigned int rgb_to_pixel32(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    return (r << 16) | (g << 8) | b;
}

static unsigned int rgb_to_pixel32bgr(unsigned int r, unsigned int g,
                                             unsigned int b)
{
    return (b << 16) | (g << 8) | r;
}

static unsigned int rgb_to_pixel8_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel8(r, g, b);
    col |= col << 8;
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel15(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel15bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel15bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel16(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel16bgr_dup(unsigned int r, unsigned int g,
                                          unsigned int b)
{
    unsigned int col;
    col = rgb_to_pixel16bgr(r, g, b);
    col |= col << 16;
    return col;
}

static unsigned int rgb_to_pixel32_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32(r, g, b);
    return col;
}

static unsigned int rgb_to_pixel32bgr_dup(unsigned int r, unsigned int g, unsigned b)
{
    unsigned int col;
    col = rgb_to_pixel32bgr(r, g, b);
    return col;
}


static int ds_get_linesize(DisplayState *ds)
{
    return np2wab.realWidth*4;
}

static uint8_t* ds_get_data(DisplayState *ds)
{
    return ds->surface->data;
}

static int ds_get_width(DisplayState *ds)
{
    return np2wab.realWidth;//ds->surface->width;
}

static int ds_get_height(DisplayState *ds)
{
    return np2wab.realHeight;//ds->surface->height;
}

static int ds_get_bits_per_pixel(DisplayState *ds)
{
    return 32;//ds->surface->pf.bits_per_pixel;
}

static int ds_get_bytes_per_pixel(DisplayState *ds)
{
    return 4;//ds->surface->pf.bytes_per_pixel;
}

static void dpy_update(DisplayState *s, int x, int y, int w, int h)
{
    struct DisplayChangeListener *dcl = s->listeners;
    while (dcl != NULL) {
        dcl->dpy_update(s, x, y, w, h);
        dcl = dcl->next;
    }
}

static rgb_to_pixel_dup_func *rgb_to_pixel_dup_table[NB_DEPTHS] = {
    rgb_to_pixel8_dup,
    rgb_to_pixel15_dup,
    rgb_to_pixel16_dup,
    rgb_to_pixel32_dup,
    rgb_to_pixel32bgr_dup,
    rgb_to_pixel15bgr_dup,
    rgb_to_pixel16bgr_dup,
};

/***************************************
 *
 *  prototypes.
 *
 ***************************************/


static void cirrus_bitblt_reset(CirrusVGAState *s);
static void cirrus_update_memory_access(CirrusVGAState *s);

/***************************************
 *
 *  raster operations
 *
 ***************************************/

static void cirrus_bitblt_rop_nop(CirrusVGAState *s,
                                  uint8_t *dst,const uint8_t *src,
                                  int dstpitch,int srcpitch,
                                  int bltwidth,int bltheight)
{
}

static void cirrus_bitblt_fill_nop(CirrusVGAState *s,
                                   uint8_t *dst,
                                   int dstpitch, int bltwidth,int bltheight)
{
}

#define ROP_NAME 0
#define ROP_OP(d, s) d = 0
#include "cirrus_vga_rop.h"

#define ROP_NAME src_and_dst
#define ROP_OP(d, s) d = (s) & (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_and_notdst
#define ROP_OP(d, s) d = (s) & (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME notdst
#define ROP_OP(d, s) d = ~(d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src
#define ROP_OP(d, s) d = s
#include "cirrus_vga_rop.h"

#define ROP_NAME 1
#define ROP_OP(d, s) d = ~0
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_and_dst
#define ROP_OP(d, s) d = (~(s)) & (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_xor_dst
#define ROP_OP(d, s) d = (s) ^ (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME src_or_dst
#define ROP_OP(d, s) d = (s) | (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_or_notdst
#define ROP_OP(d, s) d = (~(s)) | (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME src_notxor_dst
#define ROP_OP(d, s) d = ~((s) ^ (d))
#include "cirrus_vga_rop.h"

#define ROP_NAME src_or_notdst
#define ROP_OP(d, s) d = (s) | (~(d))
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc
#define ROP_OP(d, s) d = (~(s))
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_or_dst
#define ROP_OP(d, s) d = (~(s)) | (d)
#include "cirrus_vga_rop.h"

#define ROP_NAME notsrc_and_notdst
#define ROP_OP(d, s) d = (~(s)) & (~(d))
#include "cirrus_vga_rop.h"

static const cirrus_bitblt_rop_t cirrus_fwd_rop[16] = {
    cirrus_bitblt_rop_fwd_0,
    cirrus_bitblt_rop_fwd_src_and_dst,
    cirrus_bitblt_rop_nop,
    cirrus_bitblt_rop_fwd_src_and_notdst,
    cirrus_bitblt_rop_fwd_notdst,
    cirrus_bitblt_rop_fwd_src,
    cirrus_bitblt_rop_fwd_1,
    cirrus_bitblt_rop_fwd_notsrc_and_dst,
    cirrus_bitblt_rop_fwd_src_xor_dst,
    cirrus_bitblt_rop_fwd_src_or_dst,
    cirrus_bitblt_rop_fwd_notsrc_or_notdst,
    cirrus_bitblt_rop_fwd_src_notxor_dst,
    cirrus_bitblt_rop_fwd_src_or_notdst,
    cirrus_bitblt_rop_fwd_notsrc,
    cirrus_bitblt_rop_fwd_notsrc_or_dst,
    cirrus_bitblt_rop_fwd_notsrc_and_notdst,
};

static const cirrus_bitblt_rop_t cirrus_bkwd_rop[16] = {
    cirrus_bitblt_rop_bkwd_0,
    cirrus_bitblt_rop_bkwd_src_and_dst,
    cirrus_bitblt_rop_nop,
    cirrus_bitblt_rop_bkwd_src_and_notdst,
    cirrus_bitblt_rop_bkwd_notdst,
    cirrus_bitblt_rop_bkwd_src,
    cirrus_bitblt_rop_bkwd_1,
    cirrus_bitblt_rop_bkwd_notsrc_and_dst,
    cirrus_bitblt_rop_bkwd_src_xor_dst,
    cirrus_bitblt_rop_bkwd_src_or_dst,
    cirrus_bitblt_rop_bkwd_notsrc_or_notdst,
    cirrus_bitblt_rop_bkwd_src_notxor_dst,
    cirrus_bitblt_rop_bkwd_src_or_notdst,
    cirrus_bitblt_rop_bkwd_notsrc,
    cirrus_bitblt_rop_bkwd_notsrc_or_dst,
    cirrus_bitblt_rop_bkwd_notsrc_and_notdst,
};

#define TRANSP_ROP(name) {\
    name ## _8,\
    name ## _16,\
        }
#define TRANSP_NOP(func) {\
    func,\
    func,\
        }

static const cirrus_bitblt_rop_t cirrus_fwd_transp_rop[16][2] = {
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_0),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_and_dst),
    TRANSP_NOP(cirrus_bitblt_rop_nop),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_and_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_1),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_and_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_xor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_notxor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_src_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_fwd_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_bkwd_transp_rop[16][2] = {
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_0),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_and_dst),
    TRANSP_NOP(cirrus_bitblt_rop_nop),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_and_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_1),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_and_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_xor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_notxor_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_src_or_notdst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_or_dst),
    TRANSP_ROP(cirrus_bitblt_rop_bkwd_transp_notsrc_and_notdst),
};

#define ROP2(name) {\
    name ## _8,\
    name ## _16,\
    name ## _24,\
    name ## _32,\
        }

#define ROP_NOP2(func) {\
    func,\
    func,\
    func,\
    func,\
        }

static const cirrus_bitblt_rop_t cirrus_patternfill[16][4] = {
    ROP2(cirrus_patternfill_0),
    ROP2(cirrus_patternfill_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_patternfill_src_and_notdst),
    ROP2(cirrus_patternfill_notdst),
    ROP2(cirrus_patternfill_src),
    ROP2(cirrus_patternfill_1),
    ROP2(cirrus_patternfill_notsrc_and_dst),
    ROP2(cirrus_patternfill_src_xor_dst),
    ROP2(cirrus_patternfill_src_or_dst),
    ROP2(cirrus_patternfill_notsrc_or_notdst),
    ROP2(cirrus_patternfill_src_notxor_dst),
    ROP2(cirrus_patternfill_src_or_notdst),
    ROP2(cirrus_patternfill_notsrc),
    ROP2(cirrus_patternfill_notsrc_or_dst),
    ROP2(cirrus_patternfill_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_transp[16][4] = {
    ROP2(cirrus_colorexpand_transp_0),
    ROP2(cirrus_colorexpand_transp_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_transp_src_and_notdst),
    ROP2(cirrus_colorexpand_transp_notdst),
    ROP2(cirrus_colorexpand_transp_src),
    ROP2(cirrus_colorexpand_transp_1),
    ROP2(cirrus_colorexpand_transp_notsrc_and_dst),
    ROP2(cirrus_colorexpand_transp_src_xor_dst),
    ROP2(cirrus_colorexpand_transp_src_or_dst),
    ROP2(cirrus_colorexpand_transp_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_transp_src_notxor_dst),
    ROP2(cirrus_colorexpand_transp_src_or_notdst),
    ROP2(cirrus_colorexpand_transp_notsrc),
    ROP2(cirrus_colorexpand_transp_notsrc_or_dst),
    ROP2(cirrus_colorexpand_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand[16][4] = {
    ROP2(cirrus_colorexpand_0),
    ROP2(cirrus_colorexpand_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_src_and_notdst),
    ROP2(cirrus_colorexpand_notdst),
    ROP2(cirrus_colorexpand_src),
    ROP2(cirrus_colorexpand_1),
    ROP2(cirrus_colorexpand_notsrc_and_dst),
    ROP2(cirrus_colorexpand_src_xor_dst),
    ROP2(cirrus_colorexpand_src_or_dst),
    ROP2(cirrus_colorexpand_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_src_notxor_dst),
    ROP2(cirrus_colorexpand_src_or_notdst),
    ROP2(cirrus_colorexpand_notsrc),
    ROP2(cirrus_colorexpand_notsrc_or_dst),
    ROP2(cirrus_colorexpand_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_pattern_transp[16][4] = {
    ROP2(cirrus_colorexpand_pattern_transp_0),
    ROP2(cirrus_colorexpand_pattern_transp_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_pattern_transp_src_and_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_src),
    ROP2(cirrus_colorexpand_pattern_transp_1),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_and_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_xor_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_or_dst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_src_notxor_dst),
    ROP2(cirrus_colorexpand_pattern_transp_src_or_notdst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_or_dst),
    ROP2(cirrus_colorexpand_pattern_transp_notsrc_and_notdst),
};

static const cirrus_bitblt_rop_t cirrus_colorexpand_pattern[16][4] = {
    ROP2(cirrus_colorexpand_pattern_0),
    ROP2(cirrus_colorexpand_pattern_src_and_dst),
    ROP_NOP2(cirrus_bitblt_rop_nop),
    ROP2(cirrus_colorexpand_pattern_src_and_notdst),
    ROP2(cirrus_colorexpand_pattern_notdst),
    ROP2(cirrus_colorexpand_pattern_src),
    ROP2(cirrus_colorexpand_pattern_1),
    ROP2(cirrus_colorexpand_pattern_notsrc_and_dst),
    ROP2(cirrus_colorexpand_pattern_src_xor_dst),
    ROP2(cirrus_colorexpand_pattern_src_or_dst),
    ROP2(cirrus_colorexpand_pattern_notsrc_or_notdst),
    ROP2(cirrus_colorexpand_pattern_src_notxor_dst),
    ROP2(cirrus_colorexpand_pattern_src_or_notdst),
    ROP2(cirrus_colorexpand_pattern_notsrc),
    ROP2(cirrus_colorexpand_pattern_notsrc_or_dst),
    ROP2(cirrus_colorexpand_pattern_notsrc_and_notdst),
};

static const cirrus_fill_t cirrus_fill[16][4] = {
    ROP2(cirrus_fill_0),
    ROP2(cirrus_fill_src_and_dst),
    ROP_NOP2(cirrus_bitblt_fill_nop),
    ROP2(cirrus_fill_src_and_notdst),
    ROP2(cirrus_fill_notdst),
    ROP2(cirrus_fill_src),
    ROP2(cirrus_fill_1),
    ROP2(cirrus_fill_notsrc_and_dst),
    ROP2(cirrus_fill_src_xor_dst),
    ROP2(cirrus_fill_src_or_dst),
    ROP2(cirrus_fill_notsrc_or_notdst),
    ROP2(cirrus_fill_src_notxor_dst),
    ROP2(cirrus_fill_src_or_notdst),
    ROP2(cirrus_fill_notsrc),
    ROP2(cirrus_fill_notsrc_or_dst),
    ROP2(cirrus_fill_notsrc_and_notdst),
};

static void cirrus_bitblt_fgcol(CirrusVGAState *s)
{
    unsigned int color;
    switch (s->cirrus_blt_pixelwidth) {
    case 1:
        s->cirrus_blt_fgcol = s->cirrus_shadow_gr1;
        break;
    case 2:
        color = s->cirrus_shadow_gr1 | (s->gr[0x11] << 8);
        s->cirrus_blt_fgcol = le16_to_cpu(color);
        break;
    case 3:
        s->cirrus_blt_fgcol = s->cirrus_shadow_gr1 |
            (s->gr[0x11] << 8) | (s->gr[0x13] << 16);
        break;
    default:
    case 4:
        color = s->cirrus_shadow_gr1 | (s->gr[0x11] << 8) |
            (s->gr[0x13] << 16) | (s->gr[0x15] << 24);
        s->cirrus_blt_fgcol = le32_to_cpu(color);
        break;
    }
}

static void cirrus_bitblt_bgcol(CirrusVGAState *s)
{
    unsigned int color;
    switch (s->cirrus_blt_pixelwidth) {
    case 1:
        s->cirrus_blt_bgcol = s->cirrus_shadow_gr0;
        break;
    case 2:
        color = s->cirrus_shadow_gr0 | (s->gr[0x10] << 8);
        s->cirrus_blt_bgcol = le16_to_cpu(color);
        break;
    case 3:
        s->cirrus_blt_bgcol = s->cirrus_shadow_gr0 |
            (s->gr[0x10] << 8) | (s->gr[0x12] << 16);
        break;
    default:
    case 4:
        color = s->cirrus_shadow_gr0 | (s->gr[0x10] << 8) |
            (s->gr[0x12] << 16) | (s->gr[0x14] << 24);
        s->cirrus_blt_bgcol = le32_to_cpu(color);
        break;
    }
}

static void cirrus_invalidate_region(CirrusVGAState * s, int off_begin,
				     int off_pitch, int bytesperline,
				     int lines)
{
    int y;
    int off_cur;
    int off_cur_end;

    for (y = 0; y < lines; y++) {
		off_cur = off_begin;
		off_cur_end = (off_cur + bytesperline) & s->cirrus_addr_mask;
		off_cur &= TARGET_PAGE_MASK;
		while (off_cur < off_cur_end) {
			cpu_physical_memory_set_dirty(s->vram_offset + off_cur);
			off_cur += TARGET_PAGE_SIZE;
		}
		off_begin += off_pitch;
    }
}

static int cirrus_bitblt_common_patterncopy(CirrusVGAState * s,
					    const uint8_t * src)
{
    uint8_t *dst;

    dst = s->vram_ptr + (s->cirrus_blt_dstaddr & s->cirrus_addr_mask);

    if (BLTUNSAFE(s))
        return 0;

    (*s->cirrus_rop) (s, dst, src,
                      s->cirrus_blt_dstpitch, 0,
                      s->cirrus_blt_width, s->cirrus_blt_height);
    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
                             s->cirrus_blt_dstpitch, s->cirrus_blt_width,
                             s->cirrus_blt_height);
    return 1;
}

/* fill */

static int cirrus_bitblt_solidfill(CirrusVGAState *s, int blt_rop)
{
    cirrus_fill_t rop_func;

    if (BLTUNSAFE(s))
        return 0;
    rop_func = cirrus_fill[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
    rop_func(s, s->vram_ptr + (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
             s->cirrus_blt_dstpitch,
             s->cirrus_blt_width, s->cirrus_blt_height);
    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
			     s->cirrus_blt_dstpitch, s->cirrus_blt_width,
			     s->cirrus_blt_height);
    cirrus_bitblt_reset(s);
    return 1;
}

/***************************************
 *
 *  bitblt (video-to-video)
 *
 ***************************************/

static int cirrus_bitblt_videotovideo_patterncopy(CirrusVGAState * s)
{
    return cirrus_bitblt_common_patterncopy(s,
					    s->vram_ptr + ((s->cirrus_blt_srcaddr & ~7) &
                                            s->cirrus_addr_mask));
}

static void cirrus_do_copy(CirrusVGAState *s, int dst, int src, int w, int h)
{
    int sx, sy;
    int dx, dy;
    int width, height;
    int depth;
    int notify = 0;

    depth = s->get_bpp((VGAState *)s) / 8;
	if(depth==0) return;
	if(s->cirrus_blt_srcpitch==0) return;
	if(s->cirrus_blt_dstpitch==0) return;
    s->get_resolution((VGAState *)s, &width, &height);

    /* extra x, y */
    sx = (src % ABS(s->cirrus_blt_srcpitch)) / depth;
    sy = (src / ABS(s->cirrus_blt_srcpitch));
    dx = (dst % ABS(s->cirrus_blt_dstpitch)) / depth;
    dy = (dst / ABS(s->cirrus_blt_dstpitch));

    /* normalize width */
    w /= depth;

    /* if we're doing a backward copy, we have to adjust
       our x/y to be the upper left corner (instead of the lower
       right corner) */
    if (s->cirrus_blt_dstpitch < 0) {
		sx -= (s->cirrus_blt_width / depth) - 1;
		dx -= (s->cirrus_blt_width / depth) - 1;
		sy -= s->cirrus_blt_height - 1;
		dy -= s->cirrus_blt_height - 1;
    }

    /* are we in the visible portion of memory? */
    if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 &&
		(sx + w) <= width && (sy + h) <= height &&
		(dx + w) <= width && (dy + h) <= height) {
		notify = 1;
    }

    /* make to sure only copy if it's a plain copy ROP */
    if (*s->cirrus_rop != cirrus_bitblt_rop_fwd_src &&
		*s->cirrus_rop != cirrus_bitblt_rop_bkwd_src)
		notify = 0;

    /* we have to flush all pending changes so that the copy
       is generated at the appropriate moment in time */
    if (notify){
		vga_hw_update();
	}
    (*s->cirrus_rop) (s, s->vram_ptr +
		      (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
		      s->vram_ptr +
		      (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
		      s->cirrus_blt_dstpitch, s->cirrus_blt_srcpitch,
		      s->cirrus_blt_width, s->cirrus_blt_height);

    if (notify){
		qemu_console_copy(s->ds,
				  sx, sy, dx, dy,
				  s->cirrus_blt_width / depth,
				  s->cirrus_blt_height);
	}

    /* we don't have to notify the display that this portion has
       changed since qemu_console_copy implies this */

    cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
				s->cirrus_blt_dstpitch, s->cirrus_blt_width,
				s->cirrus_blt_height);
}

static int cirrus_bitblt_videotovideo_copy(CirrusVGAState * s)
{
    if (BLTUNSAFE(s))
        return 0;

    cirrus_do_copy(s, s->cirrus_blt_dstaddr - s->start_addr,
            s->cirrus_blt_srcaddr - s->start_addr,
            s->cirrus_blt_width, s->cirrus_blt_height);

    return 1;
}

/***************************************
 *
 *  bitblt (cpu-to-video)
 *
 ***************************************/

static void cirrus_bitblt_cputovideo_next(CirrusVGAState * s)
{
    int copy_count;
    uint8_t *end_ptr;

    if (s->cirrus_srccounter > 0) {
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
			cirrus_bitblt_common_patterncopy(s, s->cirrus_bltbuf);
        the_end:
            s->cirrus_srccounter = 0;
            cirrus_bitblt_reset(s);
        } else {
            /* at least one scan line */
            do {
				(*s->cirrus_rop)(s, s->vram_ptr +
                                (s->cirrus_blt_dstaddr & s->cirrus_addr_mask),
                                s->cirrus_bltbuf, 0, 0, s->cirrus_blt_width, 1);
                cirrus_invalidate_region(s, s->cirrus_blt_dstaddr, 0,
                                         s->cirrus_blt_width, 1);
                s->cirrus_blt_dstaddr += s->cirrus_blt_dstpitch;
                s->cirrus_srccounter -= s->cirrus_blt_srcpitch;
                if (s->cirrus_srccounter <= 0)
                    goto the_end;
                /* more bytes than needed can be transfered because of
                   word alignment, so we keep them for the next line */
                /* XXX: keep alignment to speed up transfer */
                end_ptr = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
                copy_count = (int)(s->cirrus_srcptr_end - end_ptr);
                memmove(s->cirrus_bltbuf, end_ptr, copy_count);
                s->cirrus_srcptr = s->cirrus_bltbuf + copy_count;
                s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
            } while (s->cirrus_srcptr >= s->cirrus_srcptr_end);
        }
    }
}

/***************************************
 *
 *  bitblt (video-to-cpu)
 *
 ***************************************/

static void cirrus_bitblt_videotocpu_next(CirrusVGAState * s)
{
    int copy_count;
    uint8_t *end_ptr;

    if (s->cirrus_srccounter > 0) {
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
			// XXX: not implemented
			//cirrus_bitblt_common_patterncopy(s, s->cirrus_bltbuf);
        the_end:
            s->cirrus_srccounter = 0;
            cirrus_bitblt_reset(s);
        } else {
            s->cirrus_blt_srcaddr += s->cirrus_blt_srcpitch;
            s->cirrus_srccounter -= s->cirrus_blt_dstpitch;
            if (s->cirrus_srccounter <= 0)
                goto the_end;
			(*s->cirrus_rop)(s, s->cirrus_bltbuf, s->vram_ptr +
                            (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
                            0, 0, s->cirrus_blt_width, 1);
			s->cirrus_srcptr = s->cirrus_bltbuf;
			s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
        }
    }
}

/***************************************
 *
 *  bitblt wrapper
 *
 ***************************************/

static void cirrus_bitblt_reset(CirrusVGAState * s)
{
    int need_update;

    s->gr[0x31] &=
	~(CIRRUS_BLT_START | CIRRUS_BLT_BUSY | CIRRUS_BLT_FIFOUSED);
    need_update = s->cirrus_srcptr != &s->cirrus_bltbuf[0]
        || s->cirrus_srcptr_end != &s->cirrus_bltbuf[0];
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
    s->cirrus_srccounter = 0;
    if (!need_update)
        return;
    cirrus_update_memory_access(s);
}

static int cirrus_bitblt_cputovideo(CirrusVGAState * s)
{
    int w;
	
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_MEMSYSSRC;
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
	
    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
			s->cirrus_blt_srcpitch = 8;
		} else {
				/* XXX: check for 24 bpp */
			s->cirrus_blt_srcpitch = 8 * 8 * s->cirrus_blt_pixelwidth;
		}
		s->cirrus_srccounter = s->cirrus_blt_srcpitch;
    } else {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
            w = s->cirrus_blt_width / s->cirrus_blt_pixelwidth;
            if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_DWORDGRANULARITY)
                s->cirrus_blt_srcpitch = ((w + 31) >> 5);
            else
                s->cirrus_blt_srcpitch = ((w + 7) >> 3);
		} else {
				/* always align input size to 32 bits */
			s->cirrus_blt_srcpitch = (s->cirrus_blt_width + 3) & ~3;
		}
        s->cirrus_srccounter = s->cirrus_blt_srcpitch * s->cirrus_blt_height;
    }
    s->cirrus_srcptr = s->cirrus_bltbuf;
    s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_srcpitch;
    cirrus_update_memory_access(s);
    return 1;
}

static int cirrus_bitblt_videotocpu(CirrusVGAState * s)
{
	// bitblt (video to cpu) 暫定プログラム（cputovideoからの推測・正しいかは不明）
    int w;
	
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_MEMSYSSRC;
    s->cirrus_srcptr = &s->cirrus_bltbuf[0];
    s->cirrus_srcptr_end = &s->cirrus_bltbuf[0];
	
    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
			s->cirrus_blt_dstpitch = 8;
		} else {
				/* XXX: check for 24 bpp */
			s->cirrus_blt_dstpitch = 8 * 8 * s->cirrus_blt_pixelwidth;
		}
		s->cirrus_srccounter = s->cirrus_blt_dstpitch;
    } else {
		if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
            w = s->cirrus_blt_width / s->cirrus_blt_pixelwidth;
            if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_DWORDGRANULARITY)
                s->cirrus_blt_dstpitch = ((w + 31) >> 5);
            else
                s->cirrus_blt_dstpitch = ((w + 7) >> 3);
		} else {
				/* always align input size to 32 bits */
			s->cirrus_blt_dstpitch = (s->cirrus_blt_width + 3) & ~3;
		}
        s->cirrus_srccounter = s->cirrus_blt_dstpitch * s->cirrus_blt_height;
    }
    s->cirrus_srcptr = s->cirrus_bltbuf;
    s->cirrus_srcptr_end = s->cirrus_bltbuf + s->cirrus_blt_dstpitch;
	if(s->cirrus_blt_dstpitch<0){
		cirrus_update_memory_access(s);
	}
    cirrus_update_memory_access(s);

	memset(s->cirrus_bltbuf, 0, s->cirrus_blt_width);

	// Copy 1st data
	(*s->cirrus_rop)(s, s->cirrus_bltbuf, s->vram_ptr +
                    (s->cirrus_blt_srcaddr & s->cirrus_addr_mask),
                    0, 0, s->cirrus_blt_width, 1);
    return 1;
}

static int cirrus_bitblt_videotovideo(CirrusVGAState * s)
{
    int ret;

    if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
		ret = cirrus_bitblt_videotovideo_patterncopy(s);
    } else {
		ret = cirrus_bitblt_videotovideo_copy(s);
    }
    if (ret)
		cirrus_bitblt_reset(s);
    return ret;
}

static void cirrus_bitblt_start(CirrusVGAState * s)
{
    uint8_t blt_rop;
	
    s->gr[0x31] |= CIRRUS_BLT_BUSY;

    s->cirrus_blt_width = (s->gr[0x20] | (s->gr[0x21] << 8)) + 1;
    s->cirrus_blt_height = (s->gr[0x22] | (s->gr[0x23] << 8)) + 1;
    s->cirrus_blt_dstpitch = (s->gr[0x24] | (s->gr[0x25] << 8));
    s->cirrus_blt_srcpitch = (s->gr[0x26] | (s->gr[0x27] << 8));
    s->cirrus_blt_dstaddr = (s->gr[0x28] | (s->gr[0x29] << 8) | (s->gr[0x2a] << 16));
    s->cirrus_blt_srcaddr = (s->gr[0x2c] | (s->gr[0x2d] << 8) | (s->gr[0x2e] << 16));
    s->cirrus_blt_mode = s->gr[0x30];
    blt_rop = s->gr[0x32];
	s->cirrus_blt_modeext = 0; //s->gr[0x33];  // ver0.86 rev8

#ifdef DEBUG_BITBLT
    printf("rop=0x%02x mode=0x%02x modeext=0x%02x w=%d h=%d dpitch=%d spitch=%d daddr=0x%08x saddr=0x%08x writemask=0x%02x\n",
           blt_rop,
           s->cirrus_blt_mode,
           s->cirrus_blt_modeext,
           s->cirrus_blt_width,
           s->cirrus_blt_height,
           s->cirrus_blt_dstpitch,
           s->cirrus_blt_srcpitch,
           s->cirrus_blt_dstaddr,
           s->cirrus_blt_srcaddr,
           s->gr[0x2f]);
#endif

    switch (s->cirrus_blt_mode & CIRRUS_BLTMODE_PIXELWIDTHMASK) {
		case CIRRUS_BLTMODE_PIXELWIDTH8:
			s->cirrus_blt_pixelwidth = 1;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH16:
			s->cirrus_blt_pixelwidth = 2;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH24:
			s->cirrus_blt_pixelwidth = 3;
			break;
		case CIRRUS_BLTMODE_PIXELWIDTH32:
			s->cirrus_blt_pixelwidth = 4;
			break;
		default:
#ifdef DEBUG_BITBLT
			printf("cirrus: bitblt - pixel width is unknown\n");
#endif
			goto bitblt_ignore;
    }
    s->cirrus_blt_mode &= ~CIRRUS_BLTMODE_PIXELWIDTHMASK;

    if ((s->
	 cirrus_blt_mode & (CIRRUS_BLTMODE_MEMSYSSRC | CIRRUS_BLTMODE_MEMSYSDEST))
				== (CIRRUS_BLTMODE_MEMSYSSRC | CIRRUS_BLTMODE_MEMSYSDEST)) {
#ifdef DEBUG_BITBLT
		printf("cirrus: bitblt - memory-to-memory copy is requested\n");
#endif
		goto bitblt_ignore;
    }

    if ((s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_SOLIDFILL) &&
        (s->cirrus_blt_mode & (CIRRUS_BLTMODE_MEMSYSDEST |
                               CIRRUS_BLTMODE_TRANSPARENTCOMP |
                               CIRRUS_BLTMODE_PATTERNCOPY |
                               CIRRUS_BLTMODE_COLOREXPAND)) ==
         (CIRRUS_BLTMODE_PATTERNCOPY | CIRRUS_BLTMODE_COLOREXPAND)) {
        cirrus_bitblt_fgcol(s);
        cirrus_bitblt_solidfill(s, blt_rop);
    } else {
        if ((s->cirrus_blt_mode & (CIRRUS_BLTMODE_COLOREXPAND |
                                   CIRRUS_BLTMODE_PATTERNCOPY)) ==
            CIRRUS_BLTMODE_COLOREXPAND) {

            if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
                if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_COLOREXPINV)
                    cirrus_bitblt_bgcol(s);
                else
                    cirrus_bitblt_fgcol(s);
                s->cirrus_rop = cirrus_colorexpand_transp[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            } else {
                cirrus_bitblt_fgcol(s);
                cirrus_bitblt_bgcol(s);
                s->cirrus_rop = cirrus_colorexpand[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            }
        } else if (s->cirrus_blt_mode & CIRRUS_BLTMODE_PATTERNCOPY) {
            if (s->cirrus_blt_mode & CIRRUS_BLTMODE_COLOREXPAND) {
                if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
                    if (s->cirrus_blt_modeext & CIRRUS_BLTMODEEXT_COLOREXPINV)
                        cirrus_bitblt_bgcol(s);
                    else
                        cirrus_bitblt_fgcol(s);
                    s->cirrus_rop = cirrus_colorexpand_pattern_transp[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
                } else {
                    cirrus_bitblt_fgcol(s);
                    cirrus_bitblt_bgcol(s);
                    s->cirrus_rop = cirrus_colorexpand_pattern[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
                }
            } else {
                s->cirrus_rop = cirrus_patternfill[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
            }
        } else {
			if (s->cirrus_blt_mode & CIRRUS_BLTMODE_TRANSPARENTCOMP) {
				if (s->cirrus_blt_pixelwidth > 2) {
					printf("src transparent without colorexpand must be 8bpp or 16bpp\n");
					goto bitblt_ignore;
				}
				if (s->cirrus_blt_mode & CIRRUS_BLTMODE_BACKWARDS) {
					s->cirrus_blt_dstpitch = -s->cirrus_blt_dstpitch;
					s->cirrus_blt_srcpitch = -s->cirrus_blt_srcpitch;
					s->cirrus_rop = cirrus_bkwd_transp_rop[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
				} else {
					s->cirrus_rop = cirrus_fwd_transp_rop[rop_to_index[blt_rop]][s->cirrus_blt_pixelwidth - 1];
				}
			} else {
				if (s->cirrus_blt_mode & CIRRUS_BLTMODE_BACKWARDS) {
					s->cirrus_blt_dstpitch = -s->cirrus_blt_dstpitch;
					s->cirrus_blt_srcpitch = -s->cirrus_blt_srcpitch;
					s->cirrus_rop = cirrus_bkwd_rop[rop_to_index[blt_rop]];
				} else {
					s->cirrus_rop = cirrus_fwd_rop[rop_to_index[blt_rop]];
				}
			}
		}
        // setup bitblt engine.
        if (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSSRC) {
            if (!cirrus_bitblt_cputovideo(s))
                goto bitblt_ignore;
        } else if (s->cirrus_blt_mode & CIRRUS_BLTMODE_MEMSYSDEST) {
            if (!cirrus_bitblt_videotocpu(s))
                goto bitblt_ignore;
        } else {
            if (!cirrus_bitblt_videotovideo(s))
                goto bitblt_ignore;
        }
    }
    return;
  bitblt_ignore:;
    cirrus_bitblt_reset(s);
}

static void cirrus_write_bitblt(CirrusVGAState * s, unsigned reg_value)
{
    unsigned old_value;

    old_value = s->gr[0x31];
    s->gr[0x31] = reg_value;

    if (((old_value & CIRRUS_BLT_RESET) != 0) &&
		((reg_value & CIRRUS_BLT_RESET) == 0)) {
		cirrus_bitblt_start(s);// XXX: Win2000のハードウェアアクセラレーションを正常に動かすのに必要。根拠無し。
		cirrus_bitblt_reset(s);
    } else if (((old_value & CIRRUS_BLT_START) == 0) &&
			   ((reg_value & CIRRUS_BLT_START) != 0)) {
		cirrus_bitblt_start(s);
	}
}


/***************************************
 *
 *  basic parameters
 *
 ***************************************/

static void cirrus_get_offsets(VGAState *s1,
                               uint32_t_ *pline_offset,
                               uint32_t_ *pstart_addr,
                               uint32_t_ *pline_compare)
{
    CirrusVGAState * s = (CirrusVGAState *)s1;
    uint32_t_ start_addr, line_offset, line_compare;

    line_offset = s->cr[0x13]
	| ((s->cr[0x1b] & 0x10) << 4);
    line_offset <<= 3;
    *pline_offset = line_offset;

    start_addr = (s->cr[0x0c] << 8)
	| s->cr[0x0d]
	| ((s->cr[0x1b] & 0x01) << 16)
	| ((s->cr[0x1b] & 0x0c) << 15)
	| ((s->cr[0x1d] & 0x80) << 12);
    *pstart_addr = start_addr;

    line_compare = s->cr[0x18] |
        ((s->cr[0x07] & 0x10) << 4) |
        ((s->cr[0x09] & 0x40) << 3);
    *pline_compare = line_compare;
}

static uint32_t_ cirrus_get_bpp16_depth(CirrusVGAState * s)
{
    uint32_t_ ret = 16;

    switch (s->cirrus_hidden_dac_data & 0xf) {
    case 0:
	ret = 15;
	break;			/* Sierra HiColor */
    case 1:
	ret = 16;
	break;			/* XGA HiColor */
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: invalid DAC value %x in 16bpp\n",
	       (s->cirrus_hidden_dac_data & 0xf));
#endif
	ret = 15;		/* XXX */
	break;
    }
    return ret;
}

static int cirrus_get_bpp(VGAState *s1)
{
    CirrusVGAState * s = (CirrusVGAState *)s1;
    uint32_t_ ret = 8;

    if ((s->sr[0x07] & 0x01) != 0) {
		/* Cirrus SVGA */
		switch (s->sr[0x07] & CIRRUS_SR7_BPP_MASK) {
		case CIRRUS_SR7_BPP_8:
			ret = 8;
			break;
		case CIRRUS_SR7_BPP_16_DOUBLEVCLK:
			ret = cirrus_get_bpp16_depth(s);
			break;
		case CIRRUS_SR7_BPP_24:
			ret = 24;
			break;
		case CIRRUS_SR7_BPP_16:
			ret = cirrus_get_bpp16_depth(s);
			break;
		case CIRRUS_SR7_BPP_32:
			ret = 32;
			break;
		default:
#ifdef DEBUG_CIRRUS
			printf("cirrus: unknown bpp - sr7=%x\n", s->sr[0x7]);
#endif
			ret = 8;
			break;
		}
    } else {
		//ret = 8;
		/* VGA */
		ret = 0;
    }

    return ret;
}

static void cirrus_get_resolution(VGAState *s, int *pwidth, int *pheight)
{
    int width, height;

    width = (s->cr[0x01] + 1) * 8;
    height = s->cr[0x12] |
        ((s->cr[0x07] & 0x02) << 7) |
        ((s->cr[0x07] & 0x40) << 3);
    height = (height + 1);
    /* interlace support */
    if (s->cr[0x1a] & 0x01)
        height = height * 2;
	if(width==320) height /= 2; // XXX: とりあえず仮
	if(width==400) height = 300; // XXX: とりあえず仮
	if(width==512) height = 384; // XXX: とりあえず仮
	//if(width<320){
	//	width = 1024; // XXX: とりあえず仮
	//}
	//if(height<200){
	//	height = 768; // XXX: とりあえず仮
	//}

    *pwidth = width;
    *pheight = height;
}

/***************************************
 *
 * bank memory
 *
 ***************************************/

static void cirrus_update_bank_ptr(CirrusVGAState * s, unsigned bank_index)
{
    unsigned offset;
    unsigned limit;

    if ((s->gr[0x0b] & 0x01) != 0)	/* dual bank */
		offset = s->gr[0x09 + bank_index];
    else			/* single bank */
		offset = s->gr[0x09];

    if ((s->gr[0x0b] & 0x20) != 0)
		offset <<= 14;
    else
		offset <<= 12;

    if ((unsigned int)s->real_vram_size <= offset)
		limit = 0;
    else
		limit = s->real_vram_size - offset;

    if (((s->gr[0x0b] & 0x01) == 0) && (bank_index != 0)) {
		if (limit > 0x8000) {
			offset += 0x8000;
			limit -= 0x8000;
		} else {
			limit = 0;
		}
    }

    if (limit > 0) {
        /* Thinking about changing bank base? First, drop the dirty bitmap information
         * on the current location, otherwise we lose this pointer forever */
        if (s->lfb_vram_mapped) {
            target_phys_addr_t base_addr = isa_mem_base + 0xF80000 + bank_index * 0x8000;
            cpu_physical_sync_dirty_bitmap(base_addr, base_addr + 0x8000);
			TRACEOUT(("UNSUPPORT1"));
        }
		s->cirrus_bank_base[bank_index] = offset;
		s->cirrus_bank_limit[bank_index] = limit;
    } else {
		s->cirrus_bank_base[bank_index] = 0;
		s->cirrus_bank_limit[bank_index] = 0;
    }
}

/***************************************
 *
 *  I/O access between 0x3c4-0x3c5
 *
 ***************************************/

static int
cirrus_hook_read_sr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x06:			// Unlock Cirrus extensions
		*reg_value = s->sr[reg_index];
		break;
    case 0x10:
    case 0x30:
    case 0x50:
    case 0x70:			// Graphics Cursor X
    case 0x90:
    case 0xb0:
    case 0xd0:
    case 0xf0:			// Graphics Cursor X
		*reg_value = s->sr[0x10];
		break;
    case 0x11:
    case 0x31:
    case 0x51:
    case 0x71:			// Graphics Cursor Y
    case 0x91:
    case 0xb1:
    case 0xd1:
    case 0xf1:			// Graphics Cursor Y
		*reg_value = s->sr[0x11];
		break;
    case 0x05:			// ???
    case 0x07:			// Extended Sequencer Mode
		cirrus_update_memory_access(s);
    case 0x08:			// EEPROM Control
    case 0x09:			// Scratch Register 0
    case 0x0a:			// Scratch Register 1
    case 0x0b:			// VCLK 0
    case 0x0c:			// VCLK 1
    case 0x0d:			// VCLK 2
    case 0x0e:			// VCLK 3
    case 0x0f:			// DRAM Control
    case 0x12:			// Graphics Cursor Attribute
    case 0x13:			// Graphics Cursor Pattern Address
    case 0x14:			// Scratch Register 2
    case 0x15:			// Scratch Register 3
    case 0x16:			// Performance Tuning Register
    case 0x17:			// Configuration Readback and Extended Control
    case 0x18:			// Signature Generator Control
    case 0x19:			// Signal Generator Result
    case 0x1a:			// Signal Generator Result
    case 0x1b:			// VCLK 0 Denominator & Post
    case 0x1c:			// VCLK 1 Denominator & Post
    case 0x1d:			// VCLK 2 Denominator & Post
    case 0x1e:			// VCLK 3 Denominator & Post
    case 0x1f:			// BIOS Write Enable and MCLK select
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled inport sr_index %02x\n", reg_index);
#endif
		*reg_value = s->sr[reg_index];
		break;
    default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: inport sr_index %02x\n", reg_index);
#endif
		*reg_value = 0xff;
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_sr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x06:			// Unlock Cirrus extensions
		reg_value &= 0x17;
		if (reg_value == 0x12) {
			s->sr[reg_index] = 0x12;
		} else {
			s->sr[reg_index] = 0x0f;
		}
		break;
    case 0x10:
    case 0x30:
    case 0x50:
    case 0x70:			// Graphics Cursor X
    case 0x90:
    case 0xb0:
    case 0xd0:
    case 0xf0:			// Graphics Cursor X
		s->sr[0x10] = reg_value;
		s->hw_cursor_x = (reg_value << 3) | (reg_index >> 5);
	break;
    case 0x11:
    case 0x31:
    case 0x51:
    case 0x71:			// Graphics Cursor Y
    case 0x91:
    case 0xb1:
    case 0xd1:
    case 0xf1:			// Graphics Cursor Y
		s->sr[0x11] = reg_value;
		s->hw_cursor_y = (reg_value << 3) | (reg_index >> 5);
	break;
	case 0x07:			// Extended Sequencer Mode
		cirrus_update_memory_access(s);
    case 0x08:			// EEPROM Control
    case 0x09:			// Scratch Register 0
    case 0x0a:			// Scratch Register 1
    case 0x0b:			// VCLK 0
    case 0x0c:			// VCLK 1
    case 0x0d:			// VCLK 2
    case 0x0e:			// VCLK 3
    case 0x0f:			// DRAM Control
    case 0x12:			// Graphics Cursor Attribute
    case 0x13:			// Graphics Cursor Pattern Address
    case 0x14:			// Scratch Register 2
    case 0x15:			// Scratch Register 3
    case 0x16:			// Performance Tuning Register
    case 0x18:			// Signature Generator Control
    case 0x19:			// Signature Generator Result
    case 0x1a:			// Signature Generator Result
    case 0x1b:			// VCLK 0 Denominator & Post
    case 0x1c:			// VCLK 1 Denominator & Post
    case 0x1d:			// VCLK 2 Denominator & Post
    case 0x1e:			// VCLK 3 Denominator & Post
    case 0x1f:			// BIOS Write Enable and MCLK select
		s->sr[reg_index] = reg_value;
#ifdef DEBUG_CIRRUS
		printf("cirrus: handled outport sr_index %02x, sr_value %02x\n",
			   reg_index, reg_value);
#endif
		break;
    case 0x17:			// Configuration Readback and Extended Control
		s->sr[reg_index] = (s->sr[reg_index] & 0x38) | (reg_value & 0xc7);
        cirrus_update_memory_access(s);
        break;
    default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: outport sr_index %02x, sr_value %02x\n", reg_index,
			   reg_value);
#endif
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access at 0x3c6
 *
 ***************************************/

static void cirrus_read_hidden_dac(CirrusVGAState * s, int *reg_value)
{
    *reg_value = 0xff;
    if (++s->cirrus_hidden_dac_lockindex == 5) {
        *reg_value = s->cirrus_hidden_dac_data;
	s->cirrus_hidden_dac_lockindex = 0;
    }
}

static void cirrus_write_hidden_dac(CirrusVGAState * s, int reg_value)
{
    if (s->cirrus_hidden_dac_lockindex == 4) {
	s->cirrus_hidden_dac_data = reg_value;
#if defined(DEBUG_CIRRUS)
	printf("cirrus: outport hidden DAC, value %02x\n", reg_value);
#endif
    }
    s->cirrus_hidden_dac_lockindex = 0;
}

/***************************************
 *
 *  I/O access at 0x3c9
 *
 ***************************************/

static int cirrus_hook_read_palette(CirrusVGAState * s, int *reg_value)
{
    if (!(s->sr[0x12] & CIRRUS_CURSOR_HIDDENPEL))
	return CIRRUS_HOOK_NOT_HANDLED;
    *reg_value =
        s->cirrus_hidden_palette[(s->dac_read_index & 0x0f) * 3 +
                                 s->dac_sub_index];
    if (++s->dac_sub_index == 3) {
	s->dac_sub_index = 0;
	s->dac_read_index++;
    }
    return CIRRUS_HOOK_HANDLED;
}

static int cirrus_hook_write_palette(CirrusVGAState * s, int reg_value)
{
    if (!(s->sr[0x12] & CIRRUS_CURSOR_HIDDENPEL))
	return CIRRUS_HOOK_NOT_HANDLED;
    s->dac_cache[s->dac_sub_index] = reg_value;
    if (++s->dac_sub_index == 3) {
        memcpy(&s->cirrus_hidden_palette[(s->dac_write_index & 0x0f) * 3],
               s->dac_cache, 3);
        /* XXX update cursor */
	s->dac_sub_index = 0;
	s->dac_write_index++;
    }
    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access between 0x3ce-0x3cf
 *
 ***************************************/

static int
cirrus_hook_read_gr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00: // Standard VGA, BGCOLOR 0x000000ff
      *reg_value = s->cirrus_shadow_gr0;
      return CIRRUS_HOOK_HANDLED;
    case 0x01: // Standard VGA, FGCOLOR 0x000000ff
      *reg_value = s->cirrus_shadow_gr1;
      return CIRRUS_HOOK_HANDLED;
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x05:			// Standard VGA, Cirrus extended mode
	default:
		break;
    }

    if (reg_index < 0x3a) {
		*reg_value = s->gr[reg_index];
    } else {
#ifdef DEBUG_CIRRUS
		printf("cirrus: inport gr_index %02x\n", reg_index);
#endif
		*reg_value = 0xff;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_gr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
#if defined(DEBUG_BITBLT) && 0
    printf("gr%02x: %02x\n", reg_index, reg_value);
#endif
    switch (reg_index) {
    case 0x00:			// Standard VGA, BGCOLOR 0x000000ff
		s->cirrus_shadow_gr0 = reg_value;
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x01:			// Standard VGA, FGCOLOR 0x000000ff
		s->cirrus_shadow_gr1 = reg_value;
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x05:			// Standard VGA, Cirrus extended mode
		s->gr[reg_index] = reg_value & 0x7f;
        cirrus_update_memory_access(s);
		break;
    case 0x09:			// bank offset #0
    case 0x0A:			// bank offset #1
		s->gr[reg_index] = reg_value;
		cirrus_update_bank_ptr(s, 0);
		cirrus_update_bank_ptr(s, 1);
        cirrus_update_memory_access(s);
        break;
    case 0x0B:
		s->gr[reg_index] = reg_value;
		cirrus_update_bank_ptr(s, 0);
		cirrus_update_bank_ptr(s, 1);
        cirrus_update_memory_access(s);
		break;
    case 0x10:			// BGCOLOR 0x0000ff00
    case 0x11:			// FGCOLOR 0x0000ff00
    case 0x12:			// BGCOLOR 0x00ff0000
    case 0x13:			// FGCOLOR 0x00ff0000
    case 0x14:			// BGCOLOR 0xff000000
    case 0x15:			// FGCOLOR 0xff000000
    case 0x20:			// BLT WIDTH 0x0000ff
    case 0x22:			// BLT HEIGHT 0x0000ff
    case 0x24:			// BLT DEST PITCH 0x0000ff
    case 0x26:			// BLT SRC PITCH 0x0000ff
    case 0x28:			// BLT DEST ADDR 0x0000ff
    case 0x29:			// BLT DEST ADDR 0x00ff00
    case 0x2c:			// BLT SRC ADDR 0x0000ff
    case 0x2d:			// BLT SRC ADDR 0x00ff00
    case 0x2f:          // BLT WRITEMASK
    case 0x30:			// BLT MODE
    case 0x32:			// RASTER OP
    case 0x33:			// BLT MODEEXT
    case 0x34:			// BLT TRANSPARENT COLOR 0x00ff
    case 0x35:			// BLT TRANSPARENT COLOR 0xff00
    case 0x38:			// BLT TRANSPARENT COLOR MASK 0x00ff
    case 0x39:			// BLT TRANSPARENT COLOR MASK 0xff00
		s->gr[reg_index] = reg_value;
		break;
    case 0x21:			// BLT WIDTH 0x001f00
    case 0x23:			// BLT HEIGHT 0x001f00
    case 0x25:			// BLT DEST PITCH 0x001f00
    case 0x27:			// BLT SRC PITCH 0x001f00
		s->gr[reg_index] = reg_value & 0x1f;
		break;
    case 0x2a:			// BLT DEST ADDR 0x3f0000
		s->gr[reg_index] = reg_value & 0x3f;
        /* if auto start mode, starts bit blt now */
        if (s->gr[0x31] & CIRRUS_BLT_AUTOSTART) {
            cirrus_bitblt_start(s);
        }
		break;
    case 0x2e:			// BLT SRC ADDR 0x3f0000
		s->gr[reg_index] = reg_value & 0x3f;
		break;
    case 0x31:			// BLT STATUS/START
		cirrus_write_bitblt(s, reg_value);
		//cirrus_write_bitblt(s, reg_value);
		break;
	default:
#ifdef DEBUG_CIRRUS
		printf("cirrus: outport gr_index %02x, gr_value %02x\n", reg_index,
			   reg_value);
#endif
		break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  I/O access between 0x3d4-0x3d5
 *
 ***************************************/

static int
cirrus_hook_read_cr(CirrusVGAState * s, unsigned reg_index, int *reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x05:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
    case 0x09:			// Standard VGA
    case 0x0a:			// Standard VGA
    case 0x0b:			// Standard VGA
    case 0x0c:			// Standard VGA
    case 0x0d:			// Standard VGA
    case 0x0e:			// Standard VGA
    case 0x0f:			// Standard VGA
    case 0x10:			// Standard VGA
    case 0x11:			// Standard VGA
    case 0x12:			// Standard VGA
    case 0x13:			// Standard VGA
    case 0x14:			// Standard VGA
    case 0x15:			// Standard VGA
    case 0x16:			// Standard VGA
    case 0x17:			// Standard VGA
    case 0x18:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x24:			// Attribute Controller Toggle Readback (R)
        *reg_value = (s->ar_flip_flop << 7);
        break;
    case 0x19:			// Interlace End
    case 0x1a:			// Miscellaneous Control
    case 0x1b:			// Extended Display Control
    case 0x1c:			// Sync Adjust and Genlock
    case 0x1d:			// Overlay Extended Control
    case 0x22:			// Graphics Data Latches Readback (R)
		*reg_value = s->cr[reg_index];
        break;
    case 0x25:			// Part Status
		*reg_value = s->cr[reg_index];
        break;
    case 0x27:			// Part ID (R)
		if(np2clvga.gd54xxtype <= 0xff){
			*reg_value = s->cr[reg_index];
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
			*reg_value = 0x98;
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
			*reg_value = 0xA8; // XXX
		}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
			*reg_value = 0xA8; // XXX
		}else{
			*reg_value = s->cr[reg_index];
		}
        break;
    case 0x28:			// Class ID Register
		if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
			*reg_value = 0x03;
		}
		break;
    case 0x26:			// Attribute Controller Index Readback (R)
		*reg_value = s->ar_index & 0x3f;
		break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: inport cr_index %02x\n", reg_index);
	*reg_value = 0xff;
#endif
	break;
    }

    return CIRRUS_HOOK_HANDLED;
}

static int
cirrus_hook_write_cr(CirrusVGAState * s, unsigned reg_index, int reg_value)
{
    switch (reg_index) {
    case 0x00:			// Standard VGA
    case 0x01:			// Standard VGA
    case 0x02:			// Standard VGA
    case 0x03:			// Standard VGA
    case 0x04:			// Standard VGA
    case 0x05:			// Standard VGA
    case 0x06:			// Standard VGA
    case 0x07:			// Standard VGA
    case 0x08:			// Standard VGA
    case 0x09:			// Standard VGA
    case 0x0a:			// Standard VGA
    case 0x0b:			// Standard VGA
    case 0x0c:			// Standard VGA
    case 0x0d:			// Standard VGA
    case 0x0e:			// Standard VGA
    case 0x0f:			// Standard VGA
    case 0x10:			// Standard VGA
    case 0x11:			// Standard VGA
    case 0x12:			// Standard VGA
    case 0x13:			// Standard VGA
    case 0x14:			// Standard VGA
    case 0x15:			// Standard VGA
    case 0x16:			// Standard VGA
    case 0x17:			// Standard VGA
    case 0x18:			// Standard VGA
		return CIRRUS_HOOK_NOT_HANDLED;
    case 0x19:			// Interlace End
    case 0x1a:			// Miscellaneous Control
    case 0x1b:			// Extended Display Control
    case 0x1c:			// Sync Adjust and Genlock
    case 0x1d:			// Overlay Extended Control
	s->cr[reg_index] = reg_value;
#ifdef DEBUG_CIRRUS
	printf("cirrus: handled outport cr_index %02x, cr_value %02x\n",
	       reg_index, reg_value);
#endif
	break;
    case 0x22:			// Graphics Data Latches Readback (R)
    case 0x24:			// Attribute Controller Toggle Readback (R)
    case 0x26:			// Attribute Controller Index Readback (R)
    case 0x27:			// Part ID (R)
	break;
    case 0x25:			// Part Status
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: outport cr_index %02x, cr_value %02x\n", reg_index,
	       reg_value);
#endif
	break;
    }

    return CIRRUS_HOOK_HANDLED;
}

/***************************************
 *
 *  memory-mapped I/O (bitblt)
 *
 ***************************************/

static uint8_t cirrus_mmio_blt_read(CirrusVGAState * s, unsigned address)
{
    int value = 0xff;

    switch (address) {
    case (CIRRUS_MMIO_BLTBGCOLOR + 0):
	cirrus_hook_read_gr(s, 0x00, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 1):
	cirrus_hook_read_gr(s, 0x10, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 2):
	cirrus_hook_read_gr(s, 0x12, &value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 3):
	cirrus_hook_read_gr(s, 0x14, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 0):
	cirrus_hook_read_gr(s, 0x01, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 1):
	cirrus_hook_read_gr(s, 0x11, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 2):
	cirrus_hook_read_gr(s, 0x13, &value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 3):
	cirrus_hook_read_gr(s, 0x15, &value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 0):
	cirrus_hook_read_gr(s, 0x20, &value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 1):
	cirrus_hook_read_gr(s, 0x21, &value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 0):
	cirrus_hook_read_gr(s, 0x22, &value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 1):
	cirrus_hook_read_gr(s, 0x23, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 0):
	cirrus_hook_read_gr(s, 0x24, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 1):
	cirrus_hook_read_gr(s, 0x25, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 0):
	cirrus_hook_read_gr(s, 0x26, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 1):
	cirrus_hook_read_gr(s, 0x27, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 0):
	cirrus_hook_read_gr(s, 0x28, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 1):
	cirrus_hook_read_gr(s, 0x29, &value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 2):
	cirrus_hook_read_gr(s, 0x2a, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 0):
	cirrus_hook_read_gr(s, 0x2c, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 1):
	cirrus_hook_read_gr(s, 0x2d, &value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 2):
	cirrus_hook_read_gr(s, 0x2e, &value);
	break;
    case CIRRUS_MMIO_BLTWRITEMASK:
	cirrus_hook_read_gr(s, 0x2f, &value);
	break;
    case CIRRUS_MMIO_BLTMODE:
	cirrus_hook_read_gr(s, 0x30, &value);
	break;
    case CIRRUS_MMIO_BLTROP:
	cirrus_hook_read_gr(s, 0x32, &value);
	break;
    case CIRRUS_MMIO_BLTMODEEXT:
	cirrus_hook_read_gr(s, 0x33, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 0):
	cirrus_hook_read_gr(s, 0x34, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 1):
	cirrus_hook_read_gr(s, 0x35, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 0):
	cirrus_hook_read_gr(s, 0x38, &value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 1):
	cirrus_hook_read_gr(s, 0x39, &value);
	break;
    case CIRRUS_MMIO_BLTSTATUS:
	cirrus_hook_read_gr(s, 0x31, &value);
	break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: mmio read - address 0x%04x\n", address);
#endif
	break;
    }

    return (uint8_t) value;
}

static void cirrus_mmio_blt_write(CirrusVGAState * s, unsigned address,
				  uint8_t value)
{
    switch (address) {
    case (CIRRUS_MMIO_BLTBGCOLOR + 0):
	cirrus_hook_write_gr(s, 0x00, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 1):
	cirrus_hook_write_gr(s, 0x10, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 2):
	cirrus_hook_write_gr(s, 0x12, value);
	break;
    case (CIRRUS_MMIO_BLTBGCOLOR + 3):
	cirrus_hook_write_gr(s, 0x14, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 0):
	cirrus_hook_write_gr(s, 0x01, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 1):
	cirrus_hook_write_gr(s, 0x11, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 2):
	cirrus_hook_write_gr(s, 0x13, value);
	break;
    case (CIRRUS_MMIO_BLTFGCOLOR + 3):
	cirrus_hook_write_gr(s, 0x15, value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 0):
	cirrus_hook_write_gr(s, 0x20, value);
	break;
    case (CIRRUS_MMIO_BLTWIDTH + 1):
	cirrus_hook_write_gr(s, 0x21, value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 0):
	cirrus_hook_write_gr(s, 0x22, value);
	break;
    case (CIRRUS_MMIO_BLTHEIGHT + 1):
	cirrus_hook_write_gr(s, 0x23, value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 0):
	cirrus_hook_write_gr(s, 0x24, value);
	break;
    case (CIRRUS_MMIO_BLTDESTPITCH + 1):
	cirrus_hook_write_gr(s, 0x25, value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 0):
	cirrus_hook_write_gr(s, 0x26, value);
	break;
    case (CIRRUS_MMIO_BLTSRCPITCH + 1):
	cirrus_hook_write_gr(s, 0x27, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 0):
	cirrus_hook_write_gr(s, 0x28, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 1):
	cirrus_hook_write_gr(s, 0x29, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 2):
	cirrus_hook_write_gr(s, 0x2a, value);
	break;
    case (CIRRUS_MMIO_BLTDESTADDR + 3):
	/* ignored */
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 0):
	cirrus_hook_write_gr(s, 0x2c, value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 1):
	cirrus_hook_write_gr(s, 0x2d, value);
	break;
    case (CIRRUS_MMIO_BLTSRCADDR + 2):
	cirrus_hook_write_gr(s, 0x2e, value);
	break;
    case CIRRUS_MMIO_BLTWRITEMASK:
	cirrus_hook_write_gr(s, 0x2f, value);
	break;
    case CIRRUS_MMIO_BLTMODE:
	cirrus_hook_write_gr(s, 0x30, value);
	break;
    case CIRRUS_MMIO_BLTROP:
	cirrus_hook_write_gr(s, 0x32, value);
	break;
    case CIRRUS_MMIO_BLTMODEEXT:
	cirrus_hook_write_gr(s, 0x33, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 0):
	cirrus_hook_write_gr(s, 0x34, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLOR + 1):
	cirrus_hook_write_gr(s, 0x35, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 0):
	cirrus_hook_write_gr(s, 0x38, value);
	break;
    case (CIRRUS_MMIO_BLTTRANSPARENTCOLORMASK + 1):
	cirrus_hook_write_gr(s, 0x39, value);
	break;
    case CIRRUS_MMIO_BLTSTATUS:
	cirrus_hook_write_gr(s, 0x31, value);
	break;
    default:
#ifdef DEBUG_CIRRUS
	printf("cirrus: mmio write - addr 0x%04x val 0x%02x (ignored)\n",
	       address, value);
#endif
	break;
    }
}

/***************************************
 *
 *  write mode 4/5
 *
 * assume TARGET_PAGE_SIZE >= 16
 *
 ***************************************/

static void cirrus_mem_writeb_mode4and5_8bpp(CirrusVGAState * s,
					     unsigned mode,
					     unsigned offset,
					     uint32_t_ mem_value)
{
    int x;
    unsigned val = mem_value;
    unsigned mask = s->sr[0x2];
    uint8_t *dst;
	if(s->gr[0xb] & 0x04){
	}else{
		mask = 0xff;
	}

    dst = s->vram_ptr + (offset &= s->cirrus_addr_mask);
    for (x = 0; x < 8; x++) {
		if (mask & 0x80) {
			if (val & 0x80) {
				*dst = s->cirrus_shadow_gr1;
			} else if (mode == 5) {
				*dst = s->cirrus_shadow_gr0;
			}
		}
		val <<= 1;
		mask <<= 1;
		dst++;
    }
    cpu_physical_memory_set_dirty(s->vram_offset + offset);
    cpu_physical_memory_set_dirty(s->vram_offset + offset + 7);
}

static void cirrus_mem_writeb_mode4and5_16bpp(CirrusVGAState * s,
					      unsigned mode,
					      unsigned offset,
					      uint32_t_ mem_value)
{
    int x;
    unsigned val = mem_value;
    unsigned mask = s->sr[0x2];
    uint8_t *dst;
	
	if(s->gr[0xb] & 0x04){
	}else{
		mask = 0xffff;
	}
	if(s->gr[0xb] & 0x10){
		// SR2 Doubling Enabled
		mask |= mask << 8;
	}

    dst = s->vram_ptr + (offset &= s->cirrus_addr_mask);
    for (x = 0; x < 8; x++) {
		if (mask & 0x80) {
			if (val & 0x80) {
				*dst = s->cirrus_shadow_gr1;
				*(dst + 1) = s->gr[0x11];
			} else if (mode == 5) {
				*dst = s->cirrus_shadow_gr0;
				*(dst + 1) = s->gr[0x10];
			}
		}
		val <<= 1;
		mask <<= 1;
		dst += 2;
    }
    cpu_physical_memory_set_dirty(s->vram_offset + offset);
    cpu_physical_memory_set_dirty(s->vram_offset + offset + 15);
}

/***************************************
 *
 *  memory access between 0xa0000-0xbffff
 *
 ***************************************/

#define ADDR_SH1	0x000

uint32_t_ cirrus_vga_mem_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;
    unsigned bank_index;
    unsigned bank_offset;
    uint32_t_ val;
	

	addr += ADDR_SH1;

    if ((s->sr[0x07] & 0x01) == 0) {
	return vga_mem_readb(s, addr);
    }
    addr &= 0x1ffff;

    if (addr < 0x10000) {
		/* XXX handle bitblt */
		/* video memory */
		bank_index = addr >> 15;
		bank_offset = addr & 0x7fff;
		if (bank_offset < s->cirrus_bank_limit[bank_index]) {
			bank_offset += s->cirrus_bank_base[bank_index];
			if ((s->gr[0x0B] & 0x14) == 0x14) {
				bank_offset <<= 4;
			} else if (s->gr[0x0B] & 0x02) {
				bank_offset <<= 3;
			}
			bank_offset &= s->cirrus_addr_mask;
			val = *(s->vram_ptr + bank_offset);
		} else
			val = 0xff;
	} else if (addr >= 0x18000 && addr < 0x18100) {
		/* memory-mapped I/O */
		val = 0xff;
		if ((s->sr[0x17] & 0x44) == 0x04) {
			val = cirrus_mmio_blt_read(s, addr & 0xff);
		}
	} else {
		val = 0xff;
#ifdef DEBUG_CIRRUS
		printf("cirrus: mem_readb %06x\n", addr);
#endif
    }
    return val;
}

uint32_t_ cirrus_vga_mem_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_vga_mem_readb(opaque, addr) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 1);
#else
    v = cirrus_vga_mem_readb(opaque, addr);
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_vga_mem_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_vga_mem_readb(opaque, addr) << 24;
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 16;
    v |= cirrus_vga_mem_readb(opaque, addr + 2) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 3);
#else
    v = cirrus_vga_mem_readb(opaque, addr);
    v |= cirrus_vga_mem_readb(opaque, addr + 1) << 8;
    v |= cirrus_vga_mem_readb(opaque, addr + 2) << 16;
    v |= cirrus_vga_mem_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_vga_mem_writeb(void *opaque, target_phys_addr_t addr,
                                  uint32_t_ mem_value)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;
    unsigned bank_index;
    unsigned bank_offset;
    unsigned mode;
	
	addr += ADDR_SH1;

    if ((s->sr[0x07] & 0x01) == 0) {
	vga_mem_writeb(s, addr, mem_value);
        return;
    }
	
    addr &= 0x1ffff;

    if (addr < 0x10000) {
		if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
			/* bitblt */
			*s->cirrus_srcptr++ = (uint8_t) mem_value;
			*s->cirrus_srcptr++;
			if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
				cirrus_bitblt_cputovideo_next(s);
			}
		} else {
			/* video memory */
			bank_index = addr >> 15;
			bank_offset = addr & 0x7fff;
			if (bank_offset < s->cirrus_bank_limit[bank_index]) {
				bank_offset += s->cirrus_bank_base[bank_index];
				if ((s->gr[0x0B] & 0x14) == 0x14) {
					bank_offset <<= 4;
				} else if (s->gr[0x0B] & 0x02) {
					bank_offset <<= 3;
				}
				bank_offset &= s->cirrus_addr_mask;
				mode = s->gr[0x05] & 0x7;
				if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
					*(s->vram_ptr + bank_offset) = mem_value;
					cpu_physical_memory_set_dirty(s->vram_offset +
								  bank_offset);
				} else {
					if ((s->gr[0x0B] & 0x14) != 0x14) {
						cirrus_mem_writeb_mode4and5_8bpp(s, mode,
									 bank_offset,
									 mem_value);
					} else {
						cirrus_mem_writeb_mode4and5_16bpp(s, mode,
									  bank_offset,
									  mem_value);
					}
				}
			}
		}
    } else if (addr >= 0x18000 && addr < 0x18100) {
		/* memory-mapped I/O */
		if ((s->sr[0x17] & 0x44) == 0x04) {
			cirrus_mmio_blt_write(s, addr & 0xff, mem_value);
		}
    } else {
#ifdef DEBUG_CIRRUS
	printf("cirrus: mem_writeb %06x value %02x\n", addr, mem_value);
#endif
    }
}

void cirrus_vga_mem_writew(void *opaque, target_phys_addr_t addr, uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_vga_mem_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_vga_mem_writeb(opaque, addr, val & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_vga_mem_writel(void *opaque, target_phys_addr_t addr, uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_vga_mem_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_vga_mem_writeb(opaque, addr, val & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_vga_mem_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}

static CPUReadMemoryFunc *cirrus_vga_mem_read[3] = {
    cirrus_vga_mem_readb,
    cirrus_vga_mem_readw,
    cirrus_vga_mem_readl,
};

static CPUWriteMemoryFunc *cirrus_vga_mem_write[3] = {
    cirrus_vga_mem_writeb,
    cirrus_vga_mem_writew,
    cirrus_vga_mem_writel,
};

/***************************************
 *
 *  hardware cursor
 *
 ***************************************/

static void invalidate_cursor1(CirrusVGAState *s)
{
    if (s->last_hw_cursor_size) {
        vga_invalidate_scanlines((VGAState *)s,
                                 s->last_hw_cursor_y + s->last_hw_cursor_y_start,
                                 s->last_hw_cursor_y + s->last_hw_cursor_y_end);
    }
}

static void cirrus_cursor_compute_yrange(CirrusVGAState *s)
{
    const uint8_t *src;
    uint32_t_ content;
    int y, y_min, y_max;

    src = s->vram_ptr + s->real_vram_size - 16 * 1024;
    if (s->sr[0x12] & CIRRUS_CURSOR_LARGE) {
        src += (s->sr[0x13] & 0x3c) * 256;
        y_min = 64;
        y_max = -1;
        for(y = 0; y < 64; y++) {
            content = ((uint32_t_ *)src)[0] |
                ((uint32_t_ *)src)[1] |
                ((uint32_t_ *)src)[2] |
                ((uint32_t_ *)src)[3];
            if (content) {
                if (y < y_min)
                    y_min = y;
                if (y > y_max)
                    y_max = y;
            }
            src += 16;
        }
    } else {
        src += (s->sr[0x13] & 0x3f) * 256;
        y_min = 32;
        y_max = -1;
        for(y = 0; y < 32; y++) {
            content = ((uint32_t_ *)src)[0] |
                ((uint32_t_ *)(src + 128))[0];
            if (content) {
                if (y < y_min)
                    y_min = y;
                if (y > y_max)
                    y_max = y;
            }
            src += 4;
        }
    }
    if (y_min > y_max) {
        s->last_hw_cursor_y_start = 0;
        s->last_hw_cursor_y_end = 0;
    } else {
        s->last_hw_cursor_y_start = y_min;
        s->last_hw_cursor_y_end = y_max + 1;
    }
}

/* NOTE: we do not currently handle the cursor bitmap change, so we
   update the cursor only if it moves. */
static void cirrus_cursor_invalidate(VGAState *s1)
{
    CirrusVGAState *s = (CirrusVGAState *)s1;
    int size;

    if (!s->sr[0x12] & CIRRUS_CURSOR_SHOW) {
        size = 0;
    } else {
        if (s->sr[0x12] & CIRRUS_CURSOR_LARGE)
            size = 64;
        else
            size = 32;
    }
    /* invalidate last cursor and new cursor if any change */
    if (s->last_hw_cursor_size != size ||
        s->last_hw_cursor_x != s->hw_cursor_x ||
        s->last_hw_cursor_y != s->hw_cursor_y) {

        invalidate_cursor1(s);

        s->last_hw_cursor_size = size;
        s->last_hw_cursor_x = s->hw_cursor_x;
        s->last_hw_cursor_y = s->hw_cursor_y;
        /* compute the real cursor min and max y */
        cirrus_cursor_compute_yrange(s);
        invalidate_cursor1(s);
    }
}

static int get_depth_index(DisplayState *s)
{
    switch(ds_get_bits_per_pixel(s)) {
    default:
    case 8:
		return 0;
    case 15:
        return 1;
    case 16:
        return 2;
    case 32:
        return 3;
    }
}
static void cirrus_cursor_draw_line(VGAState *s1, uint8_t *d1, int scr_y)
{
}

/***************************************
 *
 *  LFB memory access
 *
 ***************************************/

uint32_t_ cirrus_linear_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    uint32_t_ ret;

    addr &= s->cirrus_addr_mask;

    if (((s->sr[0x17] & 0x44) == 0x44) &&
        ((addr & s->linear_mmio_mask) == s->linear_mmio_mask)) {
		/* memory-mapped I/O */
		ret = cirrus_mmio_blt_read(s, addr & 0xff);
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* XXX handle bitblt */
		//ret = 0xff;
		ret = *s->cirrus_srcptr;
		*s->cirrus_srcptr++;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_videotocpu_next(s);
		}
    } else {
		/* video memory */
		if ((s->gr[0x0B] & 0x14) == 0x14) {
			addr <<= 4;
		} else if (s->gr[0x0B] & 0x02) {
			addr <<= 3;
		}
		addr &= s->cirrus_addr_mask;
		ret = *(s->vram_ptr + addr);
    }

    return ret;
}

uint32_t_ cirrus_linear_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_readb(opaque, addr) << 8;
    v |= cirrus_linear_readb(opaque, addr + 1);
#else
    v = cirrus_linear_readb(opaque, addr);
    v |= cirrus_linear_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_linear_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_readb(opaque, addr) << 24;
    v |= cirrus_linear_readb(opaque, addr + 1) << 16;
    v |= cirrus_linear_readb(opaque, addr + 2) << 8;
    v |= cirrus_linear_readb(opaque, addr + 3);
#else
    v = cirrus_linear_readb(opaque, addr);
    v |= cirrus_linear_readb(opaque, addr + 1) << 8;
    v |= cirrus_linear_readb(opaque, addr + 2) << 16;
    v |= cirrus_linear_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_linear_writeb(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    unsigned mode;

    addr &= s->cirrus_addr_mask;

    if (((s->sr[0x17] & 0x44) == 0x44) &&
        ((addr & s->linear_mmio_mask) ==  s->linear_mmio_mask)) {
		/* memory-mapped I/O */
		cirrus_mmio_blt_write(s, addr & 0xff, val);
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* bitblt */
		*s->cirrus_srcptr++ = (uint8_t) val;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_cputovideo_next(s);
		}
    } else {
		/* video memory */
		if ((s->gr[0x0B] & 0x14) == 0x14) {
			addr <<= 4;
		} else if (s->gr[0x0B] & 0x02) {
			addr <<= 3;
		}
		addr &= s->cirrus_addr_mask;

		mode = s->gr[0x05] & 0x7;
		if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
			*(s->vram_ptr + addr) = (uint8_t) val;
			cpu_physical_memory_set_dirty(s->vram_offset + addr);
		} else {
			if ((s->gr[0x0B] & 0x14) != 0x14) {
				cirrus_mem_writeb_mode4and5_8bpp(s, mode, addr, val);
			} else {
				cirrus_mem_writeb_mode4and5_16bpp(s, mode, addr, val);
			}
		}
    }
}

void cirrus_linear_writew(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_linear_writeb(opaque, addr, val & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_linear_writel(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_linear_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_linear_writeb(opaque, addr, val & 0xff);
    cirrus_linear_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_linear_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_linear_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


static CPUReadMemoryFunc *cirrus_linear_read[3] = {
    cirrus_linear_readb,
    cirrus_linear_readw,
    cirrus_linear_readl,
};

static CPUWriteMemoryFunc *cirrus_linear_write[3] = {
    cirrus_linear_writeb,
    cirrus_linear_writew,
    cirrus_linear_writel,
};

static void cirrus_linear_mem_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    *(s->vram_ptr + addr) = val;
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

static void cirrus_linear_mem_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    cpu_to_le16w((uint16_t_ *)(s->vram_ptr + addr), val);
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

static void cirrus_linear_mem_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= s->cirrus_addr_mask;
    cpu_to_le32w((uint32_t_ *)(s->vram_ptr + addr), val);
    cpu_physical_memory_set_dirty(s->vram_offset + addr);
}

/***************************************
 *
 *  E-BANK memory access
 *
 ***************************************/

// convert E-BANK VRAM window address to linear address 
void cirrus_linear_memwnd_addr_convert(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	
	if(np2clvga.gd54xxtype <= 0xff){
		//addr &= s->cirrus_addr_mask;
		addr -= np2clvga.VRAMWindowAddr2;
		//addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
		addr &= s->cirrus_addr_mask;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
		addr &= s->cirrus_addr_mask;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
		addr &= s->cirrus_addr_mask;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x4000){
				offset = s->gr[0x09];
			}else{
				addr -= 0x4000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;

		addr += (offset);
	}else{
		addr &= 0x7fff;
		if ((s->gr[0x0b] & 0x01) != 0)	/* dual bank */
			offset = s->gr[0x09/* + bank_index*/];
		else			/* single bank */
			offset = s->gr[0x09];

		if ((s->gr[0x0b] & 0x20) != 0)
			addr += (offset) << 14L;
		else
			addr += (offset) << 12L;
		addr &= s->cirrus_addr_mask;
	}
	*addrval = addr;
}

void cirrus_linear_memwnd_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	cirrus_linear_memwnd_addr_convert(opaque, &addr);

	g_cirrus_linear_write[0](opaque, addr, val);
	//cirrus_linear_mem_writeb(opaque, addr, val);
	//cirrus_linear_writeb(opaque, addr, val);
}

void cirrus_linear_memwnd_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd_addr_convert(opaque, &addr);
	
	g_cirrus_linear_write[1](opaque, addr, val);
	//cirrus_linear_mem_writew(opaque, addr, val);
	//cirrus_linear_writew(opaque, addr, val);
}

void cirrus_linear_memwnd_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd_addr_convert(opaque, &addr);
	
	g_cirrus_linear_write[2](opaque, addr, val);
	//cirrus_linear_mem_writel(opaque, addr, val);
	//cirrus_linear_writel(opaque, addr, val);
}

uint32_t_ cirrus_linear_memwnd_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd_addr_convert(opaque, &addr);

	return cirrus_linear_readb(opaque, addr);
}

uint32_t_ cirrus_linear_memwnd_readw(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd_addr_convert(opaque, &addr);

	return cirrus_linear_readw(opaque, addr);
}

uint32_t_ cirrus_linear_memwnd_readl(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd_addr_convert(opaque, &addr);

	return cirrus_linear_readl(opaque, addr);
}

/***************************************
 *
 *  XXX: F00000 memory access ?
 *
 ***************************************/

// XXX: convert F00000 VRAM window address to linear address（どうしたら良いか分からん）
void cirrus_linear_memwnd3_addr_convert(void *opaque, target_phys_addr_t *addrval){
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	int offset;
	target_phys_addr_t addr = *addrval;
	
	addr -= np2clvga.VRAMWindowAddr3;
	if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		offset = s->gr[0x09];
		offset *= 0x20000;
	}else{
		if ((s->gr[0x0b] & 0x01) != 0){
			/* dual bank */
			if(addr < 0x8000){
				offset = s->gr[0x09];
			//}else if(addr < 0x8000){
			//	addr -= 0x4000;
			//	offset = s->gr[0x0a];
			}else{
				addr -= 0x8000;
				offset = s->gr[0x0a];
			}
		}else{
			/* single bank ? */
			offset = s->gr[0x09];
		}
		if ((s->gr[0x0b] & 0x20) != 0)
			offset <<= 14;
		else
			offset <<= 12;
	}

	addr += (offset);

	addr &= s->cirrus_addr_mask;
	*addrval = addr;
}

void cirrus_linear_memwnd3_writeb(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

	cirrus_linear_memwnd3_addr_convert(opaque, &addr);

	//cirrus_linear_bitblt_writeb(opaque, addr, val);
	g_cirrus_linear_write[0](opaque, addr, val);
	//cirrus_linear_mem_writeb(opaque, addr, val);
	//cirrus_linear_writeb(opaque, addr, val);
}

void cirrus_linear_memwnd3_writew(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd3_addr_convert(opaque, &addr);
	
	//cirrus_linear_bitblt_writew(opaque, addr, val);
	g_cirrus_linear_write[1](opaque, addr, val);
	//cirrus_linear_mem_writew(opaque, addr, val);
	//cirrus_linear_writew(opaque, addr, val);
}

void cirrus_linear_memwnd3_writel(void *opaque, target_phys_addr_t addr,
                                     uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd3_addr_convert(opaque, &addr);
	
	//cirrus_linear_bitblt_writel(opaque, addr, val);
	g_cirrus_linear_write[2](opaque, addr, val);
	//cirrus_linear_mem_writel(opaque, addr, val);
	//cirrus_linear_writel(opaque, addr, val);
}

uint32_t_ cirrus_linear_memwnd3_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd3_addr_convert(opaque, &addr);

	return cirrus_linear_readb(opaque, addr);
}

uint32_t_ cirrus_linear_memwnd3_readw(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd3_addr_convert(opaque, &addr);

	return cirrus_linear_readw(opaque, addr);
}

uint32_t_ cirrus_linear_memwnd3_readl(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
	
	cirrus_linear_memwnd3_addr_convert(opaque, &addr);

	return cirrus_linear_readl(opaque, addr);
}


/***************************************
 *
 *  system to screen memory access
 *
 ***************************************/


uint32_t_ cirrus_linear_bitblt_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;
    uint32_t_ ret;

    /* handle bitblt */
    if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		ret = *s->cirrus_srcptr;
		*s->cirrus_srcptr++;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_videotocpu_next(s);
		}
	}
    return ret;
}

uint32_t_ cirrus_linear_bitblt_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_bitblt_readb(opaque, addr) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1);
#else
    v = cirrus_linear_bitblt_readb(opaque, addr);
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_linear_bitblt_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_linear_bitblt_readb(opaque, addr) << 24;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 16;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 2) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 3);
#else
    v = cirrus_linear_bitblt_readb(opaque, addr);
    v |= cirrus_linear_bitblt_readb(opaque, addr + 1) << 8;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 2) << 16;
    v |= cirrus_linear_bitblt_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_linear_bitblt_writeb(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
		/* bitblt */
		*s->cirrus_srcptr++ = (uint8_t) val;
		if (s->cirrus_srcptr >= s->cirrus_srcptr_end) {
			cirrus_bitblt_cputovideo_next(s);
		}
    }
}

void cirrus_linear_bitblt_writew(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_bitblt_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_linear_bitblt_writeb(opaque, addr, val & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_linear_bitblt_writel(void *opaque, target_phys_addr_t addr,
				 uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_linear_bitblt_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_linear_bitblt_writeb(opaque, addr, val & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_linear_bitblt_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


static CPUReadMemoryFunc *cirrus_linear_bitblt_read[3] = {
    cirrus_linear_bitblt_readb,
    cirrus_linear_bitblt_readw,
    cirrus_linear_bitblt_readl,
};

static CPUWriteMemoryFunc *cirrus_linear_bitblt_write[3] = {
    cirrus_linear_bitblt_writeb,
    cirrus_linear_bitblt_writew,
    cirrus_linear_bitblt_writel,
};

static void map_linear_vram(CirrusVGAState *s)
{
	g_cirrus_linear_map_enabled = 1;

    vga_dirty_log_stop((VGAState *)s);

    if (!s->map_addr && s->lfb_addr && s->lfb_end) {
        s->map_addr = s->lfb_addr;
        s->map_end = s->lfb_end;
        cpu_register_physical_memory(s->map_addr, s->map_end - s->map_addr, s->vram_offset);
    }

    if (!s->map_addr)
        return;

    s->lfb_vram_mapped = 0;
	
	//　このcpu_register_physical_memoryの詳細がさっぱり分からない･･･
    cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x8000,
                                (s->vram_offset + s->cirrus_bank_base[0]) | IO_MEM_UNASSIGNED);
    cpu_register_physical_memory(isa_mem_base + 0xF88000, 0x8000,
                                (s->vram_offset + s->cirrus_bank_base[1]) | IO_MEM_UNASSIGNED);
    if (!(s->cirrus_srcptr != s->cirrus_srcptr_end)
        && !((s->sr[0x07] & 0x01) == 0)
        && !((s->gr[0x0B] & 0x14) == 0x14)
        && !(s->gr[0x0B] & 0x02)) {

        vga_dirty_log_stop((VGAState *)s);
		//　ここも
        cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x8000,
                                    (s->vram_offset + s->cirrus_bank_base[0]) | IO_MEM_RAM);
        cpu_register_physical_memory(isa_mem_base + 0xF88000, 0x8000,
                                    (s->vram_offset + s->cirrus_bank_base[1]) | IO_MEM_RAM);

        s->lfb_vram_mapped = 1;
    }
    else {
        cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x20000,
                                     s->vga_io_memory);
    }

    vga_dirty_log_start((VGAState *)s);
}

static void unmap_linear_vram(CirrusVGAState *s)
{
	g_cirrus_linear_map_enabled = 0;

    vga_dirty_log_stop((VGAState *)s);

    if (s->map_addr && s->lfb_addr && s->lfb_end)
        s->map_addr = s->map_end = 0;
	
	//　ここも
    cpu_register_physical_memory(isa_mem_base + 0xF80000, 0x20000,
                                 s->vga_io_memory);

    vga_dirty_log_start((VGAState *)s);
}

/* Compute the memory access functions */
static void cirrus_update_memory_access(CirrusVGAState *s)
{
    unsigned mode;

    if ((s->sr[0x17] & 0x44) == 0x44) {
        goto generic_io;
    } else if (s->cirrus_srcptr != s->cirrus_srcptr_end) {
        goto generic_io;
    } else {
	if ((s->gr[0x0B] & 0x14) == 0x14) {
            goto generic_io;
	} else if (s->gr[0x0B] & 0x02) {
            goto generic_io;
        }

	mode = s->gr[0x05] & 0x7;
	if (mode < 4 || mode > 5 || ((s->gr[0x0B] & 0x4) == 0)) {
            map_linear_vram(s);
            g_cirrus_linear_write[0] = cirrus_linear_mem_writeb;
            g_cirrus_linear_write[1] = cirrus_linear_mem_writew;
            g_cirrus_linear_write[2] = cirrus_linear_mem_writel;
        } else {
        generic_io:
            unmap_linear_vram(s);
            g_cirrus_linear_write[0] = cirrus_linear_writeb;
            g_cirrus_linear_write[1] = cirrus_linear_writew;
            g_cirrus_linear_write[2] = cirrus_linear_writel;
        }
    }
}


/* I/O ports */
uint32_t_ vga_convert_ioport(uint32_t_ addr){
	if(np2clvga.gd54xxtype <= 0xff){
		if((addr & 0xFF0) == 0xCA0 || (addr & 0xFF0) == 0xC50){
			addr = 0x3C0 | (addr & 0xf);
			//if(addr==0xCA0) addr = 0x3C0;
			//if(addr==0xCA1) addr = 0x3C1;
			//if(addr==0xCA2) addr = 0x3C2;
			//if(addr==0xCA3) addr = 0x3C3;
			//if(addr==0xCA4) addr = 0x3C4;
			//if(addr==0xCA5) addr = 0x3C5;
			//if(addr==0xCA6) addr = 0x3C6;
			//if(addr==0xCA7) addr = 0x3C7;
			//if(addr==0xCA8) addr = 0x3C8;
			//if(addr==0xCA9) addr = 0x3C9;
			//if(addr==0xCAA) addr = 0x3CA;
			//if(addr==0xCAB) addr = 0x3CB;
			//if(addr==0xCAC) addr = 0x3CC;
			//if(addr==0xCAD) addr = 0x3CD;
			//if(addr==0xCAE) addr = 0x3CE;
			//if(addr==0xCAF) addr = 0x3CF;
		}else{
			//if(addr==0x904 || addr==0x904) addr = 0x094;
			if(addr==0xBA4 || addr==0xB54) addr = 0x3B4;
			if(addr==0xBA5 || addr==0xB55) addr = 0x3B5;
			if(addr==0xDA4 || addr==0xD54) addr = 0x3D4;
			if(addr==0xDA5 || addr==0xD55) addr = 0x3D5;
			if(addr==0xBAA || addr==0xB5A) addr = 0x3BA;
			if(addr==0xDAA || addr==0xD5A) addr = 0x3DA;
		}
	}else{
		if((addr & 0xF0FF) == (0x40E0 | CIRRUS_MELCOWAB_OFS)){
			addr = 0x3C0 | ((addr >> 8) & 0xf);
			//if(addr==0x40E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C0;
			//if(addr==0x41E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C1;
			//if(addr==0x42E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C2;
			//if(addr==0x43E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C3;
			//if(addr==0x44E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C4;
			//if(addr==0x45E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C5;
			//if(addr==0x46E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C6;
			//if(addr==0x47E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C7;
			//if(addr==0x48E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C8;
			//if(addr==0x49E0+CIRRUS_MELCOWAB_OFS) addr = 0x3C9;
			//if(addr==0x4AE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CA;
			//if(addr==0x4BE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CB;
			//if(addr==0x4CE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CC;
			//if(addr==0x4DE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CD;
			//if(addr==0x4EE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CE;
			//if(addr==0x4FE0+CIRRUS_MELCOWAB_OFS) addr = 0x3CF;
		}else{
			if(addr==0x51E1+CIRRUS_MELCOWAB_OFS) addr = 0x3B4; // ???
			if(addr==0x57E1+CIRRUS_MELCOWAB_OFS) addr = 0x3B5; // ???
			if(addr==0x54E0+CIRRUS_MELCOWAB_OFS) addr = 0x3D4;
			if(addr==0x55E0+CIRRUS_MELCOWAB_OFS) addr = 0x3D5;
			if(addr==0x5BE1+CIRRUS_MELCOWAB_OFS) addr = 0x3BA; // ???
			if(addr==0x5AE0+CIRRUS_MELCOWAB_OFS) addr = 0x3DA;
		}
	}
	return addr;
}

static uint32_t_ vga_ioport_read(void *opaque, uint32_t_ addr)
{
    CirrusVGAState *s = (CirrusVGAState *)opaque;
    int val, index;
	
	//　ポート決め打ちなので無理矢理変換
	addr = vga_convert_ioport(addr);
	
	TRACEOUT(("CIRRUS VGA: read %04X", addr));

    /* check port range access depending on color/monochrome mode */
    if ((addr >= 0x3b0 && addr <= 0x3bf && (s->msr & MSR_COLOR_EMULATION))
	|| (addr >= 0x3d0 && addr <= 0x3df
	    && !(s->msr & MSR_COLOR_EMULATION))) {
		val = 0xff;
    } else {
		switch (addr) {
		case 0x3c0:
			if (s->ar_flip_flop == 0) {
			val = s->ar_index;
			} else {
			val = 0;
			}
			break;
		case 0x3c1:
			index = s->ar_index & 0x1f;
			if (index < 21)
			val = s->ar[index];
			else
			val = 0;
			break;
		case 0x3c2:
			val = s->st00;
			break;
		case 0x3c4:
			val = s->sr_index;
			break;
		case 0x3c5:
			if (cirrus_hook_read_sr(s, s->sr_index, &val))
				break;
			val = s->sr[s->sr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read SR%x = 0x%02x\n", s->sr_index, val);
#endif
			break;
		case 0x3c6:
			cirrus_read_hidden_dac(s, &val);
			break;
		case 0x3c7:
			val = s->dac_state;
			break;
		case 0x3c8:
			val = s->dac_write_index;
			s->cirrus_hidden_dac_lockindex = 0;
			break;
		case 0x3c9:
			if (cirrus_hook_read_palette(s, &val))
				break;
			val = s->palette[s->dac_read_index * 3 + s->dac_sub_index];
			if (++s->dac_sub_index == 3) {
				s->dac_sub_index = 0;
				s->dac_read_index++;
			}
			break;
		case 0x3ca:
			val = s->fcr;
			break;
		case 0x3cc:
			val = s->msr;
			break;
		case 0x3ce:
			val = s->gr_index;
			break;
		case 0x3cf:
			if (cirrus_hook_read_gr(s, s->gr_index, &val))
				break;
			val = s->gr[s->gr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read GR%x = 0x%02x\n", s->gr_index, val);
#endif
			break;
		case 0x3b4:
		case 0x3d4:
			val = s->cr_index;
			break;
		case 0x3b5:
		case 0x3d5:
			if (cirrus_hook_read_cr(s, s->cr_index, &val))
			break;
			val = s->cr[s->cr_index];
#ifdef DEBUG_VGA_REG
			printf("vga: read CR%x = 0x%02x\n", s->cr_index, val);
#endif
			break;
		case 0x3ba:
		case 0x3da:
			/* just toggle to fool polling */
			val = s->st01 = s->retrace((VGAState *) s);
			//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
			//	val = 0xC2;
			//}
			//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
			//	val = 0xc7;
			//}
			s->ar_flip_flop = 0;
			break;
		default:
			val = 0x00;
			break;
		}
    }
#if defined(TRACE)
    //TRACEOUT(("VGA: read addr=0x%04x data=0x%02x\n", addr, val));
#endif
    return val;
}
static REG8 __fastcall vga_ioport_read_wrap(UINT addr)
{
	return vga_ioport_read(cirrusvga, addr);
}
UINT16 __fastcall cirrusvga_ioport_read_wrap16(UINT addr)
{
	UINT16 ret;
	addr = vga_convert_ioport(addr);
	ret  = ((REG16)vga_ioport_read(cirrusvga, addr  )     );
	ret |= ((REG16)vga_ioport_read(cirrusvga, addr+1) << 8);
	return ret;
}
UINT32 __fastcall cirrusvga_ioport_read_wrap32(UINT addr)
{
	UINT32 ret;
	addr = vga_convert_ioport(addr);
	ret  = ((UINT32)vga_ioport_read(cirrusvga, addr  )      );
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+1) <<  8);
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+2) << 16);
	ret |= ((UINT32)vga_ioport_read(cirrusvga, addr+3) << 24);
	return ret;
}

static void vga_ioport_write(void *opaque, uint32_t_ addr, uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *)opaque;
    int index;
	
	//　ポート決め打ちなので無理矢理変換
	addr = vga_convert_ioport(addr);
	
	TRACEOUT(("CIRRUS VGA: write %04X %02X", addr, val));

    /* check port range access depending on color/monochrome mode */
    if ((addr >= 0x3b0 && addr <= 0x3bf && (s->msr & MSR_COLOR_EMULATION))
		|| (addr >= 0x3d0 && addr <= 0x3df
			&& !(s->msr & MSR_COLOR_EMULATION)))
		return;

#ifdef TRACE
    //TRACEOUT(("VGA: write addr=0x%04x data=0x%02x\n", addr, val));
#endif

    switch (addr) {
    case 0x3c0:
		if (s->ar_flip_flop == 0) {
			val &= 0x3f;
			s->ar_index = val;
		} else {
			index = s->ar_index & 0x1f;
			switch (index) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
			case 0x0e:
			case 0x0f:
				s->ar[index] = val & 0x3f;
				break;
			case 0x10:
				s->ar[index] = val & ~0x10;
				break;
			case 0x11:
				s->ar[index] = val;
				break;
			case 0x12:
				s->ar[index] = val & ~0xc0;
				break;
			case 0x13:
				s->ar[index] = val & ~0xf0;
				break;
			case 0x14:
				s->ar[index] = val & ~0xf0;
				break;
			default:
				break;
			}
		}
		s->ar_flip_flop ^= 1;
		break;
    case 0x3c2:
		s->msr = val & ~0x10;
		s->update_retrace_info((VGAState *) s);
		break;
    case 0x3c4:
		s->sr_index = val;
		break;
    case 0x3c5:
		if (cirrus_hook_write_sr(s, s->sr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write SR%x = 0x%02x\n", s->sr_index, val);
#endif
		s->sr[s->sr_index] = val & sr_mask[s->sr_index];
		if (s->sr_index == 1) s->update_retrace_info((VGAState *) s);
			break;
    case 0x3c6:
		cirrus_write_hidden_dac(s, val);
		break;
    case 0x3c7:
		s->dac_read_index = val;
		s->dac_sub_index = 0;
		s->dac_state = 3;
		break;
    case 0x3c8:
		s->dac_write_index = val;
		s->dac_sub_index = 0;
		s->dac_state = 0;
		break;
    case 0x3c9:
		if (cirrus_hook_write_palette(s, val))
			break;
		s->dac_cache[s->dac_sub_index] = val;
		if (++s->dac_sub_index == 3) {
			memcpy(&s->palette[s->dac_write_index * 3], s->dac_cache, 3);
			s->dac_sub_index = 0;
			s->dac_write_index++;
			np2wab.paletteChanged = 1; // パレット変えました
		}
		break;
    case 0x3ce:
		s->gr_index = val;
		break;
    case 0x3cf:
		if (cirrus_hook_write_gr(s, s->gr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write GR%x = 0x%02x\n", s->gr_index, val);
#endif
		s->gr[s->gr_index] = val & gr_mask[s->gr_index];
		break;
    case 0x3b4:
    case 0x3d4:
		s->cr_index = val;
		break;
    case 0x3b5:
    case 0x3d5:
		if (cirrus_hook_write_cr(s, s->cr_index, val))
			break;
#ifdef DEBUG_VGA_REG
		printf("vga: write CR%x = 0x%02x\n", s->cr_index, val);
#endif
		/* handle CR0-7 protection */
		if ((s->cr[0x11] & 0x80) && s->cr_index <= 7) {
			/* can always write bit 4 of CR7 */
			if (s->cr_index == 7)
			s->cr[7] = (s->cr[7] & ~0x10) | (val & 0x10);
			return;
		}
		switch (s->cr_index) {
		case 0x01:		/* horizontal display end */
		case 0x07:
		case 0x09:
		case 0x0c:
		case 0x0d:
		case 0x12:		/* vertical display end */
			s->cr[s->cr_index] = val;
			break;

		default:
			s->cr[s->cr_index] = val;
			break;
		}

		switch(s->cr_index) {
		case 0x00:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x11:
		case 0x17:
			s->update_retrace_info((VGAState *) s);
			break;
		}
		//if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		//	int width, height;
		//	cirrus_get_resolution((VGAState *) s, &width, &height);
		//	switch(s->cr_index) {
		//	case 0x04:
		//		if(width==640) 
		//			np2wab.shiftX = val - 0x54;
		//		else if(width==800) 
		//			np2wab.shiftX = val - 0x69;
		//		else if(width==1024) 
		//			np2wab.shiftX = val - 0x85;
		//		else 
		//			np2wab.shiftX = 0;
		//		break;
		//	case 0x10:
		//		if(width==640) 
		//			np2wab.shiftX = val - 0xEA;
		//		else if(width==800) 
		//			np2wab.shiftX = val - 0x5D;
		//		else if(width==1024) 
		//			np2wab.shiftX = val - 0x68;
		//		else 
		//			np2wab.shiftX = 0;
		//		break;
		//	}
		//}
		break;
    case 0x3ba:
    case 0x3da:
		s->fcr = val & 0x10;
		break;
    }
}
static void __fastcall vga_ioport_write_wrap(UINT addr, REG8 dat)
{
	vga_ioport_write(cirrusvga, addr, dat);
}
void __fastcall cirrusvga_ioport_write_wrap16(UINT addr, UINT16 dat)
{
	addr = vga_convert_ioport(addr);
	vga_ioport_write(cirrusvga, addr  , ((UINT32)dat     ) & 0xff);
	vga_ioport_write(cirrusvga, addr+1, ((UINT32)dat >> 8) & 0xff);
}
void __fastcall cirrusvga_ioport_write_wrap32(UINT addr, UINT32 dat)
{
	addr = vga_convert_ioport(addr);
	vga_ioport_write(cirrusvga, addr  , (dat      ) & 0xff);
	vga_ioport_write(cirrusvga, addr+1, (dat >>  8) & 0xff);
	vga_ioport_write(cirrusvga, addr+2, (dat >> 16) & 0xff);
	vga_ioport_write(cirrusvga, addr+3, (dat >> 24) & 0xff);
}

/***************************************
 *
 *  memory-mapped I/O access
 *
 ***************************************/

uint32_t_ cirrus_mmio_readb(void *opaque, target_phys_addr_t addr)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= CIRRUS_PNPMMIO_SIZE - 1;

    if (addr >= 0x100) {
        return cirrus_mmio_blt_read(s, addr - 0x100);
    } else {
        return vga_ioport_read(s, addr + 0x3c0);
    }
}

uint32_t_ cirrus_mmio_readw(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb(opaque, addr) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 1);
#else
    v = cirrus_mmio_readb(opaque, addr);
    v |= cirrus_mmio_readb(opaque, addr + 1) << 8;
#endif
    return v;
}

uint32_t_ cirrus_mmio_readl(void *opaque, target_phys_addr_t addr)
{
    uint32_t_ v;
#ifdef TARGET_WORDS_BIGENDIAN
    v = cirrus_mmio_readb(opaque, addr) << 24;
    v |= cirrus_mmio_readb(opaque, addr + 1) << 16;
    v |= cirrus_mmio_readb(opaque, addr + 2) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 3);
#else
    v = cirrus_mmio_readb(opaque, addr);
    v |= cirrus_mmio_readb(opaque, addr + 1) << 8;
    v |= cirrus_mmio_readb(opaque, addr + 2) << 16;
    v |= cirrus_mmio_readb(opaque, addr + 3) << 24;
#endif
    return v;
}

void cirrus_mmio_writeb(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
    CirrusVGAState *s = (CirrusVGAState *) opaque;

    addr &= CIRRUS_PNPMMIO_SIZE - 1;

    if (addr >= 0x100) {
		cirrus_mmio_blt_write(s, addr - 0x100, val);
    } else {
        vga_ioport_write(s, addr + 0x3c0, val);
    }
}

void cirrus_mmio_writew(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb(opaque, addr, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, val & 0xff);
#else
    cirrus_mmio_writeb(opaque, addr, val & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 8) & 0xff);
#endif
}

void cirrus_mmio_writel(void *opaque, target_phys_addr_t addr,
			       uint32_t_ val)
{
#ifdef TARGET_WORDS_BIGENDIAN
    cirrus_mmio_writeb(opaque, addr, (val >> 24) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 16) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 2, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 3, val & 0xff);
#else
    cirrus_mmio_writeb(opaque, addr, val & 0xff);
    cirrus_mmio_writeb(opaque, addr + 1, (val >> 8) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 2, (val >> 16) & 0xff);
    cirrus_mmio_writeb(opaque, addr + 3, (val >> 24) & 0xff);
#endif
}


CPUReadMemoryFunc *cirrus_mmio_read[3] = {
    cirrus_mmio_readb,
    cirrus_mmio_readw,
    cirrus_mmio_readl,
};

CPUWriteMemoryFunc *cirrus_mmio_write[3] = {
    cirrus_mmio_writeb,
    cirrus_mmio_writew,
    cirrus_mmio_writel,
};

#define array_write(ary, pos, data, len) \
	memcpy(ary+pos, data, len); \
	pos += len;  

#define array_read(ary, pos, data, len) \
	memcpy(data, ary+pos, len); \
	pos += len;  


/* load/save state */
void pc98_cirrus_vga_save()
{
    CirrusVGAState *s = cirrusvga;
    int pos = 0;
	UINT8 *f = cirrusvga_statsavebuf; 
	//char test[500] = {0};
	uint32_t_ state_ver = 2;
	uint32_t_ intbuf;
	
    array_write(f, pos, &state_ver, sizeof(state_ver)); // ステートセーブ バージョン番号
	
	// この際全部保存
	array_write(f, pos, vramptr, CIRRUS_VRAM_SIZE);
	array_write(f, pos, &s->vram_offset, sizeof(s->vram_offset));
	array_write(f, pos, &s->vram_size, sizeof(s->vram_size));
	array_write(f, pos, &s->lfb_addr, sizeof(s->lfb_addr));
	array_write(f, pos, &s->lfb_end, sizeof(s->lfb_end));
	array_write(f, pos, &s->map_addr, sizeof(s->map_addr));
	array_write(f, pos, &s->map_end, sizeof(s->map_end));
	array_write(f, pos, &s->lfb_vram_mapped, sizeof(s->lfb_vram_mapped));
	array_write(f, pos, &s->bios_offset, sizeof(s->bios_offset));
	array_write(f, pos, &s->bios_size, sizeof(s->bios_size));
	array_write(f, pos, &s->it_shift, sizeof(s->it_shift));
	
	array_write(f, pos, &s->latch, sizeof(s->latch));
	array_write(f, pos, &s->sr_index, sizeof(s->sr_index));
	array_write(f, pos, s->sr, sizeof(s->sr));
	array_write(f, pos, &s->gr_index, sizeof(s->gr_index));
	array_write(f, pos, s->gr, sizeof(s->gr));
	array_write(f, pos, &s->ar_index, sizeof(s->ar_index));
	array_write(f, pos, s->ar, sizeof(s->ar));
	array_write(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
	array_write(f, pos, &s->cr_index, sizeof(s->cr_index));
	array_write(f, pos, s->cr, sizeof(s->cr));
	array_write(f, pos, &s->msr, sizeof(s->msr));
	array_write(f, pos, &s->fcr, sizeof(s->fcr));
	array_write(f, pos, &s->st00, sizeof(s->st00));
	array_write(f, pos, &s->st01, sizeof(s->st01));
	array_write(f, pos, &s->dac_state, sizeof(s->dac_state));
	array_write(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
	array_write(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
	array_write(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
	array_write(f, pos, s->dac_cache, sizeof(s->dac_cache));
	array_write(f, pos, &s->dac_8bit, sizeof(s->dac_8bit));
	array_write(f, pos, s->palette, sizeof(s->palette));
	array_write(f, pos, &s->bank_offset, sizeof(s->bank_offset));
	array_write(f, pos, &s->vga_io_memory, sizeof(s->vga_io_memory));
	array_write(f, pos, &s->vbe_index, sizeof(s->vbe_index));
	array_write(f, pos, s->vbe_regs, sizeof(s->vbe_regs));
	array_write(f, pos, &s->vbe_start_addr, sizeof(s->vbe_start_addr));
	array_write(f, pos, &s->vbe_line_offset, sizeof(s->vbe_line_offset));
	array_write(f, pos, &s->vbe_bank_mask, sizeof(s->vbe_bank_mask));
	
	array_write(f, pos, s->font_offsets, sizeof(s->font_offsets));
	array_write(f, pos, &s->graphic_mode, sizeof(s->graphic_mode));
	array_write(f, pos, &s->shift_control, sizeof(s->shift_control));
	array_write(f, pos, &s->double_scan, sizeof(s->double_scan));
	array_write(f, pos, &s->line_offset, sizeof(s->line_offset));
	array_write(f, pos, &s->line_compare, sizeof(s->line_compare));
	array_write(f, pos, &s->start_addr, sizeof(s->start_addr));
	array_write(f, pos, &s->plane_updated, sizeof(s->plane_updated));
	array_write(f, pos, &s->last_line_offset, sizeof(s->last_line_offset));
	array_write(f, pos, &s->last_cw, sizeof(s->last_cw));
	array_write(f, pos, &s->last_ch, sizeof(s->last_ch));
	array_write(f, pos, &s->last_width, sizeof(s->last_width));
	array_write(f, pos, &s->last_height, sizeof(s->last_height));
	array_write(f, pos, &s->last_scr_width, sizeof(s->last_scr_width));
	array_write(f, pos, &s->last_scr_height, sizeof(s->last_scr_height));
	array_write(f, pos, &s->last_depth, sizeof(s->last_depth));
	array_write(f, pos, &s->cursor_start, sizeof(s->cursor_start));
	array_write(f, pos, &s->cursor_end, sizeof(s->cursor_end));
	array_write(f, pos, &s->cursor_offset, sizeof(s->cursor_offset));
	
	array_write(f, pos, s->invalidated_y_table, sizeof(s->invalidated_y_table));
	array_write(f, pos, s->last_palette, sizeof(s->last_palette));
	array_write(f, pos, s->last_ch_attr, sizeof(s->last_ch_attr));
	
	array_write(f, pos, &s->cirrus_linear_io_addr, sizeof(s->cirrus_linear_io_addr));
	array_write(f, pos, &s->cirrus_linear_bitblt_io_addr, sizeof(s->cirrus_linear_bitblt_io_addr));
	array_write(f, pos, &s->cirrus_mmio_io_addr, sizeof(s->cirrus_mmio_io_addr));
	array_write(f, pos, &s->cirrus_addr_mask, sizeof(s->cirrus_addr_mask));
	array_write(f, pos, &s->linear_mmio_mask, sizeof(s->linear_mmio_mask));
	array_write(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
	array_write(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
	array_write(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
	array_write(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));
	array_write(f, pos, s->cirrus_bank_base, sizeof(s->cirrus_bank_base));
	array_write(f, pos, s->cirrus_bank_limit, sizeof(s->cirrus_bank_limit));
	array_write(f, pos, s->cirrus_hidden_palette, sizeof(s->cirrus_hidden_palette));
	array_write(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
	array_write(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
	array_write(f, pos, &s->cirrus_blt_pixelwidth, sizeof(s->cirrus_blt_pixelwidth));
	array_write(f, pos, &s->cirrus_blt_width, sizeof(s->cirrus_blt_width));
	array_write(f, pos, &s->cirrus_blt_height, sizeof(s->cirrus_blt_height));
	array_write(f, pos, &s->cirrus_blt_dstpitch, sizeof(s->cirrus_blt_dstpitch));
	array_write(f, pos, &s->cirrus_blt_srcpitch, sizeof(s->cirrus_blt_srcpitch));
	array_write(f, pos, &s->cirrus_blt_fgcol, sizeof(s->cirrus_blt_fgcol));
	array_write(f, pos, &s->cirrus_blt_bgcol, sizeof(s->cirrus_blt_bgcol));
	array_write(f, pos, &s->cirrus_blt_dstaddr, sizeof(s->cirrus_blt_dstaddr));
	array_write(f, pos, &s->cirrus_blt_srcaddr, sizeof(s->cirrus_blt_srcaddr));
	array_write(f, pos, &s->cirrus_blt_mode, sizeof(s->cirrus_blt_mode));
	array_write(f, pos, &s->cirrus_blt_modeext, sizeof(s->cirrus_blt_modeext));
	array_write(f, pos, s->cirrus_bltbuf, sizeof(s->cirrus_bltbuf));
	intbuf = s->cirrus_srcptr - s->cirrus_bltbuf;
	array_write(f, pos, &intbuf, sizeof(intbuf));
	intbuf = s->cirrus_srcptr_end - s->cirrus_bltbuf;
	array_write(f, pos, &intbuf, sizeof(intbuf));
	array_write(f, pos, &s->cirrus_srccounter, sizeof(s->cirrus_srccounter));
	array_write(f, pos, &s->last_hw_cursor_size, sizeof(s->last_hw_cursor_size));
	array_write(f, pos, &s->last_hw_cursor_x, sizeof(s->last_hw_cursor_x));
	array_write(f, pos, &s->last_hw_cursor_y, sizeof(s->last_hw_cursor_y));
	array_write(f, pos, &s->last_hw_cursor_y_start, sizeof(s->last_hw_cursor_y_start));
	array_write(f, pos, &s->last_hw_cursor_y_end, sizeof(s->last_hw_cursor_y_end));
	array_write(f, pos, &s->real_vram_size, sizeof(s->real_vram_size));
	array_write(f, pos, &s->device_id, sizeof(s->device_id));
	array_write(f, pos, &s->bustype, sizeof(s->bustype));
	
	array_write(f, pos, &np2clvga.VRAMWindowAddr3, sizeof(np2clvga.VRAMWindowAddr3));

    //array_write(f, pos, &s->latch, sizeof(s->latch));
    //array_write(f, pos, &s->sr_index, sizeof(s->sr_index));
    //array_write(f, pos, s->sr, 256);
    //array_write(f, pos, &s->gr_index, sizeof(s->gr_index));
    //array_write(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
    //array_write(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
    //array_write(f, pos, s->gr + 2, 254);
    //array_write(f, pos, &s->ar_index, sizeof(s->ar_index));
    //array_write(f, pos, s->ar, 21);
    //array_write(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
    //array_write(f, pos, &s->cr_index, sizeof(s->cr_index));
    //array_write(f, pos, s->cr, 256);
    //array_write(f, pos, &s->msr, sizeof(s->msr));
    //array_write(f, pos, &s->fcr, sizeof(s->fcr));
    //array_write(f, pos, &s->st00, sizeof(s->st00));
    //array_write(f, pos, &s->st01, sizeof(s->st01));

    //array_write(f, pos, &s->dac_state, sizeof(s->dac_state));
    //array_write(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
    //array_write(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
    //array_write(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
    //array_write(f, pos, s->dac_cache, 3);
    //array_write(f, pos, s->palette, 768);

    //array_write(f, pos, &s->bank_offset, sizeof(s->bank_offset));

    //array_write(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
    //array_write(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));

    //array_write(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
    //array_write(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
	
    //array_write(f, pos, vramptr, CIRRUS_VRAM_SIZE);
	
    //array_write(f, pos, s->cirrus_hidden_palette, 48);

    /* XXX: we do not save the bitblt state - we assume we do not save
       the state when the blitter is active */
	//sprintf(test, "CIRRUS VGA datalen=%d", pos);
	//msgbox("CIRRUS VGA datalen", test);
	TRACEOUT(("CIRRUS VGA datalen=%d", pos));
}

void pc98_cirrus_vga_load()
{
    CirrusVGAState *s = cirrusvga;
	UINT8 *f = cirrusvga_statsavebuf; 
    int pos = 0;
	uint32_t_ state_ver = 0;
	uint32_t_ intbuf;
	int width, height;
	
	array_read(f, pos, &state_ver, sizeof(state_ver)); // バージョン番号
	switch(state_ver){
	case 0:
		s->latch = state_ver; // ver0.86 rev22以前はlatchは常にゼロ（のはず）
		array_read(f, pos, &s->sr_index, sizeof(s->sr_index));
		array_read(f, pos, s->sr, 256);
		array_read(f, pos, &s->gr_index, sizeof(s->gr_index));
		array_read(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
		array_read(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
		s->gr[0x00] = s->cirrus_shadow_gr0 & 0x0f;
		s->gr[0x01] = s->cirrus_shadow_gr1 & 0x0f;
		array_read(f, pos, s->gr + 2, 254);
		array_read(f, pos, &s->ar_index, sizeof(s->ar_index));
		array_read(f, pos, s->ar, 21);
		array_read(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
		array_read(f, pos, &s->cr_index, sizeof(s->cr_index));
		array_read(f, pos, s->cr, 256);
		array_read(f, pos, &s->msr, sizeof(s->msr));
		array_read(f, pos, &s->fcr, sizeof(s->fcr));
		array_read(f, pos, &s->st00, sizeof(s->st00));
		array_read(f, pos, &s->st01, sizeof(s->st01));

		array_read(f, pos, &s->dac_state, sizeof(s->dac_state));
		array_read(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
		array_read(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
		array_read(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
		array_read(f, pos, s->dac_cache, 3);
		array_read(f, pos, s->palette, 768);

		array_read(f, pos, &s->bank_offset, sizeof(s->bank_offset));

		array_read(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
		array_read(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));

		array_read(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
		array_read(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
	
		array_read(f, pos, vramptr, CIRRUS_VRAM_SIZE);

		break;
	case 1:
	case 2:
		// この際全部保存
		array_read(f, pos, vramptr, CIRRUS_VRAM_SIZE);
		array_read(f, pos, &s->vram_offset, sizeof(s->vram_offset));
		array_read(f, pos, &s->vram_size, sizeof(s->vram_size));
		array_read(f, pos, &s->lfb_addr, sizeof(s->lfb_addr));
		array_read(f, pos, &s->lfb_end, sizeof(s->lfb_end));
		array_read(f, pos, &s->map_addr, sizeof(s->map_addr));
		array_read(f, pos, &s->map_end, sizeof(s->map_end));
		array_read(f, pos, &s->lfb_vram_mapped, sizeof(s->lfb_vram_mapped));
		array_read(f, pos, &s->bios_offset, sizeof(s->bios_offset));
		array_read(f, pos, &s->bios_size, sizeof(s->bios_size));
		array_read(f, pos, &s->it_shift, sizeof(s->it_shift));
	
		array_read(f, pos, &s->latch, sizeof(s->latch));
		array_read(f, pos, &s->sr_index, sizeof(s->sr_index));
		array_read(f, pos, s->sr, sizeof(s->sr));
		array_read(f, pos, &s->gr_index, sizeof(s->gr_index));
		array_read(f, pos, s->gr, sizeof(s->gr));
		array_read(f, pos, &s->ar_index, sizeof(s->ar_index));
		array_read(f, pos, s->ar, sizeof(s->ar));
		array_read(f, pos, &s->ar_flip_flop, sizeof(s->ar_flip_flop));
		array_read(f, pos, &s->cr_index, sizeof(s->cr_index));
		array_read(f, pos, s->cr, sizeof(s->cr));
		array_read(f, pos, &s->msr, sizeof(s->msr));
		array_read(f, pos, &s->fcr, sizeof(s->fcr));
		array_read(f, pos, &s->st00, sizeof(s->st00));
		array_read(f, pos, &s->st01, sizeof(s->st01));
		array_read(f, pos, &s->dac_state, sizeof(s->dac_state));
		array_read(f, pos, &s->dac_sub_index, sizeof(s->dac_sub_index));
		array_read(f, pos, &s->dac_read_index, sizeof(s->dac_read_index));
		array_read(f, pos, &s->dac_write_index, sizeof(s->dac_write_index));
		array_read(f, pos, s->dac_cache, sizeof(s->dac_cache));
		array_read(f, pos, &s->dac_8bit, sizeof(s->dac_8bit));
		array_read(f, pos, s->palette, sizeof(s->palette));
		array_read(f, pos, &s->bank_offset, sizeof(s->bank_offset));
		array_read(f, pos, &s->vga_io_memory, sizeof(s->vga_io_memory));
		array_read(f, pos, &s->vbe_index, sizeof(s->vbe_index));
		array_read(f, pos, s->vbe_regs, sizeof(s->vbe_regs));
		array_read(f, pos, &s->vbe_start_addr, sizeof(s->vbe_start_addr));
		array_read(f, pos, &s->vbe_line_offset, sizeof(s->vbe_line_offset));
		array_read(f, pos, &s->vbe_bank_mask, sizeof(s->vbe_bank_mask));
	
		array_read(f, pos, s->font_offsets, sizeof(s->font_offsets));
		array_read(f, pos, &s->graphic_mode, sizeof(s->graphic_mode));
		array_read(f, pos, &s->shift_control, sizeof(s->shift_control));
		array_read(f, pos, &s->double_scan, sizeof(s->double_scan));
		array_read(f, pos, &s->line_offset, sizeof(s->line_offset));
		array_read(f, pos, &s->line_compare, sizeof(s->line_compare));
		array_read(f, pos, &s->start_addr, sizeof(s->start_addr));
		array_read(f, pos, &s->plane_updated, sizeof(s->plane_updated));
		array_read(f, pos, &s->last_line_offset, sizeof(s->last_line_offset));
		array_read(f, pos, &s->last_cw, sizeof(s->last_cw));
		array_read(f, pos, &s->last_ch, sizeof(s->last_ch));
		array_read(f, pos, &s->last_width, sizeof(s->last_width));
		array_read(f, pos, &s->last_height, sizeof(s->last_height));
		array_read(f, pos, &s->last_scr_width, sizeof(s->last_scr_width));
		array_read(f, pos, &s->last_scr_height, sizeof(s->last_scr_height));
		array_read(f, pos, &s->last_depth, sizeof(s->last_depth));
		array_read(f, pos, &s->cursor_start, sizeof(s->cursor_start));
		array_read(f, pos, &s->cursor_end, sizeof(s->cursor_end));
		array_read(f, pos, &s->cursor_offset, sizeof(s->cursor_offset));
	
		array_read(f, pos, s->invalidated_y_table, sizeof(s->invalidated_y_table));
		array_read(f, pos, s->last_palette, sizeof(s->last_palette));
		array_read(f, pos, s->last_ch_attr, sizeof(s->last_ch_attr));
	
		array_read(f, pos, &s->cirrus_linear_io_addr, sizeof(s->cirrus_linear_io_addr));
		array_read(f, pos, &s->cirrus_linear_bitblt_io_addr, sizeof(s->cirrus_linear_bitblt_io_addr));
		array_read(f, pos, &s->cirrus_mmio_io_addr, sizeof(s->cirrus_mmio_io_addr));
		array_read(f, pos, &s->cirrus_addr_mask, sizeof(s->cirrus_addr_mask));
		array_read(f, pos, &s->linear_mmio_mask, sizeof(s->linear_mmio_mask));
		array_read(f, pos, &s->cirrus_shadow_gr0, sizeof(s->cirrus_shadow_gr0));
		array_read(f, pos, &s->cirrus_shadow_gr1, sizeof(s->cirrus_shadow_gr1));
		array_read(f, pos, &s->cirrus_hidden_dac_lockindex, sizeof(s->cirrus_hidden_dac_lockindex));
		array_read(f, pos, &s->cirrus_hidden_dac_data, sizeof(s->cirrus_hidden_dac_data));
		array_read(f, pos, s->cirrus_bank_base, sizeof(s->cirrus_bank_base));
		array_read(f, pos, s->cirrus_bank_limit, sizeof(s->cirrus_bank_limit));
		array_read(f, pos, s->cirrus_hidden_palette, sizeof(s->cirrus_hidden_palette));
		array_read(f, pos, &s->hw_cursor_x, sizeof(s->hw_cursor_x));
		array_read(f, pos, &s->hw_cursor_y, sizeof(s->hw_cursor_y));
		array_read(f, pos, &s->cirrus_blt_pixelwidth, sizeof(s->cirrus_blt_pixelwidth));
		array_read(f, pos, &s->cirrus_blt_width, sizeof(s->cirrus_blt_width));
		array_read(f, pos, &s->cirrus_blt_height, sizeof(s->cirrus_blt_height));
		array_read(f, pos, &s->cirrus_blt_dstpitch, sizeof(s->cirrus_blt_dstpitch));
		array_read(f, pos, &s->cirrus_blt_srcpitch, sizeof(s->cirrus_blt_srcpitch));
		array_read(f, pos, &s->cirrus_blt_fgcol, sizeof(s->cirrus_blt_fgcol));
		array_read(f, pos, &s->cirrus_blt_bgcol, sizeof(s->cirrus_blt_bgcol));
		array_read(f, pos, &s->cirrus_blt_dstaddr, sizeof(s->cirrus_blt_dstaddr));
		array_read(f, pos, &s->cirrus_blt_srcaddr, sizeof(s->cirrus_blt_srcaddr));
		array_read(f, pos, &s->cirrus_blt_mode, sizeof(s->cirrus_blt_mode));
		array_read(f, pos, &s->cirrus_blt_modeext, sizeof(s->cirrus_blt_modeext));
		array_read(f, pos, s->cirrus_bltbuf, sizeof(s->cirrus_bltbuf));
		array_read(f, pos, &intbuf, sizeof(intbuf));
		s->cirrus_srcptr = s->cirrus_bltbuf + intbuf;
		array_read(f, pos, &intbuf, sizeof(intbuf));
		s->cirrus_srcptr_end = s->cirrus_bltbuf + intbuf;
		array_read(f, pos, &s->cirrus_srccounter, sizeof(s->cirrus_srccounter));
		array_read(f, pos, &s->last_hw_cursor_size, sizeof(s->last_hw_cursor_size));
		array_read(f, pos, &s->last_hw_cursor_x, sizeof(s->last_hw_cursor_x));
		array_read(f, pos, &s->last_hw_cursor_y, sizeof(s->last_hw_cursor_y));
		array_read(f, pos, &s->last_hw_cursor_y_start, sizeof(s->last_hw_cursor_y_start));
		array_read(f, pos, &s->last_hw_cursor_y_end, sizeof(s->last_hw_cursor_y_end));
		array_read(f, pos, &s->real_vram_size, sizeof(s->real_vram_size));
		array_read(f, pos, &s->device_id, sizeof(s->device_id));
		array_read(f, pos, &s->bustype, sizeof(s->bustype));

		if(state_ver >= 2){
			array_read(f, pos, &np2clvga.VRAMWindowAddr3, sizeof(np2clvga.VRAMWindowAddr3));
		}

		break;
	default:
		break;
	}

    cirrus_update_memory_access(s);
    
    s->graphic_mode = -1;
    cirrus_update_bank_ptr(s, 0);
    cirrus_update_bank_ptr(s, 1);
	
	if(cirrusvga->get_resolution){
		cirrusvga->get_resolution((VGAState*)cirrusvga, &width, &height);
		np2wab_setScreenSize(width, height);
	}

	np2wab.paletteChanged = 1;
}

/***************************************
 *
 *  initialize
 *
 ***************************************/
void cirrus_reset(void *opaque)
{
    CirrusVGAState *s = (CirrusVGAState*)opaque;

	memset(s->sr, 0, sizeof(s->sr));
	memset(s->cr, 0, sizeof(s->cr));
	memset(s->gr, 0, sizeof(s->gr));

    vga_reset(s);
    unmap_linear_vram(s);
    s->sr[0x06] = 0x0f;
    if (s->device_id == CIRRUS_ID_CLGD5446) {
        /* 4MB 64 bit memory config, always PCI */
        s->sr[0x1F] = 0x2d;		// MemClock
        s->gr[0x18] = 0x0f;             // fastest memory configuration
        s->sr[0x0f] = 0x98;
        s->sr[0x17] = 0x20;
        s->sr[0x15] = 0x04; /* memory size, 3=2MB, 4=4MB */
    } else {
        s->sr[0x1F] = 0x22;		// MemClock
        s->sr[0x0F] = CIRRUS_MEMSIZE_2M;
        s->sr[0x17] = s->bustype;
        s->sr[0x15] = 0x03; /* memory size, 3=2MB, 4=4MB */
    }
    s->cr[0x27] = s->device_id;
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WAB) {
        s->sr[0x0F] = CIRRUS_MEMSIZE_1M;
        s->sr[0x15] = 0x02;
	}
	if (np2clvga.gd54xxtype == CIRRUS_98ID_WSN) {
        //s->sr[0x1F] = 0x2d;		// MemClock
        //s->gr[0x18] = 0x0f;             // fastest memory configuration
        s->sr[0x0f] = 0x98;
        //s->sr[0x17] = 0x20;
        s->sr[0x15] = 0x04; /* memory size, 3=2MB, 4=4MB */
        //s->sr[0x0F] = 0x20;
	}
	if (np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB) {
        //s->sr[0x1F] = 0x2d;		// MemClock
        //s->gr[0x18] = 0x0f;             // fastest memory configuration
        //s->sr[0x0f] = 0x98;
        //s->sr[0x17] = 0x20;
        //s->sr[0x15] = 0x04; /* memory size, 3=2MB, 4=4MB */
        s->sr[0x0F] = 0x98;
        s->sr[0x15] = 0x04;
	}

	if (np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F) {
		memset(s->vram_ptr, 0x00, s->real_vram_size);
	}else{
		/* Win2K seems to assume that the pattern buffer is at 0xff
		   initially ! */
		memset(s->vram_ptr, 0xff, s->real_vram_size);
	}
	memset(s->palette, 0, sizeof(s->palette));
	memset(s->cirrus_hidden_palette, 0, sizeof(s->cirrus_hidden_palette));
	
    s->cirrus_hidden_dac_lockindex = 5;
    //s->cirrus_hidden_dac_data = 0;

	// XXX: for WinNT4.0
	s->cirrus_hidden_dac_data = 1;
	
	// XXX: Win2000のハードウェアアクセラレーションを動かすのに必要。理由は謎
	s->gr[0x25] = 0x06;
	s->gr[0x26] = 0x20;
	
	//fh = fopen("vgadump.bin", "w+");
	//if (fh != FILEH_INVALID) {
	//	fwrite(s, sizeof(CirrusVGAState), 1, fh);
	//	fclose(fh);
	//}
}

LOGPALETTE * NewLogPal(const uint8_t *pCirrusPalette , int iSize) {
	LOGPALETTE *lpPalette;
	int count;

	lpPalette = (LOGPALETTE*)malloc(sizeof (LOGPALETTE) + iSize * sizeof (PALETTEENTRY));
	lpPalette->palVersion = 0x0300;
	lpPalette->palNumEntries = iSize;
	
	for (count = 0 ; count < iSize ; count++) {
		lpPalette->palPalEntry[count].peRed = c6_to_8(pCirrusPalette[count*3]);
		lpPalette->palPalEntry[count].peGreen = c6_to_8(pCirrusPalette[count*3+1]);
		lpPalette->palPalEntry[count].peBlue = c6_to_8(pCirrusPalette[count*3+2]);
		lpPalette->palPalEntry[count].peFlags = 0;
	}
	return lpPalette;
}
//　画面表示(仮)　本当はQEMUのオリジナルのコードを移植すべきなんだけど･･･
void cirrusvga_drawGraphic(){
	//static UINT32 kdown = 0;
	//static UINT32 kdownc = 0;
	//static INT32 memshift = 0;
	int i, width, height, bpp;
	uint32_t_ line_offset = 0;
	LOGPALETTE * lpPalette;
	static HPALETTE hPalette = NULL, oldPalette = NULL;
	HDC hdc = np2wabwnd.hDCBuf;
	static int waitscreenchange = 0;
	int r;
	int scanW = 0; // VRAM上の1ラインのデータ幅(byte)
	int scanpixW = 0; // 実際に転送すべき1ラインのピクセル数(pixel)
	int scanshift = 0;
	uint8_t *scanptr;
	uint8_t *vram_ptr;


	// VRAM上での1ラインのサイズ（表示幅と等しくない場合有り）
	line_offset = cirrusvga->cr[0x13] | ((cirrusvga->cr[0x1b] & 0x10) << 4);
    line_offset <<= 3;

	vram_ptr = cirrusvga->vram_ptr + np2wab.vramoffs;
	
	//	vram_ptr = mem + 640*16*memshift;
	//if(GetKeyState(VK_SHIFT)<0){
	//	memshift++;
	////	kdown = 1;
	////}else if(kdown){
	////	//vram_ptr = vram_ptr + 1024*768*1;
	////	kdown = 0;
	//}
	//if(GetKeyState(VK_CONTROL)<0){
	//	memshift--;
	//	//vram_ptr = mem + 1024*768*memshift;
	////	kdownc = 1;
	////}else if(kdownc){
	////	//vram_ptr = vram_ptr + 1024*768*1;
	////	kdownc = 0;
	//}
	////if(GetKeyState(VK_CONTROL)<0){
	////	vram_ptr = vram_ptr + 1024*768*2;
	////}

    bpp = cirrusvga->get_bpp((VGAState*)cirrusvga);
	//bpp = cirrusvga->cirrus_blt_pixelwidth*8;
    cirrusvga->get_resolution((VGAState*)cirrusvga, &width, &height);
	np2wab.realWidth = width;
	np2wab.realHeight = height;
    
	//　謎の表示幅調整（2^nにパディングされることがあるらしいけど条件が分からないので無理矢理）
	scanW = width*bpp/8;
	scanpixW = width;
	if(bpp && line_offset){
		scanW = line_offset;
		if(bpp==8)
			scanpixW = scanW*8/bpp;
	}
	if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		// XXX: WSN用やっつけ修正
		if((width==1024 || width==800 || width==640) && bpp==32){
			scanW = width*bpp/8;
			scanpixW = width;
		}
	}
	//if(width==640 && bpp==24 && ((np2clvga.VRAMWindowAddr>>24)&0xff)==0xf0){
	//	// XXX: Win2k用やっつけ修正
	//	scanpixW = 1024;
	//}
	//if((cirrusvga->cr[0x08] & 8)){
	//	if(scanpixW<=512 ) scanpixW = 512;
	//	else if(scanpixW<=1024) scanpixW = 1024;
	//	else if(scanpixW<=2048) scanpixW = 2048;
	//	else if(scanpixW<=4096) scanpixW = 4096;
	//	else if(scanpixW<=8192) scanpixW = 8192;
	//}
	//if(width==640){
	//	// XXX: 何も補正しない
	//}else{
	//	if(scanpixW<=256 ) scanpixW = 256;
	//	else if(scanpixW<=512) scanpixW = 512;
	//	else if(scanpixW<=1024) scanpixW = 1024;
	//	else if(scanpixW<=2048) scanpixW = 2048;
	//}
	//if(width==800 && bpp==8 && (((np2clvga.VRAMWindowAddr>>24)&0xff)==0xf0 || np2clvga.gd54xxtype > 0xff)){
	//	// XXX: Win2k用やっつけ修正
	//	scanW = width*2;
	//	scanpixW = width;
	//}
	//if(np2clvga.gd54xxtype == CIRRUS_98ID_Be && width==640 && bpp==8 && np2clvga.VRAMWindowAddr==0){
	//	// XXX: SC2k用やっつけ修正
	//	scanpixW = 1024;
	//}
	if(bpp==16){
		uint32_t_* bitfleld = (uint32_t_*)(ga_bmpInfo->bmiColors);
		scanW = width*2;
		bitfleld[0] = 0x0000F800;
		bitfleld[1] = 0x000007E0;
		bitfleld[2] = 0x0000001F;
		ga_bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
	}else{
		ga_bmpInfo->bmiHeader.biCompression = BI_RGB;
	}
	if(bpp==15){
		bpp = 16;
		scanW = width*2;
	}
	
	if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		// XXX: 画面表示開始位置シフト 本来ははみ出した分が反対側に転送される？？？
		scanshift = ((((int)cirrusvga->cr[0x0c])<<8)|(((int)cirrusvga->cr[0x0d])));
		if(scanshift){
			scanshift = ((scanshift)/* % (height)*/);//;
		}
	}

	if(ga_bmpInfo->bmiHeader.biBitCount!=8 && bpp==8){
		np2wab.paletteChanged = 1;
	}

	ga_bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ga_bmpInfo->bmiHeader.biWidth = width; // 仮セット
	ga_bmpInfo->bmiHeader.biHeight = 1; // 仮セット
	ga_bmpInfo->bmiHeader.biPlanes = 1;
	ga_bmpInfo->bmiHeader.biBitCount = bpp;
	if(bpp<=8){
		if(np2wab.paletteChanged){
			WORD* PalIndexes = (WORD*)((char*)ga_bmpInfo + sizeof(BITMAPINFOHEADER));
			for (i = 0; i < 256; ++i) PalIndexes[i] = i;
			lpPalette = NewLogPal(cirrusvga->palette , 1<<bpp);
			if(hPalette){
				SelectPalette(hdc , oldPalette , FALSE);
				DeleteObject(hPalette);
			}
			hPalette = CreatePalette(lpPalette);
			free(lpPalette);
			oldPalette = SelectPalette(hdc , hPalette , FALSE);
			RealizePalette(hdc);
			np2wab.paletteChanged = 0;
		}
		// 256モードなら素直に転送して良し
		if(scanshift){
			// XXX: スキャン位置シフト
			ga_bmpInfo->bmiHeader.biWidth = scanpixW;
			ga_bmpInfo->bmiHeader.biHeight = -height;
			scanptr = vram_ptr;
			SetDIBitsToDevice(
				hdc , 0 , 0 ,
				ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
				0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
				vram_ptr+scanshift*bpp/8 , ga_bmpInfo , DIB_PAL_COLORS
			);
			//int scanshiftY = scanshift/scanpixW;
			//ga_bmpInfo->bmiHeader.biWidth = scanpixW;
			//ga_bmpInfo->bmiHeader.biHeight = -height;
			//SetDIBitsToDevice(
			//	hdc , 0 , 0 ,
			//	ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
			//	0 , 0 , scanshiftY , -ga_bmpInfo->bmiHeader.biHeight ,
			//	vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
			//);
			//SetDIBitsToDevice(
			//	hdc , 0 , 0 ,
			//	ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
			//	0 , 0 , 0 , scanshiftY ,
			//	vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
			//);
		}else{
			ga_bmpInfo->bmiHeader.biWidth = scanpixW;
			ga_bmpInfo->bmiHeader.biHeight = -height;
			scanptr = vram_ptr;
			SetDIBitsToDevice(
				hdc , 0 , 0 ,
				ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
				0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
				vram_ptr , ga_bmpInfo , DIB_PAL_COLORS
			);
		}
	}else{
		if(scanpixW*bpp/8==scanW){
			if(scanshift){
				// XXX: スキャン位置シフト
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr+scanshift*bpp/8 , ga_bmpInfo , DIB_RGB_COLORS
				);
			}else{
				ga_bmpInfo->bmiHeader.biWidth = scanpixW;
				ga_bmpInfo->bmiHeader.biHeight = -height;
				scanptr = vram_ptr;
				SetDIBitsToDevice(
					hdc , 0 , 0 ,
					ga_bmpInfo->bmiHeader.biWidth , -ga_bmpInfo->bmiHeader.biHeight ,
					0 , 0 , 0 , -ga_bmpInfo->bmiHeader.biHeight ,
					vram_ptr , ga_bmpInfo , DIB_RGB_COLORS
				);
			}
		}else{
			// ズレがあるなら1ラインずつ転送
			scanptr = vram_ptr;
			for(i=0;i<height;i++){
				ga_bmpInfo->bmiHeader.biWidth = width;
				ga_bmpInfo->bmiHeader.biHeight = 1;
				r = SetDIBitsToDevice(
					hdc , 0 , i ,
					width , 1 ,
					0 , 0 , 0 , 1 ,
					scanptr , ga_bmpInfo , DIB_RGB_COLORS
				);
				scanptr += scanW;
			}
		}
	}
    if ((cirrusvga->sr[0x12] & CIRRUS_CURSOR_SHOW)){
		int hwcur_x = cirrusvga->hw_cursor_x;
		int hwcur_y = cirrusvga->hw_cursor_y;
		//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
		//	// XXX: なんだこれ
		//	if(cirrusvga->sr[0x13]==0x3E && bpp==16) hwcur_x -= 8;
		//}
		if(np2cfg.gd5430fakecur){
			// ハードウェアカーソルが上手く表示できない場合用
			DrawIcon(hdc, hwcur_x, hwcur_y, ga_hFakeCursor);
		}else{
			HPALETTE oldCurPalette;
			int cursize = 32;
			int x, y, tmp;
			int x1, x2, h, w;
			unsigned int basecolor;
			unsigned int color0, color1;
			uint8_t *d1 = cirrusvga->vram_ptr;
			uint8_t *palette, *src, *base;
			uint32_t_ *dst;
			uint32_t_ content;
			int poffset;
			if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
				cursize = 64;
			}
			BitBlt(ga_hdc_cursor , 0 , 0 , cursize , cursize , hdc , hwcur_x , hwcur_y , SRCCOPY);
		
			if(np2clvga.gd54xxtype <= 0xff){
				base = cirrusvga->vram_ptr + 1024 * 1024 - 16 * 1024; // ??? 1MB前提？
			}else{
				base = cirrusvga->vram_ptr + cirrusvga->real_vram_size - 16 * 1024;
			}
			if (height >= hwcur_y && 0 <= (hwcur_y + cursize)){
				int y1, y2;
				y1 = hwcur_y; 
				y2 = hwcur_y+cursize; 
				h = cursize;
				dst = (uint32_t_ *)cursorptr;
				dst += 64*64;
				dst -= 64;
				for(y=y1;y<y2;y++){
					src = base;
					if (cirrusvga->sr[0x12] & CIRRUS_CURSOR_LARGE) {
						src += (cirrusvga->sr[0x13] & 0x3c) * 256;
						src += (y - (int)hwcur_y) * 16;
						poffset = 8;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)src)[1] |
							((uint32_t_ *)src)[2] |
							((uint32_t_ *)src)[3];
					} else {
						src += (cirrusvga->sr[0x13] & 0x3f) * 256;
						src += (y - (int)hwcur_y) * 4;
						poffset = 128;
						content = ((uint32_t_ *)src)[0] |
							((uint32_t_ *)(src + 128))[0];
					}
					/* if nothing to draw, no need to continue */
					if (1){
						uint32_t_ colortmp;
						const uint8_t *plane0, *plane1;
						int b0, b1;
						x1 = hwcur_x;
						if (x1 < width){
							x2 = hwcur_x + cursize;
							w = x2 - x1;
							palette = cirrusvga->cirrus_hidden_palette;
							color0 = rgb_to_pixel32(c6_to_8(palette[0x0 * 3]),
														c6_to_8(palette[0x0 * 3 + 1]),
														c6_to_8(palette[0x0 * 3 + 2]));
							color1 = rgb_to_pixel32(c6_to_8(palette[0xf * 3]),
														c6_to_8(palette[0xf * 3 + 1]),
														c6_to_8(palette[0xf * 3 + 2]));
							for(x=0;x<w;x++){
								colortmp = *dst;
								plane0 = src;
								plane1 = src + poffset;
								b0 = (plane0[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								b1 = (plane1[(x) >> 3] >> (7 - ((x) & 7))) & 1;
								switch(b0 | (b1 << 1)) {
								case 0:
									break;
								case 1:
									colortmp ^= 0xffffff;
									break;
								case 2:
									colortmp = color0;
									break;
								case 3:
									colortmp = color1;
									break;
								}
								*dst = colortmp;
								dst++;
							}
							dst += (64 - w);
						}
					}
					dst -= 64*2;
				}
			}
			BitBlt(hdc , hwcur_x , hwcur_y , cursize , cursize , ga_hdc_cursor , 0 , 0 , SRCCOPY);
		}
	}
	ga_bmpInfo->bmiHeader.biWidth = width; // 前回の解像度を保存
	ga_bmpInfo->bmiHeader.biHeight = height; // 前回の解像度を保存
}

/***************************************
 *
 *  PC-9821 support
 *
 ***************************************/
static void IOOUTCALL cirrusvga_ofa2(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: set register index %02X", dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_regindexA2 = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifa2(UINT port) {
	TRACEOUT(("CIRRUS VGA: get register index %02X", cirrusvga_regindex));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_regindexA2;
}
static void IOOUTCALL cirrusvga_ofa3(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%.2X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindexA2){
	case 0x00:
		break;
	case 0x01:
		switch(dat){
		case 0x10:
			np2clvga.VRAMWindowAddr2 = 0x0b0000;
			break;
		case 0x80:
			np2clvga.VRAMWindowAddr2 = 0xf20000;
			break;
		case 0xA0:
			np2clvga.VRAMWindowAddr2 = 0xf00000;
			break;
		case 0xC0:
			np2clvga.VRAMWindowAddr2 = 0xf40000;
			break;
		case 0xE0:
			np2clvga.VRAMWindowAddr2 = 0xf60000;
			break;
		}
		break;
	case 0x02:
		if(np2clvga.gd54xxtype <= 0xff){
			if(dat!=0x00 && dat!=0xff) np2clvga.VRAMWindowAddr = (dat<<24);
		}
		break;
	case 0x03:
		if((!!np2wab.relaystateint) != (!!(dat&0x2))){
			np2wab.relaystateint = dat & 0x2;
			np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
		}
		np2clvga.mmioenable = (dat&0x1);
		break;
	}
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifa3(UINT port) {
	REG8 ret = 0xff;
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindexA2){
	case 0x00:
		if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
			ret = (REG8)np2clvga.gd54xxtype;
		}else{
			ret = 0xff;
		}
		break;
	case 0x01:
		if(np2clvga.gd54xxtype <= 0xff){
			switch(np2clvga.VRAMWindowAddr2){
			case 0x0b0000:
				ret = 0x10;
				break;
			case 0xf00000:
				ret = 0xA0;
				break;
			case 0xf20000:
				ret = 0x80;
				break;
			case 0xf40000:
				ret = 0xC0;
				break;
			case 0xf60000:
				ret = 0xE0;
				break;
			}
		}else{
			ret = 0xff;
		}
		break;
	case 0x02:
		if(np2clvga.gd54xxtype <= 0xff){
			ret = (np2clvga.VRAMWindowAddr>>24)&0xff;
		}else{
			ret = 0xff;
		}
		break;
	case 0x03:
		ret = ((np2wab.relaystateint&0x2) ? 0x2 : 0x0) | np2clvga.mmioenable;
		break;
	case 0x04:
		ret = 0x00;
		break;
	}
	return ret;
}

static void IOOUTCALL cirrusvga_ofaa(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: set register index %02X", dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_regindex = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifaa(UINT port) {
	TRACEOUT(("CIRRUS VGA: get register index %02X", cirrusvga_regindex));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_regindex;
}
static void IOOUTCALL cirrusvga_ofab(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%.2X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindex){
	case 0x00:
		break;
	case 0x01:
		switch(dat){
		case 0x10:
			np2clvga.VRAMWindowAddr2 = 0x0b0000;
			break;
		case 0x80:
			np2clvga.VRAMWindowAddr2 = 0xf20000;
			break;
		case 0xA0:
			np2clvga.VRAMWindowAddr2 = 0xf00000;
			break;
		case 0xC0:
			np2clvga.VRAMWindowAddr2 = 0xf40000;
			break;
		case 0xE0:
			np2clvga.VRAMWindowAddr2 = 0xf60000;
			break;
		}
		break;
	case 0x02:
		if(dat!=0x00 && dat!=0xff) np2clvga.VRAMWindowAddr = (dat<<24);
		//cirrusvga->vram_offset = np2clvga.VRAMWindowAddr;
		break;
	case 0x03:
		if((!!np2wab.relaystateint) != (!!(dat&0x2))){
			np2wab.relaystateint = dat & 0x2;
			np2wab.relaystateext = dat & 0x2; // （￣∀￣;）
			np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
		}
		np2clvga.mmioenable = (dat&0x1);
		break;
	}
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_ifab(UINT port) {
	REG8 ret = 0xff;
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	switch(cirrusvga_regindex){
	case 0x00:
		if(np2clvga.gd54xxtype <= 0xff){
			ret = (REG8)np2clvga.gd54xxtype;//0x5B;
		}else{
			ret = 0xff;
		}
		break;
	case 0x01:
		if(np2clvga.gd54xxtype <= 0xff){
			switch(np2clvga.VRAMWindowAddr2){
			case 0x0b0000:
				ret = 0x10;
				break;
			case 0xf20000:
				ret = 0x80;
				break;
			case 0xf00000:
				ret = 0xA0;
				break;
			case 0xf40000:
				ret = 0xC0;
				break;
			case 0xf60000:
				ret = 0xE0;
				break;
			}
		}else{
			ret = 0xff;
		}
		break;
	case 0x02:
		if(np2clvga.gd54xxtype <= 0xff){
			ret = (np2clvga.VRAMWindowAddr>>24)&0xff;
		}else{
			ret = 0xff;
		}
		break;
	case 0x03:
		ret = (np2wab.relay ? 0x2 : 0x0) | np2clvga.mmioenable;
		break;
	}
	return ret;
}

int cirrusvga_videoenable = 0x00;
static void IOOUTCALL cirrusvga_off82(UINT port, REG8 dat) {
	TRACEOUT(("CIRRUS VGA: out %04X d=%02X", port, dat));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_videoenable = dat & 0x1;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_iff82(UINT port) {
	TRACEOUT(("CIRRUS VGA: inp %04X", port));
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		np2clvga.gd54xxtype = CIRRUS_98ID_Xe10;
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return cirrusvga_videoenable;
}

// WAB, WSN用
int cirrusvga_wab_51e1 = 0xC2;
int cirrusvga_wab_40e1 = 0x7b;
//int cirrusvga_wab_0fe1 = 0xC2;
int cirrusvga_wab_46e8 = 0x18;
static REG8 IOINPCALL cirrusvga_i51e1(UINT port) {
	REG8 ret = cirrusvga_wab_51e1;
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
	//	ret = 0xC2;
	//}
	return ret;
}
static void IOOUTCALL cirrusvga_o51e1(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_wab_51e1 = dat;
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i5be3(UINT port) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	return 0xf7; // XXX: 0x08 is VRAM 4M flag?
}
static void IOOUTCALL cirrusvga_o5be3(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	(void)port;
	(void)dat;
}

static REG8 IOINPCALL cirrusvga_i40e1(UINT port) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	//cirrusvga_wab_40e1--;
	//TRACEOUT(("CIRRUS VGA: out %04X d=%02X", port, cirrusvga_wab_40e1));
	return cirrusvga_wab_40e1;
}
static void IOOUTCALL cirrusvga_o40e1(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_wab_40e1 = dat;
	np2wab.relaystateint = (np2wab.relaystateint & ~0x1) | (cirrusvga_wab_40e1 & 0x1);
	np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
	(void)port;
	(void)dat;
}
static REG8 IOINPCALL cirrusvga_i46e8(UINT port) {
	REG8 ret = cirrusvga_wab_46e8;
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	//np2wab.relaystateint = (np2wab.relaystateint & ~0x1) | (cirrusvga_wab_40e1 & 0x1);
	//np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext); // リレーはORで･･･（暫定やっつけ修正）
	return ret;
}
static void IOOUTCALL cirrusvga_o46e8(UINT port, REG8 dat) {
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		switch(np2clvga.gd54xxtype){
		case CIRRUS_98ID_AUTO_XE10_WABS:
			np2clvga.gd54xxtype = CIRRUS_98ID_WAB;
			break;
		case CIRRUS_98ID_AUTO_XE10_WSN4:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN;
			break;
		default:
			np2clvga.gd54xxtype = CIRRUS_98ID_WSN_A2F;
			break;
		}
		pc98_cirrus_vga_setvramsize();
		pc98_cirrus_vga_initVRAMWindowAddr();
	}
	cirrusvga_wab_46e8 = dat;
	(void)port;
	(void)dat;
}

//int cirrusvga_wab_52E2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i52E2(UINT port) {
//	REG8 ret = cirrusvga_wab_52E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o52E2(UINT port, REG8 dat) {
//	cirrusvga_wab_52E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_56E2 = 0x80;
//static REG8 IOINPCALL cirrusvga_i56E2(UINT port) {
//	REG8 ret = cirrusvga_wab_56E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o56E2(UINT port, REG8 dat) {
//	cirrusvga_wab_56E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_59E2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i59E2(UINT port) {
//	REG8 ret = cirrusvga_wab_59E2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o59E2(UINT port, REG8 dat) {
//	cirrusvga_wab_59E2 = dat;
//	(void)port;
//	(void)dat;
//}
//int cirrusvga_wab_5BE2 = 0x18;
//static REG8 IOINPCALL cirrusvga_i5BE2(UINT port) {
//	REG8 ret = cirrusvga_wab_5BE2;
//	return ret;
//}
//static void IOOUTCALL cirrusvga_o5BE2(UINT port, REG8 dat) {
//	cirrusvga_wab_5BE2 = dat;
//	(void)port;
//	(void)dat;
//}

static void vga_dumb_update_retrace_info(VGAState *s)
{
    (void) s;
}

void pc98_cirrus_vga_initVRAMWindowAddr(){
	if(np2clvga.gd54xxtype == CIRRUS_98ID_Be){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf00000;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf00000;
	}else if(np2clvga.gd54xxtype <= 0xff){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xf60000;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F || np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0xE0000;
	}else{
		np2clvga.VRAMWindowAddr = 0;
		np2clvga.VRAMWindowAddr2 = 0;
	}

}
void pc98_cirrus_vga_setvramsize(){
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_1MB; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_Be){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_1MB; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype <= 0xff){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE; //(cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_WAB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_2MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE_4MB;
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		cirrusvga->real_vram_size = CIRRUS_VRAM_SIZE;
	}else{
		cirrusvga->real_vram_size = (cirrusvga->device_id == CIRRUS_ID_CLGD5446) ? CIRRUS_VRAM_SIZE : CIRRUS_VRAM_SIZE;
	}
	cirrusvga->vram_ptr = vramptr;
    cirrusvga->vram_offset = 0;
    cirrusvga->vram_size = cirrusvga->real_vram_size;

    /* XXX: s->vram_size must be a power of two */
	cirrusvga->cirrus_addr_mask = cirrusvga->real_vram_size - 1;
	cirrusvga->linear_mmio_mask = cirrusvga->real_vram_size - 256;

}
static void pc98_cirrus_reset(CirrusVGAState * s, int device_id, int is_pci)
{
	//np2clvga.VRAMWindowAddr = (0x0F<<24);
	//np2clvga.VRAMWindowAddr2 = (0xf20000);
	
	np2clvga.VRAMWindowAddr2 = 0;
	np2clvga.VRAMWindowAddr3 = 0;
	
	pc98_cirrus_vga_initVRAMWindowAddr();

	np2wab.relaystateext = 0;
	np2wab_setRelayState(np2wab.relaystateint|np2wab.relaystateext);
	ShowWindow(np2wabwnd.hWndWAB, SW_HIDE); // 設定変更対策

	np2clvga.mmioenable = 0;
	np2wab.paletteChanged = 1;
}
static void pc98_cirrus_init_common(CirrusVGAState * s, int device_id, int is_pci)
{
    int i;

    for(i = 0;i < 256; i++)
        rop_to_index[i] = CIRRUS_ROP_NOP_INDEX; /* nop rop */
    rop_to_index[CIRRUS_ROP_0] = 0;
    rop_to_index[CIRRUS_ROP_SRC_AND_DST] = 1;
    rop_to_index[CIRRUS_ROP_NOP] = 2;
    rop_to_index[CIRRUS_ROP_SRC_AND_NOTDST] = 3;
    rop_to_index[CIRRUS_ROP_NOTDST] = 4;
    rop_to_index[CIRRUS_ROP_SRC] = 5;
    rop_to_index[CIRRUS_ROP_1] = 6;
    rop_to_index[CIRRUS_ROP_NOTSRC_AND_DST] = 7;
    rop_to_index[CIRRUS_ROP_SRC_XOR_DST] = 8;
    rop_to_index[CIRRUS_ROP_SRC_OR_DST] = 9;
    rop_to_index[CIRRUS_ROP_NOTSRC_OR_NOTDST] = 10;
    rop_to_index[CIRRUS_ROP_SRC_NOTXOR_DST] = 11;
    rop_to_index[CIRRUS_ROP_SRC_OR_NOTDST] = 12;
    rop_to_index[CIRRUS_ROP_NOTSRC] = 13;
    rop_to_index[CIRRUS_ROP_NOTSRC_OR_DST] = 14;
    rop_to_index[CIRRUS_ROP_NOTSRC_AND_NOTDST] = 15;
    s->device_id = device_id;
    s->bustype = CIRRUS_BUSTYPE_ISA;
	
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype <= 0xff){
		// ONBOARD
		iocore_attachout(0xfa2, cirrusvga_ofa2);
		iocore_attachinp(0xfa2, cirrusvga_ifa2);
	
		iocore_attachout(0xfa3, cirrusvga_ofa3);
		iocore_attachinp(0xfa3, cirrusvga_ifa3);
	
		iocore_attachout(0xfaa, cirrusvga_ofaa);
		iocore_attachinp(0xfaa, cirrusvga_ifaa);
	
		iocore_attachout(0xfab, cirrusvga_ofab);
		iocore_attachinp(0xfab, cirrusvga_ifab);
	
		if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype == CIRRUS_98ID_96){
			iocore_attachout(0x0902, cirrusvga_off82);
			iocore_attachinp(0x0902, cirrusvga_iff82);

			// XXX: 102Access Control Register 0904h は無視

			for(i=0;i<16;i++){
				iocore_attachout(0xc50 + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
				iocore_attachinp(0xc50 + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
			}
	
			//　この辺のマッピング本当にあってる？
			iocore_attachout(0xb54, vga_ioport_write_wrap);	// 0x3B4
			iocore_attachinp(0xb54, vga_ioport_read_wrap);	// 0x3B4
			iocore_attachout(0xb55, vga_ioport_write_wrap);	// 0x3B5
			iocore_attachinp(0xb55, vga_ioport_read_wrap);	// 0x3B5

			iocore_attachout(0xd54, vga_ioport_write_wrap);	// 0x3D4
			iocore_attachinp(0xd54, vga_ioport_read_wrap);	// 0x3D4
			iocore_attachout(0xd55, vga_ioport_write_wrap);	// 0x3D5
			iocore_attachinp(0xd55, vga_ioport_read_wrap);	// 0x3D5
	
			iocore_attachout(0xb5a, vga_ioport_write_wrap);	// 0x3BA
			iocore_attachinp(0xb5a, vga_ioport_read_wrap);	// 0x3BA

			iocore_attachout(0xd5a, vga_ioport_write_wrap);	// 0x3DA
			iocore_attachinp(0xd5a, vga_ioport_read_wrap);	// 0x3DA

			//iocore_attachout(0x46E8, cirrusvga_o46e8);
			//iocore_attachinp(0x46E8, cirrusvga_i46e8);
		}
		if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype != CIRRUS_98ID_96){
			iocore_attachout(0xff82, cirrusvga_off82);
			iocore_attachinp(0xff82, cirrusvga_iff82);

			for(i=0;i<16;i++){
				iocore_attachout(0xca0 + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
				iocore_attachinp(0xca0 + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
			}
	
			//　この辺のマッピング本当にあってる？
			iocore_attachout(0xba4, vga_ioport_write_wrap);	// 0x3B4
			iocore_attachinp(0xba4, vga_ioport_read_wrap);	// 0x3B4
			iocore_attachout(0xba5, vga_ioport_write_wrap);	// 0x3B5
			iocore_attachinp(0xba5, vga_ioport_read_wrap);	// 0x3B5

			iocore_attachout(0xda4, vga_ioport_write_wrap);	// 0x3D4
			iocore_attachinp(0xda4, vga_ioport_read_wrap);	// 0x3D4
			iocore_attachout(0xda5, vga_ioport_write_wrap);	// 0x3D5
			iocore_attachinp(0xda5, vga_ioport_read_wrap);	// 0x3D5
	
			iocore_attachout(0xbaa, vga_ioport_write_wrap);	// 0x3BA
			iocore_attachinp(0xbaa, vga_ioport_read_wrap);	// 0x3BA

			iocore_attachout(0xdaa, vga_ioport_write_wrap);	// 0x3DA
			iocore_attachinp(0xdaa, vga_ioport_read_wrap);	// 0x3DA
		}
	}
	if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) == CIRRUS_98ID_AUTOMSK || np2clvga.gd54xxtype > 0xff){
		// WAB, WSN, GA-98NB
		for(i=0;i<0x1000;i+=0x100){
			iocore_attachout(0x40E0 + CIRRUS_MELCOWAB_OFS + i, vga_ioport_write_wrap);	// 0x3C0 to 0x3CF
			iocore_attachinp(0x40E0 + CIRRUS_MELCOWAB_OFS + i, vga_ioport_read_wrap);	// 0x3C0 to 0x3CF
		}
	
		//　この辺のマッピング本当にあってる？
		iocore_attachout(0x5AE0 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3B4
		iocore_attachinp(0x5AE0 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3B4
		iocore_attachout(0x5BE1 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3B5
		iocore_attachinp(0x5BE1 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3B5

		iocore_attachout(0x54E0 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3D4
		iocore_attachinp(0x54E0 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3D4
		iocore_attachout(0x55E0 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3D5
		iocore_attachinp(0x55E0 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3D5
	
		iocore_attachout(0x5BE1 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3BA
		iocore_attachinp(0x5BE1 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3BA
		//if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
		//	iocore_attachout(0x5BE3, cirrusvga_o5be3);
		//	iocore_attachinp(0x5BE3, cirrusvga_i5be3);
		//}

		iocore_attachout(0x5AE0 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3DA
		iocore_attachinp(0x5AE0 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3DA

		iocore_attachout(0x51E1 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3BA
		iocore_attachinp(0x51E1 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3BA

		iocore_attachout(0x57E1 + CIRRUS_MELCOWAB_OFS, vga_ioport_write_wrap);	// 0x3DA
		iocore_attachinp(0x57E1 + CIRRUS_MELCOWAB_OFS, vga_ioport_read_wrap);		// 0x3DA
	
		iocore_attachout(0x40E1 + CIRRUS_MELCOWAB_OFS, cirrusvga_o40e1);
		iocore_attachinp(0x40E1 + CIRRUS_MELCOWAB_OFS, cirrusvga_i40e1);
	
		iocore_attachout(0x46E8, cirrusvga_o46e8);
		iocore_attachinp(0x46E8, cirrusvga_i46e8);
		
		if((np2clvga.gd54xxtype & CIRRUS_98ID_AUTOMSK) != CIRRUS_98ID_AUTOMSK){
			np2clvga.VRAMWindowAddr2 = 0xE0000;
		}
		//np2clvga.VRAMWindowAddr3 = 0xF00000; // XXX
		//np2clvga.VRAMWindowAddr3size = 256*1024;
		
		//iocore_attachout(0x52E2, cirrusvga_o52E2);
		//iocore_attachinp(0x52E2, cirrusvga_i52E2);
		//
		//iocore_attachout(0x56E2, cirrusvga_o56E2);
		//iocore_attachinp(0x56E2, cirrusvga_i56E2);
		//iocore_attachout(0x56E3, cirrusvga_o56E2);
		//iocore_attachinp(0x56E3, cirrusvga_i56E2);
		//
		//iocore_attachout(0x59E2, cirrusvga_o59E2);
		//iocore_attachinp(0x59E2, cirrusvga_i59E2);
		//iocore_attachout(0x59E3, cirrusvga_o59E2);
		//iocore_attachinp(0x59E3, cirrusvga_i59E2);
		//
		//iocore_attachout(0x58E2, cirrusvga_o5BE2);
		//iocore_attachinp(0x58E2, cirrusvga_i5BE2);
		//iocore_attachout(0x5BE2, cirrusvga_o5BE2);
		//iocore_attachinp(0x5BE2, cirrusvga_i5BE2);
	}

	pc98_cirrus_vga_setvramsize();

    s->get_bpp = cirrus_get_bpp;
    s->get_offsets = cirrus_get_offsets;
    s->get_resolution = cirrus_get_resolution;
    s->cursor_invalidate = cirrus_cursor_invalidate;
    s->cursor_draw_line = cirrus_cursor_draw_line;

	s->update_retrace_info = vga_dumb_update_retrace_info;
	s->retrace = vga_dumb_retrace;

    qemu_register_reset(cirrus_reset, s);
    cirrus_reset(s);
    cirrus_update_memory_access(s);
    //register_savevm("cirrus_vga", 0, 2, cirrus_vga_save, cirrus_vga_load, s);// XXX:
	//if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB || np2clvga.gd54xxtype == CIRRUS_98ID_WSN){
	//	//s->sr[0x06] = 0x12; // Unlock Cirrus extensions by default
	//}
}


void pc98_cirrus_vga_init(void)
{
	HDC hdc;
	UINT i;
	WORD* PalIndexes;
    CirrusVGAState *s;
	HBITMAP hbmp;
	BOOL b;

	ga_bmpInfo = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO)+sizeof(WORD)*256);	
	PalIndexes = (WORD*)((char*)ga_bmpInfo + sizeof(BITMAPINFOHEADER));
	for (i = 0; i < 256; ++i) PalIndexes[i] = i;
	
	ga_bmpInfo_cursor = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO));	

	vramptr = (uint8_t*)malloc(CIRRUS_VRAM_SIZE*2); // 2倍取っておく
	
	ga_bmpInfo_cursor->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	ga_bmpInfo_cursor->bmiHeader.biPlanes = 1;
	ga_bmpInfo_cursor->bmiHeader.biBitCount = 32;
	ga_bmpInfo_cursor->bmiHeader.biCompression = BI_RGB;
	ga_bmpInfo_cursor->bmiHeader.biWidth = 64;
	ga_bmpInfo_cursor->bmiHeader.biHeight = 64;
	ga_hbmp_cursor = CreateDIBSection(NULL, ga_bmpInfo_cursor, DIB_RGB_COLORS, (void **)(&cursorptr), NULL, 0);
	hdc = GetDC(NULL);
	ga_hdc_cursor = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	SelectObject(ga_hdc_cursor, ga_hbmp_cursor);
	
	ds.surface = &np2vga_ds_surface;
	ds.listeners = &np2vga_ds_listeners;
	ds.mouse_set = np2vga_ds_mouse_set;
	ds.cursor_define = np2vga_ds_cursor_define;
	ds.next = NULL;

	ga_hFakeCursor = LoadCursor(NULL, IDC_ARROW);

	cirrusvga_opaque = cirrusvga = s = (CirrusVGAState*)calloc(1, sizeof(CirrusVGAState));
}
void pc98_cirrus_vga_reset(const NP2CFG *pConfig)
{
    CirrusVGAState *s;

	np2clvga.enabled = np2cfg.usegd5430;
	if(!np2clvga.enabled){
		TRACEOUT(("CL-GD54xx: Window Accelerator Disabled"));
		return;
	}
	
	np2clvga.defgd54xxtype = np2cfg.gd5430type;
	np2clvga.gd54xxtype = np2cfg.gd5430type;
	
	s = cirrusvga;
	//memset(s, 0, sizeof(CirrusVGAState));
	if(np2clvga.gd54xxtype <= 0x57){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype <= 0xff){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5426, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5434, 0);
	}else{
		pc98_cirrus_reset(s, CIRRUS_ID_CLGD5430, 0);
	}
}
void pc98_cirrus_vga_bind(void)
{
    CirrusVGAState *s;
	if(!np2clvga.enabled){
		TRACEOUT(("CL-GD54xx: Window Accelerator Disabled"));
		return;
	}
	//np2clvga.defgd54xxtype = np2cfg.gd5430type;
	
	s = cirrusvga;
	//memset(s, 0, sizeof(CirrusVGAState));
	if(np2clvga.gd54xxtype <= 0x57){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_96){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5428, 0);
	}else if(np2clvga.gd54xxtype <= 0xff){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5430, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WAB){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5426, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_WSN || np2clvga.gd54xxtype == CIRRUS_98ID_WSN_A2F){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5434, 0);
	}else if(np2clvga.gd54xxtype == CIRRUS_98ID_GA98NB){
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5434, 0);
	}else{
		pc98_cirrus_init_common(s, CIRRUS_ID_CLGD5430, 0);
	}
	s->ds = graphic_console_init(s->update, s->invalidate, s->screen_dump, s->text_update, s);
	
	np2wabwnd.drawframe = cirrusvga_drawGraphic;

	ga_bmpInfo->bmiHeader.biWidth = 0;
	ga_bmpInfo->bmiHeader.biHeight = 0;

	TRACEOUT(("CL-GD54xx: Window Accelerator Enabled"));
}
void pc98_cirrus_vga_shutdown(void)
{
	//DeleteObject(ga_hpal);
	np2wabwnd.drawframe = NULL;
	free(ga_bmpInfo_cursor);
	free(ga_bmpInfo);
	free(vramptr);
	DeleteDC(ga_hdc_cursor);
	DeleteObject(ga_hbmp_cursor);
	//free(cursorptr);
}

// MELCO WAB系ポートならTRUE
int pc98_cirrus_isWABport(UINT port){
	if((port & 0xF0FF) == (0x40E0 + CIRRUS_MELCOWAB_OFS)) return 1;
	if(port == 0x5AE0 + CIRRUS_MELCOWAB_OFS) return 1;
	if(port == 0x5BE1 + CIRRUS_MELCOWAB_OFS) return 1;
	if(port == 0x54E0 + CIRRUS_MELCOWAB_OFS) return 1;
	if(port == 0x55E0 + CIRRUS_MELCOWAB_OFS) return 1;
	if(port == 0x51E1 + CIRRUS_MELCOWAB_OFS) return 1;
	if(port == 0x57E1 + CIRRUS_MELCOWAB_OFS) return 1;
	return 0;
}

#endif