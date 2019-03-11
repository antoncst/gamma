
#include "platform.h"
#include "randomroutines.h"
#include "display.h"

#include <chrono>
#include <cstring>

#ifdef GAMMA_WINDOWS
#include <conio.h>
#include <windows.h>
#endif

#ifdef GAMMA_LINUX
#include <unistd.h>
#include <termios.h>

int getch()
{
    int ch;
    struct termios oldt, newt;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}
#endif


void GenerateRandom( unsigned * block_random , const size_t n_quantum )
{
    memset ( block_random , 0 , (n_quantum + 1) * sizeof( unsigned) ) ;
    
    display_str( "Type any text. This is required for strong encryption" ) ;
    
    size_t current_bit_offset = 0 ;
    size_t current_block_index = 0 ;
    
    #ifdef GAMMA_LINUX
    const int n_bit_used = 23 ;
    const unsigned mask = 0x7FFFFF ;
    auto mcs1 = std::chrono::high_resolution_clock::now() ;
    #endif

    #ifdef GAMMA_WINDOWS
    const int n_bit_used = 14 ;
    const unsigned mask = 0x3FFF ;
    LARGE_INTEGER mcs1 , mcs2 ;
    ::QueryPerformanceCounter( &mcs1 ) ;
    #endif

    while ( 1 )
    {

        int ch = getch() ;
        putchar ( ch );

        #ifdef GAMMA_LINUX
        auto mcs2 = std::chrono::high_resolution_clock::now() ;
        std::chrono::duration<double> mcs = mcs2 - mcs1 ;
        uint64_t rval ; //random value
        rval = mcs.count() * 1000000000 ;
        #endif
        
        #ifdef GAMMA_WINDOWS
        ::QueryPerformanceCounter( &mcs2 ) ;
        uint64_t rval = mcs2.QuadPart - mcs1.QuadPart ;
        #endif
        
        if ( rval <= mask ) // i.e. time interval is too short
            continue ;
        rval &= mask ;
        mcs1 = mcs2 ;
        
        rval = rval << current_bit_offset ;
        
        *( block_random + current_block_index ) |=  rval & 0xFFFFFFFF ;
        if ( current_bit_offset > 31 - n_bit_used ) // got out for 32 bits / вылез за 32 бита
            * (block_random + current_block_index +1 ) |= ( rval >> 32 ) ;
        
        current_bit_offset += n_bit_used ;
        if ( current_bit_offset > 31 )
        {
            current_bit_offset -= 32 ;
            current_block_index++ ;
        }
        if ( current_block_index >= n_quantum ) 
            break ;
    }
    display_str("\nrandom generated") ;

/*
    srand( 1 ) ; //time( NULL )
    for ( unsigned i=0 ; i < n_quantum  ; i++ ) 
    {
        unsigned val = rand() ;// + (rand() << 15 ) + ( (rand() && 0x3) << 30 ) ;
        *( block_random + i ) = val ;
    }
*/
}


void TransformRandomCycle( unsigned * block_random , const size_t n_quantum )
{
    unsigned first_bit = *block_random & 0x1 ;

    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        unsigned cur_mask = 1 ;
        for ( size_t j = 0 ; j < sizeof( unsigned ) * 8 -1 ; j++ )
        {
            *(block_random+i) = ( (*(block_random+i) & cur_mask ) ^ ( ( *(block_random+i) & (cur_mask << 1) ) >>1) ) | ( *(block_random+i) & (~cur_mask) ) ;
            cur_mask <<= 1 ;
        }
        
        if ( i != n_quantum -1 )
            *(block_random+i) = (( *(block_random+i) & 0x80000000 ) ^ (( *(block_random+i+1) & 0x1 ) << 31 )) | ( *(block_random+i) & 0x7FFFFFFF ) ;
    }
    *(block_random+n_quantum -1) = (( *(block_random+n_quantum -1) & 0x80000000 ) ^ ( first_bit << 31 )) | ( *(block_random+n_quantum -1) & 0x7FFFFFFF ) ;
}


void TransformRandom( unsigned * block_random , const size_t n_quantum )
{
    unsigned first_bit = block_random[0] & 0x1 ;

    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        //                          1st bit                     2nd bit                             reset 1st bit
        block_random[i] = ( (block_random[i] & 0x1 ) ^ (( block_random[i] & 0x2 ) >>1) ) | ( block_random[i] & 0xFFFFFFFE ) ;
        //                          2st bit                     3d bit                              reset 2st bit
        block_random[i] = ( (block_random[i] & 0x2 ) ^ (( block_random[i] & 0x4 ) >>1)) | ( block_random[i] & 0xFFFFFFFD ) ;
        //                          3d bit                      4th bit                             reset 3d bit
        block_random[i] = ( (block_random[i] & 0x4 ) ^ (( block_random[i] & 0x8 ) >>1)) | ( block_random[i] & 0xFFFFFFFB ) ;
        //                          4th bit                     5th bit                             reset 4th bit
        block_random[i] = ( (block_random[i] & 0x8 ) ^ (( block_random[i] & 0x10 ) >>1)) | ( block_random[i] & 0xFFFFFFF7 ) ;
        //                          5th bit                     6th bit                             reset 5th bit
        block_random[i] = ( (block_random[i] & 0x10 ) ^ (( block_random[i] & 0x20 ) >>1)) | ( block_random[i] & 0xFFFFFFEF ) ;
        //                          6th bit                     7th bit                             reset 6th bit
        block_random[i] = ( (block_random[i] & 0x20 ) ^ (( block_random[i] & 0x40 ) >>1)) | ( block_random[i] & 0xFFFFFFDF ) ;
        //                          7th bit                     8th bit                             reset 7th bit
        block_random[i] = ( (block_random[i] & 0x40 ) ^ (( block_random[i] & 0x80 ) >>1)) | ( block_random[i] & 0xFFFFFFBF ) ;
        //                          8th bit                     9th bit                             reset 8th bit
        block_random[i] = ( (block_random[i] & 0x80 ) ^ (( block_random[i] & 0x100 ) >>1)) | ( block_random[i] & 0xFFFFFF7F ) ;
        // etc
        block_random[i] = ((block_random[i] & 0x100 ) ^ (( block_random[i] & 0x200 )>>1)) | ( block_random[i] &    0xFFFFFEFF ) ;
        block_random[i] = ((block_random[i] & 0x200 ) ^ (( block_random[i] & 0x400 )>>1)) | ( block_random[i] &    0xFFFFFDFF ) ;
        block_random[i] = ((block_random[i] & 0x400 ) ^ (( block_random[i] & 0x800 )>>1)) | ( block_random[i] &    0xFFFFFBFF ) ;
        block_random[i] = ((block_random[i] & 0x800 ) ^ (( block_random[i] & 0x1000 )>>1)) | ( block_random[i] &   0xFFFFF7FF ) ;
        block_random[i] = ((block_random[i] & 0x1000 ) ^ (( block_random[i] & 0x2000 )>>1)) | ( block_random[i] &  0xFFFFEFFF ) ;
        block_random[i] = ((block_random[i] & 0x2000 ) ^ (( block_random[i] & 0x4000 )>>1)) | ( block_random[i] &  0xFFFFDFFF ) ;
        block_random[i] = ((block_random[i] & 0x4000 ) ^ (( block_random[i] & 0x8000 )>>1)) | ( block_random[i] &  0xFFFFBFFF ) ;
        block_random[i] = ((block_random[i] & 0x8000 ) ^ (( block_random[i] & 0x10000 )>>1)) | ( block_random[i] & 0xFFFF7FFF ) ;

        block_random[i] = ((block_random[i] & 0x10000 ) ^ (( block_random[i] &  0x20000 )>>1)) | ( block_random[i] &   0xFFFEFFFF ) ;
        block_random[i] = ((block_random[i] & 0x20000 ) ^ (( block_random[i] &  0x40000 )>>1)) | ( block_random[i] &   0xFFFDFFFF ) ;
        block_random[i] = ((block_random[i] & 0x40000 ) ^ (( block_random[i] &  0x80000 )>>1)) | ( block_random[i] &   0xFFFBFFFF ) ;
        block_random[i] = ((block_random[i] & 0x80000 ) ^ (( block_random[i] &  0x100000 )>>1)) | ( block_random[i] &  0xFFF7FFFF ) ;
        block_random[i] = ((block_random[i] & 0x100000 ) ^ (( block_random[i] & 0x200000 )>>1)) | ( block_random[i] &  0xFFEFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x200000 ) ^ ((block_random[i] &  0x400000 )>>1)) | ( block_random[i] &  0xFFDFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x400000 ) ^ (( block_random[i] & 0x800000 )>>1)) | ( block_random[i] &  0xFFBFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x800000 ) ^ (( block_random[i] & 0x1000000 )>>1)) | ( block_random[i] & 0xFF7FFFFF ) ;

        block_random[i] = ((block_random[i] & 0x1000000 ) ^ (( block_random[i] &  0x2000000 )>>1)) | ( block_random[i] &   0xFEFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x2000000 ) ^ (( block_random[i] &  0x4000000 )>>1)) | ( block_random[i] &   0xFDFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x4000000 ) ^ (( block_random[i] &  0x8000000 )>>1)) | ( block_random[i] &   0xFBFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x8000000 ) ^ (( block_random[i] &  0x10000000 )>>1)) | ( block_random[i] &  0xF7FFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x10000000 ) ^ (( block_random[i] & 0x20000000 )>>1)) | ( block_random[i] &  0xEFFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x20000000 ) ^ (( block_random[i] & 0x40000000 )>>1)) | ( block_random[i] &  0xDFFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x40000000 ) ^ (( block_random[i] & 0x80000000 )>>1)) | ( block_random[i] &  0xBFFFFFFF ) ;
        if ( i != n_quantum -1 )
            block_random[i] = ((block_random[i] & 0x80000000 ) ^ (( block_random[i+1] & 0x1 ) << 31)) | ( block_random[i] & 0x7FFFFFFF ) ;
    }
    block_random[n_quantum -1] = ((block_random[n_quantum -1] & 0x80000000 ) ^ ( first_bit <<31 )) | ( block_random[n_quantum -1] & 0x7FFFFFFF ) ;
}

/*
//repeat after 128 iterations when block_size = 32 bytes
void GammaCrypt::Bit2TransformRandom()
{
    unsigned first_byte = m_block_random[0] & 0x3 ;
    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        m_block_random[i] = ( ( m_block_random[i] & 0x3) ^ (( m_block_random[i] & 0xC) >>2 ) ) | ( m_block_random[i] &        0xFFFFFFFC ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC) ^ (( m_block_random[i] & 0x30) >>2 ) ) | ( m_block_random[i] &       0xFFFFFFF3 ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x30) ^ (( m_block_random[i] & 0xC0) >>2 ) ) | ( m_block_random[i] &      0xFFFFFFCF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC0) ^ (( m_block_random[i] & 0x300) >>2 ) ) | ( m_block_random[i] &     0xFFFFFF3F ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x300) ^ (( m_block_random[i] & 0xC00) >>2 ) ) | ( m_block_random[i] &    0xFFFFFCFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC00) ^ (( m_block_random[i] & 0x3000) >>2 ) ) | ( m_block_random[i] &   0xFFFFF3FF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x3000) ^ (( m_block_random[i] & 0xC000) >>2 ) ) | ( m_block_random[i] &  0xFFFFCFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC000) ^ (( m_block_random[i] & 0x30000) >>2 ) ) | ( m_block_random[i] & 0xFFFF3FFF ) ;

        m_block_random[i] = ( ( m_block_random[i] & 0x30000) ^ (( m_block_random[i] &    0xC0000) >>2 ) ) | ( m_block_random[i] &     0xFFFCFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC0000) ^ (( m_block_random[i] &    0x300000) >>2 ) ) | ( m_block_random[i] &    0xFFF3FFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x300000) ^ (( m_block_random[i] &   0xC00000) >>2 ) ) | ( m_block_random[i] &    0xFFCFFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC00000) ^ (( m_block_random[i] &   0x3000000) >>2 ) ) | ( m_block_random[i] &   0xFF3FFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x3000000) ^ (( m_block_random[i] &  0xC000000) >>2 ) ) | ( m_block_random[i] &   0xFCFFFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xC000000) ^ (( m_block_random[i] &  0x30000000) >>2 ) ) | ( m_block_random[i] &  0xF3FFFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0x30000000) ^ (( m_block_random[i] & 0xC0000000) >>2 ) ) | ( m_block_random[i] &  0xCFFFFFFF ) ;
        if ( i != n_quantum -1 )
            m_block_random[i] = ( ( m_block_random[i] & 0xC0000000) ^ (( m_block_random[i+1] & 0x3) <<30 ) ) | ( m_block_random[i] & 0x3FFFFFFF ) ;
    }
    m_block_random[n_quantum -1] = ( ( m_block_random[n_quantum -1] & 0xC0000000) ^ first_byte <<30) | ( m_block_random[n_quantum -1] & 0x3FFFFFFF ) ;

}



// repeat after 64 when block_size = 32 bytes
void GammaCrypt::Bit4TransformRandom()
{
    unsigned first_byte = m_block_random[0] & 0xF ;
    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        m_block_random[i] = ( ( m_block_random[i] & 0xF) ^ (( m_block_random[i] & 0xF0) >>4 ) ) | ( m_block_random[i] & 0xFFFFFFF0 ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF0) ^ (( m_block_random[i] & 0xF00) >>4 ) ) | ( m_block_random[i] & 0xFFFFFF0F ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF00) ^ (( m_block_random[i] & 0xF000) >>4 ) ) | ( m_block_random[i] & 0xFFFFF0FF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF000) ^ (( m_block_random[i] & 0xF0000) >>4 ) ) | ( m_block_random[i] & 0xFFFF0FFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF0000) ^ (( m_block_random[i] & 0xF00000) >>4 ) ) | ( m_block_random[i] & 0xFFF0FFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF00000) ^ (( m_block_random[i] & 0xF000000) >>4 ) ) | ( m_block_random[i] & 0xFF0FFFFF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xF000000) ^ (( m_block_random[i] & 0xF0000000) >>4 ) ) | ( m_block_random[i] & 0xF0FFFFFF ) ;
        if ( i != n_quantum -1 )
            m_block_random[i] = ( ( m_block_random[i] & 0xF0000000) ^ (( m_block_random[i+1] & 0xF) <<28 ) ) | ( m_block_random[i] & 0x0FFFFFFF ) ;
    }
    m_block_random[n_quantum -1] = ( ( m_block_random[n_quantum -1] & 0xF0000000) ^ (first_byte <<28 ) ) | ( m_block_random[n_quantum-1] & 0x0FFFFFFF ) ;

}

// repeat after 32 when block_size = 32 bytes
void GammaCrypt::ByteTransformRandom()
{
    unsigned first_byte = m_block_random[0] & 0xFF ;
    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        m_block_random[i] = ( ( m_block_random[i] & 0xFF) ^ (( m_block_random[i] & 0xFF00) >>8 ) ) | ( m_block_random[i] & 0xFFFFFF00 ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xFF00) ^ (( m_block_random[i] & 0xFF0000) >>8 ) ) | ( m_block_random[i] & 0xFFFF00FF ) ;
        m_block_random[i] = ( ( m_block_random[i] & 0xFF0000) ^ (( m_block_random[i] & 0xFF000000) >>8 ) ) | ( m_block_random[i] & 0xFF00FFFF ) ;
        if ( i != n_quantum -1 )
            m_block_random[i] = ((m_block_random[i] & 0xFF000000 ) ^ (( m_block_random[i+1] & 0xFF ) << 24 )) | ( m_block_random[i] & 0x00FFFFFF ) ;
    }
    m_block_random[n_quantum -1] = ((m_block_random[n_quantum -1] & 0xFF000000 ) ^ (( first_byte ) << 24)) | ( m_block_random[n_quantum -1] & 0x00FFFFFF ) ;

}

// repeat after 16 when block_size = 32 bytes
void GammaCrypt::WordTransformRandom()
{
    unsigned first_word = m_block_random[0] & 0xFFFF ;
    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        m_block_random[i] = ( ( m_block_random[i] & 0xFFFF) ^ (( m_block_random[i] & 0xFFFF0000) >>16 ) ) | ( m_block_random[i] & 0xFFFF0000 ) ;
        if ( i != n_quantum -1 )
            m_block_random[i] = ((m_block_random[i] & 0xFFFF0000 ) ^ (( m_block_random[i+1] & 0xFFFF ) << 16)) | ( m_block_random[i] & 0x0000FFFF ) ;
    }
    m_block_random[n_quantum -1] = ((m_block_random[n_quantum -1] & 0xFFFF0000 ) ^ (( first_word ) << 16)) | ( m_block_random[n_quantum -1] & 0x0000FFFF ) ;

}

// repeat after 8 when block_size = 32 bytes
void GammaCrypt::QuadTransformRandom()
{
    unsigned first_word = m_block_random[0] ;
    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        if ( i != n_quantum -1 )
            m_block_random[i] = (m_block_random[i] ^  m_block_random[i+1] ) ;
    }
    m_block_random[n_quantum -1] = (m_block_random[n_quantum -1] ^ first_word ) ;
}
*/

