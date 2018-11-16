#pragma once


struct BtInitPacket
{
	be_t<u32> equeue_id;
	be_t<u32> pad;
	be_t<u32> page_proc_addr;
	be_t<u32> pad_;
};

// syscalls
error_code sys_btsetting_if(u64 cmd, vm::ptr<void> msg);
