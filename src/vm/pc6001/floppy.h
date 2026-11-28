/*
	NEC PC-6601 Emulator 'yaPC-6601'
	NEC PC-6601SR Emulator 'yaPC-6801'

	[ internal floppy disk drive ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"
#include "../disk.h"

class NOISE;

class FLOPPY : public DEVICE
{
private:
	DISK* disk[2];
	NOISE* d_noise_seek;
	DEVICE* d_ext;

	// 1KB FDD Buffer RAM
	uint8_t fdd_buf[1024];

	// Command / Result Buffer
	struct {
		uint8_t data[10];
		int index;
	} cmd_in, cmd_out;

	// uPD765A FDC State
	enum {
		SK_STOP = 0,
		SK_SEEK,
		SK_END
	};

	struct {
		uint8_t command;
		uint8_t srt;
		uint8_t hut;
		uint8_t hlt;
		bool nd;

		int seek_sta[4];
		uint8_t ncn[4];
		uint8_t pcn[4];

		uint8_t mt, mf, sk, hd, us;
		uint8_t c, h, r, n, eot, gpl, dtl;
		uint8_t st0, st1, st2, st3;
		uint8_t status;
		bool intr;
	} fdc;

	// FDCI State
	int send_bytes;
	bool ext_drv;
	bool b2_dir;
	int seek_event_id[4];
	int exwait_event_id;

	void buf_write(int addr, uint8_t data);
	uint8_t buf_read(int addr);

	void push_status(uint8_t data);
	uint8_t pop_status();

	void out_fdc(uint8_t data);
	uint8_t in_fdc();
	void exec_fdc();

	bool search_sector(uint8_t* status);

	void cmd_read_diagnostic();
	void cmd_specify();
	void cmd_sense_device_status();
	void cmd_read_data();
	void cmd_write_data();
	void cmd_recalibrate();
	void cmd_seek();
	void cmd_sense_interrupt_status();

public:
	FLOPPY(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		for(int i = 0; i < 2; i++) {
			disk[i] = new DISK(emu);
		}
		d_noise_seek = NULL;
		d_ext = NULL;
		set_device_name(_T("Internal FDD"));
	}
	~FLOPPY() {}

	// common functions
	void initialize();
	void reset();
	void release();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	uint32_t read_signal(int id);
	void event_callback(int event_id, int err);
	bool process_state(FILEIO* state_fio, bool loading);

	// unique functions
	void set_context_noise_seek(NOISE* device)
	{
		d_noise_seek = device;
	}
	void set_context_ext(DEVICE* device)
	{
		d_ext = device;
	}
	DISK* get_disk_handler(int drv)
	{
		return (drv < 2) ? disk[drv] : NULL;
	}
	void open_disk(int drv, const _TCHAR* file_path, int bank);
	void close_disk(int drv);
	bool is_disk_inserted(int drv);
	void is_disk_protected(int drv, bool value);
	bool is_disk_protected(int drv);
};

#endif
