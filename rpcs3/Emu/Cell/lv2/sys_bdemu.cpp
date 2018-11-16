#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_bdemu.h"


logs::channel sys_bdemu("sys_bdemu");

error_code sys_bdemu_send_command(u64 cmd, u64 a2, u64 a3, vm::ptr<void> buf, u64 buf_len)
{
	sys_bdemu.todo("cmd=0%x, a2=0x%x, a3=0x%x, buf=0x%x, buf_len=0x%x", cmd, a2, a3, buf, buf_len);
	// todo: only debug kernel has this
	return CELL_ENOSYS;
	/*if (cmd == 0)
	{
		auto out = vm::static_ptr_cast<u64>(buf);
		*out     = 0x101000000000008;
	}
	return CELL_OK;*/
}
