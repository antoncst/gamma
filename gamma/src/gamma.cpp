#include <string>
#include <fstream>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <cstring>

#include <array> // for debug only
#include <iostream>  // for debug only

#include "gamma.h"
#include "display.h"

using namespace std ;

#include <chrono>
#include <stdio.h>

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


void GammaCrypt::GenerateRandom()
{
    memset ( m_block_random , 0 , block_size + quantum_size ) ;
    
/*    display_str( "Type any text. This is required for strong encryption" ) ;
    
    int current_bit_offset = 0 ;
    int current_block_index = 0 ;
    
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
        
        m_block_random[ current_block_index ] |=  rval & 0xFFFFFFFF ;
        if ( current_bit_offset > 31 - n_bit_used ) // got out for 32 bits / вылез за 32 бита
            m_block_random[ current_block_index +1 ] |= ( rval >> 32 ) ;
        
        current_bit_offset += n_bit_used ;
        if ( current_bit_offset > 31 )
        {
            current_bit_offset -= 32 ;
            current_block_index++ ;
        }
        if ( current_block_index > 7 ) 
            break ;
    }
*/    
    
    srand( 1 ) ; //time( NULL )
    for ( unsigned i=0 ; i < n_quantum  ; i++ ) 
    {
        unsigned val = rand() ;// + (rand() << 15 ) + ( (rand() && 0x3) << 30 ) ;
        m_block_random[i] = val ;
    }
}


GammaCrypt::GammaCrypt( const string in_filename, const string out_filename, const string password ) 
    : m_in_filename( in_filename) , m_out_filename( out_filename) , m_password( password )
{
    memset( & m_block_password , 0 , block_size ) ;
    memcpy( & m_block_password , m_password.c_str(), m_password.length() ) ;
}

void GammaCrypt::Encrypt()
{
    if ( !OpenFiles() ) 
    {
        CloseFiles() ; // because 'in' file can be opened, but 'out' is not opened
        return;
    }
    
    GenerateRandom() ;
    WriteHead() ;
    Crypt() ;
    
    CloseFiles() ;
    
}

void GammaCrypt::Decrypt()
{
    if ( !OpenFiles() ) 
    {
        CloseFiles() ; // because 'in' file can be opened, but 'out' is not opened
        return;
    }
    
    ReadHead() ;
    Crypt() ;
    
    CloseFiles() ;
    
}

void GammaCrypt::ReadHead()
{
    t_block block_key ;
    m_ifs.read( (char*) block_key , block_size ) ;
    for ( unsigned i = 0 ; i < n_quantum ; i++ )
        m_block_random[i] = block_key[i] ^ m_block_password[i] ;
}


void GammaCrypt::WriteHead()
{
    t_block block_key ;
    for ( unsigned i = 0 ; i < n_quantum ; i++ )
        block_key[i] = m_block_random[i] ^ m_block_password[i] ;
    m_ofs.write( (char*) block_key , block_size ) ;
}


//repeat after 128 iterations
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

//repeat after 256 iterations
void GammaCrypt::TransformRandom()
{
    unsigned first_bit = m_block_random[0] & 0x1 ;

    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        unsigned cur_mask = 1 ;
        for ( size_t j = 0 ; j < sizeof( unsigned ) * 8 -1 ; j++ )
        {
            m_block_random[i] = ( (m_block_random[i] & cur_mask ) ^ ( ( m_block_random[i] & (cur_mask << 1) ) >>1) ) | ( m_block_random[i] & (~cur_mask) ) ;
            cur_mask <<= 1 ;
        }
        
        if ( i != n_quantum -1 )
            m_block_random[i] = ((m_block_random[i] & 0x80000000 ) ^ (( m_block_random[i+1] & 0x1 ) << 31 )) | ( m_block_random[i] & 0x7FFFFFFF ) ;
    }
    m_block_random[n_quantum -1] = ((m_block_random[n_quantum -1] & 0x80000000 ) ^ ( first_bit << 31 )) | ( m_block_random[n_quantum -1] & 0x7FFFFFFF ) ;
}

//repeat after 32 iterations
void GammaCrypt::BitTransformRandom()
{
    unsigned first_bit = m_block_random[0] & 0x1 ;

    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        //                          1st bit                     2nd bit                             reset 1st bit
        m_block_random[i] = ( (m_block_random[i] & 0x1 ) ^ (( m_block_random[i] & 0x2 ) >>1) ) | ( m_block_random[i] & 0xFFFFFFFE ) ;
        //                          2st bit                     3d bit                              reset 2st bit
        m_block_random[i] = ( (m_block_random[i] & 0x2 ) ^ (( m_block_random[i] & 0x4 ) >>1)) | ( m_block_random[i] & 0xFFFFFFFD ) ;
        //                          3d bit                      4th bit                             reset 3d bit
        m_block_random[i] = ( (m_block_random[i] & 0x4 ) ^ (( m_block_random[i] & 0x8 ) >>1)) | ( m_block_random[i] & 0xFFFFFFFB ) ;
        //                          4th bit                     5th bit                             reset 4th bit
        m_block_random[i] = ( (m_block_random[i] & 0x8 ) ^ (( m_block_random[i] & 0x10 ) >>1)) | ( m_block_random[i] & 0xFFFFFFF7 ) ;
        //                          5th bit                     6th bit                             reset 5th bit
        m_block_random[i] = ( (m_block_random[i] & 0x10 ) ^ (( m_block_random[i] & 0x20 ) >>1)) | ( m_block_random[i] & 0xFFFFFFEF ) ;
        //                          6th bit                     7th bit                             reset 6th bit
        m_block_random[i] = ( (m_block_random[i] & 0x20 ) ^ (( m_block_random[i] & 0x40 ) >>1)) | ( m_block_random[i] & 0xFFFFFFDF ) ;
        //                          7th bit                     8th bit                             reset 7th bit
        m_block_random[i] = ( (m_block_random[i] & 0x40 ) ^ (( m_block_random[i] & 0x80 ) >>1)) | ( m_block_random[i] & 0xFFFFFFBF ) ;
        //                          8th bit                     9th bit                             reset 8th bit
        m_block_random[i] = ( (m_block_random[i] & 0x80 ) ^ (( m_block_random[i] & 0x100 ) >>1)) | ( m_block_random[i] & 0xFFFFFF7F ) ;
        // etc
        m_block_random[i] = ((m_block_random[i] & 0x100 ) ^ (( m_block_random[i] & 0x200 )>>1)) | ( m_block_random[i] &    0xFFFFFEFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x200 ) ^ (( m_block_random[i] & 0x400 )>>1)) | ( m_block_random[i] &    0xFFFFFDFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x400 ) ^ (( m_block_random[i] & 0x800 )>>1)) | ( m_block_random[i] &    0xFFFFFBFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x800 ) ^ (( m_block_random[i] & 0x1000 )>>1)) | ( m_block_random[i] &   0xFFFFF7FF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x1000 ) ^ (( m_block_random[i] & 0x2000 )>>1)) | ( m_block_random[i] &  0xFFFFEFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x2000 ) ^ (( m_block_random[i] & 0x4000 )>>1)) | ( m_block_random[i] &  0xFFFFDFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x4000 ) ^ (( m_block_random[i] & 0x8000 )>>1)) | ( m_block_random[i] &  0xFFFFBFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x8000 ) ^ (( m_block_random[i] & 0x10000 )>>1)) | ( m_block_random[i] & 0xFFFF7FFF ) ;

        m_block_random[i] = ((m_block_random[i] & 0x10000 ) ^ (( m_block_random[i] &  0x20000 )>>1)) | ( m_block_random[i] &   0xFFFEFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x20000 ) ^ (( m_block_random[i] &  0x40000 )>>1)) | ( m_block_random[i] &   0xFFFDFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x40000 ) ^ (( m_block_random[i] &  0x80000 )>>1)) | ( m_block_random[i] &   0xFFFBFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x80000 ) ^ (( m_block_random[i] &  0x100000 )>>1)) | ( m_block_random[i] &  0xFFF7FFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x100000 ) ^ (( m_block_random[i] & 0x200000 )>>1)) | ( m_block_random[i] &  0xFFEFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x200000 ) ^ ((m_block_random[i] &  0x400000 )>>1)) | ( m_block_random[i] &  0xFFDFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x400000 ) ^ (( m_block_random[i] & 0x800000 )>>1)) | ( m_block_random[i] &  0xFFBFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x800000 ) ^ (( m_block_random[i] & 0x1000000 )>>1)) | ( m_block_random[i] & 0xFF7FFFFF ) ;

        m_block_random[i] = ((m_block_random[i] & 0x1000000 ) ^ (( m_block_random[i] &  0x2000000 )>>1)) | ( m_block_random[i] &   0xFEFFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x2000000 ) ^ (( m_block_random[i] &  0x4000000 )>>1)) | ( m_block_random[i] &   0xFDFFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x4000000 ) ^ (( m_block_random[i] &  0x8000000 )>>1)) | ( m_block_random[i] &   0xFBFFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x8000000 ) ^ (( m_block_random[i] &  0x10000000 )>>1)) | ( m_block_random[i] &  0xF7FFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x10000000 ) ^ (( m_block_random[i] & 0x20000000 )>>1)) | ( m_block_random[i] &  0xEFFFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x20000000 ) ^ (( m_block_random[i] & 0x40000000 )>>1)) | ( m_block_random[i] &  0xDFFFFFFF ) ;
        m_block_random[i] = ((m_block_random[i] & 0x40000000 ) ^ (( m_block_random[i] & 0x80000000 )>>1)) | ( m_block_random[i] &  0xBFFFFFFF ) ;
        if ( i != n_quantum -1 )
            m_block_random[i] = ((m_block_random[i] & 0x80000000 ) ^ (( m_block_random[i+1] & 0x1 ) << 31)) | ( m_block_random[i] & 0x7FFFFFFF ) ;
    }
    m_block_random[n_quantum -1] = ((m_block_random[n_quantum -1] & 0x80000000 ) ^ ( first_bit <<31 )) | ( m_block_random[n_quantum -1] & 0x7FFFFFFF ) ;
}



// repeat after 64
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

// repeat after 32
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

// repeat after 16
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

// repeat after 8
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



void GammaCrypt::Crypt()
{
    
/*    std::array<t_block, 10000> v ;
    size_t n = 0 ;
    bool b_found = false ;
*/    
    while ( ! m_ifs.eof() )
    {
            TransformRandom() ;
            //Bit4TransformRandom() ;
        
        m_ifs.read( (char*) m_block_source, block_size ) ;
        for ( unsigned i = 0 ; i < n_quantum ; i++ )
            m_block_dest[i] = m_block_source[i] ^ m_block_random[i] ;
        unsigned bytes_read = m_ifs.gcount() ;
        m_ofs.write( (char*) m_block_dest , bytes_read ) ;
/*        
        memcpy (&v[n] , m_block_dest , 32 ) ;
        for ( size_t i = 0 ; i <n  ; i++ )
        {
            if ( memcmp( &v[i] , m_block_dest , 32) == 0 )
            {
                std::cout << "found:" << n ;
                b_found = true ;
                break ;
            }
        }
        if (b_found)
            break ;
        n++;
*/
    }
}

bool GammaCrypt::OpenFiles()
{
    m_ifs.open( m_in_filename.c_str() , ifstream::in | ifstream::binary ) ;
    if ( ! m_ifs.is_open() )
    {
        display_err( " Error opening input file for reading" ) ;
        return false ;
    }

    m_ofs.open( m_out_filename.c_str(), ifstream::out | ifstream::binary | ifstream::trunc ) ;

    if ( ! m_ofs.is_open() )
    {
        display_err( " Error opening output file for writing" ) ;
        return false ;
    }

    return true ;
}

void GammaCrypt::CloseFiles()
{
    if ( m_ifs.is_open() )
        m_ifs.close() ;
    if ( m_ofs.is_open() )
        m_ofs.close() ;
}

void CmdLnParser::ParseCommandLine( int argc , char **argv )
{
    m_b_error = false ;
    int arg_counter = 0;
    
    action = none ;
    infile = 0 ;
    outfile = 0 ;
    
    bool now_keys_coming = true ;
    for ( int i = 1 ; i < argc ; i++ )
    {
        string s( argv[i] ) ;
        if ( now_keys_coming )
        {
            if ( s[0] == '-')
            {
                if ( s == "-e" || s == "--encrypt" )
                    if ( action == none )
                        action = encrypt ;
                    else
                        { m_b_error = true ; break ; } 
                else if ( s == "-d" || s == "--decrypt" )
                    if ( action == none )
                        action = decrypt ;
                    else
                        { m_b_error = true ; break ; } 
                else if ( s == "-h" || s == "--help" )
                    { m_b_error = true ; break ; } 
                else
                    { m_b_error = true ; break ; } 
            }
            else
                now_keys_coming = false ;
        }
        if ( !now_keys_coming )
        {
            if ( s[0] == '-')
                { m_b_error = true ; break ; } 
            else
            {
                arg_counter++ ;
                if ( arg_counter >2 )
                    { m_b_error = true ; break ; } 
                if ( arg_counter == 1 )
                    infile = i ;
                if ( arg_counter == 2 )
                    outfile = i ;
                    
                
            }
        }
    }
    if ( m_b_error )
        display_str( help_string ) ;
    else
        if ( action == none)
            action = encrypt ;
        
}
