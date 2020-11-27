#include "breakpoint_handler.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/IdManager.h"

extern void ppu_breakpoint(u32 loc, bool isAdding);

breakpoint_handler::breakpoint_handler()
{
	// TODO: load breakpoints from file
}

breakpoint_handler::~breakpoint_handler()
{
	// TODO: save breakpoints to a file
}

std::pair<bs_t<bp_type>, bool> breakpoint_handler::contains(u64 addr, u32 id, bp_type type) const
{
	// Not atomic but it does not ought to be

	auto bits = m_state.load()._u64[1];
	const bool any = bits != 0;
	bs_t<bp_type> flags{};

	for (; bits;)
	{
		const auto data = m_bps[std::countr_zero(bits)].load();

		const u32 pc = static_cast<u32>(addr >> 32);

		if ((data.match(static_cast<u32>(addr), id) && data.type & type) || (data.match(pc, id) && data.type & bp_type::exec))
		{
			if (data.type & bp_type::once)
			{
				// Try to erase breakpoint
				if (!m_bps[std::countr_zero(bits)].compare_and_swap_test(data, breakpoint{}))
				{
					continue;
				}
			}

			flags |= data.type;
		}

		bits &= bits - 1;
	}

	return {flags, any};
}

s32 breakpoint_handler::add_impl(u32 addr, bs_t<bp_type> type, u32 id, enum breakpoint_handler::add_op op)
{
	if (op != add_op::force) type += bp_type::debug;

	while (true)
	{
		auto [old, ok] = m_state.fetch_op([](v128& state)
		{
			u64& bits = state._u64[1];

			if (~bits) [[likely]]
			{
				// Set lowest clear bit
				bits |= bits + 1;
				return true;
			}

			return false;
		});

		if (!ok)
		{
			return -1;
		}

		// Get actual slot number
		const u64 i = std::countr_one(old._u64[1]);

		m_bps[i] = breakpoint{.addr = addr, .thread_id = id, .type = type};

		if (op != add_op::force)
		{
			if (![&old = old, i, this]
			{
				//for (u)

				old._u64[1] |= 1ull << i;

				v128 _new = old;
				_new._u64[0]++;

				return m_state.compare_and_swap_test(old, _new);
			}())
			{
				m_bps[i].store({});
				m_state.atomic_op([&](v128& state){ state._u64[1] &= ~(1ull << i); });
				continue;
			}
		}

		return i;
	}
}

bool breakpoint_handler::remove(u32 addr, u32 id, s32 pos, bs_t<bp_type> type)
{
	if (pos >= 0)
	{
		// Fixed position removal
		return m_bps[pos].fetch_op([&](breakpoint& data)
		{
			if (data.match(addr, id))
			{
				data = {};
				return true;
			}

			return false;
		}).second;
	}

	auto bits = m_state.load()._u64[1];

	for (; bits; bits &= bits - 1)
	{
		if (m_bps[std::countr_zero(bits)].fetch_op([&](breakpoint& data)
		{
			if (data.match(addr, id))
			{
				if (type && data.type == type)
				{
					data = {};
					return true;
				}
			}

			return false;
		}).second)
		{
			return true;
		}
	}

	return false;
}
