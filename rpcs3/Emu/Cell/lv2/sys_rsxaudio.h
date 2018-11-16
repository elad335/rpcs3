#pragma once

error_code sys_rsxaudio_initialize(vm::ptr<u32>);
error_code sys_rsxaudio_import_shared_memory(u32, vm::ptr<u64>);
