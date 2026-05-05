#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <string>

namespace syncflow {

    inline std::ofstream g_log_file;
    inline std::mutex g_log_mutex;

    inline void init_log(const std::string& filename) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        if (g_log_file.is_open()) {
            g_log_file.close();
        }
        g_log_file.open(filename, std::ios::out | std::ios::app);
        if (!g_log_file.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }

    inline void close_log() {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        if (g_log_file.is_open()) {
            g_log_file.close();
        }
    }

    #define SYNC_LOG(msg) \
       do{ \
        std::ostringstream oss; \
        oss << "[tid" << std::this_thread::get_id() << "] " << msg; \
        std::lock_guard<std::mutex> lock(syncflow::g_log_mutex); \
        if (syncflow::g_log_file.is_open()) { \
                syncflow::g_log_file << oss.str() << std::endl; \
            } \
       }while (0)
}