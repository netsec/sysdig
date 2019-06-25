/*
Copyright (C) 2019 Draios Inc dba Sysdig.

This file is part of sysdig.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#pragma once

#include <mutex>
#include <thread>

namespace libsinsp {
template<typename T>
class ConstMutexGuard;

/**
 * \brief A wrapper to allow synchronized access to a value owned by a Mutex<T>
 *
 * @tparam T type of the value protected by the mutex
 *
 * It works by simply holding a `std::unique_lock` object that keeps the mutex
 * locked while it exists and unlocks it upon destruction
 */
template<typename T>
class MutexGuard {
public:
	MutexGuard(std::unique_lock<std::mutex> lock, T *inner) : m_lock(std::move(lock)), m_inner(inner) {}

	// we cannot copy a MutexGuard, only move
	MutexGuard(MutexGuard &&rhs) noexcept : m_lock(std::move(rhs.m_lock)),
						m_inner(rhs.m_inner) {}

	T *operator->()
	{
		return m_inner;
	}

	T &operator*()
	{
		return *m_inner;
	}

private:
	std::unique_lock<std::mutex> m_lock;
	T *m_inner;

	friend class ConstMutexGuard<T>;
};

/**
 * \brief A wrapper to allow synchronized const access to a value owned by a Mutex<T>
 *
 * @tparam T type of the value protected by the mutex
 *
 * It works by simply holding a `std::unique_lock` object that keeps the mutex
 * locked while it exists and unlocks it upon destruction
 */
template<typename T>
class ConstMutexGuard {
public:
	ConstMutexGuard(std::unique_lock<std::mutex> lock, const T *inner) : m_lock(std::move(lock)),
									     m_inner(inner) {
	}

	// we cannot copy a ConstMutexGuard, only move
	ConstMutexGuard(ConstMutexGuard &&rhs) noexcept : m_lock(std::move(rhs.m_lock)),
							  m_inner(rhs.m_inner) {}

	// a writable guard can be demoted to a read-only one, but *not* the other way around
	ConstMutexGuard(MutexGuard<T> &&rhs) noexcept : m_lock(std::move(rhs.m_lock)),
	                                                m_inner(rhs.m_inner) // NOLINT(google-explicit-constructor)
	{}

	const T *operator->() const
	{
		return m_inner;
	}

	const T &operator*() const
	{
		return *m_inner;
	}

private:
	std::unique_lock<std::mutex> m_lock;
	const T *m_inner;
};

/**
 * \brief Wrap a value of type T, enforcing synchronized access
 *
 * @tparam T type of the wrapped value
 *
 * The class owns a value of type T and a mutex. The only way to access the T inside
 * is via the lock() method, which returns a guard object that unlocks the mutex
 * once it falls out of scope
 */
template<typename T>
class Mutex {
public:
	Mutex() = default;

	Mutex(T inner) : m_inner(std::move(inner)) {}

	MutexGuard<T> lock()
	{
		return MutexGuard<T>(std::unique_lock<std::mutex>(m_lock), &m_inner);
	}

	ConstMutexGuard<T> lock() const
	{
		return ConstMutexGuard<T>(std::unique_lock<std::mutex>(m_lock), &m_inner);
	}

private:
	mutable std::mutex m_lock;
	T m_inner;
};
}