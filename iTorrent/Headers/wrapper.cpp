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
#include "result_struct.h"
#include "file_struct.h"
#include "torrentHandle.h"

#include "helper_code.h"

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
    
    Engine(std::string client_name, std::string download_path, std::string config_path) {
        Engine::standart = this;
        this->download_path = download_path;
		this->config_path = config_path;
        this->client_name = client_name;
        
        settings_pack pack;
        pack.set_str(settings_pack::handshake_client_version, client_name);
        pack.set_str(settings_pack::listen_interfaces, "0.0.0.0:6881");
        pack.set_int(lt::settings_pack::alert_mask
                     , lt::alert::error_notification
                     | lt::alert::storage_notification
                     | lt::alert::status_notification);
        s = new lt::session(pack);
    }
    
    char* addTorrentByName(char* torrent) {
        auto handle = addTorrent(torrent);
		
		char* res = new char[hash_to_string(handle.status().info_hash).length() + 1];
		strcpy(res, hash_to_string(handle.status().info_hash).c_str());
		return res;
    }
    
    void addTorrentWithStates(char* torrent, int states[]) {
        auto handle = addTorrent(torrent);
        std::shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent);
		handle.prioritize_files(vector<int>(&states[0], &states[sti->num_files()]));
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

            p = lt::read_resume_data(buf);
        }
        p.ti = sti;
        p.save_path = download_path;
        torrent_handle handle = s->add_torrent(p, ec);
        handlers.push_back(handle);
        
        return handle;
    }
    
    char* addMagnet(char* magnetLink) {
        lt::error_code ec;
        add_torrent_params p = parse_magnet_uri(magnetLink);
        p.save_path = download_path;
		torrent_handle handle = s->add_torrent(p, ec);
        handle.unset_flags(lt::torrent_flags::stop_when_ready);
		//handlers.push_back(handle);
		handlers = s->get_torrents();
		
		std::string hash = hash_to_string(handle.status().info_hash);
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
            printf("\r%.2f%% complete (down: %.1d kb/s up: %.1d kB/s peers: %d) %s", stat.progress * 100, stat.download_rate / 1000, stat.upload_rate / 1000, stat.num_peers, state_str[stat.state].c_str());
        }
    }
};

Engine *Engine::standart = NULL;
extern "C" int init_engine(char* client_name, char* download_path, char* config_path) {
    new Engine(client_name, download_path, config_path);
    return 0;
}

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
    try {
        std::shared_ptr<torrent_info> sti = std::make_shared<torrent_info>(torrent_path);
        if (sti != NULL) {
                std::string s = hash_to_string(sti->info_hash());
                char* res = new char[s.length() + 1];
                strcpy(res, s.c_str());
                return res;
        } else {
            return (char*)"-1";
        }
    } catch (boost::system::system_error const & e) {
        printf("%s: %i - %s", e.what(), e.code().value(), e.code().message().c_str());
        return (char*)"-1";
    }
}

extern "C" char* get_magnet_hash(char* magnet_link) {
	add_torrent_params params;
	lt::error_code ec;
	parse_magnet_uri(magnet_link, params, ec);
	std::string s = hash_to_string(params.info_hash);
	char* res = new char[s.length() + 1];
	strcpy(res, s.c_str());
	return res;
}

extern "C" char* get_torrent_magnet_link(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    std::string sres = make_magnet_uri(handle->get_torrent_info()).c_str();
    char* res = new char[sres.size() + 1];
    strcpy(res, sres.c_str());
    return res;
}

extern "C" Files get_files_of_torrent(torrent_info* info) {
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
    }
    return files;
}

extern "C" int get_torrent_files_sequental(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    return handle->is_sequential_download();
}

extern "C" void set_torrent_files_sequental(char* torrent_hash, int sequental) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    handle->set_sequential_download(sequental == 1);
}

extern "C" void set_torrent_files_priority(char* torrent_hash, int states[]) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    torrent_info* info = (torrent_info*)&handle->get_torrent_info();
	handle->prioritize_files(vector<int>(&states[0], &states[info->num_files()]));
//    for (int i = 0; i < info->num_files(); i++) {
//        handle->file_priority(i, states[i]);
//		printf("%d : %d\n", i, states[i]);
//		printf("%d : %d\n------\n", i, handle->file_priority(i));
//    }
//    printf("SETTED! %d\n", states[0]);
//    printf("%d\n", Engine::standart->getHandleByHash(torrent_hash)->file_priority(0));
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
    printf("Started");
}

extern "C" void stop_torrent(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    
    if (TorrentHandle::isChecking(*handle)) {
        handle->set_flags(lt::torrent_flags::stop_when_ready);
    }
    
    setAutoManaged(*handle, false);
	handle->pause();
    printf("Stopped");
}

extern "C" void rehash_torrent(char* torrent_hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
    if (!handle->has_metadata()) return;
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
    torrent_info* info = (torrent_info*)&handle->get_torrent_info();
    Files files = get_files_of_torrent(info);
    file_storage storage = handle->get_torrent_info().files();
    handle->file_progress(progress);
    torrent_status stat = handle->status();
    
    for (int i = 0; i < files.size; i++) {
        files.files[i].file_priority = handle->file_priority(i);
        files.files[i].file_downloaded = progress[i];
		
		std::string s = storage.file_path(i);
		files.files[i].file_path = new char[s.length() + 1];
		strcpy(files.files[i].file_path, s.c_str());
        
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
    int trackers = handle->trackers().size();
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
	Trackers trackers {
		.size = static_cast<int>(trackers_list.size()),
		.tracker_url = new char*[trackers_list.size()],
		.messages = new char*[trackers_list.size()],
		.seeders = new int[trackers_list.size()],
        .leechs = new int[trackers_list.size()],
		.peers = new int[trackers_list.size()],
		.working = new int[trackers_list.size()],
		.verified = new int[trackers_list.size()]
	};
	
	for (int i = 0; i < trackers_list.size(); i++) {
		trackers.tracker_url[i] = new char[trackers_list[i].url.length() + 1];
		strcpy(trackers.tracker_url[i], trackers_list[i].url.c_str());
		
		trackers.messages[i] = new char[strlen("MSG") + 1];
		strcpy(trackers.messages[i], "MSG");
		
		trackers.working[i] = trackers_list[i].is_working() ? 1 : 0;
		trackers.verified[i] = trackers_list[i].verified ? 1 : 0;
		
        trackers.seeders[i] = -1;
        trackers.peers[i] = -1;
        trackers.leechs[i] = -1;
        
        for (const lt::announce_endpoint &endpoint : trackers_list[i].endpoints) {
            trackers.seeders[i] = std::max(trackers.seeders[i], endpoint.scrape_complete);
            trackers.peers[i] = std::max(trackers.peers[i], endpoint.scrape_incomplete);
            trackers.leechs[i] = std::max(trackers.leechs[i], endpoint.scrape_downloaded);
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
	torrent_info torinfo = handle->get_torrent_info();
	
	std::ofstream out((Engine::standart->config_path + "/" + handle->name().c_str() + ".torrent").c_str(), std::ios_base::binary);
	out.unsetf(std::ios_base::skipws);
	create_torrent ct = create_torrent(torinfo);
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
            printf("Not valid\n");
            continue;
        }
        if (!torrent->has_metadata()) {
            printf("No metadata\n");
            continue;
        }
        if (!final && !torrent->need_save_resume_data()) {
            printf("Not need to save ");
            continue;
        }

        if (!final && !torrent->need_save_resume_data()) continue;
        if (torrent->status().state == torrent_status::checking_files
            || torrent->status().state == torrent_status::checking_resume_data //
            //|| torrent->is_paused()
            || (torrent->status().flags & torrent_flags::paused && torrent->status().errc)) { // HasError
            printf("Error in %s is occured", torrent->status().name.c_str());
            continue;
        }

        torrent->save_resume_data();
        m_numResumeData++;
        
        printf("%s - ADDED!!\n", torrent->name().c_str());
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
    printf("Start with count %d!!\n", m_numResumeData);
    while (m_numResumeData > 0) {
        std::vector<lt::alert *> alerts;
        getPendingAlerts(alerts, 30 * 1000);
        if (alerts.empty()) {
            printf(" aborting with %d outstanding torrents to save resume data for\n", m_numResumeData);
            break;
        }
        
        for (const auto a : alerts) {
            printf("Type: %d!!\n", a->type());
            
            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) {
                printf("Error!!\n");
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
                printf("%s - SAVED!!\n", h.name().c_str());
                --m_numResumeData;
            }
        }
    }
    printf("Done!!\n");
}

extern "C" void set_download_limit(int limit_in_bytes) {
	session* ses = Engine::standart->s;
	settings_pack ss = ses->get_settings();
	ss.set_int(settings_pack::int_types::download_rate_limit, limit_in_bytes);
	ses->apply_settings(ss);
}

extern "C" void set_upload_limit(int limit_in_bytes) {
	session* ses = Engine::standart->s;
	settings_pack ss = ses->get_settings();
	ss.set_int(settings_pack::int_types::upload_rate_limit, limit_in_bytes);
	ses->apply_settings(ss);
}

extern "C" Result getTorrentInfo() {
    std::string state_str[] = {"Queued", "Hashing", "Metadata", "Downloading", "Finished", "Seeding", "Allocating", "Checking fastresume"};
    
    int size = (int)Engine::standart->handlers.size();
    Result res{
        .count = size,
        .torrents = new TorrentInfo[size]
    };
    
    for (int i = 0; i < Engine::standart->handlers.size(); i++) {
        torrent_handle handler = Engine::standart->handlers[i];
        torrent_status stat = handler.status();
        torrent_info* info = NULL;
        
        if (handler.has_metadata()) {
            info = (torrent_info*)&handler.get_torrent_info();
            
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
        }
        
        if (info != NULL) {
            res.torrents[i].comment = new char[info->comment().length() + 1];
            strcpy(res.torrents[i].comment, info->comment().c_str());
        } else {
            res.torrents[i].comment = new char[1];
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
        
        res.torrents[i].num_seeds = stat.num_seeds;
        
        res.torrents[i].num_peers = stat.num_peers;
        
        res.torrents[i].sequential_download = bool {stat.flags & lt::torrent_flags::sequential_download} ? 1 : 0;
		
		try {
        	res.torrents[i].creation_date = info != NULL ? info->creation_date() : 0;
		} catch (...) {
			res.torrents[i].creation_date = 0;
		}
        
        res.torrents[i].is_paused = handler.is_paused();
        res.torrents[i].is_finished = stat.total_wanted == stat.total_wanted_done;
        res.torrents[i].is_seed = handler.is_seed();
		res.torrents[i].has_metadata = handler.has_metadata();
    }
    
    return res;
}
