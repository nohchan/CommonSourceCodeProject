/*
	ASCII MSX1 Emulator 'yaMSX1'
	ASCII MSX2 Emulator 'yaMSX2'
	Pioneer PX-7 Emulator 'ePX-7'

	Author : tanam
	Date   : 2013.06.29-

	modified by Takeda.Toshiya
	modified by umaiboux

	[ joystick ]
*/

#include "joystick.h"
#include "../ay_3_891x.h"

#if defined(MSX_KEYBOARD_50ON)
#define PSG14_MASK 0x3f
#else
#define PSG14_MASK 0x7f
#endif

static uint8_t get_safe_joy(uint32_t raw, int select)
{
	// ポート2(select == 1)は常に未入力扱いにして誤作動を防ぐ
	if(select != 0) {
		return PSG14_MASK;
	}
	uint8_t val = raw & 0x3f;
	// 上下同時(0x03)や左右同時(0x0c)などの異常値（未初期化・センサー暴走）は弾く
	if((val & 0x03) == 0x03 || (val & 0x0c) == 0x0c) {
		val = 0;
	}
	return PSG14_MASK & ~val;
}

void JOYSTICK::initialize()
{
	joy_stat = emu->get_joy_buffer();
	select = 0;
	
	// 起動直後のポートAを「何も押されていない状態(High)」に初期化
	d_psg->write_signal(SIG_AY_3_891X_PORT_A, PSG14_MASK, 0x7f);
	
	// register event to update the key status
	register_frame_event(this);
}

void JOYSTICK::event_frame()
{
	d_psg->write_signal(SIG_AY_3_891X_PORT_A, get_safe_joy(joy_stat[select], select), 0x7f);
}

void JOYSTICK::write_signal(int id, uint32_t data, uint32_t mask)
{
	if(id == SIG_JOYSTICK_SEL) {
		if(select != ((data & mask) != 0)) {
			select = ((data & mask) != 0);
			d_psg->write_signal(SIG_AY_3_891X_PORT_A, get_safe_joy(joy_stat[select], select), 0x7f);
		}
	}
}

#define STATE_VERSION	1

bool JOYSTICK::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(select);
	return true;
}
