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

extern "C" void free_result(Result res) {
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
