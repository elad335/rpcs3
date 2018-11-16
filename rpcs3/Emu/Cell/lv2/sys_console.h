#pragma once


error_code sys_console_write(ppu_thread& ppu, vm::cptr<char> buf, u32 len);
