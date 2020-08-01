/*

Copyright (c) 2016, Steven Siloti
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the distribution.
    * Neither the name of the author nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "libtorrent/aux_/ip_notifier.hpp"
#include "libtorrent/assert.hpp"

#if defined TORRENT_BUILD_SIMULATOR
// TODO: simulator support
#elif TORRENT_USE_NETLINK
#include "libtorrent/netlink.hpp"
#include "libtorrent/socket.hpp"
#include <array>
#elif TORRENT_USE_SYSTEMCONFIGURATION
#include <SystemConfiguration/SystemConfiguration.h>
#elif defined TORRENT_WINDOWS
#include "libtorrent/aux_/throw.hpp"
#include "libtorrent/aux_/disable_warnings_push.hpp"
#include <boost/asio/windows/object_handle.hpp>
#include <iphlpapi.h>
#include "libtorrent/aux_/disable_warnings_pop.hpp"
#endif

namespace libtorrent { namespace aux {

namespace {


template <typename T> void CFRefRetain(T h) { CFRetain(h); }
template <typename T> void CFRefRelease(T h) { CFRelease(h); }

template <typename T
	, void (*Retain)(T) = CFRefRetain<T>, void (*Release)(T) = CFRefRelease<T>>
struct CFRef
{
	CFRef() = default;
	explicit CFRef(T h) : m_h(h) {} // take ownership
	~CFRef() { release(); }

	CFRef(CFRef&& rhs) : m_h(rhs.m_h) { rhs.m_h = nullptr; }
	CFRef& operator=(CFRef&& rhs)
	{
		if (m_h == rhs.m_h) return *this;
		release();
		m_h = rhs.m_h;
		rhs.m_h = nullptr;
		return *this;
	}

	CFRef(CFRef const& rhs) : m_h(rhs.m_h) { retain(); }
	CFRef& operator=(CFRef const& rhs)
	{
		if (m_h == rhs.m_h) return *this;
		release();
		m_h = rhs.m_h;
		retain();
		return *this;
	}

	CFRef& operator=(T h) { m_h = h; return *this;}
	CFRef& operator=(std::nullptr_t) { release(); return *this;}

	T get() const { return m_h; }
	explicit operator bool() const { return m_h != nullptr; }

private:
	T m_h = nullptr; // handle

	void retain() { if (m_h != nullptr) Retain(m_h); }
	void release() { if (m_h != nullptr) Release(m_h); m_h = nullptr; }
};

void CFDispatchRetain(dispatch_queue_t q) { dispatch_retain(q); }
void CFDispatchRelease(dispatch_queue_t q) { dispatch_release(q); }
using CFDispatchRef = CFRef<dispatch_queue_t, CFDispatchRetain, CFDispatchRelease>;

struct ip_change_notifier_impl final : ip_change_notifier
{
	explicit ip_change_notifier_impl(io_service& ios)
		: m_ios(ios) {}

	void async_wait(std::function<void(error_code const&)> cb) override
	{
		m_ios.post([cb]()
		{ cb(make_error_code(boost::system::errc::not_supported)); });
	}

	void cancel() override {}

private:
	io_service& m_ios;
};

} // anonymous namespace

	std::unique_ptr<ip_change_notifier> create_ip_notifier(io_service& ios)
	{
		return std::unique_ptr<ip_change_notifier>(new ip_change_notifier_impl(ios));
	}
}}
