#pragma once

#include "Utilities/types.h"
#include "Utilities/mutex.h"

#include <memory>
#include <vector>

#include "Emu/Cell/lv2/sys_cond.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_interrupt.h"
#include "Emu/Cell/lv2/sys_lwcond.h"
#include "Emu/Cell/lv2/sys_lwmutex.h"
#include "Emu/Cell/lv2/sys_mmapper.h"
#include "Emu/Cell/lv2/sys_mutex.h"
#include "Emu/Cell/lv2/sys_overlay.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_rwlock.h"
#include "Emu/Cell/lv2/sys_semaphore.h"
#include "Emu/Cell/lv2/sys_timer.h"

static constexpr size_t lv2_obj_size = 
std::max<size_t>(sizeof(lv2_cond),
std::max<size_t>(sizeof(lv2_event_flag),
std::max<size_t>(sizeof(lv2_event_port),
std::max<size_t>(sizeof(lv2_event_queue),
std::max<size_t>(sizeof(lv2_int_tag),
std::max<size_t>(sizeof(lv2_int_serv),
std::max<size_t>(sizeof(lv2_lwcond),
std::max<size_t>(sizeof(lv2_lwmutex),
std::max<size_t>(sizeof(lv2_overlay),
std::max<size_t>(sizeof(lv2_prx),
std::max<size_t>(sizeof(lv2_rwlock),
std::max<size_t>(sizeof(lv2_sema),
std::max<size_t>(sizeof(lv2_timer_context), 0)))))))))))));

static constexpr size_t lv2_obj_align = 
std::max<size_t>(alignof(lv2_cond),
std::max<size_t>(alignof(lv2_event_flag),
std::max<size_t>(alignof(lv2_event_port),
std::max<size_t>(alignof(lv2_event_queue),
std::max<size_t>(alignof(lv2_int_tag),
std::max<size_t>(alignof(lv2_int_serv),
std::max<size_t>(alignof(lv2_lwcond),
std::max<size_t>(alignof(lv2_lwmutex),
std::max<size_t>(alignof(lv2_overlay),
std::max<size_t>(alignof(lv2_prx),
std::max<size_t>(alignof(lv2_rwlock),
std::max<size_t>(alignof(lv2_sema),
std::max<size_t>(alignof(lv2_timer_context), 0)))))))))))));

extern atomic_t<u64> idm_count;

// Helper namespace
namespace id_manager
{
	// Common global mutex
	extern shared_mutex g_mutex;

	// ID traits
	template <typename T, typename = void>
	struct id_traits
	{
		static_assert(sizeof(T) == 0, "ID object must specify: id_base, id_step, id_count");

		static const u32 base    = 1;     // First ID (N = 0)
		static const u32 step    = 1;     // Any ID: N * id_step + id_base
		static const u32 count   = 65535; // Limit: N < id_count
		static const u32 invalid = 0;
	};

	template <typename T>
	struct id_traits<T, std::void_t<decltype(&T::id_base), decltype(&T::id_step), decltype(&T::id_count)>>
	{
		static const u32 base    = T::id_base;
		static const u32 step    = T::id_step;
		static const u32 count   = T::id_count;
		static const u32 invalid = base > 0 ? 0 : -1;

		static_assert(u64{step} * count + base < UINT32_MAX, "ID traits: invalid object range");
	};

	// Correct usage testing
	template <typename T, typename T2, typename = void>
	struct id_verify : std::integral_constant<bool, std::is_base_of<T, T2>::value>
	{
		// If common case, T2 shall be derived from or equal to T
	};

	template <typename T, typename T2>
	struct id_verify<T, T2, std::void_t<typename T2::id_type>> : std::integral_constant<bool, std::is_same<T, typename T2::id_type>::value>
	{
		// If T2 contains id_type type, T must be equal to it
	};

	template<typename T>
	class typeinfo_t
	{
	public:
		struct registered
		{
			shared_mutex refmtx;
			volatile u32 alloc_state{};
			volatile u64 creation_count = 0;
			u32 type;
			u32 id;
			alignas(std::is_same_v<T, lv2_obj> ? lv2_obj_align : alignof(T)) 
			char obj[std::is_same_v<T, lv2_obj> ? lv2_obj_size : sizeof(T)]{};
		};

		template<typename T>
		registered id_array[id_traits::count];

		atomic_t<u32> max_index = 0;
		shared_mutex g_mutex;
	};

	template<typename T>
	static typeinfo_t<T> typeinfo;

	//using id_map = std::vector<std::pair<id_key, std::shared_ptr<void>>>;
}

// Object manager for emulated process. Multiple objects of specified arbitrary type are given unique IDs.
class idm
{
	template <typename T>
	using entry_type = id_manager::typeinfo_t<T>::registered;

public:
	enum entry_state : u64 { dealloc = 0, withdraw = 1, min_allowed };

private:
	// Last allocated ID for constructors
	static thread_local u32 g_id;

	// Checks if id entry is taken
	template<bool for_creation = false> static bool check_state(u64);

	template <typename T>
	static constexpr u32 get_index(u32 id)
	{
		return (id - id_manager::id_traits<T>::base) / id_manager::id_traits<T>::step;
	}

	// Helper
	template <typename F>
	struct function_traits;

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using result_type = R;
	};

	template <typename F, typename R, typename A1, typename A2>
	struct function_traits<R (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using result_type = R;
	};

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&) const>
	{
		using object_type = A2;
		using void_type   = void;
	};

	template <typename F, typename A1, typename A2>
	struct function_traits<void (F::*)(A1, A2&)>
	{
		using object_type = A2;
		using void_type   = void;
	};

	// Prepare new ID (returns nullptr if out of resources)
	template<typename T> static u32 allocate_id(u32 base, u32 step, u32 count);

	// Find ID (additionally check type if types are not equal)
	template <typename T, typename Type, bool exclusive = false, bool lock = true>
	static entry_type<T>* find_id(u32 id)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		const u32 index = get_index<Type>(id);

		if (index >= id_manager::id_traits<Type>::count)
		{
			return nullptr;
		}

		auto& data = id_manager::typeinfo<T>.id_array[index];

		if constexpr (exclusive)
		{
			data.refmtx.lock();
		}
		else if constexpr (lock)
		{
			data.refmtx.lock_shared();
		}

		static_assert(lock || std::is_same<T, Type>::value, HERE);

		if (check_state(data.alloc_state))
		{
			if constexpr (std::is_same<T, Type>::value)
			{
				return &data;
			}

			if (data.id == id)
			{
				return &data;
			}
		}

		if constexpr (exclusive)
		{
			data.refmtx.unlock();
		}
		else if constexpr (lock)
		{
			data.refmtx.unlock_shared();
		}

		return nullptr;
	}

	// Allocate new ID and assign the object from the provider()
	template <typename T, typename Type, typename F>
	static id_manager::id_map::pointer create_id(F&& provider)
	{
		static_assert(id_manager::id_verify<T, Type>::value, "Invalid ID type combination");

		// ID traits
		using traits = id_manager::id_traits<Type>;

		if (u32 index = allocate_id<T>(traits::base, traits::step, traits::count); index < traits::count)
		{
			// Get object, store it
			const bool success = provider(&id_manager::typeinfo<T>.id_array[index].obj[0]);

			if (success)
			{
				return &id_manager::typeinfo<T>.id_array[index];
			}
		}

		return nullptr;
	}

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Get last ID (updated in create_id/allocate_id)
	static inline u32 last_id()
	{
		return g_id;
	}

	template <typename T, typename Get = T>
	class lock_object
	{
	protected:
		entry_type<T>* control;

	public:
		lock_object() = default; 

		~lock_object()
		{
			if (control) control->refmtx.unlock();
		}

		void operator ++(int) const noexcept
 		{
			control->refmtx.lock();
		}

		void operator --(int) const noexcept
 		{
			control->refmtx.unlock();
		}

		explicit operator bool() const
		{
			return !!control;
		}

		Get* operator ->() const
		{
			return reinterpret_cast<Get*>(+control->obj);
		}

		Get& operator *() const
		{
			return *(operator ->());
		}

		ref_object& operator =(ref_object& other)
		{
			control = other.control;
		}
	};

	template <typename Result, typename T, typename Get = T>
	class lock_object_res
	{
		entry_type<T>* const control;

	public:
		const Result ret;

		lock_object_res() = default; 

		~lock_object_res()
		{
			if (control) control->refmtx.lock();
		}

		void operator ++(int) const noexcept
 		{
			control->refmtx.lock();
		}

		void operator --(int) const noexcept
 		{
			control->refmtx.unlock();
		}

		explicit operator bool() const
		{
			return !!control;
		}

		Get* operator ->() const
		{
			return reinterpret_cast<Get*>(+control->obj);
		}

		Get& operator *() const
		{
			return *(operator ->());
		}

		u32 id() const
		{
			return control->id;
		}
	};

	template <typename T, typename Get = T>
	class ref_object
	{
	protected:
		entry_type<T>* control;

	public:
		ref_object() = default; 

		~ref_object()
		{
			if (control) control->refmtx.unlock_shared();
		}

		void operator ++(int) const noexcept
 		{
			control->refmtx.lock_shared();
		}

		void operator --(int) const noexcept
 		{
			control->refmtx.unlock_shared();
		}

		explicit operator bool() const
		{
			return !!control;
		}

		Get* operator ->() const
		{
			return reinterpret_cast<Get*>(+control->obj);
		}

		Get& operator *() const
		{
			return *(operator ->());
		}

		ref_object& operator = (ref_object& other)
		{
			control = other.control;
			return *this;
		}

		u32 id() const
		{
			return control->id;
		}
	};

	template <typename Result, typename T, typename Get = T>
	class ref_object_res
	{
		entry_type<T>* const control;

	public:
		const Result ret;

		ref_object_res() = default; 

		~ref_object_res()
		{
			if (control) control->refmtx.unlock_shared();
		}

		void operator ++(int) const noexcept
 		{
			control->refmtx.lock_shared();
		}

		void operator --(int) const noexcept
 		{
			control->refmtx.unlock_shared();
		}

		explicit operator bool() const
		{
			return !!control;
		}

		Get* operator ->() const
		{
			return reinterpret_cast<Get*>(+control->obj);
		}

		Get& operator *() const
		{
			return *(operator ->());
		}
	};

	template <typename Result>
	class raw_object_res
	{
		void* const control;

	public:
		const Result ret;

		raw_object_res() = default; 

		explicit operator bool() const
		{
			return !!control;
		}
	};

	template <typename T, typename Get = T>
	class weak_ref
	{
		friend class ref_object;
		friend class lock_object;

		entry_type<T>* control;
		u64 creation_count;
	public:
		weak_ref()
			: control({})
			, creation_count({})
		{
		}

		weak_ref(ref_object<T, Get>& ref)
			: control(ref.control)
			, creation_count(ref.control->creation_count)
		{
		}

		ref_object<T, Get> ref() const
		{
			control->refmtx.lock_shared();

			if (control->creation_count != creation_count || !idm::check_state(control->alloc_state))
			{
				control->refmtx.unlock_shared();
				return {};
			}
	
			return {control};
		}

		lock_object<T, Get> lock() const
		{
			control->refmtx.lock();

			if (control->creation_count != creation_count || !idm::check_state(control->alloc_state))
			{
				control->refmtx.unlock();
				return {};
			}

			return {control};
		}

		operator bool() const
		{
			return !!control &&
				(volatile u64&)control->creation_count == creation_count && idm::check_state(control->alloc_state);
		}

		void clear()
		{
			control = {};
		}

		weak_ref& operator =(ref_object& other)
		{
			control = other.control;
			if (control) creation_count = control->creation_count;
			return *this;
		}
	};

	// Add a new ID of specified type with specified constructor arguments (returns object or nullptr)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, lock_object<T, Make>> make_ptr(Args&&... args)
	{
		if (auto obj = create_id<T, Make>([&] (Make& obj) { ::new(&obj) Make(std::forward<Args>(args)...); return true; }))
		{
			return {obj};
		}

		return {};
	}

	// Add a new ID of specified type with specified constructor arguments (returns id)
	template <typename T, typename Make = T, typename... Args>
	static inline std::enable_if_t<std::is_constructible<Make, Args...>::value, u32> make(Args&&... args)
	{
		if (auto obj = create_id<T, Make>([&] (Make& obj) { ::new(&obj) Make(std::forward<Args>(args)...); return true; }))
		{
			obj->refmtx.unlock();
			return last_id();
		}

		return id_manager::id_traits<Make>::invalid;
	}

	// Add a new ID for an existing object provided (returns new id)
	template <typename T, typename Made = T>
	static inline u32 import_existing(T&& obj)
	{
		if (auto found = create_id<T, Made>([&] { return obj; }))
		{
			found->refmtx.unlock();
			return last_id();
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Add a new ID for an object returned by provider()
	template <typename T, typename Made = T, typename F, typename = std::invoke_result_t<F, Made&>>
	static inline u32 import(F&& provider)
	{
		if (const auto obj = create_id<T, Made>(std::forward<F>(provider)))
		{
			obj->refmtx.unlock();
			return last_id();
		}

		return id_manager::id_traits<Made>::invalid;
	}

	// Check the ID without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline bool check(u32 id)
	{
		if (const auto found = find_id<T, Get, false, !std::is_same<T, Get>::value>(id))
		{
			if constexpr (!std::is_same<T, Get>::value)
			{
				if (found) found->refmtx.unlock_shared();
			}

			return !!found;
		}

		return nullptr;
	}

	// Check the ID, access object under shared lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline std::conditional_t<std::is_void_v<FRT>, bool, raw_object_res<FRT>> check(u32 id, F&& func)
	{
		if (const auto ptr = find_id<T, Get>(id))
		{
			if constexpr (!std::is_void_v<FRT>)
			{
				const auto res = func(*ptr);
				ptr->refmtx.unlock_shared();
				return {true, std::move(res)};
			}
			else
			{
				func(*ptr);
				ptr->refmtx.unlock_shared();
				return true;
			}
		}

		if constexpr (!std::is_void_v<FRT>)
		{
			return {};
		}
		else
		{
			return false;
		}
	}

	// Get the object without locking (can be called from other method)
	template <typename T, typename Get = T>
	static inline ref_object<T, Get> get_unlocked(u32 id)
	{
		const auto found = find_id<T, Get>(id);

		return {found};
	}

	// Get the object
	template <typename T, typename Get = T>
	static inline ref_object<T, Get> get(u32 id)
	{
		reader_lock lock(id_manager::typeinfo<T>.g_mutex);

		const auto found = find_id<T, Get>(id);

		return {found};
	}

	// Get the object, access object under reader lock
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline std::conditional_t<std::is_void_v<FRT>, ref_object<T, Get>, ref_object_res<FRT, T, Get>> get(u32 id, F&& func)
	{
		reader_lock lock(id_manager::typeinfo<T>.g_mutex);

		const auto found = find_id<T, Get>(id);

		if (UNLIKELY(found == nullptr))
		{
			return {};
		}

		const auto ptr = reinterpret_cast<Get*>(+found->obj);

		if constexpr (std::is_void_v<FRT>)
		{
			func(*ptr);
			return {ptr};
		}
		else
		{
			return {ptr, func(*ptr)};
		}
	}

	// Get the object with a writer lock
	template <typename T, typename Get = T>
	static inline lock_object<T, Get> lock(u32 id)
	{
		const auto found = find_id<T, Get, true>(id);

		return {found};
	}

	// Access all objects of specified type. Returns the number of objects processed.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::void_type>
	static inline u32 select(F&& func, int = 0)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		using object_type = typename function_traits<FT>::object_type;

		u32 result = 0;

		for (auto& id : id_manager::typeinfo<T>.id_array)
		{
			if (!check_state(id.alloc_state))
			{
				continue;
			}

			id.refmtx.lock_shared();

			if (check_state(id.alloc_state))
			{
				if (std::is_same<T, Type>::value || id.type == id_manager::id_traits<Get>::base)
				{
					func(id.id, *reinterpret_cast<object_type*>(+id.obj));
					result++;
				}
			}

			id.refmtx.unlock_shared();
		}

		return result;
	}

	// Access all objects of specified type. If function result evaluates to true, stop and return the object and the value.
	template <typename T, typename Get = T, typename F, typename FT = decltype(&std::decay_t<F>::operator()), typename FRT = typename function_traits<FT>::result_type>
	static inline std::conditional_t<std::is_void_v<FRT>, ref_object<T, Get>, ref_object_res<FRT, T, Get>> select(F&& func)
	{
		static_assert(id_manager::id_verify<T, Get>::value, "Invalid ID type combination");

		using object_type = typename function_traits<FT>::object_type;

		for (auto& id : id_manager::typeinfo<T>.id_array)
		{
			if (!check_state(id.alloc_state))
			{
				continue;
			}

			id.refmtx.lock_shared();

			if (check_state(id.alloc_state))
			{
				const auto ptr = reinterpret_cast<object_type*>(+id.obj);

				if (std::is_same<T, Get>::value || id.type == id_manager::id_traits<Get>::base)
				{
					if (FRT result = func(id.id, *ptr))
					{
						return {ptr, std::move(result)};
					}
				}
			}

			id.refmtx.unlock_shared();
		}

		return {};
	}

	// Remove the ID
	template <typename T, typename Get = T>
	static inline bool remove(u32 id)
	{
		if (const auto found = find_id<T, Get, true>(id))
		{
			// Reset state
			found->id = id_manager::id_traits<Get>::invalid;
			found->alloc_state = entry_state::dealloc;
			found->refmtx.unlock();
			return true;
		}

		return false;
	}

	// Remove the ID and return the object
	template <typename T, typename Get = T>
	static inline lock_object<T, Get> withdraw(u32 id)
	{
		if (const auto found = find_id<T, Get>(id))
		{
			found->id = id_manager::id_traits<Get>::invalid;
			found->alloc_state = entry_state::dealloc;
			return {found};
		}

		return {};
	}

	// Remove the ID after accessing the object under writer lock, return the object and propagate return value
	template <typename T, typename Get = T, typename F, typename FRT = std::invoke_result_t<F, Get&>>
	static inline std::conditional_t<std::is_void_v<FRT>, lock_object<T, Get>, lock_object_res<FRT, T, Get>> withdraw(u32 id, F&& func)
	{
		if (const auto found = find_id<T, Get, true>(id))
		{
			const auto _ptr = reinterpret_cast<Get*>(+found->obj);

			if constexpr (std::is_void_v<FRT>)
			{
				func(*_ptr);
				return {found};
			}
			else
			{
				FRT ret = func(*_ptr);

				if (ret)
				{
					found->refmtx.unlock();

					// If return value evaluates to true, don't delete the object (error code)
					return {nullptr, std::move(ret)};
				}

				found->id = id_manager::id_traits<Make>::invalid;
				found->alloc_state = entry_state::withdraw;
				return {found, std::move(ret)};
			}
		}

		return {};
	}
};

// Object manager for emulated process. One unique object per type, or zero.
class fxm
{
	friend class idm;

public:
	// Initialize object manager
	static void init();

	// Remove all objects
	static void clear();

	// Create the object (returns nullptr if it already exists)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			auto& cur = g_vec[get_type<T>()];

			if (!cur)
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);
				cur = ptr;
			}
			else
			{
				return nullptr;
			}
		}

		return ptr;
	}

	// Create the object unconditionally (old object will be removed if it exists)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> make_always(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			std::lock_guard lock(id_manager::g_mutex);

			auto& cur = g_vec[get_type<T>()];

			ptr = std::make_shared<Make>(std::forward<Args>(args)...);
			old = std::move(cur);
			cur = ptr;
		}

		return ptr;
	}

	// Emplace the object returned by provider() and return it if no object exists
	template <typename T, typename F, typename... Args>
	static auto import(F&& provider, Args&&... args) -> decltype(static_cast<std::shared_ptr<T>>(provider(std::forward<Args>(args)...)))
	{
		std::shared_ptr<T> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			auto& cur = g_vec[get_type<T>()];

			if (!cur)
			{
				ptr = provider(std::forward<Args>(args)...);

				if (ptr)
				{
					cur = ptr;
				}
			}

			if (!ptr)
			{
				return nullptr;
			}
		}

		return ptr;
	}

	// Emplace the object return by provider() (old object will be removed if it exists)
	template <typename T, typename F, typename... Args>
	static auto import_always(F&& provider, Args&&... args) -> decltype(static_cast<std::shared_ptr<T>>(provider(std::forward<Args>(args)...)))
	{
		std::shared_ptr<T> ptr;
		std::shared_ptr<void> old;
		{
			std::lock_guard lock(id_manager::g_mutex);

			auto& cur = g_vec[get_type<T>()];

			ptr = provider(std::forward<Args>(args)...);

			if (ptr)
			{
				old = std::move(cur);
				cur = ptr;
			}
			else
			{
				return nullptr;
			}
		}

		return ptr;
	}

	// Get the object unconditionally (create an object if it doesn't exist)
	template <typename T, typename Make = T, typename... Args>
	static std::enable_if_t<std::is_constructible<Make, Args...>::value, std::shared_ptr<T>> get_always(Args&&... args)
	{
		std::shared_ptr<T> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);

			auto& old = g_vec[get_type<T>()];

			if (old)
			{
				return {old, static_cast<T*>(old.get())};
			}
			else
			{
				ptr = std::make_shared<Make>(std::forward<Args>(args)...);
				old = ptr;
			}
		}

		return ptr;
	}

	// Unsafe version of check(), can be used in some cases
	template <typename T>
	static inline T* check_unlocked()
	{
		return static_cast<T*>(g_vec[get_type<T>()].get());
	}

	// Check whether the object exists
	template <typename T>
	static inline T* check()
	{
		reader_lock lock(id_manager::g_mutex);

		return check_unlocked<T>();
	}

	// Get the object (returns nullptr if it doesn't exist)
	template <typename T>
	static inline std::shared_ptr<T> get()
	{
		reader_lock lock(id_manager::g_mutex);

		auto& ptr = g_vec[get_type<T>()];

		return {ptr, static_cast<T*>(ptr.get())};
	}

	// Delete the object
	template <typename T>
	static inline bool remove()
	{
		std::shared_ptr<void> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()]);
		}

		return ptr.operator bool();
	}

	// Delete the object and return it
	template <typename T>
	static inline std::shared_ptr<T> withdraw()
	{
		std::shared_ptr<void> ptr;
		{
			std::lock_guard lock(id_manager::g_mutex);
			ptr = std::move(g_vec[get_type<T>()]);
		}

		return {ptr, static_cast<T*>(ptr.get())};
	}
};

#include "Utilities/typemap.h"

extern utils::typemap g_typemap;

constexpr utils::typemap* g_idm = &g_typemap;

using utils::id_new;
using utils::id_any;
using utils::id_always;
