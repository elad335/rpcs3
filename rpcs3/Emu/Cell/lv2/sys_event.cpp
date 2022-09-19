#include "stdafx.h"
#include "sys_event.h"

#include "Emu/IdManager.h"
#include "Emu/IPC.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "sys_process.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_event);

lv2_event_queue::lv2_event_queue(u32 protocol, s32 type, s32 size, u64 name, u64 ipc_key) noexcept
	: id(idm::last_id())
	, protocol{static_cast<u8>(protocol)}
	, type(static_cast<u8>(type))
	, size(static_cast<u8>(size))
	, name(name)
	, key(ipc_key)
{
}

lv2_event_queue::lv2_event_queue(utils::serial& ar) noexcept
	: id(idm::last_id())
	, protocol(ar)
	, type(ar)
	, size(ar)
	, name(ar)
	, key(ar)
{
	const std::vector<lv2_event> events_save = ar;
	std::copy(events_save.begin(), events_save.end(), events.begin());

	auto& c = control.raw();
	c.push = ::narrow<s8>(events_save.size());
}

std::shared_ptr<void> lv2_event_queue::load(utils::serial& ar)
{
	auto queue = std::make_shared<lv2_event_queue>(ar);
	return lv2_obj::load(queue->key, queue);
}

void lv2_event_queue::save(utils::serial& ar)
{
	ar(protocol, type, size, name, key);

	const auto c = control.load();

	std::vector<lv2_event> events_save;
	events_save.reserve((c.push - c.pop) % events_size);

	for (u32 i = c.push % events_size; i != c.pop % events_size; i = (i + 1) % events_size)
	{
		events_save.emplace_back(events[i]);
	}

	ar(events_save);
}

void lv2_event_queue::save_ptr(utils::serial& ar, lv2_event_queue* q)
{
	if (!lv2_obj::check(q))
	{
		ar(u32{0});
		return;
	}

	ar(q->id);
}

std::shared_ptr<lv2_event_queue> lv2_event_queue::load_ptr(utils::serial& ar, std::shared_ptr<lv2_event_queue>& queue)
{
	const u32 id = ar.operator u32();

	if (!id)
	{
		return nullptr;
	}

	if (auto q = idm::get_unlocked<lv2_obj, lv2_event_queue>(id))
	{
		// Already initialized
		return q;
	}

	Emu.DeferDeserialization([id, &queue]()
	{
		// Defer resolving
		queue = ensure(idm::get_unlocked<lv2_obj, lv2_event_queue>(id));
	});

	// Null until resolved
	return nullptr;
}

lv2_event_port::lv2_event_port(utils::serial& ar)
	: type(ar)
	, name(ar)
	, queue(lv2_event_queue::load_ptr(ar, queue))
{
}

void lv2_event_port::save(utils::serial& ar)
{
	ar(type, name);

	lv2_event_queue::save_ptr(ar, queue.get());
}

std::shared_ptr<lv2_event_queue> lv2_event_queue::find(u64 ipc_key)
{
	if (ipc_key == SYS_EVENT_QUEUE_LOCAL)
	{
		// Invalid IPC key
		return {};
	}

	return g_fxo->get<ipc_manager<lv2_event_queue, u64>>().get(ipc_key);
}

extern void resume_spu_thread_group_from_waiting(spu_thread& spu);

CellError lv2_event_queue::send(lv2_event event)
{
	std::unique_lock lock(mutex, std::defer_lock);

	cpu_thread* cpu{};
	cpu_thread* restore_next{};
	CellError error{};

	auto old = control.fetch_op([&](lv2_event_queue::control_t& data) -> atomic_op_result
	{
		if (data.extinct)
		{
			error = CELL_ENOTCONN;
			return atomic_op_result::abort;
		}

		if (data.sq)
		{
			if (!lock)
			{
				lock.lock();

				// Retry
				return atomic_op_result::ignore;
			}

			if (restore_next)
			{
				if (type == SYS_PPU_QUEUE)
				{
					static_cast<ppu_thread*>(cpu)->next_cpu = static_cast<ppu_thread*>(restore_next);
				}
				else
				{
					static_cast<spu_thread*>(cpu)->next_cpu = static_cast<spu_thread*>(restore_next);
				}

				restore_next = nullptr;
			}

			error = CELL_EAGAIN;

			auto old = data.sq;

			if (type == SYS_PPU_QUEUE)
			{
				ppu_thread* sq = static_cast<ppu_thread*>(data.sq);
				restore_next = sq->next_cpu;
				cpu = schedule<ppu_thread>(sq, protocol);

				if (sq == old)
				{
					return atomic_op_result::abort;
				}

				data.sq = sq;
			}
			else
			{
				spu_thread* sq = static_cast<spu_thread*>(data.sq);
				restore_next = sq->next_cpu;
				cpu = schedule<spu_thread>(sq, protocol);

				if (sq == old)
				{
					return atomic_op_result::abort;
				}

				data.sq = sq;
			}

			return atomic_op_result::ok;
		}

		if ((data.push2 - data.pop) % events_size < this->size)
		{
			// Reserve a slot for an event
			data.push2++;
			error = {};
			return atomic_op_result::ok;
		}

		error = CELL_EBUSY;
		return atomic_op_result::abort;
	}).first;

	switch (error)
	{
	case CellError{}:
	{
		// Store the event (relaxed operation)
		events[old.push2 % events_size] = event;

		// Complete the insert
		while (true)
		{
			u32 push = atomic_storage<u32>::load(control.raw().push);
			const u32 distance = (old.push2 - push) % events_size;

			if (!distance)
			{
				// Increment the push index and perform queued increments
				if (atomic_storage<u32>::compare_exchange(control.raw().push, push, (push + 1 + push / events_size) % events_size))
				{
					break;
				}
			}
			else
			{
				if (distance - 1 != (push / events_size))
				{
					// Wait for the previous store to complete
					busy_wait(100);
					continue;
				}

				// If the 'first' store hasn't been completed, don't wait for it
				// Queue the incrementation and return
				if (atomic_storage<u32>::compare_exchange(control.raw().push, push, push + events_size))
				{
					break;
				}
			}
		}

		return {};
	}
	case CELL_EAGAIN: break;
	default: return error;
	}

	if (cpu->state & cpu_flag::again)
	{
		if (auto current = cpu_thread::get_current())
		{
			current->state += cpu_flag::again;
		}

		sys_event.warning("Ignored event!");

		// Fake error for abort
		return CELL_EAGAIN;
	}

	if (type == SYS_PPU_QUEUE)
	{
		// Store event in registers
		auto& ppu = static_cast<ppu_thread&>(*cpu);

		std::tie(ppu.gpr[4], ppu.gpr[5], ppu.gpr[6], ppu.gpr[7]) = event;

		awake(&ppu);
	}
	else
	{
		// Store event in In_MBox
		auto& spu = static_cast<spu_thread&>(*cpu);

		const u32 data1 = static_cast<u32>(std::get<1>(event));
		const u32 data2 = static_cast<u32>(std::get<2>(event));
		const u32 data3 = static_cast<u32>(std::get<3>(event));
		spu.ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
		resume_spu_thread_group_from_waiting(spu);

		if (!lv2_obj::g_postpone_notify_barrier)
		{
			lock.unlock();
			lv2_obj::notify_all();
		}
	}

	return {};
}

error_code sys_event_queue_create(cpu_thread& cpu, vm::ptr<u32> equeue_id, vm::ptr<sys_event_queue_attribute_t> attr, u64 ipc_key, s32 size)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_queue_create(equeue_id=*0x%x, attr=*0x%x, ipc_key=0x%llx, size=%d)", equeue_id, attr, ipc_key, size);

	if (size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	const u32 protocol = attr->protocol;

	if (protocol != SYS_SYNC_FIFO && protocol != SYS_SYNC_PRIORITY)
	{
		sys_event.error("sys_event_queue_create(): unknown protocol (0x%x)", protocol);
		return CELL_EINVAL;
	}

	const u32 type = attr->type;

	if (type != SYS_PPU_QUEUE && type != SYS_SPU_QUEUE)
	{
		sys_event.error("sys_event_queue_create(): unknown type (0x%x)", type);
		return CELL_EINVAL;
	}

	const u32 pshared = ipc_key == SYS_EVENT_QUEUE_LOCAL ? SYS_SYNC_NOT_PROCESS_SHARED : SYS_SYNC_PROCESS_SHARED;
	constexpr u32 flags = SYS_SYNC_NEWLY_CREATED;
	const u64 name = attr->name_u64;

	if (const auto error = lv2_obj::create<lv2_event_queue>(pshared, ipc_key, flags, [&]()
	{
		return std::make_shared<lv2_event_queue>(protocol, type, size, name, ipc_key);
	}))
	{
		return error;
	}

	*equeue_id = idm::last_id();
	return CELL_OK;
}

error_code sys_event_queue_destroy(ppu_thread& ppu, u32 equeue_id, s32 mode)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_queue_destroy(equeue_id=0x%x, mode=%d)", equeue_id, mode);

	if (mode && mode != SYS_EVENT_QUEUE_DESTROY_FORCE)
	{
		return CELL_EINVAL;
	}

	std::unique_lock<shared_mutex> qlock;

	lv2_event_queue::control_t control{};

	const auto queue = idm::withdraw<lv2_obj, lv2_event_queue>(equeue_id, [&](lv2_event_queue& queue) -> CellError
	{
		qlock = std::unique_lock{queue.mutex};

		control = queue.control.fetch_op([&](lv2_event_queue::control_t& data)
		{
			if (!mode && data.sq)
			{
				return false;
			}

			for (auto cpu = data.sq; cpu; cpu = cpu->get_next_cpu())
			{
				if (cpu->state & cpu_flag::again)
				{
					ppu.state += cpu_flag::again;
					return false;
				}
			}

			data.extinct = true;
			data.pop = data.push;
			return true;
		}).first;

		if (ppu.state & cpu_flag::again)
		{
			return CELL_EAGAIN;
		}

		if (!mode && control.sq)
		{
			return CELL_EBUSY;
		}

		lv2_obj::on_id_destroy(queue, queue.key);

		if (!control.sq)
		{
			qlock.unlock();
		}

		return {};
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (ppu.state & cpu_flag::again)
	{
		return {};
	}

	if (queue.ret)
	{
		return queue.ret;
	}

	std::string lost_data;

	if (qlock.owns_lock())
	{
		if (sys_event.warning)
		{
			u32 size = 0;

			for (auto cpu = control.sq; cpu; cpu = cpu->get_next_cpu())
			{
				size++;
			}

			fmt::append(lost_data, "Forcefully awaken waiters (%u):\n", size);

			for (auto cpu = control.sq; cpu; cpu = cpu->get_next_cpu())
			{
				lost_data += cpu->get_name();
				lost_data += '\n';
			}
		}

		if (queue->type == SYS_PPU_QUEUE)
		{
			for (auto cpu = static_cast<ppu_thread*>(control.sq); cpu; cpu = cpu->next_cpu)
			{
				cpu->gpr[3] = CELL_ECANCELED;
				queue->append(cpu);
			}

			lv2_obj::awake_all();
		}
		else
		{
			for (auto cpu = static_cast<spu_thread*>(control.sq); cpu; cpu = cpu->next_cpu)
			{
				cpu->ch_in_mbox.set_values(1, CELL_ECANCELED);
				resume_spu_thread_group_from_waiting(*cpu);
			}
		}

		qlock.unlock();
	}

	if (sys_event.warning)
	{
		if ((control.push - control.pop) % queue->events_size)
		{
			fmt::append(lost_data, "Unread queue events (%u):\n", (control.push - control.pop) % queue->events_size);
		}

		for (u32 i = control.push % queue->events_size; i != control.pop % queue->events_size; i = (i + 1) % queue->events_size)
		{
			auto& evt = queue->events[i];
			fmt::append(lost_data, "data0=0x%x, data1=0x%x, data2=0x%x, data3=0x%x\n"
				, std::get<0>(evt), std::get<1>(evt), std::get<2>(evt), std::get<3>(evt));
		}

		if (!lost_data.empty())
		{
			sys_event.warning("sys_event_queue_destroy(): %s", lost_data);
		}
	}

	return CELL_OK;
}

error_code sys_event_queue_tryreceive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> event_array, s32 size, vm::ptr<u32> number)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_tryreceive(equeue_id=0x%x, event_array=*0x%x, size=%d, number=*0x%x)", equeue_id, event_array, size, number);

	const auto queue = idm::get<lv2_obj, lv2_event_queue>(equeue_id);

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue->type != SYS_PPU_QUEUE)
	{
		return CELL_EINVAL;
	}

	bool extinct = false;
	bool success = false;
	s32 count = 0;
	alignas(32) std::array<lv2_event, 127> events;

	while (!success)
	{
		queue->control.fetch_op([&](lv2_event_queue::control_t& data)
		{
			if (data.extinct)
			{
				extinct = true;
				return false;
			}

			const s32 qsize = (data.push - data.pop) % queue->events_size;

			if (size > qsize && (data.push2 - data.push) % queue->events_size)
			{
				success = false;
				return false;
			}

			success = true;

			if (count = std::min<s32>(qsize, size); count > 0)
			{
				// Pull the events into temporary storage
				std::copy_n(&queue->events[data.pop % queue->events_size], count, events.data());
				data.pop += count;
				return true;
			}

			return false;
		});

		if (extinct)
		{
			return CELL_ESRCH;
		}
	}

	for (s32 i = 0; i < count; i++)
	{
		// Write out the events after the pop has been completed
		auto& dest = event_array[i];
		std::tie(dest.source, dest.data1, dest.data2, dest.data3) = events[i];
	}

	*number = count;

	return CELL_OK;
}

error_code sys_event_queue_receive(ppu_thread& ppu, u32 equeue_id, vm::ptr<sys_event_t> dummy_event, u64 timeout)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx)", equeue_id, dummy_event, timeout);

	ppu.gpr[3] = CELL_OK;

	const auto queue = idm::get<lv2_obj, lv2_event_queue>(equeue_id, [&, notify = lv2_obj::notify_all_t()](lv2_event_queue& queue) -> CellError
	{
		if (queue.type != SYS_PPU_QUEUE)
		{
			return CELL_EINVAL;
		}

		bool prep = false;
		bool success = false;
	
		while (!queue.control.fetch_op([&](lv2_event_queue::control_t& data)
		{
			success = false;

			if ((data.push - data.pop) % queue.events_size)
			{
				std::tie(ppu.gpr[4], ppu.gpr[5], ppu.gpr[6], ppu.gpr[7]) = queue.events[data.pop++ % queue.events_size];
				success = true;
				return true;
			}

			if ((data.push2 - data.pop) % queue.events_size)
			{
				// An event is currently being pushed, wait for it instead of sleeping
				return false;
			}

			if (!prep)
			{
				ppu.cancel_sleep = 1;
				lv2_obj::prepare_for_sleep(ppu);
				prep = true;
			}

			ppu.next_cpu = static_cast<ppu_thread*>(data.sq);
			data.sq = &ppu;
			return true;
		}).second)
		{
			busy_wait(100);
		}

		if (success)
		{
			ppu.cancel_sleep = 0;
			ppu.next_cpu = nullptr;
			return {};
		}

		// "/dev_flash/vsh/module/msmw2.sprx" seems to rely on some cryptic shared memory behaviour that we don't emulate correctly
		// This is a hack to avoid waiting for 1m40s every time we boot vsh
		if (queue.key == 0x8005911000000012 && g_ps3_process_info.get_cellos_appname() == "vsh.self"sv)
		{
			sys_event.todo("sys_event_queue_receive(equeue_id=0x%x, *0x%x, timeout=0x%llx) Bypassing timeout for msmw2.sprx", equeue_id, dummy_event, timeout);
			timeout = 1;
		}

		success = !queue.sleep(ppu, timeout);
		notify.cleanup();

		if (!success)
		{
			return CELL_EBUSY;
		}

		return {};
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	if (queue.ret)
	{
		if (queue.ret != CELL_EBUSY)
		{
			return queue.ret;
		}
	}
	else
	{
		return CELL_OK;
	}

	// If cancelled, gpr[3] will be non-zero. Other registers must contain event data.
	while (auto state = +ppu.state)
	{
		if (state & cpu_flag::signal && ppu.state.test_and_reset(cpu_flag::signal))
		{
			break;
		}

		if (is_stopped(state))
		{
			std::lock_guard lock(queue->mutex);

			for (auto cpu = static_cast<ppu_thread*>(queue->control.load().sq); cpu; cpu = cpu->next_cpu)
			{
				if (cpu == &ppu)
				{
					ppu.state += cpu_flag::again;
					return {};
				}
			}

			break;
		}

		for (usz i = 0; cpu_flag::signal - ppu.state && i < 50; i++)
		{
			busy_wait(500);
		}

		if (ppu.state & cpu_flag::signal)
 		{
			continue;
		}

		if (timeout)
		{
			if (lv2_obj::wait_timeout(timeout, &ppu))
			{
				// Wait for rescheduling
				if (ppu.check_state())
				{
					continue;
				}

				cpu_thread* restore_next{};

				std::lock_guard lock(queue->mutex);

				bool success = false;

				queue->control.fetch_op([&](lv2_event_queue::control_t& data)
				{
					success = false;

					if (restore_next)
					{
						ppu.next_cpu = static_cast<ppu_thread*>(restore_next);
						restore_next = nullptr;
					}

					if (!data.sq)
					{
						return false;
					}

					ppu_thread* sq = static_cast<ppu_thread*>(data.sq);
					restore_next = sq->next_cpu;

					const bool retval = &ppu == sq;

					if (!queue->unqueue(sq, &ppu))
					{
						return false;
					}

					success = true;

					if (!retval)
					{
						return false;
					}

					data.sq = sq;
					return true;
				});

				if (success)
				{
					break;
				}

				ppu.gpr[3] = CELL_ETIMEDOUT;
				break;
			}
		}
		else
		{
			thread_ctrl::wait_on(ppu.state, state);
		}
	}

	return not_an_error(ppu.gpr[3]);
}

error_code sys_event_queue_drain(ppu_thread& ppu, u32 equeue_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.trace("sys_event_queue_drain(equeue_id=0x%x)", equeue_id);

	const auto queue = idm::check<lv2_obj, lv2_event_queue>(equeue_id, [&](lv2_event_queue& queue)
	{
		bool success = false;

		while (!success)
		{
			queue.control.fetch_op([&](lv2_event_queue::control_t& data)
			{
				success = true;

				if ((data.push2 - data.pop) % queue.events_size)
				{
					if ((data.push2 - data.push) % queue.events_size)
					{
						success = false;
						return false;
					}

					data.pop = data.push;
					return true;
				}

				return false;
			});
		}
	});

	if (!queue)
	{
		return CELL_ESRCH;
	}

	return CELL_OK;
}

error_code sys_event_port_create(cpu_thread& cpu, vm::ptr<u32> eport_id, s32 port_type, u64 name)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_create(eport_id=*0x%x, port_type=%d, name=0x%llx)", eport_id, port_type, name);

	if (port_type != SYS_EVENT_PORT_LOCAL && port_type != 3)
	{
		sys_event.error("sys_event_port_create(): unknown port type (%d)", port_type);
		return CELL_EINVAL;
	}

	if (const u32 id = idm::make<lv2_obj, lv2_event_port>(port_type, name))
	{
		*eport_id = id;
		return CELL_OK;
	}

	return CELL_EAGAIN;
}

error_code sys_event_port_destroy(ppu_thread& ppu, u32 eport_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_destroy(eport_id=0x%x)", eport_id);

	const auto port = idm::withdraw<lv2_obj, lv2_event_port>(eport_id, [](lv2_event_port& port) -> CellError
	{
		if (lv2_obj::check(port.queue))
		{
			return CELL_EISCONN;
		}

		return {};
	});

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (port.ret)
	{
		return port.ret;
	}

	return CELL_OK;
}

error_code sys_event_port_connect_local(cpu_thread& cpu, u32 eport_id, u32 equeue_id)
{
	cpu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_connect_local(eport_id=0x%x, equeue_id=0x%x)", eport_id, equeue_id);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port || !idm::check_unlocked<lv2_obj, lv2_event_queue>(equeue_id))
	{
		return CELL_ESRCH;
	}

	if (port->type != SYS_EVENT_PORT_LOCAL)
	{
		return CELL_EINVAL;
	}

	if (lv2_obj::check(port->queue))
	{
		return CELL_EISCONN;
	}

	port->queue = idm::get_unlocked<lv2_obj, lv2_event_queue>(equeue_id);

	return CELL_OK;
}

error_code sys_event_port_connect_ipc(ppu_thread& ppu, u32 eport_id, u64 ipc_key)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_connect_ipc(eport_id=0x%x, ipc_key=0x%x)", eport_id, ipc_key);

	if (ipc_key == 0)
	{
		return CELL_EINVAL;
	}

	auto queue = lv2_event_queue::find(ipc_key);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port || !queue)
	{
		return CELL_ESRCH;
	}

	if (port->type != SYS_EVENT_PORT_IPC)
	{
		return CELL_EINVAL;
	}

	if (lv2_obj::check(port->queue))
	{
		return CELL_EISCONN;
	}

	port->queue = std::move(queue);

	return CELL_OK;
}

error_code sys_event_port_disconnect(ppu_thread& ppu, u32 eport_id)
{
	ppu.state += cpu_flag::wait;

	sys_event.warning("sys_event_port_disconnect(eport_id=0x%x)", eport_id);

	std::lock_guard lock(id_manager::g_mutex);

	const auto port = idm::check_unlocked<lv2_obj, lv2_event_port>(eport_id);

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (!lv2_obj::check(port->queue))
	{
		return CELL_ENOTCONN;
	}

	// TODO: return CELL_EBUSY if necessary (can't detect the condition)

	port->queue.reset();

	return CELL_OK;
}

error_code sys_event_port_send(u32 eport_id, u64 data1, u64 data2, u64 data3)
{
	if (auto cpu = get_current_cpu_thread())
	{
		cpu->state += cpu_flag::wait;
	}

	sys_event.trace("sys_event_port_send(eport_id=0x%x, data1=0x%llx, data2=0x%llx, data3=0x%llx)", eport_id, data1, data2, data3);

	const auto port = idm::check<lv2_obj, lv2_event_port>(eport_id, [&, notify = lv2_obj::notify_all_t()](lv2_event_port& port) -> CellError
	{
		if (lv2_obj::check(port.queue))
		{
			const u64 source = port.name ? port.name : (s64{process_getpid()} << 32) | u64{eport_id};

			return port.queue->send(source, data1, data2, data3);
		}

		return CELL_ENOTCONN;
	});

	if (!port)
	{
		return CELL_ESRCH;
	}

	if (port.ret)
	{
		if (port.ret == CELL_EAGAIN)
		{
			// Not really an error code exposed to games (thread has raised cpu_flag::again)
			return not_an_error(CELL_EAGAIN);
		}

		if (port.ret == CELL_EBUSY)
		{
			return not_an_error(CELL_EBUSY);
		}

		return port.ret;
	}

	return CELL_OK;
}
