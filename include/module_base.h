#pragma once

#include <atomic>
#include <thread>

namespace syncflow {
    /**
     * @brief 模块基类
     * 封装线程生命周期
     */ 
class ModuleBase {
public: 
    virtual ~ModuleBase() = default;

    void start() {
        if (runing_) return;
        runing_ = true;
        thread_ = std::thread(&ModuleBase::run, this);
    }

    void stop() {
        if (!runing_) return;
        runing_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    bool is_running() const {
        return runing_;
    }

protected:
    virtual void run() = 0;

    std::atomic<bool> runing_{false};
    std::thread thread_;
};
}