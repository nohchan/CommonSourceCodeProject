/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6001mkII Emulator 'yaPC-6201'
	NEC PC-6001mkIISR Emulator 'yaPC-6401'
	NEC PC-6601 Emulator 'yaPC-6601'
	NEC PC-6601SR Emulator 'yaPC-6801'

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MEMORY_PIO_PORT_C	0

// memory offset
#define RAM_BASE		0
#define RAM_SIZE		0x10000
#define BASICROM_BASE	(RAM_BASE + RAM_SIZE)
#define BASICROM_SIZE	0x10000
#define EXTROM_BASE		(BASICROM_BASE + BASICROM_SIZE)
#define EXTROM_SIZE		0x4000
#define CGROM1_BASE		(EXTROM_BASE + EXTROM_SIZE)
#define CGROM1_SIZE		0x4000
#define KANJIROM_BASE	(CGROM1_BASE + CGROM1_SIZE)
#define KANJIROM_SIZE	0x8000
#define VOICEROM_BASE	(KANJIROM_BASE + KANJIROM_SIZE)
#define VOICEROM_SIZE	0x4000
#define SYSROM2_BASE	(VOICEROM_BASE + VOICEROM_SIZE)
#define SYSROM2_SIZE	0x10000
#define EmptyRAM_BASE	(SYSROM2_BASE + SYSROM2_SIZE)
#define EmptyRAM_SIZE	0x2000

#define MEMORY_SIZE		(EmptyRAM_BASE + EmptyRAM_SIZE)

class TIMER;

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu;
	TIMER *d_timer;

	uint8_t MEMORY_BASE[MEMORY_SIZE];
	uint8_t *CGROM;
	uint8_t *EXTROM1;
	uint8_t *EXTROM2;
	uint8_t *VRAM;

	uint8_t *read_bank[8];
	uint8_t *write_bank[8];
	bool write_enable[8];

	uint8_t CGSW93;
	uint8_t CRTKILL;
	bool inserted;

	// mk2 / 6601 / SR registers
	uint8_t portB0;
	uint8_t portC0;
	uint8_t portC1;
	uint8_t portC2;
	uint8_t portC3;
	uint8_t portC8;
	uint8_t portC9;
	uint8_t portCA;
	uint8_t portF0;
	uint8_t portF1;
	uint8_t portF2;

	// SR registers
	uint8_t port40[16];
	uint8_t port50[16];
	uint8_t port60[16];

	scrntype_t palette_pc[16];

	void update_bank();
	void update_palette();

public:
	MEMORY(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		d_cpu = NULL;
		d_timer = NULL;
		inserted = false;
		set_device_name(_T("Memory Bus"));
	}
	~MEMORY() {}

	// common functions
	void initialize();
	void reset();
	void write_data8(uint32_t addr, uint32_t data);
	uint32_t read_data8(uint32_t addr);
	void write_data8w(uint32_t addr, uint32_t data, int *wait);
	uint32_t read_data8w(uint32_t addr, int *wait);
	uint32_t fetch_op(uint32_t addr, int *wait);
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	void write_io8w(uint32_t addr, uint32_t data, int* wait);
	uint32_t read_io8w(uint32_t addr, int* wait);
	void event_vline(int v, int clock);
	void event_callback(int event_id, int err);
	void write_signal(int id, uint32_t data, uint32_t mask);
	bool process_state(FILEIO* state_fio, bool loading);

	// unique functions
	void set_context_cpu(DEVICE* device)
	{
		d_cpu = device;
	}
	void set_context_timer(TIMER* device)
	{
		d_timer = device;
	}
	void open_cart(const _TCHAR* file_path);
	void close_cart();
	bool is_cart_inserted()
	{
		return inserted;
	}
	uint8_t* get_vram()
	{
		return MEMORY_BASE + RAM_BASE;
	}
	void draw_screen();
};

#endif
