#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"

#include "Emu/Cell/ErrorCodes.h"

#include "sys_rsxaudio.h"


logs::channel sys_rsxaudio("sys_rsxaudio");

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle)
{
	sys_rsxaudio.todo("sys_rsxaudio_initialize()");
	*handle = 0xcacad0d0;
	return CELL_OK;
}

error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> b)
{
	sys_rsxaudio.todo("sys_rsxaudio_import_shared_memory(handle=0x%x, *0x%x)", handle, b);
	b[0] = vm::alloc(0x40000, vm::main);
	return CELL_OK;
}
