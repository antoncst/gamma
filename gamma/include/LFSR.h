#pragma once

//#include <memory>

    //void Shift() ;
    void Shift64( unsigned * p_block ) noexcept ; // shift 8-byte block
    void Shift128( unsigned * p_block ) noexcept ; // shift 16-byte block
    void Shift256( unsigned * p_block ) noexcept ; // shift 32-byte block
    void Shift512( unsigned * p_block ) noexcept ; // shift 32-byte block
    void Shift1024( unsigned * p_block ) noexcept ; // shift 32-byte block

