#include "LFSR.h"

/*
void LFSR::Shift()
{
    // 8-bit
    //unsigned mask = 0xb8 ; // 10111000
    // reversed 00011101 0x1d
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x1d ;
        reg[0] >>= 1 ;
        reg[0] |= 0x80 ;
    }
    else
        reg[0] >>= 1 ;
}
*/



void Shift64( unsigned * reg ) noexcept
{
    // 64-bit
    //mask  1101 1000 00000000...
    // reversed 0001 1011 0x1b
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x1b ; // - that is right
        //shift :
        bool low_bit = reg[1] &  1 ;// 0x80000000 ;
        reg[1] >>= 1 ;

        reg[0] >>= 1 ;
        if ( low_bit )
            reg[0] |= 0x80000000 ;
        //end of shift
        reg[1] |= 0x80000000 ;
    }
    else
    {
        //shift :
        bool low_bit = reg[1] &  1 ;
        reg[1] >>= 1 ;

        reg[0] >>= 1 ;
        if ( low_bit )
            reg[0] |= 0x80000000 ;
        //end of shift
    }
}

void Shift128( unsigned * reg ) noexcept
{
    // 128-bit
    //mask  1110 0001 00000000...
    // reversed 1000 0111   0x87
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x87 ;
        
        //shift :
        bool low_bit3 = reg[3] &  1 ;
        reg[3] >>= 1 ;

        bool low_bit2 = reg[2] &  1 ;
        reg[2] >>= 1 ;
        if ( low_bit3 )
            reg[2] |= 0x80000000 ;

        bool low_bit1 = reg[1] &  1 ;
        reg[1] >>= 1 ;
        if ( low_bit2 )
            reg[1] |= 0x80000000 ;

        reg[0] >>= 1 ;
        if ( low_bit1 )
            reg[0] |= 0x80000000 ;
        //end of shift
        
        reg[3] |= 0x80000000 ;
    }
    else
    {
        //shift :
        bool low_bit3 = reg[3] &  1 ;
        reg[3] >>= 1 ;

        bool low_bit2 = reg[2] &  1 ;
        reg[2] >>= 1 ;
        if ( low_bit3 )
            reg[2] |= 0x80000000 ;

        bool low_bit1 = reg[1] &  1 ;
        reg[1] >>= 1 ;
        if ( low_bit2 )
            reg[1] |= 0x80000000 ;

        reg[0] >>= 1 ;
        if ( low_bit1 )
            reg[0] |= 0x80000000 ;
        //end of shift
    }
}

void Shift256( unsigned * reg ) noexcept
{
    // 256-bit
    //      256  252  248
    //mask  1010 0100 0010 0000...
    // reversed 0100 0010 0101   0x425
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x425 ;
        
        //shift :
        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;

        bool low_bit6 = reg[6] &  1 ;
        reg[6] >>= 1 ;
        if ( low_bit7 )
            reg[6] |= 0x80000000 ;

        bool low_bit5 = reg[5] &  1 ;
        reg[5] >>= 1 ;
        if ( low_bit6 )
            reg[5] |= 0x80000000 ;

        bool low_bit4 = reg[4] &  1 ;
        reg[4] >>= 1 ;
        if ( low_bit5 )
            reg[4] |= 0x80000000 ;

        bool low_bit3 = reg[3] &  1 ;
        reg[3] >>= 1 ;
        if ( low_bit4 )
            reg[3] |= 0x80000000 ;

        bool low_bit2 = reg[2] &  1 ;
        reg[2] >>= 1 ;
        if ( low_bit3 )
            reg[2] |= 0x80000000 ;

        bool low_bit1 = reg[1] &  1 ;
        reg[1] >>= 1 ;
        if ( low_bit2 )
            reg[1] |= 0x80000000 ;

        reg[0] >>= 1 ;
        if ( low_bit1 )
            reg[0] |= 0x80000000 ;
        //end of shift
        
        reg[7] |= 0x80000000 ;
    }
    else
    {
        //shift :
        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;

        bool low_bit6 = reg[6] &  1 ;
        reg[6] >>= 1 ;
        if ( low_bit7 )
            reg[6] |= 0x80000000 ;

        bool low_bit5 = reg[5] &  1 ;
        reg[5] >>= 1 ;
        if ( low_bit6 )
            reg[5] |= 0x80000000 ;

        bool low_bit4 = reg[4] &  1 ;
        reg[4] >>= 1 ;
        if ( low_bit5 )
            reg[4] |= 0x80000000 ;

        bool low_bit3 = reg[3] &  1 ;
        reg[3] >>= 1 ;
        if ( low_bit4 )
            reg[3] |= 0x80000000 ;

        bool low_bit2 = reg[2] &  1 ;
        reg[2] >>= 1 ;
        if ( low_bit3 )
            reg[2] |= 0x80000000 ;

        bool low_bit1 = reg[1] &  1 ;
        reg[1] >>= 1 ;
        if ( low_bit2 )
            reg[1] |= 0x80000000 ;

        reg[0] >>= 1 ;
        if ( low_bit1 )
            reg[0] |= 0x80000000 ;
        //end of shift
    }
}
