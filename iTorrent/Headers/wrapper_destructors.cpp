//
//  wrapper_destructors.cpp
//  iTorrent
//
//  Created by Daniil Vinogradov on 05.04.2020.
//  Copyright © 2020  XITRIX. All rights reserved.
//

#include <stdio.h>
#include "result_struct.h"
#include "file_struct.h"
#include "settings_pack_struct.h"
#include "peer_struct.h"

extern "C" void free_result(TorrentResult res) {
    for (int i = 0; i < res.count; i++) {
        delete[] res.torrents[i].name;
        delete[] res.torrents[i].state;
        delete[] res.torrents[i].hash;
        delete[] res.torrents[i].creator;
        delete[] res.torrents[i].comment;
        delete[] res.torrents[i].pieces;
    }
    delete[] res.torrents;
}

extern "C" void free_files(Files files) {
    delete[] files.title;
    for (int i = 0; i < files.size; i++) {
        delete[] files.files[i].file_name;
        if (files.files[i].pieces != NULL)
            delete[] files.files[i].pieces;
    }
    delete[] files.files;
}

extern "C" void free_trackers(Trackers trackers) {
    for (int i = 0; i < trackers.size; i++) {
        delete[] trackers.trackers[i].tracker_url;
        delete[] trackers.trackers[i].messages;
    }
    delete[] trackers.trackers;
}

extern "C" void free_settings_pack(settings_pack_struct settings_pack) {
    delete[] settings_pack.proxy_hostname;
    delete[] settings_pack.proxy_password;
    delete[] settings_pack.proxy_username;
}

extern "C" void free_peer_result(PeerResult res) {
    for (int i = 0; i < res.count; i++) {
        delete[] res.peers[i].address;
        delete[] res.peers[i].client;
    }
    delete[] res.peers;
}
