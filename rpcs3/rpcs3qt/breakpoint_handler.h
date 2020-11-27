#pragma once

#include "util/types.hpp"
#include <set>

enum class bp_type : u8
{
	read,
	write,
	exec,
	debug,
	once,

	__bitset_enum_max,
};

/*
* This class acts as a layer between the UI and Emu for breakpoints.
*/
class breakpoint_handler
{
public:
	static constexpr u32 all_spu = 0x02ffffff;
	static constexpr u32 all_ppu = 0x01ffffff;

private:
	struct alignas(16) breakpoint
	{
		u32 addr = -1;
		u32 size_pow = 2;
		u32 thread_id = all_ppu;
		bs_t<bp_type> type{};

		constexpr bool match(u32 _addr, u32 id) const
		{
			if (_addr != umax && addr == _addr)
			{
				const u32 shift = (~(id | thread_id) & 0xffffff) ? 0 : 24;

				if (id >> shift == thread_id >> shift)
				{
					return true;
				}
			}

			return false;
		}
	};

	enum add_op
	{
		or,
		xor,
		force,
	};

private:
	s32 add_impl(u32 addr, bs_t<bp_type> type, u32 id, add_op op);
public:

	breakpoint_handler();
	~breakpoint_handler();

	// .first: is specified BP exists
	// .second: is any BP active
	// TODO: Add arg for gameid
	// NOTE: An additional address can be used at uperr 32-bits of addr, although its only for exec bps
	std::pair<bs_t<bp_type>, bool> contains(u64 addr, u32 id, bp_type type = bp_type::read) const;

	// Returns true if added successfully
	s32 add(u32 addr, bs_t<bp_type> type = bp_type::read + bp_type::write, u32 id = all_ppu)
	{
		return add_impl(addr, type, id, add_op::or);
	}

	// XOR-like behaviour: if bp exists it removes it, otherwise it adds a new breakpoint
	// Returns true if added successfully
	s32 toggle(u32 addr, bs_t<bp_type> type = bp_type::read + bp_type::write, u32 id = all_ppu)
	{
		return add_impl(addr, type, id, add_op::xor);
	}

	// Optimization
	s32 force_add(u32 addr, bs_t<bp_type> type = bp_type::read + bp_type::write, u32 id = all_ppu)
	{
		return add_impl(addr, type, id, add_op::force);
	}

	// Returns true if removed breakpoint at loc successfully at posistion
	// If position is negative attempts to remove all all breakpoints satisfy key
	bool remove(u32 addr, u32 id = all_ppu, s32 pos = -1);

	static constexpr u32 all_threads(u32 id_type)
	{
		switch (id_type)
		{
		case 1: return all_ppu;
		case 2: return all_spu;
		default: return 0;
		}
	}

private:
	atomic_t<v128> m_state{};
	std::array<atomic_t<breakpoint>, 64> m_bps{}; 
};
