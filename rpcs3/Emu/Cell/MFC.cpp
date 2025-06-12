#include "stdafx.h"
#include "MFC.h"
#include "Utilities/Thread.h"
#include "SPUThread.h"

template <>
void fmt_class_string<MFC>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](MFC cmd)
	{
		switch (cmd)
		{
		case MFC_PUT_CMD: return "PUT";
		case MFC_PUTB_CMD: return "PUTB";
		case MFC_PUTF_CMD: return "PUTF";
		case MFC_PUTS_CMD: return "PUTS";
		case MFC_PUTBS_CMD: return "PUTBS";
		case MFC_PUTFS_CMD: return "PUTFS";
		case MFC_PUTR_CMD: return "PUTR";
		case MFC_PUTRB_CMD: return "PUTRB";
		case MFC_PUTRF_CMD: return "PUTRF";
		case MFC_GET_CMD: return "GET";
		case MFC_GETB_CMD: return "GETB";
		case MFC_GETF_CMD: return "GETF";
		case MFC_GETS_CMD: return "GETS";
		case MFC_GETBS_CMD: return "GETBS";
		case MFC_GETFS_CMD: return "GETFS";
		case MFC_PUTL_CMD: return "PUTL";
		case MFC_PUTLB_CMD: return "PUTLB";
		case MFC_PUTLF_CMD: return "PUTLF";
		case MFC_PUTRL_CMD: return "PUTRL";
		case MFC_PUTRLB_CMD: return "PUTRLB";
		case MFC_PUTRLF_CMD: return "PUTRLF";
		case MFC_GETL_CMD: return "GETL";
		case MFC_GETLB_CMD: return "GETLB";
		case MFC_GETLF_CMD: return "GETLF";

		case MFC_GETLLAR_CMD: return "GETLLAR";
		case MFC_PUTLLC_CMD: return "PUTLLC";
		case MFC_PUTLLUC_CMD: return "PUTLLUC";
		case MFC_PUTQLLUC_CMD: return "PUTQLLUC";

		case MFC_SNDSIG_CMD: return "SNDSIG";
		case MFC_SNDSIGB_CMD: return "SNDSIGB";
		case MFC_SNDSIGF_CMD: return "SNDSIGF";
		case MFC_BARRIER_CMD: return "BARRIER";
		case MFC_EIEIO_CMD: return "EIEIO";
		case MFC_SYNC_CMD: return "SYNC";

		case MFC_SDCRT_CMD: return "SDCRT";
		case MFC_SDCRTST_CMD: return "SDCRTST";
		case MFC_SDCRZ_CMD: return "SDCRZ";
		case MFC_SDCRS_CMD: return "SDCRS";
		case MFC_SDCRF_CMD: return "SDCRF";

		case MFC_BARRIER_MASK:
		case MFC_FENCE_MASK:
		case MFC_LIST_MASK:
		case MFC_START_MASK:
		case MFC_RESULT_MASK:
			break;
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_mfc_cmd>::format(std::string& out, u64 arg)
{
	const auto& cmd = get_object(arg);

	const u8 tag = cmd.tag;

	fmt::append(out, "%-8s #%02u 0x%05x:0x%08llx 0x%x%s", cmd.cmd, tag & 0x7f, cmd.lsa, u64{cmd.eah} << 32 | cmd.eal, cmd.size, (tag & 0x80) ? " (stalled)" : "");
}

using async_cmd_t = spu_thread::async_cmd_t;
using async_cmd_state = spu_thread::async_cmd_state;

void mfc_thread::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting && cpu_flag::exit - _this->state)
	{
		async_cmd_t state = _this->async_ch_mfc_cmd.fetch_op([&](async_cmd_t& state)
		{
			// Claim command
			if (state.cmd_type == async_cmd_state::pending)
			{
				state.cmd_type = async_cmd_state::executing;
			}
		});

		if (state.cmd_type != async_cmd_state::pending)
		{
			continue;
		}

		spu_mfc_cmd command{};
		command.eah = 0;
		command.tag = 0;
		command.eal = state.eal;
		command.lsa = state.lsa;
		command.size = state.size_plus_16 + 16;
		command.cmd = MFC(+state.cmd_type);
		_this->do_dma_transfer(_this, command, _this->ls);

		_this->async_ch_mfc_cmd.fetch_op([&](async_cmd_t& state)
		{
			// Notify completion
			state.cmd_type = async_cmd_state::done;
		});
	}
}