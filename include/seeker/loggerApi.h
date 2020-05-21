/**
@project seeker
@author Tao Zhang
@since 2020/3/1
@version 0.0.1-SNAPSHOT 2020/5/13
*/
#pragma once
#include "spdlog/spdlog.h"


// short for logger, you may copy following define to your CPP/H file.
#define T_LOG(...) SPDLOG_TRACE(__VA_ARGS__)
#define D_LOG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define I_LOG(...) SPDLOG_INFO(__VA_ARGS__)
#define W_LOG(...) SPDLOG_WARN(__VA_ARGS__)
#define E_LOG(...) SPDLOG_ERROR(__VA_ARGS__)


