//
//  helper_code.h
//  iTorrent
//
//  Created by Daniil Vinogradov on 17.05.2018.
//  Copyright © 2018  XITRIX. All rights reserved.
//
#include <stdio.h>
#include <fstream>

#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent_bridge/settings_pack_struct.h"
#include "libtorrent_bridge/helper_code.h"

using namespace std;

namespace TorrentHandle {
    bool isChecking(lt::torrent_handle handle)
    {
        return ((handle.status().state == lt::torrent_status::checking_files)
                || (handle.status().state == lt::torrent_status::checking_resume_data));
    }
}

std::string hash_to_string(libtorrent::sha1_hash hash) {
	std::stringstream ss;
	ss << hash;
	return ss.str();
}

std::string hash_to_string(libtorrent::sha256_hash hash) {
    std::stringstream ss;
    ss << hash;
    return ss.str();
}

bool exists (const std::string& name) {
	return ( access( name.c_str(), F_OK ) != -1 );
}

void setAutoManaged(libtorrent::torrent_handle m_nativeHandle, const bool enable)
{
    if (enable)
        m_nativeHandle.set_flags(lt::torrent_flags::auto_managed);
    else
        m_nativeHandle.unset_flags(lt::torrent_flags::auto_managed);
}

int save_file(std::string const& filename, std::vector<char> const& v)
{
    std::fstream f(filename, std::ios_base::trunc | std::ios_base::out | std::ios_base::binary);
    f.write(v.data(), v.size());
    return !f.fail();
}

std::vector<lt::download_priority_t> toLTDownloadPriorities(const vector<int> &priorities)
{
    std::vector<lt::download_priority_t> out;
    out.reserve(priorities.size());

    std::transform(priorities.cbegin(), priorities.cend()
                   , std::back_inserter(out), [](int priority)
    {
        return static_cast<lt::download_priority_t>(
                    static_cast<lt::download_priority_t>(priority));
    });
    return out;
}

lt::settings_pack::proxy_type_t proxyTypeConverter(settings_pack_struct *pack) {
    switch (pack->proxy_type) {
        case none:
            return lt::settings_pack::proxy_type_t::none;
        case socks4:
            return lt::settings_pack::proxy_type_t::socks4;
        case socks5:
            return pack->proxy_requires_auth ?
                lt::settings_pack::proxy_type_t::socks5_pw :
                lt::settings_pack::proxy_type_t::socks5;
        case http:
            return pack->proxy_requires_auth ?
                lt::settings_pack::proxy_type_t::http_pw :
                lt::settings_pack::proxy_type_t::http;
        case i2p_proxy:
            return lt::settings_pack::proxy_type_t::i2p_proxy;
    }
}

void applySettingsPackProxyHelper(lt::settings_pack *pack, settings_pack_struct *_pack) {
    pack->set_int(lt::settings_pack::proxy_type, proxyTypeConverter(_pack));
    if (_pack->proxy_type != none) {
        pack->set_int(lt::settings_pack::proxy_port, _pack->proxy_port);
        pack->set_str(lt::settings_pack::proxy_hostname, _pack->proxy_hostname);
        if (_pack->proxy_requires_auth) {
            pack->set_str(lt::settings_pack::proxy_username, _pack->proxy_username);
            pack->set_str(lt::settings_pack::proxy_password, _pack->proxy_password);
        }
        pack->set_bool(lt::settings_pack::proxy_peer_connections, _pack->proxy_peer_connections);
        pack->set_bool(lt::settings_pack::proxy_tracker_connections, true);
        pack->set_bool(lt::settings_pack::proxy_hostnames, true);
    }
}

void applySettingsPackHelper(lt::settings_pack *pack, settings_pack_struct *_pack) {
    pack->set_int(lt::settings_pack::int_types::download_rate_limit, _pack->download_limit);
    pack->set_int(lt::settings_pack::int_types::upload_rate_limit, _pack->upload_limit);
    
    pack->set_bool(lt::settings_pack::enable_dht, _pack->enable_dht);
    pack->set_bool(lt::settings_pack::enable_lsd, _pack->enable_lsd);
    pack->set_bool(lt::settings_pack::enable_incoming_utp, _pack->enable_utp);
    pack->set_bool(lt::settings_pack::enable_outgoing_utp, _pack->enable_utp);
    pack->set_bool(lt::settings_pack::enable_upnp, _pack->enable_upnp);
    pack->set_bool(lt::settings_pack::enable_natpmp, _pack->enable_natpmp);
    pack->set_str(lt::settings_pack::outgoing_interfaces, _pack->outgoing_interfaces);
    pack->set_str(lt::settings_pack::listen_interfaces, _pack->listen_interfaces);
    
    pack->set_int(lt::settings_pack::active_limit, _pack->max_active_torrents_limit);
    pack->set_int(lt::settings_pack::active_downloads, _pack->max_download_torrents_limit);
    pack->set_int(lt::settings_pack::active_seeds, _pack->max_upload_torrents_limit);
    
//    if (strlen(_pack->outgoing_interfaces) != 0) {
//        pack->set_str(lt::settings_pack::listen_interfaces, std::string(_pack->outgoing_interfaces) + ":" + std::to_string(_pack->port_range_first));
//    } else {
//        pack->set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + std::to_string(_pack->port_range_first) +  ",[::]:" + std::to_string(_pack->port_range_first));
//    }
    
    if (_pack->port_range_second - _pack->port_range_first >= 0)
        pack->set_int(lt::settings_pack::max_retry_port_bind, _pack->port_range_second - _pack->port_range_first);
    
    applySettingsPackProxyHelper(pack, _pack);
}

