#pragma once

#include "mddelta.hpp"

namespace tu_dds
{
    void dds_make_one_delta(const std::string& symbol, MdDelta& m, uint32_t session_index);
};