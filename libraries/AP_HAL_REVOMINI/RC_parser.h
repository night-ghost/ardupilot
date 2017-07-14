#pragma once 

class REVOMINI::_parser { // universal parser interface
public:
    _parser() {};
    virtual ~_parser() {};
    virtual void init(uint8_t ch) = 0;
    virtual void late_init(uint8_t b) {}
    virtual uint64_t get_last_signal() const =0;
    virtual uint64_t get_last_change() const =0;
    virtual uint8_t  get_valid_channels() const =0;
    virtual uint16_t get_val(uint8_t ch) const =0;
    virtual bool bind(int dsmMode) const  { return true; }
};
