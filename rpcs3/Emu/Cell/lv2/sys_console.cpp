#include "stdafx.h"
#include "Emu/System.h"

#include "Emu/Memory/vm_var.h"
#include "Emu/Cell/PPUThread.h"
#include "sys_tty.h"

#include "sys_console.h"


logs::channel sys_console("sys_console");

error_code sys_console_write(ppu_thread& ppu, vm::cptr<char> buf, u32 len)
{
	sys_console.todo("sys_console_write: buf=%s, len=0x%x", buf, len);

	// to make this easier to spot, also piping to tty
	std::string tmp(buf.get_ptr(), len);
	tmp = "CONSOLE: " + tmp;
	auto tty = vm::make_str(tmp);
	vm::var<u32> out(0);
	sys_tty_write(0, tty, tmp.size(), +out);

	return CELL_OK;
}
