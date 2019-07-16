#include "stdafx.h"
#include "IdManager.h"
#include "Utilities/Thread.h"

#include <atomic>

shared_mutex id_manager::g_mutex;

thread_local DECLARE(idm::g_id);
DECLARE(idm::g_map);
DECLARE(fxm::g_vec);

volatile u64 idm_count = idm::entry_state::min_allowed;

template <bool for_creation>
bool idm::check_state(u64 alloc_state)
{
	if constexpr (for_creation)
	{
		return idm_count == alloc_state || alloc_state == 1; 
	}

	return idm_count == alloc_state;
}

template <typename T>
u32 idm::allocate_id(u32 base, u32 step, u32 count)
{
	//constexpr auto base  = id_manager::id_traits<T>::base;
	//constexpr auto step  = id_manager::id_traits<T>::step;
	//constexpr auto count = id_manager::id_traits<T>::count;

	if (id_manager::typeinfo<T>.max_index < count)
	{
		obj.id = g_id = _next;

		if (std::is_same<T, lv2_obj>::value)
		{
			obj.type = base;
		}

		obj.creation_count++;
		return id_manager::typeinfo<T>.max_index++;
	}

	// Check all IDs starting from "next id" (TODO)
	for (u32 i = 0, next = base; i < count; i++, next += step)
	{
		auto& obj = id_manager::typeinfo<T>.id_array[i];

		// Look for free ID
		if (!check_state<true>(obj.alloc_state))
		{
			obj.refmtx.lock();

			if (check_state<true>(obj.alloc_state))
			{
				obj.refmtx.unlock();
				continue;
			}

			obj.id = g_id = next;

			if (std::is_same<T, lv2_obj>::value)
			{
				obj.type = base;
			}

			obj.creation_count++;
			return i;
		}
	}

	// Out of IDs
	return count;
}

void idm::init()
{
	atomic_storage<u32>::fetch_inc(const_cast<u32&>(idm_count));
}

void fxm::init()
{
	// Allocate
	g_vec.resize(id_manager::typeinfo::get_count());
	fxm::clear();
}

void fxm::clear()
{
	// Call recorded finalization functions for all IDs
	for (auto& val : g_vec)
	{
		val.reset();
	}
}
