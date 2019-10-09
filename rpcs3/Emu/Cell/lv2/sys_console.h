#pragma once

#include "Emu/Memory/vm_ptr.h"

error_code sys_console_write(ppu_thread& ppu, vm::cptr<char> buf, u32 len);
