#include "stdafx.h"
#include "sys_ss.h"

#include "sys_process.h"
#include "Emu/Cell/PPUThread.h"


extern u64 get_timebased_time();

#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>

const HCRYPTPROV s_crypto_provider = []() -> HCRYPTPROV
{
	HCRYPTPROV result;

	if (!CryptAcquireContextW(&result, nullptr, nullptr, PROV_RSA_FULL, 0) && !CryptAcquireContextW(&result, nullptr, nullptr, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		return 0;
	}

	::atexit([]()
	{
		if (s_crypto_provider)
		{
			CryptReleaseContext(s_crypto_provider, 0);
		}
	});

	return result;
}();
#endif



LOG_CHANNEL(sys_ss);

error_code sys_ss_random_number_generator(u32 arg1, vm::ptr<void> buf, u64 size)
{
	sys_ss.warning("sys_ss_random_number_generator(arg1=%u, buf=*0x%x, size=0x%x)", arg1, buf, size);

	if (arg1 != 2)
	{
		return 0x80010509;
	}

	if (size > 0x10000000)
	{
		return 0x80010501;
	}

#ifdef _WIN32
	if (!s_crypto_provider || !CryptGenRandom(s_crypto_provider, size, (BYTE*)buf.get_ptr()))
	{
		return CELL_EABORT;
	}
#else
	fs::file rnd{"/dev/urandom"};

	if (!rnd || rnd.read(buf.get_ptr(), size) != size)
	{
		return CELL_EABORT;
	}
#endif

	return CELL_OK;
}

error_code sys_ss_access_control_engine(u64 pkg_id, u64 a2, u64 a3)
{
	sys_ss.todo("sys_ss_access_control_engine(pkg_id=0x%llx, a2=0x%llx, a3=0x%llx)", pkg_id, a2, a3);

	const u64 authid = g_ps3_process_info.self_info.valid ? 
		g_ps3_process_info.self_info.app_info.authid : 0;

	switch (pkg_id)
	{
	case 0x1:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return CELL_ENOSYS;
		}

		if (!a2)
		{
			return CELL_ESRCH;
		}

		verify(HERE), a2 == process_getpid();
		vm::_ref<u64>(vm::cast(a3)) = authid;
		break;
	}
	case 0x2:
	{
		vm::_ref<u64>(vm::cast(a2)) = authid;
		break;
	}
	case 0x3:
	{
		if (!g_ps3_process_info.debug_or_root())
		{
			return CELL_ENOSYS;
		}

		break;
	}
	default:
		return 0x8001051du;
	}

	return CELL_OK;
}

s32 sys_ss_get_console_id(vm::ptr<u8> buf)
{
	sys_ss.todo("sys_ss_get_console_id(buf=*0x%x)", buf);

	// TODO: Return some actual IDPS?
	*buf = 0;

	return CELL_OK;
}

s32 sys_ss_get_open_psid(vm::ptr<CellSsOpenPSID> psid)
{
	sys_ss.warning("sys_ss_get_open_psid(psid=*0x%x)", psid);

	psid->high = 0;
	psid->low = 0;

	return CELL_OK;
}

s32 sys_ss_appliance_info_manager(u32 code, vm::ptr<u8> buffer)
{
	switch (code)
	{
	case 0x19003:
	{
		sys_ss.warning("sys_ss_appliance_info_manager(code=0x%x (IDPS), buffer=*0x%x)", code, buffer);
		u8 idps[] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x89, 0x00, 0x0B, 0x14, 0x00, 0xEF, 0xDD, 0xCA, 0x25, 0x52, 0x66 };
		memcpy(buffer.get_ptr(), idps, 16);
		break;
	}
	case 0x19004:
	{
		sys_ss.warning("sys_ss_appliance_info_manager(code=0x%x (PSCODE), buffer=*0x%x)", code, buffer);
		u8 pscode[] = { 0x00, 0x01, 0x00, 0x85, 0x00, 0x07, 0x00, 0x04 };
		memcpy(buffer.get_ptr(), pscode, 8);
		break;
	}
	default: sys_ss.todo("sys_ss_appliance_info_manager(code=0x%x, buffer=*0x%x)", code, buffer);
	}
	return CELL_OK;
}

s32 sys_ss_get_cache_of_product_mode(vm::ptr<u8> ptr)
{
	sys_ss.todo("UNIMPLEMENTED sys_ss_get_cache_of_product_mode(0x%x)", ptr);
	s32 pid = 1;

	if (false /*process == null*/)
	{
		return 0x80010003;
	}
	if (!ptr)
	{
		return 0x80010002;
	}
	// 0xff Happens when hypervisor call returns an error
	// 0 - disabled
	// 1 - enabled

	// except something segfaults when using 0, so error it is!
	*ptr = 0xFF;

	return CELL_OK;
}

// these arguments are odd and change type based on command packet
s32 sys_ss_secure_rtc(u64 cmd, u64 a2, u64 a3, u64 a4)
{
	sys_ss.todo("sys_ss_secure_rtc(cmd=0x%x, a2=0x%x, a3=0x%x, a4=0x%x)", cmd, a2, a3, a4);
	if (cmd == 0x3001)
	{
		if (a3 != 0x20)
			return 0x80010500;

		return CELL_OK;
	}
	else if (cmd == 0x3002)
	{
		// Get time
		if (a2 > 1)
			return 0x80010500;

		// a3 is actual output, not 100% sure, but best guess is its tb val
		*vm::ptr<u64>::make(a3) = get_timebased_time();
		// a4 is a pointer to status, non 0 on error
		*vm::ptr<u64>::make(a4) = 0;

		return CELL_OK;
	}
	else if (cmd == 0x3003)
	{
		return CELL_OK;
	}

	return 0x80010500;
}

s32 sys_ss_get_cache_of_flash_ext_flag(vm::ptr<u64> flag)
{
	sys_ss.todo("sys_ss_get_cache_of_flash_ext_flag(flag=*0x%x)", flag);
	*flag = 0xFE; // nand vs nor from lsb
	return CELL_OK;
}

s32 sys_ss_get_boot_device(vm::ptr<u64> dev)
{
	sys_ss.todo("sys_ss_get_boot_device(dev=*0x%x)", dev);
	*dev = 0x190;
	return CELL_OK;
}

s32 sys_ss_update_manager(u64 pkg_id, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	sys_ss.todo("sys_ss_update_manager(pkg=0x%x, a1=0x%x, a2=0x%x, a3=0x%x, a4=0x%x, a5=0x%x, a6=0x%x)", pkg_id, a1, a2, a3, a4, a5, a6);
	if (pkg_id == 0x600B)
	{
		// read eeprom
		// a1 == offset
		// a2 == *value
		if (a1 == 0x48C06) // fself ctrl?
			*vm::ptr<u8>::make(a2) = 0xFF;
		else if (a1 == 0x48C42) // hddcopymode
			*vm::ptr<u8>::make(a2) = 0xFF;
		else if (a1 >= 0x48C1C && a1 <= 0x48C1F)
		{
			// vsh target? (seems it can be 0xFFFFFFFE, 0xFFFFFFFF, 0x00000001 default: 0x00000000 /maybe QA,Debug,Retail,Kiosk?)
			*vm::ptr<u8>::make(a2) = a1 == 0x48C1F ? 0x1 : 0;
		}
		else if (a1 >= 0x48C18 && a1 <= 0x48C1B)
		{
			// system language
			// *i think* this gives english
			*vm::ptr<u8>::make(a2) = a1 == 0x48C1B ? 0x1 : 0;
		}
	}
	else if (pkg_id == 0x600C)
	{
		// write eeprom
	}
	else if (pkg_id == 0x6009)
	{
		// get seed token
	}
	else if (pkg_id == 0x600A)
	{
		// set seed token
	}

	return CELL_OK;
}

s32 sys_ss_virtual_trm_manager(u64 pkg_id, u64 a1, u64 a2, u64 a3, u64 a4) {
	sys_ss.todo("sys_ss_virtual_trm_manager(pkg=0x%x, a1=0x%llx, a2=0x%llx, a3=0x%llx, a4=0x%llx)", pkg_id, a1, a2, a3, a4);
	return CELL_OK;
}
