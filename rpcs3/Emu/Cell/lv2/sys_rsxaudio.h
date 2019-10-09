#pragma once

#include "Emu/Memory/vm_ptr.h"

error_code sys_rsxaudio_initialize(vm::ptr<u32>);
error_code sys_rsxaudio_import_shared_memory(u32, vm::ptr<u64>);
