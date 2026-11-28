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

public:
	FLOPPY(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		for(int i = 0; i < 2; i++) {
			disk[i] = new DISK(emu);
		}
		set_device_name(_T("Internal FDD"));
	}
	~FLOPPY() {}

	void release()
	{
		for(int i = 0; i < 2; i++) {
			if(disk[i]) {
				disk[i]->close();
				delete disk[i];
			}
		}
	}

	void set_context_noise_seek(NOISE* device) {}
	void set_context_ext(DEVICE* device) {}
	DISK* get_disk_handler(int drv)
	{
		return disk[drv];
	}
	void open_disk(int drv, const _TCHAR* file_path, int bank) {}
	void close_disk(int drv) {}
	bool is_disk_inserted(int drv)
	{
		return false;
	}
	void is_disk_protected(int drv, bool value) {}
	bool is_disk_protected(int drv)
	{
		return false;
	}
};

#endif
