#include "records.h"
#include "md_ref.h"


///// TextMsg //////
spnr::TextMsg::TextMsg()
{

};

spnr::TextMsg::TextMsg(const std::string& s) :m_text(s)
{

};

uint16_t spnr::TextMsg::calc_len()const 
{
    return spnr::str_archive_len(m_text);
};


///// HBMsg //////
spnr::HBMsg::HBMsg() 
{

};

spnr::HBMsg::HBMsg(uint32_t num) :counter_(num)
{

};

uint16_t spnr::HBMsg::calc_len()const
{
    return sizeof(counter_);
};


///// SubscribeMsg //////
void spnr::SubscribeMsg::add_symbols(const STR_VEC& sym) 
{
    symbols_.insert(symbols_.end(), sym.begin(), sym.end());
};

void spnr::SubscribeMsg::add_symbol(const std::string& s)
{
    symbols_.push_back(s);    
};

uint16_t spnr::SubscribeMsg::calc_len()const 
{
    return spnr::str_arr_archive_len(symbols_);
};


/////// LoginMsg //////
void spnr::LoginMsg::add_parameters(const STR_VEC& params) 
{
    params_.insert(params_.end(), params.begin(), params.end());
};

uint16_t spnr::LoginMsg::calc_len()const 
{
    uint16_t rv = sizeof(uint16_t);//version
    rv += spnr::str_archive_len(user_);
    rv += spnr::str_archive_len(token_);
    rv += spnr::str_arr_archive_len(params_);
    return rv;
};


/////// LoginReplyMsg //////
void spnr::LoginReplyMsg::add_parameters(const STR_VEC& params)
{
    params_.insert(params_.end(), params.begin(), params.end());
};

uint16_t spnr::LoginReplyMsg::calc_len()const
{
    uint16_t rv = sizeof(uint16_t) + sizeof(uint16_t);
    rv += spnr::str_archive_len(reply_desc_);
    rv += spnr::str_arr_archive_len(params_);
    return rv;
};


/////// MenuMsg ///////
spnr::MenuMsg::MenuMsg() 
{

};

bool spnr::MenuMsg::add_menu_option(ECommand c, const std::string& s)
{
    StrData is(static_cast<uint16_t>(c), s);
    menu_.push_back(is);
    return true;
};

uint16_t spnr::MenuMsg::calc_len()const
{
    uint16_t rv = sizeof(uint16_t);
    for (const auto& s : menu_)
    {
        rv += spnr::str_archive_len(s.val_);
        rv += sizeof(s.idx_);
    }
    return rv;
};


spnr::CommandMsg::CommandMsg(ECommand c, const std::string& client_ctx):cmd_(c), client_ctx_(client_ctx)
{

};

bool spnr::CommandMsg::add_parameter(const std::string& s)
{
    parameters_.push_back(s);
    return true;
};

bool spnr::CommandMsg::add_parameters(const std::vector<std::string>& v)
{
    parameters_.insert(parameters_.end(), v.begin(), v.end());
    return true;
};

uint16_t spnr::CommandMsg::calc_len()const 
{
    uint16_t rv = sizeof(uint8_t);//cmd
    rv += spnr::str_archive_len(client_ctx_);
    rv += spnr::str_arr_archive_len(parameters_);
    return rv;
};

