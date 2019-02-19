#include <string>
#include <fstream>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <cstring>

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
    
    display_str( "Type any text. This is required for strong encryption" ) ;
    
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
    
    
    /*srand( time( NULL ) ) ;
    for ( unsigned i=0 ; i < n_quantum  ; i++ ) 
    {
        unsigned val = rand() ;// + (rand() << 15 ) + ( (rand() && 0x3) << 30 ) ;
        m_block_random[i] = val ;
    }*/
}


GammaCrypt::GammaCrypt( string in_filename, string out_filename, string password ) 
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

void GammaCrypt::Crypt()
{
    while ( ! m_ifs.eof() )
    {
        m_ifs.read( (char*) m_block_source, block_size ) ;
        for ( unsigned i = 0 ; i < n_quantum ; i++ )
            m_block_dest[i] = m_block_source[i] ^ m_block_random[i] ;
        unsigned bytes_read = m_ifs.gcount() ;
        m_ofs.write( (char*) m_block_dest , bytes_read ) ;
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
