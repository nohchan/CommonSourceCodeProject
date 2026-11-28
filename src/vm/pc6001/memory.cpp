/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6001mkII Emulator 'yaPC-6201'
	NEC PC-6001mkIISR Emulator 'yaPC-6401'
	NEC PC-6601 Emulator 'yaPC-6601'
	NEC PC-6601SR Emulator 'yaPC-6801'

	[ memory ]
*/

#include "memory.h"
#include "timer.h"

#define RAM			(MEMORY_BASE + RAM_BASE)
#define BASICROM	(MEMORY_BASE + BASICROM_BASE)
#define EXTROM		(MEMORY_BASE + EXTROM_BASE)
#define CGROM1		(MEMORY_BASE + CGROM1_BASE)
#define KANJIROM	(MEMORY_BASE + KANJIROM_BASE)
#define VOICEROM	(MEMORY_BASE + VOICEROM_BASE)
#define SYSROM2		(MEMORY_BASE + SYSROM2_BASE)
#define EmptyRAM	(MEMORY_BASE + EmptyRAM_BASE)

void MEMORY::initialize()
{
	memset(MEMORY_BASE, 0xff, sizeof(MEMORY_BASE));
	FILEIO* fio = new FILEIO();

#if defined(_PC6001)
	if(fio->Fopen(create_local_path(_T("BASICROM.60")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x4000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM60.60")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x1000, 1);
		fio->Fclose();
	}
#elif defined(_PC6001MK2)
	if(fio->Fopen(create_local_path(_T("BASICROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x8000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM60.62")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x2000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM60m.62")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1 + 0x2000, 0x2000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("KANJIROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(KANJIROM, 0x8000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("VOICEROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(VOICEROM, 0x4000, 1);
		fio->Fclose();
	}
#elif defined(_PC6601)
	if(fio->Fopen(create_local_path(_T("BASICROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x8000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM60.66")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x2000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM66.66")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1 + 0x2000, 0x2000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("KANJIROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(KANJIROM, 0x8000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("VOICEROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(VOICEROM, 0x4000, 1);
		fio->Fclose();
	}
#elif defined(_PC6001MK2SR) || defined(_PC6601SR)
	if(fio->Fopen(create_local_path(_T("SYSTEMROM1.68")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x10000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("SYSTEMROM2.68")), FILEIO_READ_BINARY)) {
		fio->Fread(SYSROM2, 0x10000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("CGROM68.68")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x4000, 1);
		fio->Fclose();
	}
#endif
	delete fio;
	register_vline_event(this);
}

void MEMORY::reset()
{
	if(!inserted) {
		EXTROM1 = RAM + 0x4000;
		EXTROM2 = RAM + 0x6000;
		FILEIO* fio = new FILEIO();
#if defined(_PC6001)
		const _TCHAR* ext_name = _T("EXTROM.60");
#elif defined(_PC6001MK2)
		const _TCHAR* ext_name = _T("EXTROM.62");
#elif defined(_PC6601)
		const _TCHAR* ext_name = _T("EXTROM.66");
#else
		const _TCHAR* ext_name = _T("EXTROM.68");
#endif
		if(fio->Fopen(create_local_path(ext_name), FILEIO_READ_BINARY)) {
			fio->Fread(EXTROM, 0x4000, 1);
			fio->Fclose();
			EXTROM1 = EXTROM;
			EXTROM2 = EXTROM + 0x2000;
			inserted = true;
		}
		delete fio;
	}
	memset(RAM, 0, RAM_SIZE);
	memset(EmptyRAM, 0, EmptyRAM_SIZE);
	CGROM = CGROM1;
	VRAM = RAM;
	CGSW93 = CRTKILL = 0;

	portB0 = 0x00;
	portC0 = 0x00;
	portC1 = 0x00;
	portC2 = 0x00;
	portC3 = 0x00;
	portC8 = 0x00;
	portC9 = 0x00;
	portCA = 0x00;
	portF0 = 0x11;
	portF1 = 0xdd;
	portF2 = 0x00;

	for(int i = 0; i < 16; i++) {
		port40[i] = i;
		port50[i] = 0;
		port60[i] = i;
	}
#if defined(_PC6001MK2SR) || defined(_PC6601SR)
	static_cast<VM*>(vm)->sr_mode = 1;
#else
	static_cast<VM*>(vm)->sr_mode = 0;
#endif
	update_bank();
	update_palette();
}

void MEMORY::update_bank()
{
#if defined(_PC6001)
	read_bank[0] = BASICROM;
	read_bank[1] = BASICROM + 0x2000;
	read_bank[2] = EXTROM1;
	read_bank[3] = CGSW93 ? CGROM1 : EXTROM2;
	for(int i = 4; i < 8; i++) read_bank[i] = RAM + i * 0x2000;

	for(int i = 0; i < 8; i++) {
		write_bank[i] = RAM + i * 0x2000;
		write_enable[i] = (i >= 4) || (i >= 2 && !inserted);
	}
#else
	// Read bank selection (port F0)
	for(int blk = 0; blk < 4; blk++) {
		uint8_t sel = (portF0 >> (blk * 2)) & 0x03;
		int b0 = blk * 2;
		int b1 = blk * 2 + 1;
		switch(sel) {
		case 0: // Internal RAM
			read_bank[b0] = RAM + b0 * 0x2000;
			read_bank[b1] = RAM + b1 * 0x2000;
			break;
		case 1: // Basic ROM / System ROM
#if defined(_PC6001MK2SR) || defined(_PC6601SR)
			if(static_cast<VM*>(vm)->sr_mode) {
				read_bank[b0] = BASICROM + ((port60[b0] & 0x0f) * 0x2000);
				read_bank[b1] = BASICROM + ((port60[b1] & 0x0f) * 0x2000);
			} else
#endif
			{
				if(blk < 2) {
					read_bank[b0] = BASICROM + b0 * 0x2000;
					read_bank[b1] = BASICROM + b1 * 0x2000;
				} else {
					read_bank[b0] = EmptyRAM;
					read_bank[b1] = EmptyRAM;
				}
			}
			break;
		case 2: // Voice / Kanji / CG ROM (port F1)
			{
				uint8_t vsel = (portF1 >> (blk * 2)) & 0x03;
				if(vsel == 0) {
					read_bank[b0] = VOICEROM;
					read_bank[b1] = VOICEROM + 0x2000;
				} else if(vsel == 1) {
					int kofs = (portC2 & 0x01) ? 0x4000 : 0x0000;
					read_bank[b0] = KANJIROM + kofs;
					read_bank[b1] = KANJIROM + kofs + 0x2000;
				} else if(vsel == 2) {
					read_bank[b0] = CGROM1;
					read_bank[b1] = CGROM1 + 0x2000;
				} else {
					read_bank[b0] = EmptyRAM;
					read_bank[b1] = EmptyRAM;
				}
			}
			break;
		case 3: // External Cartridge ROM
			if(inserted) {
				read_bank[b0] = EXTROM1;
				read_bank[b1] = EXTROM2;
			} else {
				read_bank[b0] = EmptyRAM;
				read_bank[b1] = EmptyRAM;
			}
			break;
		}
	}
	if(CGSW93 && !(portF0 & 0x0c)) {
		read_bank[3] = CGROM1;
	}

	// Write bank selection (port F2)
	for(int blk = 0; blk < 4; blk++) {
		uint8_t wsel = (portF2 >> (blk * 2)) & 0x03;
		int b0 = blk * 2;
		int b1 = blk * 2 + 1;
		if(wsel == 1 || wsel == 3) {
			write_bank[b0] = RAM + b0 * 0x2000;
			write_bank[b1] = RAM + b1 * 0x2000;
			write_enable[b0] = true;
			write_enable[b1] = true;
		} else {
			write_bank[b0] = EmptyRAM;
			write_bank[b1] = EmptyRAM;
			write_enable[b0] = false;
			write_enable[b1] = false;
		}
	}
#endif
}

void MEMORY::update_palette()
{
	// Standard 15-color PC-6001mkII / 6601 palette (RGB)
	static const uint8_t pal_rgb[16][3] = {
		{   0,   0,   0 }, // 0: Black / Transparent
		{ 255, 170,   0 }, // 1: Orange
		{   0, 255, 170 }, // 2: Blue-Green
		{ 170, 255,   0 }, // 3: Yellow-Green
		{   0,   0, 255 }, // 4: Blue
		{ 255,   0, 170 }, // 5: Violet
		{   0, 170, 255 }, // 6: Sky Blue
		{ 170, 170, 170 }, // 7: Gray
		{   0,   0,   0 }, // 8: Black
		{ 255,   0,   0 }, // 9: Red
		{   0, 255,   0 }, // 10: Green
		{ 255, 255,   0 }, // 11: Yellow
		{   0,   0, 170 }, // 12: Dark Blue
		{ 255,   0, 255 }, // 13: Magenta
		{   0, 255, 255 }, // 14: Cyan
		{ 255, 255, 255 }, // 15: White
	};

	for(int i = 0; i < 16; i++) {
		int c = port40[i] & 0x0f;
		palette_pc[i] = RGB_COLOR(pal_rgb[c][0], pal_rgb[c][1], pal_rgb[c][2]);
	}
}

void MEMORY::write_data8(uint32_t addr, uint32_t data)
{
	int bank = (addr >> 13) & 7;
	if(write_enable[bank]) {
		write_bank[bank][addr & 0x1fff] = data;
	}
}

uint32_t MEMORY::read_data8(uint32_t addr)
{
	int bank = (addr >> 13) & 7;
	return read_bank[bank][addr & 0x1fff];
}

void MEMORY::write_data8w(uint32_t addr, uint32_t data, int *wait)
{
	*wait = addr < 0x8000 ? 1 : 0;
	write_data8(addr, data);
}

uint32_t MEMORY::read_data8w(uint32_t addr, int *wait)
{
	*wait = addr < 0x8000 ? 1 : 0;
	return read_data8(addr);
}

uint32_t MEMORY::fetch_op(uint32_t addr, int *wait)
{
	*wait = 1;
	return read_data8(addr);
}

void MEMORY::write_io8(uint32_t addr, uint32_t data)
{
	uint8_t port = addr & 0xff;

	if(port >= 0x40 && port <= 0x4f) {
		port40[port & 0x0f] = data;
		update_palette();
		return;
	}
	if(port >= 0x50 && port <= 0x5f) {
		port50[port & 0x0f] = data;
		return;
	}
	if(port >= 0x60 && port <= 0x6f) {
		port60[port & 0x0f] = data;
		update_bank();
		return;
	}

	switch(port) {
	case 0xb0:
		portB0 = data;
		if(d_timer) {
			d_timer->set_portB0(data);
		}
		break;
	case 0xc0:
		portC0 = data;
		break;
	case 0xc1:
		portC1 = data;
#if defined(_PC6001MK2SR) || defined(_PC6601SR)
		static_cast<VM*>(vm)->sr_mode = (data & 0x08) ? 0 : 1;
		update_bank();
#endif
		break;
	case 0xc2:
		portC2 = data;
		update_bank();
		break;
	case 0xc3:
		portC3 = data;
		break;
	case 0xc8:
		portC8 = data;
		break;
	case 0xc9:
		portC9 = data;
		break;
	case 0xca:
		portCA = data;
		break;
	case 0xf0:
		portF0 = data;
		update_bank();
		break;
	case 0xf1:
		portF1 = data;
		update_bank();
		break;
	case 0xf2:
		portF2 = data;
		update_bank();
		break;
	}
}

uint32_t MEMORY::read_io8(uint32_t addr)
{
	uint8_t port = addr & 0xff;
	if(port >= 0x40 && port <= 0x4f) return port40[port & 0x0f];
	if(port >= 0x60 && port <= 0x6f) return port60[port & 0x0f];

	switch(port) {
	case 0xb0: return portB0;
	case 0xc0: return portC0;
	case 0xc1: return portC1;
	case 0xc2: return portC2;
	case 0xc3: return portC3;
	case 0xf0: return portF0;
	case 0xf1: return portF1;
	case 0xf2: return portF2;
	}
	return 0xff;
}
#define EVENT_HBLANK	1

void MEMORY::event_vline(int v, int clock)
{
	if(!CRTKILL) {
		if(v < 200) {
			d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
			register_event_by_clock(this, EVENT_HBLANK, (uint64_t)((double)CPU_CLOCKS / FRAMES_PER_SEC / LINES_PER_FRAME * 296 / 455), false, NULL);
		}
	}
#ifndef _PC6001
	if(v == 200 && d_timer) {
		d_timer->write_signal(SIG_TIMER_IRQ_VRTC, 1, 1);
	}
#endif
}

void MEMORY::event_callback(int event_id, int err)
{
	if(event_id == EVENT_HBLANK) {
		d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 0);
	}
}

void MEMORY::write_signal(int id, uint32_t data, uint32_t mask)
{
	if(id == SIG_MEMORY_PIO_PORT_C) {
		CGSW93 = (data & 4) ? 0 : 1;
		CRTKILL = (data & 2) ? 0 : 1;
		if(CRTKILL) {
			d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 0);
		}
#ifndef _PC6001
		update_bank();
#endif
	}
}

void MEMORY::open_cart(const _TCHAR* file_path)
{
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(EXTROM, 0x4000, 1);
		fio->Fclose();
		EXTROM1 = EXTROM;
		EXTROM2 = EXTROM + 0x2000;
		inserted = true;
	} else {
		EXTROM1 = RAM + 0x4000;
		EXTROM2 = RAM + 0x6000;
		inserted = false;
	}
	delete fio;
	update_bank();
}

void MEMORY::close_cart()
{
	EXTROM1 = RAM + 0x4000;
	EXTROM2 = RAM + 0x6000;
	inserted = false;
	update_bank();
}

void MEMORY::draw_screen()
{
#ifndef _PC6001
	static const uint16_t vram_head_n60[] = { 0xc000, 0xe000, 0x8000, 0xa000 };
	static const uint16_t vram_head_mk2[] = { 0x8000, 0xc000, 0x0000, 0x4000 };

	if(CRTKILL) {
		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			scrntype_t* dest = emu->get_screen_buffer(y);
			if(dest) memset(dest, 0, SCREEN_WIDTH * sizeof(scrntype_t));
		}
		return;
	}

	bool n60_mode = (portC1 & 0x02) ? true : false;
	bool gfx_mode = (portC1 & 0x04) ? true : false;
	bool w80_mode = (portC1 & 0x01) ? false : true;
	int page = (portB0 >> 1) & 3;

#if defined(_PC6001MK2SR) || defined(_PC6601SR)
	if(static_cast<VM*>(vm)->sr_mode) {
		uint32_t sr_vram_ofs = ((portC8 & 0x0f) << 12);
		bool sr_gfx = (portC8 & 0x80) ? true : false;
		if(sr_gfx) {
			// SR 640x200 16-color bitmap mode (2 pixels per byte)
			for(int y = 0; y < 200; y++) {
				scrntype_t* d0 = emu->get_screen_buffer(y * 2);
				scrntype_t* d1 = emu->get_screen_buffer(y * 2 + 1);
				if(!d0 || !d1) continue;
				uint8_t* src = RAM + ((sr_vram_ofs + y * 320) & 0xffff);
				for(int x = 0; x < 320; x++) {
					uint8_t b = src[x];
					scrntype_t c0 = palette_pc[(b >> 4) & 0x0f];
					scrntype_t c1 = palette_pc[b & 0x0f];
					d0[x * 2]     = d1[x * 2]     = c0;
					d0[x * 2 + 1] = d1[x * 2 + 1] = c1;
				}
			}
			return;
		}
	}
#endif

	if(n60_mode) {
		// N60 Compatible Mode (32x16 text or 256x192 / 128x192 graphics -> scaled to 640x400)
		uint8_t* vbase = RAM + vram_head_n60[page];
		uint8_t mode_attr = vbase[0];
		bool is_ag = (mode_attr & 0x80) ? true : false;
		uint8_t css = (mode_attr & 0x02) ? 1 : 0;

		for(int y = 0; y < SCREEN_HEIGHT; y++) {
			scrntype_t* dest = emu->get_screen_buffer(y);
			if(dest) memset(dest, 0, SCREEN_WIDTH * sizeof(scrntype_t));
		}

		if(!is_ag) {
			// N60 Text / Semi-graphics (32x16 chars, 8x12 font -> 16x24 on 640x400)
			const uint8_t* font = CGROM1;
			for(int row = 0; row < 16; row++) {
				for(int col = 0; col < 32; col++) {
					uint8_t attr = vbase[row * 32 + col];
					uint8_t ch = vbase[0x200 + row * 32 + col];
					bool inv = (attr & 0x01) ? true : false;
					scrntype_t fg = palette_pc[css ? 10 : 15];
					scrntype_t bg = palette_pc[0];
					if(inv) { scrntype_t tmp = fg; fg = bg; bg = tmp; }

					for(int py = 0; py < 12; py++) {
						uint8_t pat = font[ch * 16 + py];
						int sy = 8 + (row * 12 + py) * 2;
						scrntype_t* d0 = emu->get_screen_buffer(sy);
						scrntype_t* d1 = emu->get_screen_buffer(sy + 1);
						if(!d0 || !d1) continue;
						for(int px = 0; px < 8; px++) {
							scrntype_t c = (pat & (0x80 >> px)) ? fg : bg;
							int sx = 64 + (col * 8 + px) * 2;
							d0[sx] = d0[sx + 1] = d1[sx] = d1[sx + 1] = c;
						}
					}
				}
			}
		} else {
			// N60 Graphics (256x192 2-color or 128x192 4-color)
			bool is_2col = ((mode_attr & 0x1c) == 0x1c);
			uint8_t* gbase = vbase + 0x200;
			for(int y = 0; y < 192; y++) {
				int sy = 8 + y * 2;
				scrntype_t* d0 = emu->get_screen_buffer(sy);
				scrntype_t* d1 = emu->get_screen_buffer(sy + 1);
				if(!d0 || !d1) continue;
				for(int bx = 0; bx < 32; bx++) {
					uint8_t b = gbase[y * 32 + bx];
					if(is_2col) {
						scrntype_t fg = palette_pc[css ? 15 : 10];
						scrntype_t bg = palette_pc[0];
						for(int px = 0; px < 8; px++) {
							scrntype_t c = (b & (0x80 >> px)) ? fg : bg;
							int sx = 64 + (bx * 8 + px) * 2;
							d0[sx] = d0[sx + 1] = d1[sx] = d1[sx + 1] = c;
						}
					} else {
						static const uint8_t ctab[2][4] = { { 10, 11, 4, 9 }, { 15, 14, 13, 1 } };
						for(int px = 0; px < 4; px++) {
							uint8_t ci = (b >> (6 - px * 2)) & 3;
							scrntype_t c = palette_pc[ctab[css][ci]];
							int sx = 64 + (bx * 8 + px * 2) * 2;
							d0[sx] = d0[sx + 1] = d0[sx + 2] = d0[sx + 3] = c;
							d1[sx] = d1[sx + 1] = d1[sx + 2] = d1[sx + 3] = c;
						}
					}
				}
			}
		}
	} else {
		// N60m (mk2) Mode
		uint8_t* vbase = RAM + vram_head_mk2[page];
		if(!gfx_mode) {
			// N60m Text Mode (40x20 or 80x20, 8x16 font -> 640x320/400)
			int cols = w80_mode ? 80 : 40;
			int char_w = w80_mode ? 1 : 2;
			const uint8_t* font = CGROM1 + 0x2000;

			for(int y = 0; y < SCREEN_HEIGHT; y++) {
				scrntype_t* dest = emu->get_screen_buffer(y);
				if(dest) memset(dest, 0, SCREEN_WIDTH * sizeof(scrntype_t));
			}

			for(int row = 0; row < 20; row++) {
				for(int col = 0; col < cols; col++) {
					uint8_t attr = vbase[row * cols + col];
					uint8_t ch   = vbase[0x800 + row * cols + col];
					scrntype_t fg = palette_pc[(attr & 0x07) ? ((attr & 0x07) + 8) : 0];
					scrntype_t bg = palette_pc[((attr >> 4) & 0x07) ? (((attr >> 4) & 0x07) + 8) : 0];
					if(attr & 0x08) { scrntype_t tmp = fg; fg = bg; bg = tmp; }

					for(int py = 0; py < 16; py++) {
						uint8_t pat = font[ch * 16 + py];
						int sy = 40 + row * 16 + py;
						scrntype_t* dest = emu->get_screen_buffer(sy);
						if(!dest) continue;
						for(int px = 0; px < 8; px++) {
							scrntype_t c = (pat & (0x80 >> px)) ? fg : bg;
							int sx = (col * 8 + px) * char_w;
							dest[sx] = c;
							if(char_w == 2) dest[sx + 1] = c;
						}
					}
				}
			}
		} else {
			// N60m Graphics Mode: Mode 5 (320x200 15-color) or Mode 6 (640x200 4-color)
			if(!w80_mode) {
				// Mode 5: 320x200 15-color (2 pixels per byte across 2 planes)
				for(int y = 0; y < 200; y++) {
					scrntype_t* d0 = emu->get_screen_buffer(y * 2);
					scrntype_t* d1 = emu->get_screen_buffer(y * 2 + 1);
					if(!d0 || !d1) continue;
					for(int bx = 0; bx < 80; bx++) {
						uint8_t p0 = vbase[y * 80 + bx];
						uint8_t p1 = vbase[0x4000 + y * 80 + bx];
						for(int px = 0; px < 4; px++) {
							int shift = 6 - px * 2;
							uint8_t ci = (((p1 >> shift) & 3) << 2) | ((p0 >> shift) & 3);
							scrntype_t c = palette_pc[ci];
							int sx = (bx * 4 + px) * 2;
							d0[sx] = d0[sx + 1] = d1[sx] = d1[sx + 1] = c;
						}
					}
				}
			} else {
				// Mode 6: 640x200 (8 pixels per byte)
				for(int y = 0; y < 200; y++) {
					scrntype_t* d0 = emu->get_screen_buffer(y * 2);
					scrntype_t* d1 = emu->get_screen_buffer(y * 2 + 1);
					if(!d0 || !d1) continue;
					for(int bx = 0; bx < 80; bx++) {
						uint8_t p0 = vbase[y * 80 + bx];
						uint8_t p1 = vbase[0x4000 + y * 80 + bx];
						for(int px = 0; px < 8; px++) {
							int shift = 7 - px;
							uint8_t ci = (((p1 >> shift) & 1) << 1) | ((p0 >> shift) & 1);
							scrntype_t c = palette_pc[ci];
							int sx = bx * 8 + px;
							d0[sx] = d1[sx] = c;
						}
					}
				}
			}
		}
	}
#endif
}

#define STATE_VERSION	2

bool MEMORY::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) return false;
	if(!state_fio->StateCheckInt32(this_device_id)) return false;
	state_fio->StateArray(RAM, RAM_SIZE, 1);
	state_fio->StateValue(CGSW93);
	state_fio->StateValue(CRTKILL);
	state_fio->StateValue(inserted);
	state_fio->StateValue(portB0);
	state_fio->StateValue(portC0);
	state_fio->StateValue(portC1);
	state_fio->StateValue(portC2);
	state_fio->StateValue(portC3);
	state_fio->StateValue(portC8);
	state_fio->StateValue(portC9);
	state_fio->StateValue(portCA);
	state_fio->StateValue(portF0);
	state_fio->StateValue(portF1);
	state_fio->StateValue(portF2);
	state_fio->StateArray(port40, sizeof(port40), 1);
	state_fio->StateArray(port50, sizeof(port50), 1);
	state_fio->StateArray(port60, sizeof(port60), 1);
	if(loading) {
		update_bank();
		update_palette();
	}
	return true;
}
