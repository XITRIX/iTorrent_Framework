//
//  helper_code.h
//  iTorrent
//
//  Created by Daniil Vinogradov on 17.05.2018.
//  Copyright © 2018  XITRIX. All rights reserved.
//

#ifndef helper_code_h
#define helper_code_h

#include <stdio.h>
#include <fstream>

#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/version.hpp"
#include "libtorrent_bridge/settings_pack_struct.h"

using namespace std;

namespace TorrentHandle {
    bool isChecking(lt::torrent_handle handle);
}

libtorrent::sha1_hash get_universal_hash_from_handle(libtorrent::torrent_handle handle);
libtorrent::sha1_hash get_universal_hash_from_status(libtorrent::torrent_status status);

std::string hash_to_string(libtorrent::sha1_hash hash);

#if LIBTORRENT_VERSION_MAJOR != 1
std::string hash_to_string(libtorrent::sha256_hash hash);
#endif

bool exists (const std::string& name);

void setAutoManaged(libtorrent::torrent_handle m_nativeHandle, const bool enable);

int save_file(std::string const& filename, std::vector<char> const& v);

std::vector<lt::download_priority_t> toLTDownloadPriorities(const vector<int> &priorities);

lt::settings_pack::proxy_type_t proxyTypeConverter(settings_pack_struct *pack);

void applySettingsPackProxyHelper(lt::settings_pack *pack, settings_pack_struct *_pack);

void applySettingsPackHelper(lt::settings_pack *pack, settings_pack_struct *_pack);

#endif /* helper_code_h */

