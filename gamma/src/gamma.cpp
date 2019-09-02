#include <string>
#include <fstream>
#include <cassert>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <memory>
#include <chrono>
#include <sstream>
//#include <array> // for debug only
//#include <iostream>  // for debug only

#include "gamma.h"
#include "LFSR.h"
#include "../../Display/ConsoleDisplay/include/display.h"

using namespace std ;


GammaCrypt::GammaCrypt( istream & ifs , ostream & ofs ,  const string & password ) 
    : m_ifs( ifs ) , m_ofs( ofs ) , m_password( password ) 
    , mp_block_random( nullptr ) , mp_block_random3( nullptr ) , mp_block_password( nullptr ) , mp_block_source( nullptr ) , mp_block_dest( nullptr )
{
}

void GammaCrypt::Initialize()
{
    
/*
    // calculate our 'constants;)'
    if ( file_size > 262143 ) // 256K - 1M
        m_block_size = 512 ;
    else if ( file_size > 65535 ) // 64K - 256K
        m_block_size = 256 ;
    else if ( file_size > 16384 ) // 16K - 64K
        m_block_size = 128 ;
    else if ( file_size > 4096 ) // 4K - 16K
        m_block_size = 64 ;
    else m_block_size = 32 ;        // 0 - 4K

    // calculate our 'constants;)'
    if ( mb_need_init_blocksize )
    {
        if ( m_header.source_file_size > 1048575 ) // >= 1M
            m_block_size = 2048 ;
        else if ( m_header.source_file_size > 262143 ) // 256K - 1M
            m_block_size = 1024 ;
        else if ( m_header.source_file_size > 65535 ) // 64K - 256K
            m_block_size = 512 ;
        else if ( m_header.source_file_size > 16384 ) // 16K - 64K
            m_block_size = 256 ;
        else if ( m_header.source_file_size > 4096 ) // 4K - 16K
            m_block_size = 128 ;
        else if ( m_header.source_file_size > 1024 ) // 1K - 4K
            m_block_size = 64 ;
        else if ( m_header.source_file_size > 256 ) // 256 - 1K
            m_block_size = 32 ;
        else if ( m_header.source_file_size > 1024 ) // 64 - 256
            m_block_size = 16 ;
        else m_block_size = 8 ;        // 0 - 16 - 64
    }
*/
    if ( mb_need_init_blocksize )
    {
        if ( m_header.source_file_size >= 67108864 ) // 64M -
            m_block_size = 128 ;
        else if ( m_header.source_file_size >= 16777216 ) // 16M - 64M
            m_block_size = 64 ;
        else if ( m_header.source_file_size >= 1048576 ) // 1M - 16M
            m_block_size = 16 ;
        else if ( m_header.source_file_size >= 4096 ) // 4K - 1M
            m_block_size = 16 ;
        else        // 0 - 4K
            m_block_size = 8 ;        
    }

    // init mpShift
    if ( m_block_size == 128 ) // 64M -
        mpShift = Shift1024 ;
    else if ( m_block_size == 64 ) // 16M - 
        mpShift = Shift512 ;
    else if ( m_block_size ==32 ) // 1M - 16M
        mpShift = Shift256 ;
    else if ( m_block_size == 16 ) // 4K - 1M
        mpShift = Shift128 ;
    else        // 0 - 4K
        mpShift = Shift64 ;
    
    m_n_quantum = m_block_size / m_quantum_size ;
    
    // allocate memory
    mp_block_password  = make_unique< t_block >( m_n_quantum ) ;
    mp_block_random    = make_unique< t_block >( m_n_quantum + 1 ) ; // to transform Random block it is required one quantum more (therefore "+1")
    mp_block_random3   = make_unique< t_block >( m_n_quantum + 1 ) ; // to transform Random block it is required one quantum more (therefore "+1")
    mp_block_source    = make_unique< t_block >( m_n_quantum ) ;
    mp_block_dest      = make_unique< t_block >( m_n_quantum ) ;

    // password block initialization
    memset( mp_block_password.get() , 0 , m_block_size ) ;
    size_t i = 0 ;
    size_t psw_size = m_password.size() ;
    if ( psw_size > m_block_size )
        psw_size = m_block_size ;
    size_t bytes_to_copy ;
    while ( i < m_block_size )
    {
        bytes_to_copy =  i + psw_size  <= m_block_size  ?  psw_size : m_block_size - i ;
        memcpy( reinterpret_cast<char*>( mp_block_password.get() ) + i , m_password.c_str(), bytes_to_copy ) ;
        i += bytes_to_copy ;
    }
    assert( i == m_block_size ) ;
    // transform psw block in cycle
    // This results in a delay that is usefull against 'dictionary' atack
    for ( unsigned i = 0 ; i < 4093 ; ++i )
        ( *mpShift )( mp_block_password.get() ) ;
}

void GammaCrypt::Encrypt()
{
    //calculate size of input file
    //определим размер входного файла
    m_ifs.seekg( 0 , ios::end ) ;
    m_header.source_file_size = m_ifs.tellg() ;
    m_ifs.seekg( 0 , ios::beg ) ;

    Initialize() ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_random3 != nullptr ) ;
        
    display_str("Generating randoms...") ;
            // time mesaure
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //
    GenerateRandoms( m_n_quantum , mp_block_random.get() ) ;
    GenerateRandoms( m_n_quantum , mp_block_random3.get() ) ;
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
    
    display_str("making reposition matrix...") ;
            // time mesaure
            mcs1 = std::chrono::high_resolution_clock::now() ;
            //
    m_Reposition.Init( m_block_size ) ;
    m_Reposition.MakeRearrangeMatrix() ;
        // cout matrix
        /*std::cout << "\n" ;
        for ( int i = 0 ; i < m_Reposition.max_index ; i ++)
            std::cout << m_Reposition[ i ] << ' ' ;
        std::cout << std::endl ;*/

            // time mesaure
            mcs2 = std::chrono::high_resolution_clock::now() ;
            mcs = mcs2 - mcs1 ;
            drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //

    WriteHead() ; 
    DisplayInfo() ;

    //CRYPT ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_random3 != nullptr ) ;
        assert( mp_block_source != nullptr ) ;
        assert( mp_block_dest != nullptr ) ;

    display_str("crypting...") ;

            // time mesaure
            mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    auto temp_block = std::make_unique< unsigned char[] >( m_Reposition.m_BIarray.block_size_bytes ) ; // for Rearrange Matrix

    uint16_t bytes_read ;

    while ( ! m_ifs.eof() )
    {
        
        m_ifs.read( (char*) mp_block_source.get(), m_block_size ) ;
        bytes_read = m_ifs.gcount() ;
        
        // XOR1
        //TransformRandom( mp_block_random.get() , m_n_quantum ) ;
        ( *mpShift )( mp_block_random.get() ) ;
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
        
        //REPOSITIONING
        m_Reposition.Rearrange( (unsigned char*) mp_block_dest.get() , bytes_read , temp_block.get() ) ;
        
        // XOR3
        //TransformRandom( mp_block_random3.get() , m_n_quantum ) ;
        ( *mpShift )( mp_block_random3.get() ) ;
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_dest[i] ^ mp_block_random3[i] ;
        
        //OUT
        m_ofs.write( (char*) mp_block_dest.get() , m_block_size /*bytes_read*/ ) ;

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
    
            // time mesaure
            mcs2 = std::chrono::high_resolution_clock::now() ;
            mcs = mcs2 - mcs1 ;
            drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
    
}

void GammaCrypt::Decrypt()
{
    ReadHead() ;
    
    m_block_size = m_header.h_block_size ;
    mb_need_init_blocksize = false ;
    
    Initialize() ;
    m_Reposition.Init( m_block_size ) ;
    ReadOverheadData() ;
    
    DisplayInfo() ;
    
    // CRYPT  Crypt() ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_random3 != nullptr ) ;
        assert( mp_block_source != nullptr ) ;
        assert( mp_block_dest != nullptr ) ;

    auto temp_block = std::make_unique< unsigned char[] >( m_Reposition.m_BIarray.block_size_bytes ) ; // for Rearrange Matrix

    int64_t i = m_header.source_file_size ;
    while ( i > 0 )
    {
        // READING
        m_ifs.read( (char*) mp_block_source.get(), m_block_size ) ;
        unsigned bytes_read = m_ifs.gcount() ;
        if ( bytes_read == 0 )
            break ;
        
        // XOR3
        //TransformRandom( mp_block_random3.get() , m_n_quantum ) ;
        ( *mpShift )( mp_block_random3.get() ) ;
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_source[i] = mp_block_source[i] ^ mp_block_random3[i] ;
        
        //REPOSITIONING
        
        m_Reposition.Rearrange( ( unsigned char*) mp_block_source.get() , bytes_read , temp_block.get() ) ;
        // XOR1
        //TransformRandom( mp_block_random.get() , m_n_quantum ) ;
        ( *mpShift )( mp_block_random.get() ) ;
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
        
        //it should not be the tail now
        // checking tail size
        unsigned bytes_to_write ;
        i -= m_block_size ;
        if ( i >=0 )
            bytes_to_write = bytes_read ;
        else
        {
            //assert( false ) ;
            bytes_to_write = i + m_block_size ;
        }
        
/*        m_ifs.peek() ; 
        if ( ! m_ifs.good() ) // Проверяем конец файла
            bytes_to_write = m_header.tail_size ;
             */
        //OUT
        m_ofs.write( (char*) mp_block_dest.get() , bytes_to_write ) ;

    }
}

void GammaCrypt::ReadHead()
{
    //Header
    m_ifs.read( (char*) &m_header , sizeof( t_header ) ) ;
    if ( (unsigned) m_ifs.gcount() <  sizeof( t_header ) )
        throw ("error: File too short, missing header") ;
}

void GammaCrypt::ReadOverheadData()
{
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_random3 != nullptr ) ;
        assert( mp_block_password != nullptr ) ;
    //Block_key
    auto block_key = make_unique< t_block >( m_n_quantum );
    //Key1
    m_ifs.read( (char*) block_key.get() , m_block_size ) ;
    if ( (unsigned) m_ifs.gcount() <  m_block_size )
        throw ("error: File too short, missing key block") ;

    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        mp_block_random[i] = block_key[i] ^ mp_block_password[i] ;

    //Key3
    m_ifs.read( (char*) block_key.get() , m_block_size ) ;
    if ( (unsigned) m_ifs.gcount() <  m_block_size )
        throw ("error: File too short, missing key block") ;

    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        mp_block_random3[i] = block_key[i] ^ mp_block_password[i] ;

    // Matrix
    m_ifs.read( (char*) m_Reposition.m_BIarray.m_array.get() , m_Reposition.m_BIarray.matrix_size_bytes ) ;
    if ( (unsigned) m_ifs.gcount() <  m_Reposition.m_BIarray.matrix_size_bytes )
        throw ("error: File too short, missing matrix block") ;
        
    unsigned block_matrix_size_words = m_Reposition.m_BIarray.matrix_size_bytes / sizeof(unsigned) ;
    if ( m_Reposition.m_BIarray.matrix_size_bytes % sizeof( unsigned ) != 0  )
        block_matrix_size_words++ ;

    auto block_matrix_xored = make_unique< unsigned[] >( block_matrix_size_words ) ;
    Matrix_Xor_Password( (uint16_t *) m_Reposition.m_BIarray.m_array.get() , block_matrix_xored.get() ) ;
    memcpy( m_Reposition.m_BIarray.m_array.get() , block_matrix_xored.get() , m_Reposition.m_BIarray.matrix_size_bytes ) ;

    m_Reposition.InverseRearrangeMatrix() ;

} ;

void GammaCrypt::WriteHead()
{
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_random3 != nullptr ) ;
        assert( mp_block_password != nullptr ) ;
    
    //Header
    m_header.data_offset = sizeof( t_header ) ;
    m_header.h_block_size = m_block_size ;
    m_ofs.write( (char*) &m_header , sizeof( t_header ) ) ;
    
    //Block_key
    auto block_key = make_unique< t_block >( m_n_quantum );
    //Key1
    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        block_key[i] = mp_block_random[i] ^ mp_block_password[i] ;
    m_ofs.write( (char*) block_key.get() , m_block_size ) ;
    //Key3
    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        block_key[i] = mp_block_random3[i] ^ mp_block_password[i] ;
    m_ofs.write( (char*) block_key.get() , m_block_size ) ;
    
    //Reposition matrix
    // 1) XOR with password_block
    unsigned block_matrix_size_words = m_Reposition.m_BIarray.matrix_size_bytes / sizeof(unsigned) ;
    if ( m_Reposition.m_BIarray.matrix_size_bytes % sizeof( unsigned ) != 0  )
        block_matrix_size_words++ ;
    auto block_matrix_xored = make_unique< unsigned[] >( block_matrix_size_words ) ;
    Matrix_Xor_Password( ( uint16_t * ) m_Reposition.m_BIarray.m_array.get() , block_matrix_xored.get() ) ;
    // 2) write to file
    m_ofs.write( (char*) block_matrix_xored.get() , block_matrix_size_words * sizeof(unsigned) ) ;
}

void GammaCrypt::Matrix_Xor_Password( uint16_t * pmatrix_in , unsigned * pmatrix_out )
{
    // матрица должна быть выровняна по размеру блока, иначе block_password будет незаксоренный
    unsigned N_blocks = m_Reposition.m_BIarray.matrix_size_bytes / m_block_size ;
    if ( m_Reposition.m_BIarray.matrix_size_bytes % m_block_size != 0  )
        N_blocks++ ;
    const unsigned block_size_words = m_block_size / sizeof( unsigned ) ; // размер блока в словах unsigned

    for ( unsigned i = 0 ; i < N_blocks ; ++i )
    {
        for ( unsigned j = 0 ; j < block_size_words  ; j ++ )
        {
            const unsigned matrix2_index = ( i * block_size_words + j ) * 2 ; // *2 because repos matrix is uint16_t
            pmatrix_out[ i * block_size_words + j ] = mp_block_password[ j ] ^ 
                ( 
                  ( unsigned( pmatrix_in[ matrix2_index + 1]) << 16 ) | 
                  unsigned( pmatrix_in[ matrix2_index ] )
                )  ;
        }
    }
} ;

void GammaCrypt::Crypt()
{ ;
/*    
    // for debugging
    std::array<t_block, 10000> v ;
    size_t n = 0 ;
    bool b_found = false ;
*/    
} ;


void GammaCrypt::SetBlockSize( unsigned block_size )
{
    if ( block_size > 128 )
        block_size = 128 ;
    mb_need_init_blocksize = false ;
    unsigned temp = block_size ;
    unsigned log2 = 0 ; 
    while ( temp >>= 1 ) log2 ++ ;
    m_block_size = 1 ;
    for ( unsigned i = 0 ; i < log2 ; ++i )
        m_block_size <<= 1 ;
    if ( m_block_size < 8)
        m_block_size = 8 ;
}

void GammaCrypt::DisplayInfo()
{
    ostringstream oss ;
    oss << "\n source file size (bytes): " << m_header.source_file_size ;
    oss << "\n block size (bytes): " << m_block_size ;
    oss << "\n matrix size (bytes): " << m_Reposition.m_BIarray.matrix_size_bytes ;
    oss << "\n number of matrix elements: " << m_Reposition.m_BIarray.max_index ;
    oss << "\n size of one matrix element (bits): " << m_Reposition.m_BIarray.index_size_bits ;
    oss << "\n" ;
    display_str( oss.str() ) ;
}
