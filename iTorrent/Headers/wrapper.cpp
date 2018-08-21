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
        add_torrent_params p;
        p.save_path = download_path;
        p.ti = boost::shared_ptr<torrent_info>(torrent);
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
        p.ti = boost::shared_ptr<torrent_info>(torrent);
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
        add_torrent_params p;
        p.save_path = download_path;
        p.url = std::string(magnetLink);
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

extern "C" Files get_files_of_torrent(torrent_info* info) {
    file_storage fs = info->files();
    Files files {
        .size = fs.num_files(),
        .file_name = new char*[fs.num_files()],
        .file_size = new long long[fs.num_files()],
        .file_priority = new int[fs.num_files()]
    };
    files.title = new char[info->name().length() + 1];
    strcpy(files.title, info->name().c_str());
    for (int i = 0; i < fs.num_files(); i++) {
        files.file_name[i] = new char[fs.file_path(i).length() + 1];
        strcpy(files.file_name[i], fs.file_path(i).c_str());
        
        files.file_size[i] = fs.file_size(i);
    }
    return files;
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
    torrent_info* info = (torrent_info*)&handle->get_torrent_info();
    Files files = get_files_of_torrent(info);
	files.file_downloaded = new long long[files.size];
	files.file_path = new char*[files.size];
	vector<int64_t> progress;
	file_storage storage = handle->get_torrent_info().files();
	handle->file_progress(progress);
    for (int i = 0; i < files.size; i++) {
        files.file_priority[i] = handle->file_priority(i);
		files.file_downloaded[i] = progress[i];
		
		//cout << storage.file_path(i) << endl;
		std::string s = storage.file_path(i);
		files.file_path[i] = new char[s.length() + 1];
		strcpy(files.file_path[i], s.c_str());
    }
    return files;
}

extern "C" Trackers get_trackers_by_hash(char* torrent_hash) {
	torrent_handle* handle = Engine::standart->getHandleByHash(torrent_hash);
	if (handle == NULL) {
		Trackers t = {
			.size = -1
		};
		return t;
	}
	torrent_info* info = (torrent_info*)&handle->get_torrent_info();
	std::vector<announce_entry> trackers_list = info->trackers();
	Trackers trackers {
		.size = static_cast<int>(trackers_list.size()),
		.tracker_url = new char*[trackers_list.size()],
		.messages = new char*[trackers_list.size()],
		.seeders = new int[trackers_list.size()],
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
		
		trackers.seeders[i] = 0; //trackers_list[i].see;
		trackers.peers[i] = 0; //trackers_list[i].next_announce_in();
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
        .name = new char*[size],
        .state = new char*[size],
        .hash = new char*[size],
        .creator = new char*[size],
        .comment = new char*[size],
        .total_wanted = new long long[size],
        .total_wanted_done = new long long[size],
        .progress = new float[size],
        .download_rate = new int[size],
        .upload_rate = new int[size],
        .total_download = new long long[size],
        .total_upload = new long long[size],
        .num_seeds = new int[size],
        .num_peers = new int[size],
        .total_size = new long long[size],
        .total_done = new long long[size],
        .creation_date = new time_t[size],
        .is_paused = new int[size],
        .is_finished = new int[size],
        .is_seed = new int[size],
		.has_metadata = new int[size]
    };
    
    for(int i = 0; i < Engine::standart->handlers.size(); i++) {
        torrent_status stat = Engine::standart->handlers[i].status();
        torrent_info* info = NULL;
        if (stat.has_metadata) {
            info = (torrent_info*)&Engine::standart->handlers[i].get_torrent_info();
        }
        
        res.name[i] = new char[stat.name.length() + 1];
        strcpy((char*)res.name[i], stat.name.c_str());
        
        res.state[i] = new char[state_str[stat.state].length() + 1];
        strcpy((char*)res.state[i], state_str[stat.state].c_str());
        
        std::string hash = hash_to_string(stat.info_hash);
        res.hash[i] = new char[hash.length() + 1];
        strcpy(res.hash[i], hash.c_str());
        
        if (info != NULL) {
            res.creator[i] = new char[info->creator().length() + 1];
            strcpy((char*)res.creator[i], info->creator().c_str());
        } else {
            res.creator[i] = new char[1];
        }
        
        if (info != NULL) {
            res.comment[i] = new char[info->comment().length() + 1];
            strcpy(res.comment[i], info->comment().c_str());
        } else {
            res.comment[i] = new char[1];
        }
        
        res.progress[i] = stat.progress;
        
        res.total_wanted[i] = stat.total_wanted;
        
        res.total_wanted_done[i] = stat.total_wanted_done;
        
        res.total_size[i] = info != NULL ? info->total_size() : 0;
        
        res.total_done[i] = stat.total_done;
        
        res.download_rate[i] = stat.download_rate;
        
        res.upload_rate[i] = stat.upload_rate;
        
        res.total_download[i] = stat.total_download;
        
        res.total_upload[i] = stat.total_upload;
        
        res.num_seeds[i] = stat.num_seeds;
        
        res.num_peers[i] = stat.num_peers;
		
		try {
        	res.creation_date[i] = info != NULL ? info->creation_date().value() : 0;
		} catch (...) {
			res.creation_date[i] = 0;
		}
        
        torrent_handle handle = Engine::standart->handlers[i];
        res.is_paused[i] = handle.is_paused();
        res.is_finished[i] = handle.is_finished();
        res.is_seed[i] = handle.is_seed();
		res.has_metadata[i] = handle.has_metadata();
    }
    
    return res;
}
