#ifndef INTELLIGENTGATEGUARD_LOGUTIL_H
#define INTELLIGENTGATEGUARD_LOGUTIL_H

#if !defined(_WIN32) && !defined(LINUX)
#include "android/log.h"
#endif //!defined(_WIN32) && !defined(LINUX)

#ifndef LOG_TAG
//调试LOG TAG
#define LOG_TAG "FaceAnalysisApi"
#endif

#if !defined(_WIN32) && !defined(LINUX)
#define IS_DEBUG true
#endif

#define LOG_NOOP (void) 0
//__FILE__ 输出文件名
//__LINE__ 输出行数
//__PRETTY_FUNCTION__  输出方法名
//可以按需选取 %s %u %s 分别与之对应
#define LOG_PRINT(level,fmt,...) __android_log_print(level,LOG_TAG,fmt,##__VA_ARGS__)
//通过IS_DEBUG来控制是否输出日志
#if IS_DEBUG
#define LOGI(fmt,...) LOG_PRINT(ANDROID_LOG_INFO,fmt,##__VA_ARGS__)
#else
#define LOGI(fmt,...) printf(fmt,##__VA_ARGS__)
#endif

#if IS_DEBUG
#define LOGW(fmt,...) LOG_PRINT(ANDROID_LOG_WARN,fmt ,##__VA_ARGS__)
#else
#define LOGW(...) LOG_NOOP
#endif

#if IS_DEBUG
#define LOGD(fmt,...) LOG_PRINT(ANDROID_LOG_DEBUG,fmt ,##__VA_ARGS__)
#else
#define LOGD(...) LOG_NOOP
#endif

#if IS_DEBUG
#define LOGE(fmt,...) LOG_PRINT(ANDROID_LOG_ERROR,fmt ,##__VA_ARGS__)
#else
#define LOGE(...) LOG_NOOP
#endif

#if IS_DEBUG
#define LOGF(fmt,...) LOG_PRINT(ANDROID_LOG_FATAL,fmt ,##__VA_ARGS__)
#else
#define LOGF(...) LOG_NOOP
#endif

#endif //INTELLIGENTGATEGUARD_LOGUTIL_H
