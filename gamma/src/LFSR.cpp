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
    // 128-bit . 16 byte
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

void Shift512( unsigned * reg ) noexcept
{
    // 512-bit
    //      512  508  504
    //mask  1010 0100 1000 0000...
    // reversed 0001 0010 0101   0x125
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x125 ;
        
        //shift :
        bool low_bit15 = reg[15] &  1 ;
        reg[15] >>= 1 ;

        bool low_bit14 = reg[14] &  1 ;
        reg[14] >>= 1 ;
        if ( low_bit15 )
            reg[14] |= 0x80000000 ;

        bool low_bit13 = reg[13] &  1 ;
        reg[13] >>= 1 ;
        if ( low_bit14 )
            reg[13] |= 0x80000000 ;

        bool low_bit12 = reg[12] &  1 ;
        reg[12] >>= 1 ;
        if ( low_bit13 )
            reg[12] |= 0x80000000 ;

        bool low_bit11 = reg[11] &  1 ;
        reg[11] >>= 1 ;
        if ( low_bit12 )
            reg[11] |= 0x80000000 ;

        bool low_bit10 = reg[10] &  1 ;
        reg[10] >>= 1 ;
        if ( low_bit11 )
            reg[10] |= 0x80000000 ;

        bool low_bit9 = reg[9] &  1 ;
        reg[9] >>= 1 ;
        if ( low_bit10 )
            reg[9] |= 0x80000000 ;

        bool low_bit8 = reg[8] &  1 ;
        reg[8] >>= 1 ;
        if ( low_bit9 )
            reg[8] |= 0x80000000 ;

        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;
        if ( low_bit8 )
            reg[7] |= 0x80000000 ;

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
        
        reg[15] |= 0x80000000 ;
    }
    else
    {
        //shift :
        bool low_bit15 = reg[15] &  1 ;
        reg[15] >>= 1 ;

        bool low_bit14 = reg[14] &  1 ;
        reg[14] >>= 1 ;
        if ( low_bit15 )
            reg[14] |= 0x80000000 ;

        bool low_bit13 = reg[13] &  1 ;
        reg[13] >>= 1 ;
        if ( low_bit14 )
            reg[13] |= 0x80000000 ;

        bool low_bit12 = reg[12] &  1 ;
        reg[12] >>= 1 ;
        if ( low_bit13 )
            reg[12] |= 0x80000000 ;

        bool low_bit11 = reg[11] &  1 ;
        reg[11] >>= 1 ;
        if ( low_bit12 )
            reg[11] |= 0x80000000 ;

        bool low_bit10 = reg[10] &  1 ;
        reg[10] >>= 1 ;
        if ( low_bit11 )
            reg[10] |= 0x80000000 ;

        bool low_bit9 = reg[9] &  1 ;
        reg[9] >>= 1 ;
        if ( low_bit10 )
            reg[9] |= 0x80000000 ;

        bool low_bit8 = reg[8] &  1 ;
        reg[8] >>= 1 ;
        if ( low_bit9 )
            reg[8] |= 0x80000000 ;

        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;
        if ( low_bit8 )
            reg[7] |= 0x80000000 ;

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

void Shift1024( unsigned * reg ) noexcept
{
    // 1024-bit
    //      1024 1020 1016 1012 1008 1004 
    //mask      1000 0000 0100 0000 0000 0011 
    // reversed 1100 0000 0000 0010 0000 0001   0x00c00201
    if ( reg[0] & 0x00000001 )
    {
        reg[0] ^= 0x00c00201 ;
        
        //shift :
        bool low_bit31 = reg[31] &  1 ;
        reg[31] >>= 1 ;

        bool low_bit30 = reg[30] &  1 ;
        reg[30] >>= 1 ;
        if ( low_bit31 )
            reg[30] |= 0x80000000 ;

        bool low_bit29 = reg[29] &  1 ;
        reg[29] >>= 1 ;
        if ( low_bit30 )
            reg[29] |= 0x80000000 ;

        bool low_bit28 = reg[28] &  1 ;
        reg[28] >>= 1 ;
        if ( low_bit29 )
            reg[28] |= 0x80000000 ;

        bool low_bit27 = reg[27] &  1 ;
        reg[27] >>= 1 ;
        if ( low_bit28 )
            reg[27] |= 0x80000000 ;

        bool low_bit26 = reg[26] &  1 ;
        reg[26] >>= 1 ;
        if ( low_bit27 )
            reg[26] |= 0x80000000 ;

        bool low_bit25 = reg[25] &  1 ;
        reg[25] >>= 1 ;
        if ( low_bit26 )
            reg[25] |= 0x80000000 ;

        bool low_bit24 = reg[24] &  1 ;
        reg[24] >>= 1 ;
        if ( low_bit25 )
            reg[24] |= 0x80000000 ;

        bool low_bit23 = reg[23] &  1 ;
        reg[23] >>= 1 ;
        if ( low_bit24 )
            reg[23] |= 0x80000000 ;

        bool low_bit22 = reg[22] &  1 ;
        reg[22] >>= 1 ;
        if ( low_bit23 )
            reg[22] |= 0x80000000 ;

        bool low_bit21 = reg[21] &  1 ;
        reg[21] >>= 1 ;
        if ( low_bit22 )
            reg[21] |= 0x80000000 ;

        bool low_bit20 = reg[20] &  1 ;
        reg[20] >>= 1 ;
        if ( low_bit21 )
            reg[20] |= 0x80000000 ;

        bool low_bit19 = reg[19] &  1 ;
        reg[19] >>= 1 ;
        if ( low_bit20 )
            reg[19] |= 0x80000000 ;

        bool low_bit18 = reg[18] &  1 ;
        reg[18] >>= 1 ;
        if ( low_bit19 )
            reg[18] |= 0x80000000 ;

        bool low_bit17 = reg[17] &  1 ;
        reg[17] >>= 1 ;
        if ( low_bit18 )
            reg[17] |= 0x80000000 ;

        bool low_bit16 = reg[16] &  1 ;
        reg[16] >>= 1 ;
        if ( low_bit17 )
            reg[16] |= 0x80000000 ;

        bool low_bit15 = reg[15] &  1 ;
        reg[15] >>= 1 ;
        if ( low_bit16 )
            reg[15] |= 0x80000000 ;

        bool low_bit14 = reg[14] &  1 ;
        reg[14] >>= 1 ;
        if ( low_bit15 )
            reg[14] |= 0x80000000 ;

        bool low_bit13 = reg[13] &  1 ;
        reg[13] >>= 1 ;
        if ( low_bit14 )
            reg[13] |= 0x80000000 ;

        bool low_bit12 = reg[12] &  1 ;
        reg[12] >>= 1 ;
        if ( low_bit13 )
            reg[12] |= 0x80000000 ;

        bool low_bit11 = reg[11] &  1 ;
        reg[11] >>= 1 ;
        if ( low_bit12 )
            reg[11] |= 0x80000000 ;

        bool low_bit10 = reg[10] &  1 ;
        reg[10] >>= 1 ;
        if ( low_bit11 )
            reg[10] |= 0x80000000 ;

        bool low_bit9 = reg[9] &  1 ;
        reg[9] >>= 1 ;
        if ( low_bit10 )
            reg[9] |= 0x80000000 ;

        bool low_bit8 = reg[8] &  1 ;
        reg[8] >>= 1 ;
        if ( low_bit9 )
            reg[8] |= 0x80000000 ;

        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;
        if ( low_bit8 )
            reg[7] |= 0x80000000 ;

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
        
        reg[31] |= 0x80000000 ;
    }
    else
    {
        //shift :
        bool low_bit31 = reg[31] &  1 ;
        reg[31] >>= 1 ;

        bool low_bit30 = reg[30] &  1 ;
        reg[30] >>= 1 ;
        if ( low_bit31 )
            reg[30] |= 0x80000000 ;

        bool low_bit29 = reg[29] &  1 ;
        reg[29] >>= 1 ;
        if ( low_bit30 )
            reg[29] |= 0x80000000 ;

        bool low_bit28 = reg[28] &  1 ;
        reg[28] >>= 1 ;
        if ( low_bit29 )
            reg[28] |= 0x80000000 ;

        bool low_bit27 = reg[27] &  1 ;
        reg[27] >>= 1 ;
        if ( low_bit28 )
            reg[27] |= 0x80000000 ;

        bool low_bit26 = reg[26] &  1 ;
        reg[26] >>= 1 ;
        if ( low_bit27 )
            reg[26] |= 0x80000000 ;

        bool low_bit25 = reg[25] &  1 ;
        reg[25] >>= 1 ;
        if ( low_bit26 )
            reg[25] |= 0x80000000 ;

        bool low_bit24 = reg[24] &  1 ;
        reg[24] >>= 1 ;
        if ( low_bit25 )
            reg[24] |= 0x80000000 ;

        bool low_bit23 = reg[23] &  1 ;
        reg[23] >>= 1 ;
        if ( low_bit24 )
            reg[23] |= 0x80000000 ;

        bool low_bit22 = reg[22] &  1 ;
        reg[22] >>= 1 ;
        if ( low_bit23 )
            reg[22] |= 0x80000000 ;

        bool low_bit21 = reg[21] &  1 ;
        reg[21] >>= 1 ;
        if ( low_bit22 )
            reg[21] |= 0x80000000 ;

        bool low_bit20 = reg[20] &  1 ;
        reg[20] >>= 1 ;
        if ( low_bit21 )
            reg[20] |= 0x80000000 ;

        bool low_bit19 = reg[19] &  1 ;
        reg[19] >>= 1 ;
        if ( low_bit20 )
            reg[19] |= 0x80000000 ;

        bool low_bit18 = reg[18] &  1 ;
        reg[18] >>= 1 ;
        if ( low_bit19 )
            reg[18] |= 0x80000000 ;

        bool low_bit17 = reg[17] &  1 ;
        reg[17] >>= 1 ;
        if ( low_bit18 )
            reg[17] |= 0x80000000 ;

        bool low_bit16 = reg[16] &  1 ;
        reg[16] >>= 1 ;
        if ( low_bit17 )
            reg[16] |= 0x80000000 ;

        bool low_bit15 = reg[15] &  1 ;
        reg[15] >>= 1 ;
        if ( low_bit16 )
            reg[15] |= 0x80000000 ;

        bool low_bit14 = reg[14] &  1 ;
        reg[14] >>= 1 ;
        if ( low_bit15 )
            reg[14] |= 0x80000000 ;

        bool low_bit13 = reg[13] &  1 ;
        reg[13] >>= 1 ;
        if ( low_bit14 )
            reg[13] |= 0x80000000 ;

        bool low_bit12 = reg[12] &  1 ;
        reg[12] >>= 1 ;
        if ( low_bit13 )
            reg[12] |= 0x80000000 ;

        bool low_bit11 = reg[11] &  1 ;
        reg[11] >>= 1 ;
        if ( low_bit12 )
            reg[11] |= 0x80000000 ;

        bool low_bit10 = reg[10] &  1 ;
        reg[10] >>= 1 ;
        if ( low_bit11 )
            reg[10] |= 0x80000000 ;

        bool low_bit9 = reg[9] &  1 ;
        reg[9] >>= 1 ;
        if ( low_bit10 )
            reg[9] |= 0x80000000 ;

        bool low_bit8 = reg[8] &  1 ;
        reg[8] >>= 1 ;
        if ( low_bit9 )
            reg[8] |= 0x80000000 ;

        bool low_bit7 = reg[7] &  1 ;
        reg[7] >>= 1 ;
        if ( low_bit8 )
            reg[7] |= 0x80000000 ;

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
