#pragma once
namespace syncflow {

// 全局统一状态码 —— 整个框架唯一的错误类型
enum class StatusCode : int {
    OK = 0,          // 成功
    TIMEDOUT = 1,    // 超时
    BUSY = 2,        // 忙
    INVALID = 3,     // 参数/状态非法
    NOT_READY = 4,   // 未就绪
    NOT_FOUND = 5,
    NO_RESOURCE = 6,
    UNKNOWN_ERROR = 7,
};

// 可以全局inline判断是否成功
inline bool is_ok(StatusCode s) {
    return s == StatusCode::OK;
}

}