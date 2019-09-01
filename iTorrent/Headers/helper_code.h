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

using namespace std;

std::string hash_to_string(libtorrent::sha1_hash hash) {
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

#endif /* helper_code_h */
