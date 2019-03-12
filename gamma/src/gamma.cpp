#include <string>
#include <fstream>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

//#include <array> // for debug only
//#include <iostream>  // for debug only

#include "gamma.h"
#include "display.h"

using namespace std ;


GammaCrypt::GammaCrypt( istream & ifs , ostream & ofs ,  const string & password ) 
    : m_ifs( ifs ) , m_ofs( ofs ) , m_password( password )
{
    memset( & m_block_password , 0 , block_size ) ;
    memcpy( & m_block_password , m_password.c_str(), m_password.length() ) ;
}

void GammaCrypt::Encrypt()
{

    GenerateRandom( m_block_random , n_quantum ) ;
    WriteHead() ;
    Crypt() ;
    
}

void GammaCrypt::Decrypt()
{
    ReadHead() ;
    Crypt() ;
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
/*    
    // for debugging
    std::array<t_block, 10000> v ;
    size_t n = 0 ;
    bool b_found = false ;
*/    
    while ( ! m_ifs.eof() )
    {
        TransformRandom( m_block_random , n_quantum ) ;
        
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
