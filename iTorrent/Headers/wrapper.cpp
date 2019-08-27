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
#include <boost/make_shared.hpp>
#include "result_struct.h"
#include "file_struct.h"

#include "helper_code.h"

extern "C" int load_file(std::string const& filename, std::vector<char>& v
              , libtorrent::error_code& ec, int limit = 8000000)
{
    ec.clear();
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL)
    {
        ec.assign(errno, boost::system::system_category());
        fprintf(stderr, "err 1\n");
        return -1;
    }
    
    int r = fseek(f, 0, SEEK_END);
    if (r != 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        fprintf(stderr, "err 2\n");
        return -1;
    }
    long s = ftell(f);
    if (s < 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        fprintf(stderr, "err 3\n");
        return -1;
    }
    
    if (s > limit)
    {
        fclose(f);
        fprintf(stderr, "err 4\n");
        return -2;
    }
    
    r = fseek(f, 0, SEEK_SET);
    if (r != 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        fprintf(stderr, "err 5\n");
        return -1;
    }
    
    v.resize(s);
    if (s == 0)
    {
        fclose(f);
        return 0;
    }
    
    r = fread(&v[0], 1, v.size(), f);
    if (r < 0)
    {
        ec.assign(errno, boost::system::system_category());
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    if (r != s) return -3;
    
    return 0;
}

libtorrent::torrent_info* get_torrent(std::string file_path) {
    using namespace libtorrent;
    namespace lt = libtorrent;
    
    //printf("%s\n", file_path.c_str());
    
    int item_limit = 1000000;
    int depth_limit = 1000;
    
    std::vector<char> buf;
	lt::error_code ec;
    int ret = load_file(file_path, buf, ec, 40 * 1000000);
    if (ret == -1)
    {
        fprintf(stderr, "file too big, aborting\n");
        return NULL;
    }
    
    if (ret != 0)
    {
        fprintf(stderr, "failed to load file: %s\n", ec.message().c_str());
        return NULL;
    }
    bdecode_node e;
    int pos = -1;
    //printf("decoding. recursion limit: %d total item count limit: %d\n", depth_limit, item_limit);
    ret = bdecode(&buf[0], &buf[0] + buf.size(), e, ec, &pos
                  , depth_limit, item_limit);
    
    //printf("\n\n----- raw info -----\n\n%s\n", print_entry(e).c_str());
    
    if (ret != 0)
    {
        fprintf(stderr, "failed to decode: '%s' at character: %d\n", ec.message().c_str(), pos);
        return NULL;
    }
    
    return new torrent_info(e, ec);
}

using namespace libtorrent;
using namespace std;
class Engine {
public:
    static Engine *standart;
    session *s;
    //std::string root_path;
	std::string config_path;
	std::string download_path;
    vector<torrent_handle> handlers;
    
    Engine(std::string download_path, std::string config_path) {
        Engine::standart = this;
        this->download_path = download_path;
		this->config_path = config_path;
        
        settings_pack sett;
        sett.set_str(settings_pack::listen_interfaces, "0.0.0.0:6881");
        sett.set_int(lt::settings_pack::alert_mask
                     , lt::alert::error_notification
                     | lt::alert::storage_notification
                     | lt::alert::status_notification);
        s = new lt::session(sett);
        
        std::ifstream in((Engine::standart->config_path + "/state.fastresume").c_str(), std::ios_base::binary);
        std::vector<char> buffer;
        
        if (!in.eof() && !in.fail())
        {
            in.seekg(0, std::ios_base::end);
            std::streampos fileSize = in.tellg();
            buffer.resize(fileSize);
            
            in.seekg(0, std::ios_base::beg);
            in.read(&buffer[0], fileSize);
        }
        
        lt::entry save = bdecode(buffer.begin(), buffer.end());
        s->save_state(save);
    }
    
    char* addTorrent(torrent_info *torrent) {
        lt::error_code ec;
        
        add_torrent_params p;// = read_resume_data(<#const bdecode_node &rd#>, ec);
        p.save_path = download_path;
        p.ti = std::make_shared<torrent_info>(torrent);
		torrent->info_hash();
        std::string path = Engine::standart->config_path + "/.FastResumes/" + hash_to_string(torrent->info_hash()) + ".fastresume";
        std::ifstream ifs(path, std::ios_base::binary);
        ifs.unsetf(std::ios_base::skipws);
        p.resume_data.assign(std::istream_iterator<char>(ifs), std::istream_iterator<char>());
        torrent_handle h = s->add_torrent(p, ec);
        handlers.push_back(h);
		
		char* res = new char[hash_to_string(h.status().info_hash).length() + 1];
		strcpy(res, hash_to_string(h.status().info_hash).c_str());
		return res;
    }
    
    void addTorrentWithStates(torrent_info *torrent, int states[]) {
        lt::error_code ec;
        add_torrent_params p;
        p.save_path = download_path;
        p.ti = std::make_shared<torrent_info>(torrent);
        torrent_handle handle = s->add_torrent(p, ec);
		handle.stop_when_ready(false);
        handlers.push_back(handle);
		handle.prioritize_files(vector<int>(&states[0], &states[torrent->num_files()]));
//        for (int i = 0; i < torrent->num_files(); i++) {
//            handle.file_priority(i, states[i] ? 4 : 0);
//        }
    }
    
    char* addMagnet(char* magnetLink) {
        lt::error_code ec;
        add_torrent_params p = parse_magnet_uri(magnetLink);
        p.save_path = download_path;
		torrent_handle handle = s->add_torrent(p, ec);
		handle.stop_when_ready(false);
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
extern "C" int init_engine(char* download_path, char* config_path) {
    new Engine(download_path, config_path);
    return 0;
}

extern "C" char* add_torrent(char* torrent_path) {
	libtorrent::torrent_info* torrent = get_torrent(torrent_path);
	if (torrent == NULL) {
		return (char*)"-1";
	}
	return Engine::standart->addTorrent(torrent);
}

extern "C" void add_torrent_with_states(char* torrent_path, int states[]) {
	libtorrent::torrent_info* torrent = get_torrent(torrent_path);
	if (torrent == NULL) {
		return;
	}
    Engine::standart->addTorrentWithStates(torrent, states);
}

extern "C" char* add_magnet(char* magnet_link) {
    return Engine::standart->addMagnet(magnet_link);
}

extern "C" void remove_torrent(char* torrent_hash, int remove_files) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	
//	if (remove_files == 1) {
//		for (int i = 0; i < handle->torrent_file()->num_files(); i++) {
//			std::string path = handle->torrent_file()->files().file_path(i);
//			remove(path.c_str());
//		}
//	}
	if (remove_files == 1) {
		Engine::standart->s->remove_torrent(*handle, session::delete_files);
	} else {
		Engine::standart->s->remove_torrent(*handle);
	}
	Engine::standart->handlers = Engine::standart->s->get_torrents();
}

extern "C" char* get_torrent_file_hash(char* torrent_path) {
    lt::torrent_info* info = get_torrent(torrent_path);
    if (info != NULL) {
        std::string s = hash_to_string(info->info_hash());
        char* res = new char[s.length() + 1];
        strcpy(res, s.c_str());
        return res;
    } else {
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
	printf("Started");
	handle->stop_when_ready(false);
	handle->resume();
}

extern "C" void stop_torrent(char* torrent_hash) {
    torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	printf("Stopped");
	handle->stop_when_ready(true);
	handle->pause();
}

extern "C" void rehash_torrent(char* torrent_hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	handle->force_recheck();
}

extern "C" Files get_files_of_torrent_by_path(char* torrent_path) {
    torrent_info* info = get_torrent(torrent_path);
	if (info == NULL) {
		Files files {
			.size = -1
		};
		return files;
	}
    return get_files_of_torrent(info);
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
        
        const auto fileSize = storage.file_size(i);
        const auto fileOffset = storage.file_offset(i);
        
        const int pieceLength = info->piece_length();
        const long long beginIdx = (fileOffset / pieceLength);
        const long long endIdx = fileSize > 0 ? ((fileOffset + fileSize - 1) / pieceLength) : 0;
        
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
	ct.set_creator("iTorrent");
	bencode(std::ostream_iterator<char>(out), ct.generate());
}

extern "C" void resume_to_app() {
	session* ses = Engine::standart->s;
	ses->resume();
}

extern "C" void save_fast_resume() {
    session* ses = Engine::standart->s;
	ses->pause();

    int outstanding_resume_data = 0; // global counter of outstanding resume data

	std::vector<torrent_handle> handles = ses->get_torrents();
    for (std::vector<torrent_handle>::iterator i = handles.begin();
         i != handles.end(); ++i)
    {
        torrent_handle& h = *i;
		if (!h.is_valid()) {
			printf("Not valid\n");
			continue;
		}
        torrent_status s = h.status();
		if (!s.has_metadata) {
			printf("No metadata\n");
			continue;
		}
		if (!s.need_save_resume) {
			printf("Not need to save ");
			continue;
		}

        h.save_resume_data();
        ++outstanding_resume_data;
        
        printf("FILE ADDED!!\n");
    }
	
	sleep(500);

    while (outstanding_resume_data > 0)
    {
        alert const* a = ses->wait_for_alert(seconds(10));

        // if we don't get an alert within 10 seconds, abort
        if (a == 0) break;

		std::vector<alert*> alerts;
		ses->pop_alerts(&alerts);
		
		for (alert* i : alerts) {
			if (alert_cast<save_resume_data_failed_alert>(i))
			{
				//process_alert(a);
				--outstanding_resume_data;
				continue;
			}
			
			save_resume_data_alert const* rd = alert_cast<save_resume_data_alert>(i);
			if (rd == 0)
			{
				//process_alert(a);
				continue;
			}
			
			torrent_handle h = rd->handle;
			torrent_info info = h.get_torrent_info();
			std::ofstream out((Engine::standart->config_path + "/.FastResumes/" + hash_to_string(info.info_hash()) + ".fastresume").c_str(), std::ios_base::binary);
			out.unsetf(std::ios_base::skipws);
			bencode(std::ostream_iterator<char>(out), *rd->resume_data);
			printf("FILE SAVED!!\n");
			
			--outstanding_resume_data;
			sleep(10);
		}
    }
    printf("SAVED!!\n");
}

extern "C" void set_download_limit(int limit_in_bytes) {
	session* ses = Engine::standart->s;
	settings_pack ss = ses->get_settings();
	ss.set_int(settings_pack::int_types::download_rate_limit, limit_in_bytes);
	ses->apply_settings(ss);
	//ses->set_download_rate_limit(limit_in_bytes);
}

extern "C" void set_upload_limit(int limit_in_bytes) {
	session* ses = Engine::standart->s;
	settings_pack ss = ses->get_settings();
	ss.set_int(settings_pack::int_types::upload_rate_limit, limit_in_bytes);
	ses->apply_settings(ss);
	//ses->set_upload_rate_limit(limit_in_bytes);
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
        res.torrents[i].is_finished = handler.is_finished();
        res.torrents[i].is_seed = handler.is_seed();
		res.torrents[i].has_metadata = handler.has_metadata();
    }
    
    return res;
}
