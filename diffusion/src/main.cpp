#include <iostream>
#include "helper.h"

const unsigned m_block_size_bytes = 32 ;
const unsigned m_blk_sz_words = m_block_size_bytes / sizeof(unsigned) ; 

inline void MakeDiffusion( unsigned * p32src) noexcept
{
//    memcpy( m8_temp_block , p32src , m_block_size_bytes ) ;
//    //для удобства:
//    p32src = m8_temp_block ;
	uint64_t * p64src = reinterpret_cast< uint64_t * >( p32src ) ; 

    for ( unsigned j = 0 ; j <= 8 ; j++ )
    {
        //arithmetic manipulations
        for ( unsigned i = 1 ; i < m_blk_sz_words / 2 ; i++ ) // /2 because of uint64_t
        {
            p64src[ i ] += p64src[ i - 1 ] ;
        }
        p64src[ 0 ] += p64src[ m_blk_sz_words/2 - 1 ] ;

        // left circular shift
        for ( unsigned i = 0 ; i < m_blk_sz_words / 2 ; i++ ) 
            if ( j == 0 || j == 2 )
                p64src[i] = ( p64src[i] << 1 ) | ( p64src[i] >> 63 ) ; 
            else if ( j == 1 )
                p64src[i] = ( p64src[i] << 2 ) | ( p64src[i] >> 62 ) ; 
            else if ( j == 3 )
                p64src[i] = ( p64src[i] << 3 ) | ( p64src[i] >> 61 ) ; 
            else if ( j == 4 )
                p64src[i] = ( p64src[i] << 7 ) | ( p64src[i] >> 57 ) ; 
            else if ( j == 5 &&  ( i % 2 == 0 ) )
                p64src[i] = ( p64src[i] << 16 ) | ( p64src[i] >> 48 ) ;
            else if ( j == 6 &&  ( i % 2 == 0 ) )
                p64src[i] = ( p64src[i] << 31 ) | ( p64src[i] >> 33 ) ;
            else if ( j == 7 &&  ( i % 2 == 0 ) )
                p64src[i] = ( p64src[i] << 58 ) | ( p64src[i] >> 6 ) ; 
    }
}


int main( int argc, char **argv )
{
    uint32_t * m32_block_source = new unsigned[ m_blk_sz_words ] ;
    uint32_t * m32_block_dest = new unsigned[ m_blk_sz_words ] ;

    char * mpbuffer = reinterpret_cast< char * > ( m32_block_source ) ;
//    char * mpbuffout = reinterpret_cast< char * > ( m32_block_dest ) ;
//    unsigned mbuf_size = m_block_size_bytes ;

    std::ifstream ifs ;
    //std::ifstream ifs_keyfile ;
    std::ofstream ofs ; // when generating keyfile , using this stream for keyfile

    CmdLnParser parser ;
    parser.ParseCommandLine( argc , argv ) ;
    if  (parser.m_b_error )
        return 1;

    // open in file
    try
    {
        OpenInFile( parser.m_in_filename , ifs ) ;
    }
    catch ( const char * s )
    {
        std::cout <<  s  ;
        return 3 ;
    }

    try
    {
            OpenOutFile( parser.m_out_filename , ofs ) ;
    }
    catch ( const char * s )
    {
        std::cout <<  s  ;
        return 4 ;
    }

    while ( ! ifs.eof() )
    {

        ifs.read(  mpbuffer , m_block_size_bytes ) ;
        unsigned bytes_read = ifs.gcount() ;
        if ( bytes_read == 0 )
            continue ;

        MakeDiffusion( m32_block_source ) ;

        ofs.write( (char*) mpbuffer , bytes_read ) ;

    }
    delete[] m32_block_source ;
    delete[] m32_block_dest ;


    ifs.close() ;
    ofs.close() ;


}

