//
//  wrapper.cpp
//  iTorrent
//
//  Created by  XITRIX on 12.05.2018.
//  Copyright © 2018  XITRIX. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fstream>
#include <list>
#include <map>
#include <thread>
#include "libtorrent/alert_types.hpp"
#include "libtorrent/announce_entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/read_resume_data.hpp"
#include "libtorrent/write_resume_data.hpp"
#include <boost/make_shared.hpp>
#include "libtorrent_bridge/result_struct.h"
#include "libtorrent_bridge/file_struct.h"
#include "libtorrent_bridge/settings_pack_struct.h"
#include "libtorrent_bridge/peer_struct.h"

#include "libtorrent_bridge/helper_code.h"

using namespace libtorrent;
using namespace std;
class Engine {
public:
    static Engine *standart;
    session *s;
    //std::string root_path;
	std::string config_path;
	std::string download_path;
    std::string client_name;
    vector<torrent_handle> handlers;
    
    bool isPreallocationEnabled = false;
    
    Engine(std::string client_name, std::string download_path, std::string config_path, settings_pack_struct settings_pack) {
        Engine::standart = this;
        this->download_path = download_path;
		this->config_path = config_path;
        this->client_name = client_name;
        
        struct settings_pack pack;
        pack.set_str(settings_pack::user_agent, client_name);
        pack.set_int(lt::settings_pack::alert_mask
                        , lt::alert_category::error
                        | lt::alert_category::peer
                        | lt::alert_category::port_mapping
                        | lt::alert_category::storage
                        | lt::alert_category::tracker
                        | lt::alert_category::connect
                        | lt::alert_category::status
                        | lt::alert_category::ip_block
                        | lt::alert_category::performance_warning
                        | lt::alert_category::dht
                        | lt::alert_category::incoming_request
                        | lt::alert_category::dht_operation
                        | lt::alert_category::port_mapping_log
                        | lt::alert_category::file_progress);
        
        applySettingsPackHelper(&pack, &settings_pack);

        printf("LibTorrent version: %s\n", version());
        s = new lt::session(pack);
    }
    
    char* addTorrentByName(char* torrent) {
        try {
            auto handle = addTorrent(torrent);
            char* res = new char[hash_to_string(handle.status().info_hash).length() + 1];
            strcpy(res, hash_to_string(handle.status().info_hash).c_str());
            return res;
        } catch (boost::system::system_error const & e) {
            // send error
            std::string sres = "-1";
            char* res = new char[sres.length() + 1];
            strcpy(res, sres.c_str());
            return res;
        }
    }
    
    void addTorrentWithStates(char* torrent, int states[]) {
        auto handle = addTorrent(torrent);
        std::shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent);
        vector<int> prior = vector<int>(&states[0], &states[sti->num_files()]);
		handle.prioritize_files(toLTDownloadPriorities(prior));
    }
    
    torrent_handle addTorrent(char* torrent) {
        lt::error_code ec;
               
        std::shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent);
        std::string path = Engine::standart->config_path + "/.FastResumes/" + hash_to_string(sti->info_hash()) + ".fastresume";
        
        lt::add_torrent_params p;
        std::ifstream ifs(path, std::ios_base::binary);
        if (ifs.good()) {
            ifs.unsetf(std::ios_base::skipws);
        
            std::vector<char> buf{std::istream_iterator<char>(ifs)
            , std::istream_iterator<char>()};
            
            auto resume = lt::read_resume_data(buf, ec);
            if (ec.value() == 0) {
                p = resume;
            }
        }
        p.ti = sti;
        p.save_path = download_path;
        
        p.storage_mode = isPreallocationEnabled
            ? lt::storage_mode_allocate : lt::storage_mode_sparse;
        
        torrent_handle handle = s->add_torrent(p, ec);
        handlers.push_back(handle);
        
        return handle;
    }
    
    char* addMagnet(char* magnetLink) {
        add_torrent_params p;
        lt::error_code ec;
        parse_magnet_uri(magnetLink, p, ec);
        
        std::string hash;
        
        if (ec.value() != 0) {
            // send error
            hash = "-1";
            char* res = new char[hash.length() + 1];
            strcpy(res, hash.c_str());
            return res;
        }
        
        p.save_path = download_path;
        
        p.storage_mode = isPreallocationEnabled
            ? lt::storage_mode_allocate : lt::storage_mode_sparse;
        
        torrent_handle handle = s->add_torrent(p, ec);
        
        if (ec.value() != 0) {
            // send error
            hash = "-1";
            char* res = new char[hash.length() + 1];
            strcpy(res, hash.c_str());
            return res;
        }
        
        handle.unset_flags(lt::torrent_flags::stop_when_ready);
        handlers = s->get_torrents();
        
        hash = hash_to_string(handle.status().info_hash);
        char* res = new char[hash.length() + 1];
        strcpy(res, hash.c_str());
        return res;
    }
    
    torrent_handle* getHandleByHash(char* torrent_hash) {
        for (int i = 0; i < handlers.size(); i++) {
			std::string s = hash_to_string(handlers[i].status().info_hash);
            if (strcmp(s.c_str(), torrent_hash) == 0) {
                return &(handlers[i]);
            }
        }
        return NULL;
    }
    
    void printAllHandles() {
        for(int i = 0; i < handlers.size(); i++) {
            torrent_status stat = handlers[i].status();
            
            std::string state_str[] = {"queued", "checking", "downloading metadata", "downloading", "finished", "seeding", "allocating", "checking fastresume"};
            //printf("\r%.2f%% complete (down: %.1d kb/s up: %.1d kB/s peers: %d) %s", stat.progress * 100, stat.download_rate / 1000, stat.upload_rate / 1000, stat.num_peers, state_str[stat.state].c_str());
        }
    }
};

Engine *Engine::standart = NULL;
extern "C" int init_engine(char* client_name, char* download_path, char* config_path, settings_pack_struct settings_pack) {
    new Engine(client_name, download_path, config_path, settings_pack);
    return 0;
}

extern "C" void apply_settings_pack(settings_pack_struct settings_pack) {
    struct settings_pack pack = Engine::standart->s->get_settings();
    applySettingsPackHelper(&pack, &settings_pack);
    Engine::standart->s->apply_settings(pack);
}

// Parameters
extern "C" void set_storage_preallocation(int preallocate) {
    Engine::standart->isPreallocationEnabled = preallocate == 1;
}

extern "C" int get_storage_preallocation() {
    return Engine::standart->isPreallocationEnabled;
}
// Parameters

extern "C" char* add_torrent(char* torrent_path) {
	return Engine::standart->addTorrentByName(torrent_path);
}

extern "C" void add_torrent_with_states(char* torrent_path, int states[]) {
    Engine::standart->addTorrentWithStates(torrent_path, states);
}

extern "C" char* add_magnet(char* magnet_link) {
    return Engine::standart->addMagnet(magnet_link);
}

extern "C" void remove_torrent(char* torrent_hash, int remove_files) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	
    std::string path = Engine::standart->config_path + "/.FastResumes/" + torrent_hash + ".fastresume";
    remove(path.c_str());
	if (remove_files == 1) {
		Engine::standart->s->remove_torrent(*handle, session::delete_files);
	} else {
		Engine::standart->s->remove_torrent(*handle);
	}
	Engine::standart->handlers = Engine::standart->s->get_torrents();
}

extern "C" char* get_torrent_file_hash(char* torrent_path) {
    std::string s;
    try {
        std::shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent_path);
        if (sti != NULL) {
                s = hash_to_string(sti->info_hash());
        } else {
            // send error
            std::string sres = "-1";
            char* res = new char[sres.length() + 1];
            strcpy(res, sres.c_str());
            return res;
        }
    } catch (boost::system::system_error const & e) {
        //printf("%s: %i - %s", e.what(), e.code().value(), e.code().message().c_str());
        // send error
        std::string sres = "-1";
        char* res = new char[sres.length() + 1];
        strcpy(res, sres.c_str());
        return res;
    }
    
    char* res = new char[s.length() + 1];
    strcpy(res, s.c_str());
    return res;
}

extern "C" char* get_magnet_hash(char* magnet_link) {
	add_torrent_params params;
	lt::error_code ec;
	parse_magnet_uri(magnet_link, params, ec);
    std::string s;
    if (ec.value() != 0) {
        s = "-1";
    } else {
        s = hash_to_string(params.info_hash);
    }
    
	char* res = new char[s.length() + 1];
	strcpy(res, s.c_str());
	return res;
}

extern "C" char* get_torrent_magnet_link(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    std::string sres = make_magnet_uri(*handle).c_str();
    char* res = new char[sres.size() + 1];
    strcpy(res, sres.c_str());
    return res;
}

extern "C" Files get_files_of_torrent(const torrent_info* info) {
    file_storage fs = info->files();
    Files files {
        .size = fs.num_files(),
        .files = new File[fs.num_files()]
    };
    files.title = new char[info->name().length() + 1];
    
    strcpy(files.title, info->name().c_str());
    for (int i = 0; i < fs.num_files(); i++) {
        files.files[i].file_name = new char[fs.file_path(i).length() + 1];
        strcpy(files.files[i].file_name, fs.file_path(i).c_str());
        
        files.files[i].file_size = fs.file_size(i);
        files.files[i].pieces = NULL;
    }
    return files;
}

extern "C" int get_torrent_files_sequental(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    return bool {handle->status().flags & lt::torrent_flags::sequential_download};
    //return handle->is_sequential_download();
}

extern "C" void set_torrent_files_sequental(char* torrent_hash, int sequental) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    
    if (sequental == 1) {
        handle->set_flags(lt::torrent_flags::sequential_download);
    }
    else {
        handle->unset_flags(lt::torrent_flags::sequential_download);
    }
}

extern "C" void set_torrent_files_priority(char* torrent_hash, int states[]) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    vector<int> prior = vector<int>(&states[0], &states[handle->torrent_file()->num_files()]);
	handle->prioritize_files(toLTDownloadPriorities(prior));
}

extern "C" void set_torrent_file_priority(char* torrent_hash, int file_number, int state) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    handle->file_priority(file_number, state);
}

extern "C" void start_torrent(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    handle->unset_flags(lt::torrent_flags::stop_when_ready);
    setAutoManaged(*handle, true);
	handle->resume();
    //printf("Started");
}

extern "C" void stop_torrent(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    
    if (TorrentHandle::isChecking(*handle)) {
        handle->set_flags(lt::torrent_flags::stop_when_ready);
    }
    
    setAutoManaged(*handle, false);
	handle->pause();
    //printf("Stopped");
}

extern "C" void rehash_torrent(char* torrent_hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    if (!handle->status().has_metadata) return;
    handle->force_recheck();
    setAutoManaged(*handle, true);
}

extern "C" Files get_files_of_torrent_by_path(char* torrent_path) {
    shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent_path);
	if (sti == NULL) {
		Files files {
			.size = -1
		};
		return files;
	}
    return get_files_of_torrent(sti.get());
}

extern "C" Files get_files_of_torrent_by_hash(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    if (handle == NULL) {
        Files files {
            .error = 1
        };
        return files;
    }
    
    vector<int64_t> progress;
    std::shared_ptr<const lt::torrent_info> info = handle->torrent_file();
    Files files = get_files_of_torrent(info.get());
    file_storage storage = info->files();
    handle->file_progress(progress);
    torrent_status stat = handle->status();
    
    for (int i = 0; i < files.size; i++) {
        files.files[i].file_priority = handle->file_priority(i);
        files.files[i].file_downloaded = progress[i];
        
        const auto fileSize = storage.file_size(i) > 0 ? storage.file_size(i) - 1 : 0;
        const auto fileOffset = storage.file_offset(i);
        
        const int pieceLength = info->piece_length();
        const long long beginIdx = (fileOffset / pieceLength);
        const long long endIdx = ((fileOffset + fileSize) / pieceLength);

        files.files[i].begin_idx = beginIdx;
        files.files[i].end_idx = endIdx;
        files.files[i].num_pieces = (int)(endIdx - beginIdx);
        files.files[i].pieces = new int[files.files[i].num_pieces];
        for (int j = 0; j < files.files[i].num_pieces; j++) {
            files.files[i].pieces[j] = stat.pieces.get_bit(j + (int)beginIdx);
        }
    }
    return files;
}

extern "C" void scrape_tracker(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    int trackers = (int)handle->trackers().size();
    for (int i = 0; i < trackers; i++) {
        handle->scrape_tracker(i);
    }
}

extern "C" Trackers get_trackers_by_hash(char* torrent_hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	if (handle == NULL) {
		Trackers t = {
			.size = -1
		};
		return t;
	}
	std::vector<announce_entry> trackers_list = handle->trackers();
    int size = static_cast<int>(trackers_list.size());
	Trackers trackers {
		.size = size,
        .trackers = new Tracker[size]
	};
	
	for (int i = 0; i < trackers_list.size(); i++) {
		trackers.trackers[i].tracker_url = new char[trackers_list[i].url.length() + 1];
		strcpy(trackers.trackers[i].tracker_url, trackers_list[i].url.c_str());
		
		trackers.trackers[i].messages = new char[strlen("MSG") + 1];
		strcpy(trackers.trackers[i].messages, "MSG");
		
		trackers.trackers[i].working = trackers_list[i].is_working() ? 1 : 0;
		trackers.trackers[i].verified = trackers_list[i].verified ? 1 : 0;
		
        trackers.trackers[i].seeders = -1;
        trackers.trackers[i].peers = -1;
        trackers.trackers[i].leechs = -1;
        
        for (const lt::announce_endpoint &endpoint : trackers_list[i].endpoints) {
            trackers.trackers[i].seeders = std::max(trackers.trackers[i].seeders, endpoint.scrape_complete);
            trackers.trackers[i].peers = std::max(trackers.trackers[i].peers, endpoint.scrape_incomplete);
            trackers.trackers[i].leechs = std::max(trackers.trackers[i].leechs, endpoint.scrape_downloaded);
        }
	}
	
	return trackers;
}

extern "C" int add_tracker_to_torrent(char* torrent_hash, char* tracker_url) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	if (handle == NULL) {
		return -1;
	}
	announce_entry* entry = new announce_entry(std::string(tracker_url));
	handle->add_tracker(*entry);
	
	return 0;
}

extern "C" int remove_tracker_from_torrent(char* torrent_hash, char *const tracker_url[], int count) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    if (handle == NULL) {
        return -1;
    }
    
    std::vector<announce_entry> trackers_list = handle->trackers();
    std::vector<int> indexes;
    for (int i = 0; i < count; i++) {
        announce_entry* tempEntry = new announce_entry(std::string(tracker_url[i]));
        for (int j = 0; j < trackers_list.size(); j++) {
            if (trackers_list[j].url == tempEntry->url) {
                indexes.push_back(j);
            }
        }
    }
    std::sort(indexes.begin(), indexes.end());
    std::reverse(indexes.begin(), indexes.end());
    
    for (int i = 0; i < indexes.size(); i++) {
        trackers_list.erase(trackers_list.begin() + indexes[i]);
    }
    
    handle->replace_trackers(trackers_list);
    
    return 0;
}

extern "C" void save_magnet_to_file(char* hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(hash);
	
	std::ofstream out((Engine::standart->config_path + "/" + handle->status().name.c_str() + ".torrent").c_str(), std::ios_base::binary);
	out.unsetf(std::ios_base::skipws);
	create_torrent ct = create_torrent(*handle->torrent_file().get());
	ct.set_creator(Engine::standart->client_name.c_str());
	bencode(std::ostream_iterator<char>(out), ct.generate());
}

extern "C" void resume_to_app() {
	session* ses = Engine::standart->s;
	ses->resume();
}

int generateResumeData(const bool final)
{
    int m_numResumeData = 0;
    session* ses = Engine::standart->s;
    std::vector<torrent_handle> handles = ses->get_torrents();
    for (std::vector<torrent_handle>::iterator torrent = handles.begin();
         torrent != handles.end(); ++torrent)
    {
        if (!torrent->is_valid()) {
            //printf("Not valid\n");
            continue;
        }
        if (!torrent->status().has_metadata) {
            //printf("No metadata\n");
            continue;
        }
        if (!final && !torrent->need_save_resume_data()) {
            //printf("Not need to save ");
            continue;
        }

        if (!final && !torrent->need_save_resume_data()) continue;
        if (torrent->status().state == torrent_status::checking_files
            || torrent->status().state == torrent_status::checking_resume_data //
            //|| torrent->is_paused()
            || (torrent->status().flags & torrent_flags::paused && torrent->status().errc)) { // HasError
            //printf("Error in %s is occured", torrent->status().name.c_str());
            continue;
        }

        torrent->save_resume_data();
        m_numResumeData++;
        
        //printf("%s - ADDED!!\n", torrent->name().c_str());
    }
    
    return m_numResumeData;
}

void getPendingAlerts(std::vector<lt::alert *> &out, const u_long time)
{
    TORRENT_ASSERT(out.empty());
    session* ses = Engine::standart->s;

    if (time > 0)
        ses->wait_for_alert(lt::milliseconds(time));
    ses->pop_alerts(&out);
}

extern "C" void save_fast_resume() {
    session* ses = Engine::standart->s;
    // Pause session
    ses->pause();
    
    int m_numResumeData = generateResumeData(true);
    
    //sleep(500);
    //printf("Start with count %d!!\n", m_numResumeData);
    while (m_numResumeData > 0) {
        std::vector<lt::alert *> alerts;
        getPendingAlerts(alerts, 30 * 1000);
        if (alerts.empty()) {
            //printf(" aborting with %d outstanding torrents to save resume data for\n", m_numResumeData);
            break;
        }
        
        for (const auto a : alerts) {
            //printf("Type: %d!!\n", a->type());
            
            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
                //printf("Error!!\n");
                --m_numResumeData;
                continue;
            }
            
            // when resume data is ready, save it
            if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                lt::torrent_handle h = static_cast<const lt::torrent_alert*>(a)->handle;
                auto path = Engine::standart->config_path + "/.FastResumes/" + hash_to_string(h.info_hash()) + ".fastresume";
                std::ofstream of(path, std::ios_base::binary);
                auto const b = write_resume_data_buf(rd->params);
                of.write(b.data(), b.size());
                //printf("%s - SAVED!!\n", h.name().c_str());
                --m_numResumeData;
            }
        }
    }
    //printf("Done!!\n");
}

extern "C" TorrentResult get_torrent_info() {
    std::string state_str[] = {"Queued", "Hashing", "Metadata", "Downloading", "Finished", "Seeding", "Allocating", "Checking fastresume"};
    
    int size = (int)Engine::standart->handlers.size();
    TorrentResult res{
        .count = size,
        .torrents = new TorrentInfo[size]
    };
    
    for (int i = 0; i < size; i++) {
        torrent_handle handler = Engine::standart->handlers[i];
        torrent_status stat = handler.status();
        const torrent_info* info = NULL;
        
        if (handler.status().has_metadata) {
            info = handler.torrent_file().get();
            
            res.torrents[i].num_pieces = info->num_pieces();
            res.torrents[i].pieces = new int[res.torrents[i].num_pieces];
            for (int j = 0; j < stat.pieces.size(); ++j) {
                res.torrents[i].pieces[j] = stat.pieces.get_bit(j) ? 1 : 0;
            }
        } else {
            res.torrents[i].num_pieces = 0;
            res.torrents[i].pieces = new int[0];
        }
        
        res.torrents[i].name = new char[stat.name.length() + 1];
        strcpy((char*)res.torrents[i].name, stat.name.c_str());
        
        res.torrents[i].state = new char[state_str[stat.state].length() + 1];
        strcpy((char*)res.torrents[i].state, state_str[stat.state].c_str());
        
        std::string hash = hash_to_string(stat.info_hash);
        res.torrents[i].hash = new char[hash.length() + 1];
        strcpy(res.torrents[i].hash, hash.c_str());
        
        if (info != NULL) {
            res.torrents[i].creator = new char[info->creator().length() + 1];
            strcpy((char*)res.torrents[i].creator, info->creator().c_str());
        } else {
            res.torrents[i].creator = new char[1];
            strcpy((char*)res.torrents[i].creator, "");
        }
        
        if (info != NULL) {
            res.torrents[i].comment = new char[info->comment().length() + 1];
            strcpy(res.torrents[i].comment, info->comment().c_str());
        } else {
            res.torrents[i].comment = new char[1];
            strcpy(res.torrents[i].comment, "");
        }
        
        res.torrents[i].progress = stat.progress;
        
        res.torrents[i].total_wanted = stat.total_wanted;
        
        res.torrents[i].total_wanted_done = stat.total_wanted_done;
        
        res.torrents[i].total_size = info != NULL ? info->total_size() : 0;
        
        res.torrents[i].total_done = stat.total_done;
        
        res.torrents[i].download_rate = stat.download_rate;
        
        res.torrents[i].upload_rate = stat.upload_rate;
        
        res.torrents[i].total_download = stat.total_download;
        
        res.torrents[i].total_upload = stat.total_upload;
        
        res.torrents[i].num_peers = stat.num_peers;
        
        res.torrents[i].num_seeds = stat.num_seeds;
        
        res.torrents[i].num_leechers = stat.num_peers - stat.num_seeds;
        
        int peers = stat.num_complete + stat.num_incomplete;
        res.torrents[i].num_total_peers = peers > 0 ? peers : stat.list_peers;
        
        int complete = stat.num_complete;
        res.torrents[i].num_total_seeds = complete > 0 ? complete : stat.list_seeds;
        
        int incomplete = stat.num_incomplete;
        res.torrents[i].num_total_leechers = incomplete > 0 ? incomplete : stat.list_peers - stat.list_seeds;
        
        res.torrents[i].sequential_download = bool {stat.flags & lt::torrent_flags::sequential_download} ? 1 : 0;
		
		try {
        	res.torrents[i].creation_date = info != NULL ? info->creation_date() : 0;
		} catch (...) {
			res.torrents[i].creation_date = 0;
		}
        
        res.torrents[i].is_paused = bool{stat.flags & torrent_flags::paused};
        res.torrents[i].is_finished = stat.total_wanted == stat.total_wanted_done;
        res.torrents[i].is_seed = stat.is_seeding;
		res.torrents[i].has_metadata = stat.has_metadata;
    }
    
    return res;
}

extern "C" PeerResult get_peers_by_hash(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    std::vector<peer_info> peers;
    handle->get_peer_info(peers);
    
    int size = (int)peers.size();
    PeerResult res{
        .count = size,
        .peers = new Peer[size]
    };
    for (int i = 0; i < size; i++)
    {
        res.peers[i].port = peers[i].ip.port();
        
        res.peers[i].client = new char[peers[i].client.length() + 1];
        strcpy((char*)res.peers[i].client, peers[i].client.c_str());
        
        res.peers[i].total_download = peers[i].total_download;
        res.peers[i].total_upload = peers[i].total_upload;
        
//        peers[i].flags;
//        peers[i].source;
        
        res.peers[i].up_speed = peers[i].up_speed;
        res.peers[i].down_speed = peers[i].down_speed;
        res.peers[i].connection_type = peers[i].connection_type;
        res.peers[i].progress = (int)(peers[i].progress * 100);
        res.peers[i].progress_ppm = peers[i].progress_ppm;
        
        auto address = peers[i].ip.address().to_string();
        res.peers[i].address = new char[address.length() + 1];
        strcpy((char*)res.peers[i].address, address.c_str());
    }
    
    return res;
}


// the number of times we've asked to save resume data
// without having received a response (successful or failure)
int num_outstanding_resume_data = 0;

// returns true if the alert was handled (and should not be printed to the log)
// returns false if the alert was not handled
bool handle_alert(lt::session&, lt::alert* a)
{
    using namespace lt;

    if (dht_stats_alert* p = alert_cast<dht_stats_alert>(a))
    {
//        dht_active_requests = p->active_requests;
//        dht_routing_table = p->routing_table;
        return true;
    }

//    if (torrent_need_cert_alert* p = alert_cast<torrent_need_cert_alert>(a))
//    {
//        torrent_handle h = p->handle;
//        std::string base_name = path_append("certificates", hash_to_string(h.info_hash()));
//        std::string cert = base_name + ".pem";
//        std::string priv = base_name + "_key.pem";
//
//        {
//            char msg[256];
//            std::snprintf(msg, sizeof(msg), "ERROR. could not load certificate %s: %s\n"
//                , cert.c_str(), std::strerror(errno));
//            if (g_log_file) std::fprintf(g_log_file, "[%s] %s\n", timestamp(), msg);
//            return true;
//        }
//
//        ret = ::stat(priv.c_str(), &st);
//        if (ret < 0 || (st.st_mode & S_IFREG) == 0)
//        {
//            char msg[256];
//            std::snprintf(msg, sizeof(msg), "ERROR. could not load private key %s: %s\n"
//                , priv.c_str(), std::strerror(errno));
//            if (g_log_file) std::fprintf(g_log_file, "[%s] %s\n", timestamp(), msg);
//            return true;
//        }
//
//        char msg[256];
//        std::snprintf(msg, sizeof(msg), "loaded certificate %s and key %s\n", cert.c_str(), priv.c_str());
//        if (g_log_file) std::fprintf(g_log_file, "[%s] %s\n", timestamp(), msg);
//
//        h.set_ssl_certificate(cert, priv, "certificates/dhparams.pem", "1234");
//        h.resume();
//    }

    // don't log every peer we try to connect to
    if (alert_cast<peer_connect_alert>(a)) return true;

    if (peer_disconnected_alert* pd = alert_cast<peer_disconnected_alert>(a))
    {
        // ignore failures to connect and peers not responding with a
        // handshake. The peers that we successfully connect to and then
        // disconnect is more interesting.
        if (pd->op == operation_t::connect
            || pd->error == errors::timed_out_no_handshake)
            return true;
    }

    if (metadata_received_alert* p = alert_cast<metadata_received_alert>(a))
    {
        torrent_handle h = p->handle;
        h.save_resume_data(torrent_handle::save_info_dict);
        ++num_outstanding_resume_data;
    }
    else if (add_torrent_alert* p = alert_cast<add_torrent_alert>(a))
    {
        if (p->error)
        {
            std::fprintf(stderr, "failed to add torrent: %s %s\n"
                , p->params.ti ? p->params.ti->name().c_str() : p->params.name.c_str()
                , p->error.message().c_str());
        }
        else
        {
            torrent_handle h = p->handle;

            h.save_resume_data(torrent_handle::save_info_dict | torrent_handle::only_if_modified);
            ++num_outstanding_resume_data;
        }
    }
    else if (torrent_finished_alert* p = alert_cast<torrent_finished_alert>(a))
    {
        //p->handle.set_max_connections(max_connections_per_torrent / 2);

        // write resume data for the finished torrent
        // the alert handler for save_resume_data_alert
        // will save it to disk
        torrent_handle h = p->handle;
        h.save_resume_data(torrent_handle::save_info_dict);
        ++num_outstanding_resume_data;
    }
    else if (save_resume_data_alert* p = alert_cast<save_resume_data_alert>(a))
    {
        --num_outstanding_resume_data;
        auto const buf = write_resume_data_buf(p->params);
        torrent_handle h = p->handle;
        auto path = Engine::standart->config_path + "/.FastResumes/" + hash_to_string(h.info_hash()) + ".fastresume";
        save_file(path, buf);
    }
    else if (save_resume_data_failed_alert* p = alert_cast<save_resume_data_failed_alert>(a))
    {
        --num_outstanding_resume_data;
        // don't print the error if it was just that we didn't need to save resume
        // data. Returning true means "handled" and not printed to the log
        return p->error == lt::errors::resume_data_not_modified;
    }
    else if (torrent_paused_alert* p = alert_cast<torrent_paused_alert>(a))
    {
        // write resume data for the finished torrent
        // the alert handler for save_resume_data_alert
        // will save it to disk
        torrent_handle h = p->handle;
        h.save_resume_data(torrent_handle::save_info_dict);
        ++num_outstanding_resume_data;
    }
    else if (state_update_alert* p = alert_cast<state_update_alert>(a))
    {
//        view.update_torrents(std::move(p->status));
        return true;
    }
    else if (torrent_removed_alert* p = alert_cast<torrent_removed_alert>(a))
    {
//        view.remove_torrent(std::move(p->handle));
    }
    return false;
}

extern "C" void pop_alerts() {
    std::vector<lt::alert*> alerts;
    Engine::standart->s->pop_alerts(&alerts);
    for (auto a : alerts)
    {
        if (::handle_alert(*Engine::standart->s, a)) continue;
    }
}
