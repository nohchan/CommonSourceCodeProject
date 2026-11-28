#include "floppy.h"
#include "../noise.h"

#define EVENT_SEEK1		0
#define EVENT_SEEK2		1
#define EVENT_SEEK3		2
#define EVENT_SEEK4		3
#define EVENT_EXWAIT	4

#define FDC_BUSY			0x10
#define FDC_FD2PC			0x40
#define FDC_DATA_READY		0x80

#define ST0_NOT_READY		0x08
#define ST0_SEEK_END		0x20
#define ST0_IC_NT			0x00
#define ST0_IC_AT			0x40
#define ST0_IC_IC			0x80

#define ST1_MISSING_AM		0x01
#define ST1_NOT_WRITABLE	0x02
#define ST1_NO_DATA			0x04
#define ST1_DATA_ERROR		0x20

#define ST2_MA_IN_DATA_FIELD	0x01
#define ST2_NO_CYLINDER			0x10
#define ST2_DE_IN_DATA_FIELD	0x20

#define ST3_TRACK0			0x10
#define ST3_READY			0x20
#define ST3_WRITE_PROTECT	0x40

#define WAIT_SEEK			2000
#define WAIT_TRACK			(60000000 / 360)
#define WAIT_BYTE			(WAIT_TRACK / 6250)

static const int Gap3size[] = { 26, 54, 84, 116, 150, 186, 224, 264 };
#define WAIT_DATA(n)		(WAIT_BYTE * (12 + 4 + (128 << (n)) + 2 + Gap3size[(n) & 7]))

void FLOPPY::initialize()
{
	memset(fdd_buf, 0, sizeof(fdd_buf));
	for(int i = 0; i < 4; i++) seek_event_id[i] = -1;
	exwait_event_id = -1;
	fdc.srt = 5; fdc.hut = 240; fdc.hlt = 38; fdc.nd = false;
}

void FLOPPY::reset()
{
	memset(&cmd_in, 0, sizeof(cmd_in));
	memset(&cmd_out, 0, sizeof(cmd_out));
	for(int i = 0; i < 4; i++) {
		fdc.ncn[i] = fdc.pcn[i] = 0;
		fdc.seek_sta[i] = SK_STOP;
		if(seek_event_id[i] != -1) { cancel_event(this, seek_event_id[i]); seek_event_id[i] = -1; }
	}
	if(exwait_event_id != -1) { cancel_event(this, exwait_event_id); exwait_event_id = -1; }
	fdc.us = 0; fdc.status = FDC_DATA_READY; fdc.intr = false;
	send_bytes = 0; ext_drv = false; b2_dir = false;
}

void FLOPPY::release()
{
	for(int i = 0; i < 2; i++) {
		if(disk[i]) { disk[i]->close(); delete disk[i]; disk[i] = NULL; }
	}
}

void FLOPPY::event_callback(int event_id, int err)
{
	if(event_id >= EVENT_SEEK1 && event_id <= EVENT_SEEK4) {
		int drv = event_id - EVENT_SEEK1;
		seek_event_id[drv] = -1;
		fdc.seek_sta[drv] = SK_END;
		fdc.pcn[drv] = fdc.ncn[drv];
		fdc.intr = true;
		fdc.status |= (1 << drv);
	} else if(event_id == EVENT_EXWAIT) {
		exwait_event_id = -1;
		fdc.intr = true;
		fdc.status |= FDC_DATA_READY;
	}
}

uint32_t FLOPPY::read_signal(int id)
{
	uint32_t status = 0;
	for(int drv = 0; drv < 2; drv++) {
		if((fdc.seek_sta[drv] == SK_SEEK) || ((fdc.status & FDC_BUSY) && (fdc.us == drv))) status |= (1 << drv);
	}
	return status;
}

void FLOPPY::buf_write(int addr, uint8_t data) { fdd_buf[addr & 0x3ff] = data; }
uint8_t FLOPPY::buf_read(int addr) { return fdd_buf[addr & 0x3ff]; }
void FLOPPY::push_status(uint8_t data) { if(cmd_out.index < 10) cmd_out.data[cmd_out.index++] = data; }
uint8_t FLOPPY::pop_status() { return (cmd_out.index > 0) ? cmd_out.data[--cmd_out.index] : 0xff; }

void FLOPPY::out_fdc(uint8_t data)
{
	const int cmd_length[] = { 0, 0, 9, 3, 2, 9, 9, 2, 1, 0, 0, 0, 0, 6, 0, 3 };
	fdc.intr = false;
	if(!(fdc.status & FDC_FD2PC)) {
		if(cmd_in.index < 10) cmd_in.data[cmd_in.index++] = data;
		if(cmd_length[cmd_in.data[0] & 0x0f] == cmd_in.index) {
			fdc.status = FDC_FD2PC | FDC_BUSY | (fdc.status & 0x0f);
			exec_fdc();
		} else {
			fdc.status = FDC_DATA_READY | FDC_BUSY | (fdc.status & 0x0f);
		}
	}
}

uint8_t FLOPPY::in_fdc()
{
	fdc.intr = false;
	if(fdc.status & FDC_FD2PC) {
		fdc.status = (cmd_out.index == 1) ? (FDC_DATA_READY | (fdc.status & 0x0f))
		                                  : (FDC_DATA_READY | FDC_FD2PC | FDC_BUSY | (fdc.status & 0x0f));
		return pop_status();
	}
	return 0xff;
}

void FLOPPY::exec_fdc()
{
	cmd_out.index = 0;
	fdc.command = cmd_in.data[0] & 0x1f;
	switch(fdc.command) {
	case 0x02: cmd_read_diagnostic(); break;
	case 0x03: cmd_specify(); break;
	case 0x04: cmd_sense_device_status(); break;
	case 0x05: case 0x09: cmd_write_data(); break;
	case 0x06: case 0x0c: cmd_read_data(); break;
	case 0x07: cmd_recalibrate(); break;
	case 0x08: cmd_sense_interrupt_status(); break;
	case 0x0f: cmd_seek(); break;
	default:
		push_status(ST0_IC_IC);
		fdc.status = FDC_DATA_READY | FDC_FD2PC | (fdc.status & 0x0f);
		break;
	}
	cmd_in.index = 0;
}

bool FLOPPY::search_sector(uint8_t* status)
{
	int drv = fdc.us;
	if(drv >= 2 || !disk[drv]->inserted) {
		*status = 0x01; fdc.st0 = ST0_IC_AT | ST0_NOT_READY; fdc.st1 = ST1_MISSING_AM; fdc.st2 = 0;
		return false;
	}
#if defined(_PC6601SR)
	int phys_trk = fdc.pcn[drv] * (disk[drv]->media_type == MEDIA_TYPE_2D ? 2 : 1);
#else
	int phys_trk = fdc.pcn[drv] * 2;
#endif
	int side = fdc.hd & 1;
	if(!disk[drv]->get_track(phys_trk, side)) {
		*status = 0x01; fdc.st0 = ST0_IC_AT; fdc.st1 = ST1_MISSING_AM; fdc.st2 = 0;
		return false;
	}
	for(int i = 0; i < disk[drv]->sector_num.sd; i++) {
		if(disk[drv]->get_sector(phys_trk, side, i)) {
			if(disk[drv]->id[0] == fdc.c && disk[drv]->id[1] == fdc.h &&
			   disk[drv]->id[2] == fdc.r && disk[drv]->id[3] == fdc.n) {
				*status = disk[drv]->status;
				if(*status == 0xe0 || *status == 0xf0) {
					fdc.st0 = ST0_IC_AT; fdc.st1 = ST1_MISSING_AM; fdc.st2 = ST2_MA_IN_DATA_FIELD;
				} else if(*status == 0xa0) {
					fdc.st0 = ST0_IC_AT; fdc.st1 = ST1_DATA_ERROR; fdc.st2 = 0;
				} else if(*status == 0xb0) {
					fdc.st0 = ST0_IC_AT; fdc.st1 = ST1_DATA_ERROR; fdc.st2 = ST2_DE_IN_DATA_FIELD;
				} else {
					fdc.st0 = ST0_IC_NT; fdc.st1 = 0; fdc.st2 = 0;
				}
				return true;
			}
		}
	}
	*status = 0x04; fdc.st0 = ST0_IC_AT; fdc.st1 = ST1_NO_DATA;
	fdc.st2 = (fdc.c != fdc.pcn[drv]) ? ST2_NO_CYLINDER : 0;
	return false;
}
void FLOPPY::cmd_read_diagnostic()
{
	int wait_us = WAIT_TRACK;

	fdc.mt  = (cmd_in.data[0] & 0x80) >> 7;
	fdc.mf  = (cmd_in.data[0] & 0x40) >> 6;
	fdc.sk  = (cmd_in.data[0] & 0x20) >> 5;
	fdc.hd  = (cmd_in.data[1] & 0x04) >> 2;
	fdc.us  =  cmd_in.data[1] & 0x03;
	fdc.c   =  cmd_in.data[2];
	fdc.h   =  cmd_in.data[3];
	fdc.r   =  cmd_in.data[4];
	fdc.n   =  cmd_in.data[5];
	fdc.eot =  cmd_in.data[6];
	fdc.gpl =  cmd_in.data[7];
	fdc.dtl =  cmd_in.data[8];
	fdc.st0 = fdc.st1 = fdc.st2 = 0;

	if(fdc.us < 2 && disk[fdc.us]->inserted) {
#if defined(_PC6601SR)
		int phys_trk = fdc.pcn[fdc.us] * (disk[fdc.us]->media_type == MEDIA_TYPE_2D ? 2 : 1);
#else
		int phys_trk = fdc.pcn[fdc.us] * 2;
#endif
		if(disk[fdc.us]->get_sector(phys_trk, fdc.hd & 1, 0)) {
			int secsize = disk[fdc.us]->sector_size.sd;
			int gap3size = Gap3size[(secsize >> 8) & 7];
			int bufad = 0;

			for(int i = 0; i < secsize; i++) buf_write(bufad++, disk[fdc.us]->sector[i]);
			bufad += 2;
			for(int i = 0; i < gap3size; i++) buf_write(bufad++, 0x4e);
			for(int i = 0; i < 12; i++) buf_write(bufad++, 0x00);

			fdc.c = disk[fdc.us]->id[0];
			fdc.h = disk[fdc.us]->id[1];
			fdc.r = disk[fdc.us]->id[2];
			fdc.n = disk[fdc.us]->id[3];
			wait_us = WAIT_DATA((secsize >> 8) & 7);
		} else {
			fdc.st0 = ST0_IC_AT;
			fdc.st1 = ST1_MISSING_AM;
		}
	} else {
		fdc.st0 = ST0_NOT_READY;
	}

	fdc.st0 |= fdc.us;
	push_status(fdc.n);
	push_status(fdc.r);
	push_status(fdc.h);
	push_status(fdc.c);
	push_status(fdc.st2);
	push_status(fdc.st1);
	push_status(fdc.st0);

	fdc.status = FDC_FD2PC | FDC_BUSY | (fdc.status & 0x0f);
	fdc.intr = false;

	if(exwait_event_id != -1) cancel_event(this, exwait_event_id);
	register_event(this, EVENT_EXWAIT, wait_us, false, &exwait_event_id);
}

void FLOPPY::cmd_specify()
{
	fdc.srt = 16 - ((cmd_in.data[1] >> 4) & 0x0f);
	fdc.hut = (cmd_in.data[1] & 0x0f) * 16;
	fdc.hlt = (cmd_in.data[2] >> 1) * 2;
	fdc.nd  = (cmd_in.data[2] & 1) ? true : false;
	fdc.status = FDC_DATA_READY | (fdc.status & 0x0f);
}

void FLOPPY::cmd_sense_device_status()
{
	fdc.hd = (cmd_in.data[1] & 0x04) >> 2;
	fdc.us =  cmd_in.data[1] & 0x03;

	fdc.st3 = fdc.us | (fdc.hd << 2);
	if(fdc.us < 2 && disk[fdc.us]->inserted) {
		fdc.st3 |= ST3_READY;
		if(fdc.pcn[fdc.us] == 0) fdc.st3 |= ST3_TRACK0;
		if(disk[fdc.us]->write_protected) fdc.st3 |= ST3_WRITE_PROTECT;
	}
	push_status(fdc.st3);
	fdc.status = FDC_DATA_READY | FDC_FD2PC | (fdc.status & 0x0f);
}

void FLOPPY::cmd_read_data()
{
	uint8_t imgsta = 0;
	int wait_us = 100;

	fdc.mt  = (cmd_in.data[0] & 0x80) >> 7;
	fdc.mf  = (cmd_in.data[0] & 0x40) >> 6;
	fdc.sk  = (cmd_in.data[0] & 0x20) >> 5;
	fdc.hd  = (cmd_in.data[1] & 0x04) >> 2;
	fdc.us  =  cmd_in.data[1] & 0x03;
	fdc.c   =  cmd_in.data[2];
	fdc.h   =  cmd_in.data[3];
	fdc.r   =  cmd_in.data[4];
	fdc.n   =  cmd_in.data[5];
	fdc.eot =  cmd_in.data[6];
	fdc.gpl =  cmd_in.data[7];
	fdc.dtl =  cmd_in.data[8];
	fdc.st0 = fdc.st1 = fdc.st2 = 0;

	if(fdc.us < 2 && disk[fdc.us]->inserted) {
		int i = 0;
		do {
			if(search_sector(&imgsta)) {
				int secbyte = disk[fdc.us]->sector_size.sd;
				wait_us += WAIT_DATA(fdc.n);
				for(int b = 0; b < secbyte; b++) buf_write(i++, disk[fdc.us]->sector[b]);
			} else {
				break;
			}
			if(fdc.r == fdc.eot) {
				fdc.c++;
				fdc.r = 1;
				i = send_bytes;
			} else {
				fdc.r++;
			}
		} while((send_bytes > i) && (fdc.st0 == ST0_IC_NT));
	} else {
		fdc.st0 = ST0_NOT_READY;
	}

	fdc.st0 |= fdc.us;
	push_status(fdc.n);
	push_status(fdc.r);
	push_status(fdc.h);
	push_status(fdc.c);
	push_status(fdc.st2);
	push_status(fdc.st1);
	push_status(fdc.st0);

	fdc.status = FDC_FD2PC | FDC_BUSY | (fdc.status & 0x0f);
	fdc.intr = false;

	if(exwait_event_id != -1) cancel_event(this, exwait_event_id);
	register_event(this, EVENT_EXWAIT, wait_us, false, &exwait_event_id);
}

void FLOPPY::cmd_write_data()
{
	uint8_t imgsta = 0;
	int wait_us = 100;

	fdc.mt  = (cmd_in.data[0] & 0x80) >> 7;
	fdc.mf  = (cmd_in.data[0] & 0x40) >> 6;
	fdc.sk  = (cmd_in.data[0] & 0x20) >> 5;
	fdc.hd  = (cmd_in.data[1] & 0x04) >> 2;
	fdc.us  =  cmd_in.data[1] & 0x03;
	fdc.c   =  cmd_in.data[2];
	fdc.h   =  cmd_in.data[3];
	fdc.r   =  cmd_in.data[4];
	fdc.n   =  cmd_in.data[5];
	fdc.eot =  cmd_in.data[6];
	fdc.gpl =  cmd_in.data[7];
	fdc.dtl =  cmd_in.data[8];
	fdc.st0 = fdc.st1 = fdc.st2 = 0;

	if(fdc.us < 2 && disk[fdc.us]->inserted) {
		if(disk[fdc.us]->write_protected) {
			fdc.st0 = ST0_IC_AT;
			fdc.st1 = ST1_NOT_WRITABLE;
		} else {
			int i = 0;
			do {
				if(search_sector(&imgsta)) {
					int secsize = disk[fdc.us]->sector_size.sd;
					int secbyte = (secsize < (send_bytes - i)) ? secsize : (send_bytes - i);
					wait_us += WAIT_DATA(fdc.n);
					for(int b = 0; b < secbyte; b++) disk[fdc.us]->sector[b] = buf_read(i++);
					disk[fdc.us]->status = 0x00;
				} else {
					break;
				}
				if(fdc.r == fdc.eot) {
					fdc.c++;
					fdc.r = 1;
					i = send_bytes;
				} else {
					fdc.r++;
				}
			} while((send_bytes > i) && (fdc.st0 == ST0_IC_NT));
		}
	} else {
		fdc.st0 = ST0_NOT_READY;
	}

	fdc.st0 |= fdc.us;
	push_status(fdc.n);
	push_status(fdc.r);
	push_status(fdc.h);
	push_status(fdc.c);
	push_status(fdc.st2);
	push_status(fdc.st1);
	push_status(fdc.st0);

	fdc.status = FDC_FD2PC | FDC_BUSY | (fdc.status & 0x0f);
	fdc.intr = false;

	if(exwait_event_id != -1) cancel_event(this, exwait_event_id);
	register_event(this, EVENT_EXWAIT, wait_us, false, &exwait_event_id);
}

void FLOPPY::cmd_recalibrate()
{
	cmd_in.data[2] = 0;
	cmd_seek();
}

void FLOPPY::cmd_seek()
{
	fdc.us = cmd_in.data[1] & 3;
	fdc.ncn[fdc.us] = cmd_in.data[2];

	if(fdc.us >= 2 || fdc.ncn[fdc.us] == fdc.pcn[fdc.us] || !disk[fdc.us]->inserted) {
		fdc.seek_sta[fdc.us] = SK_END;
		fdc.pcn[fdc.us] = fdc.ncn[fdc.us];
		fdc.intr = true;
	} else {
		int diff = abs((int)fdc.ncn[fdc.us] - (int)fdc.pcn[fdc.us]);
		fdc.seek_sta[fdc.us] = SK_SEEK;
		fdc.intr = false;
		fdc.status &= ~(1 << fdc.us);

		if(d_noise_seek && diff > 0) d_noise_seek->play();

		int wait_us = (diff * fdc.srt + fdc.hut + fdc.hlt) * WAIT_SEEK;
		if(seek_event_id[fdc.us] != -1) cancel_event(this, seek_event_id[fdc.us]);
		register_event(this, EVENT_SEEK1 + fdc.us, wait_us, false, &seek_event_id[fdc.us]);
	}
	fdc.status = FDC_DATA_READY | (fdc.status & 0x0f);
}

void FLOPPY::cmd_sense_interrupt_status()
{
	int drv;
	for(drv = 0; drv < 4; drv++) {
		if(fdc.seek_sta[drv] == SK_END) break;
	}

	if(drv >= 4) {
		push_status(ST0_IC_IC);
	} else {
		if(drv < 2 && disk[drv]->inserted) {
			fdc.st0 = ST0_IC_NT | ST0_SEEK_END | drv;
		} else {
			fdc.st0 = ST0_IC_AT | ST0_SEEK_END | ST0_NOT_READY | drv;
		}
		fdc.seek_sta[drv] = SK_STOP;
		fdc.status &= ~(1 << drv);

		push_status(fdc.pcn[drv]);
		push_status(fdc.st0);
	}
	fdc.status = FDC_DATA_READY | FDC_FD2PC | (fdc.status & 0x0f);
}

void FLOPPY::write_io8(uint32_t addr, uint32_t data)
{
	switch(addr & 0xff) {
	case 0xb1: case 0xb5: ext_drv = (data & 0x04) ? true : false; break;
	case 0xb3: case 0xb7: b2_dir = (data & 0x01) ? true : false; break;
	case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		if(ext_drv) { if(d_ext) d_ext->write_io8(addr, data); }
		else        { buf_write(((addr & 0xff) << 8) | ((addr >> 8) & 0xff), data); }
		break;
	case 0xda:
		send_bytes = ((~(data - 0x10)) & 0xff) * 256;
		if(send_bytes == 0) send_bytes = 256 * 256;
		break;
	case 0xdd: out_fdc(data); break;
	}
}

uint32_t FLOPPY::read_io8(uint32_t addr)
{
	switch(addr & 0xff) {
	case 0xb1: case 0xb5: return 0x01;
	case 0xb2: case 0xb6: return (!b2_dir && fdc.intr) ? 0xff : 0x0e;
	case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		if(ext_drv) return d_ext ? d_ext->read_io8(addr) : 0xff;
		return buf_read(((addr & 0xff) << 8) | ((addr >> 8) & 0xff));
	case 0xd4: return 0x00;
	case 0xdc: return fdc.status;
	case 0xdd: return in_fdc();
	}
	return 0xff;
}

void FLOPPY::open_disk(int drv, const _TCHAR* file_path, int bank) { if(drv < 2 && disk[drv]) disk[drv]->open(file_path, bank); }
void FLOPPY::close_disk(int drv) { if(drv < 2 && disk[drv]) disk[drv]->close(); }
bool FLOPPY::is_disk_inserted(int drv) { return (drv < 2 && disk[drv]) ? disk[drv]->inserted : false; }
void FLOPPY::is_disk_protected(int drv, bool value) { if(drv < 2 && disk[drv]) disk[drv]->write_protected = value; }
bool FLOPPY::is_disk_protected(int drv) { return (drv < 2 && disk[drv]) ? disk[drv]->write_protected : false; }

#define STATE_VERSION	1

bool FLOPPY::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) return false;
	if(!state_fio->StateCheckInt32(this_device_id)) return false;
	for(int i = 0; i < 2; i++) {
		if(!disk[i]->process_state(state_fio, loading)) return false;
	}
	state_fio->StateArray(fdd_buf, sizeof(fdd_buf), 1);
	state_fio->StateArray(cmd_in.data, sizeof(cmd_in.data), 1);
	state_fio->StateValue(cmd_in.index);
	state_fio->StateArray(cmd_out.data, sizeof(cmd_out.data), 1);
	state_fio->StateValue(cmd_out.index);

	state_fio->StateValue(fdc.command);
	state_fio->StateValue(fdc.srt);
	state_fio->StateValue(fdc.hut);
	state_fio->StateValue(fdc.hlt);
	state_fio->StateValue(fdc.nd);
	state_fio->StateArray(fdc.seek_sta, sizeof(fdc.seek_sta), 1);
	state_fio->StateArray(fdc.ncn, sizeof(fdc.ncn), 1);
	state_fio->StateArray(fdc.pcn, sizeof(fdc.pcn), 1);
	state_fio->StateValue(fdc.mt);
	state_fio->StateValue(fdc.mf);
	state_fio->StateValue(fdc.sk);
	state_fio->StateValue(fdc.hd);
	state_fio->StateValue(fdc.us);
	state_fio->StateValue(fdc.c);
	state_fio->StateValue(fdc.h);
	state_fio->StateValue(fdc.r);
	state_fio->StateValue(fdc.n);
	state_fio->StateValue(fdc.eot);
	state_fio->StateValue(fdc.gpl);
	state_fio->StateValue(fdc.dtl);
	state_fio->StateValue(fdc.st0);
	state_fio->StateValue(fdc.st1);
	state_fio->StateValue(fdc.st2);
	state_fio->StateValue(fdc.st3);
	state_fio->StateValue(fdc.status);
	state_fio->StateValue(fdc.intr);

	state_fio->StateValue(send_bytes);
	state_fio->StateValue(ext_drv);
	state_fio->StateValue(b2_dir);
    state_fio->StateArray(seek_event_id, sizeof(seek_event_id), 1);
    state_fio->StateValue(exwait_event_id);
    return true;
}
