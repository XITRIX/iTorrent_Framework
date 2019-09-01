//
//  torrentHandle.h
//  iTorrent
//
//  Created by Daniil Vinogradov on 01/09/2019.
//  Copyright © 2019  XITRIX. All rights reserved.
//

#ifndef torrentHandle_h
#define torrentHandle_h

#include "libtorrent/torrent_handle.hpp"

namespace TorrentHandle {

    using namespace libtorrent;
    bool isChecking(torrent_handle handle)
    {
        return ((handle.status().state == torrent_status::checking_files)
                || (handle.status().state == torrent_status::checking_resume_data));
    }
}

#endif /* torrentHandle_h */
