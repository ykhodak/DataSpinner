#include "md_dds_util.h"
#include "spinner_core.h"

void tu_dds::dds_make_one_delta(const std::string& symbol, MdDelta& m, uint32_t session_index)
{
    m.symbol(symbol);

	IntData int_data;
	std::array<uint16_t, INT_DELTA_SIZE> int_idx;
	std::array<int32_t, INT_DELTA_SIZE> int_value;
    for (int i = 0; i < INT_DELTA_SIZE; ++i) {
		int_idx[i] = i + 1;
		int_value[i] = (i + 1) * 10;
    }
	int_data.index(std::move(int_idx));
	int_data.value(std::move(int_value));
    m.int_data(std::move(int_data));

	DblData dbl_data;
	std::array<uint16_t, DBL_DELTA_SIZE> dbl_idx;
	std::array<double, DBL_DELTA_SIZE> dbl_value;
    for (int i = 0; i < DBL_DELTA_SIZE; ++i) {
		dbl_idx[i] = i + 1;
		dbl_value[i] = (i + 1) * 10;
    }
	dbl_data.index(std::move(dbl_idx));
	dbl_data.value(std::move(dbl_value));
    m.dbl_data(std::move(dbl_data));
	
    m.session_idx(session_index);    
    m.wire_tstamp(spnr::time_stamp());    
};
