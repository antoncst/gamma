#include <string>
#include <fstream>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <array> // for debug only
#include <iostream>  // for debug only

#include "gamma.h"
#include "display.h"

using namespace std ;


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
    
    GenerateRandom( m_block_random , n_quantum ) ;
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
    
/*    // for debugging
    std::array<t_block, 10000> v ;
    size_t n = 0 ;
    bool b_found = false ;
*/    
    while ( ! m_ifs.eof() )
    {
        TransformRandom( m_block_password , n_quantum ) ;
        //Bit4TransformRandom() ;
        
        m_ifs.read( (char*) m_block_source, block_size ) ;
        for ( unsigned i = 0 ; i < n_quantum ; i++ )
            m_block_dest[i] = m_block_source[i] ^ m_block_random[i] ;
        unsigned bytes_read = m_ifs.gcount() ;
        m_ofs.write( (char*) m_block_dest , bytes_read ) ;

/*        // for debugging
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
