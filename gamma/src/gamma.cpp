#include <string>
#include <fstream>
#include <cassert>

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <memory>
#include <chrono>
#include <sstream>
#include <thread>
#include <atomic>
//#include <array> // for debug only
//#include <iostream>  // for debug only

#include "gamma.h"
#include "LFSR.h"
#include "../../Display/ConsoleDisplay/include/display.h"

#include "../platform.h"

using namespace std ;


GammaCrypt::GammaCrypt( istream & ifs , ostream & ofs ,  const string & password ) 
    : m_ifs( ifs ) , m_ofs( ofs ) , m_password( password ) 
    , mp_block_random( nullptr ) , mp_block_password( nullptr ) , mp_block_source( nullptr ) , mp_block_dest( nullptr )
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
        if ( m_header.source_file_size >= 131072 )     // 128K -
            m_block_size = 128 ;
        else if ( m_header.source_file_size >= 65536 ) // 64K - 128K
            m_block_size = 64 ;
        else if ( m_header.source_file_size >= 16384 ) // 16K - 64K
            m_block_size = 32 ;
        else if ( m_header.source_file_size >= 2048 )  // 2K - 16K
            m_block_size = 16 ;
        else                                           // 0 - 2K
            m_block_size = 8 ;        
    }

    // init mpShift
    if ( m_block_size == 128 ) // 64M -
        mpShift = Shift1024 ;
    else if ( m_block_size == 64 ) // 16M - 
        mpShift = Shift512 ;
    else if ( m_block_size == 32 ) // 1M - 16M
        mpShift = Shift256 ;
    else if ( m_block_size == 16 ) // 4K - 1M
        mpShift = Shift128 ;
    else        // 0 - 4K
        mpShift = Shift64 ;
    
    m_n_quantum = m_block_size / m_quantum_size ;
    
    // m_Permutate Initializing
    m_Permutate.Init( m_block_size ) ;

    // allocate memory
    mp_block_password  = make_unique< t_block >( m_n_quantum ) ;
    // obsolete: mp_block_random3   = make_unique< t_block >( m_n_quantum + 1 ) ; // obsolete: to transform Random block it is required one quantum more (therefore "+1")
    mp_block_source    = make_unique< t_block >( m_n_quantum ) ;
    mp_block_dest      = make_unique< t_block >( m_n_quantum ) ;

    // block random
    // N = m_n_quantum / 4 
    // offset in bytes:
    //    KEY1           KEY2                PMA1                             PMA2                         tail KEY
    // 0  ... N-1   N  ...  N*2-1   N*2...N*2+PMAsizeBytes-1   N*2+PMAsizeBytes...N*2+PMAsizeBytes*2-1    N*2+PMAsizeBytes*2 ... N*2+PMAsizeBytes*2+tailSize-1
    mp_block_random    = make_unique< t_block >( m_n_quantum * 3 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size ) ; 

    // m_Permutate Initializing
    m_Permutate.mpc_randoms = reinterpret_cast< unsigned char * >( mp_block_random.get() ) ;

    offs_key2 = m_block_size ;
    offs_pma1 = m_block_size * 2 ;
    offs_pma2 = m_block_size * 2 + m_Permutate.m_BIarray.pma_size_bytes ;
    offs_ktail = m_block_size * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 ;


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
        
    //
    unsigned tail_size_bytes = m_header.source_file_size % m_block_size ;
    unsigned tail_size_words = tail_size_bytes / sizeof( unsigned ) ;
    if ( tail_size_bytes % sizeof( unsigned ) != 0 )
        tail_size_words ++ ;
    

    display_str("Generating randoms...") ;
            // time mesaure
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //
    mGenerateRandoms() ;
    //GenerateRandoms( m_n_quantum * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + tail_size_words , mp_block_random.get()  ) ;
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
    
    //display_str("making permutation array...") ;
            // time mesaure
            //mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    // m_Permutate initializing is inside GammaCrypt::Initialize
    if ( ! mb_decrypting )
    {
        m_Permutate.MakePermutArr( m_Permutate.e_array.get() , m_Permutate.mpc_randoms + m_block_size * 2 , m_Permutate.m_BIarray ) ;
        m_Permutate.MakePermutArr( m_Permutate.e_array2.get() , m_Permutate.mpc_randoms + m_block_size * 2 + m_Permutate.m_BIarray.pma_size_bytes , m_Permutate.m_BIarray2 ) ;
    }
    

        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n pma1 \n " ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n " ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD

            // time mesaure
/*            mcs2 = std::chrono::high_resolution_clock::now() ;
            mcs = mcs2 - mcs1 ;
            drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
*/            //

    WriteHead() ; 
    DisplayInfo() ;

    //CRYPT ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_source != nullptr ) ;
        assert( mp_block_dest != nullptr ) ;

    display_str("crypting...") ;

        // time mesaure
        mcs1 = std::chrono::high_resolution_clock::now() ;
        //

    //auto temp_block = std::make_unique< unsigned char[] >( m_Permutate.m_BIarray.block_size_bytes ) ; // for PMA
    auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size ) ; // for ePMA
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2

    uint16_t bytes_read ;

    while ( ! m_ifs.eof() )
    {
        
        m_ifs.read( (char*) mp_block_source.get(), m_block_size ) ;
        bytes_read = m_ifs.gcount() ;

        if ( bytes_read < m_block_size )
        {
            assert( bytes_read == tail_size_bytes ) ;
            // тогда остаток блока надо добить randoms[m_n_quantum*2] .. randoms[m_n_quantum*2 + tail_size_bytes]
            memcpy( (char*) mp_block_source.get() + bytes_read, /*reinterpret_cast< unsigned char * >*/ (unsigned char *) ( mp_block_random.get() ) + offs_ktail , m_block_size - bytes_read ) ;
        }
        

        // ----------- XOR1 -----------

        #ifdef XOR_ENBLD
        
        #ifdef LFSR_ENBLD
        ( *mpShift )( mp_block_random.get() ) ;
        #endif // LFSR_ENBLD
        
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
            
        #else
        
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ;
            
        #endif  // XOR_ENBLD

        // ---------- REPOSITIONING ----------

        #ifdef PERMUTATE_ENBLD
        
        m_Permutate.eTransformPMA2() ;
        m_Permutate.eRearrangePMA1( e_temp_block_pma.get() , m_Permutate.e_array2.get() ) ;

/*            #ifdef DBG_INFO_ENBLD
            // cout pma
            std::cout << "\n rearranged pma1 \n " ;
            for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
                std::cout << m_Permutate.e_array[ i ] << ' ' ;
            std::cout << std::endl ;
            #endif // DBG_INFO_ENBLD
*/
        m_Permutate.eRearrange( (unsigned char*) mp_block_dest.get() , e_temp_block.get() , m_Permutate.e_array.get() ) ;

        #endif // PERMUTATE_ENBLD
        
        // ----------- XOR2 ------------

        #ifdef XOR_ENBLD

        #ifdef LFSR_ENBLD
        ( *mpShift )( mp_block_random.get() + offs_key2 / m_quantum_size ) ;
        #endif // LFSR_ENBLD

        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_dest[i] ^ mp_block_random[i + offs_key2 / m_quantum_size ] ;
        
        #endif // XOR_ENBLD

        // ------------ OUT ------------

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
        drtn = mcs.count() * 1'000'000 ;
        std::cout << "Duration: " << drtn << std::endl ;
        //
    
}

void GammaCrypt::Decrypt()
{
    ReadHead() ;
    
    m_block_size = m_header.h_block_size ;
    mb_decrypting = true ;
    mb_need_init_blocksize = false ;
    
    Initialize() ;
    //m_Permutate.Init( m_block_size , ) ;
    ReadOverheadData() ;
    
        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD

    DisplayInfo() ;
    
    // CRYPT  Crypt() ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_source != nullptr ) ;
        assert( mp_block_dest != nullptr ) ;

    auto temp_block = std::make_unique< unsigned char[] >( m_block_size ) ; // for PMA
    //auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size ) ; // for ePMA
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
    
    auto t_invs_pma1 = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ;

            // time mesaure
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    int64_t i = m_header.source_file_size ;
    while ( i > 0 )
    {
        // READING
        m_ifs.read( (char*) mp_block_source.get(), m_block_size ) ;
        unsigned bytes_read = m_ifs.gcount() ;
        if ( bytes_read == 0 )
            break ;
        
        #ifdef XOR_ENBLD
        // XOR2
        
        #ifdef LFSR_ENBLD
        ( *mpShift )( mp_block_random.get() + m_n_quantum ) ;
        #endif
        
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_source[i] = mp_block_source[i] ^ mp_block_random[ i + offs_key2 / m_quantum_size ] ;
        
        #endif // XOR_ENBLD

        #ifdef PERMUTATE_ENBLD
        //REPOSITIONING
        
        // transform pma2
        m_Permutate.eTransformPMA2() ;

        //rearrange pma1
        m_Permutate.eRearrangePMA1( e_temp_block_pma.get() , m_Permutate.e_array2.get() ) ;
/*            #ifdef DBG_INFO_ENBLD
            // cout pma
            std::cout << "\n rearranged pma1 \n" ;
            for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
                std::cout << m_Permutate.e_array[ i ] << ' ' ;
            std::cout << std::endl ;
            #endif // DBG_INFO_ENBLD
*/
        //inverse pma1
        m_Permutate.InverseExpPermutArr( t_invs_pma1.get() , m_Permutate.e_array.get() ) ;
/*            #ifdef DBG_INFO_ENBLD
            // cout pma
            std::cout << "\n invrs pma1 \n" ;
            for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
                std::cout << t_invs_pma1[ i ] << ' ' ;
            std::cout << std::endl ;
            #endif // DBG_INFO_ENBLD
*/
        m_Permutate.eRearrange( ( unsigned char*) mp_block_source.get() , temp_block.get() , t_invs_pma1.get() ) ;

        #endif // PERMUTATE_ENBLD
        
        #ifdef XOR_ENBLD
        // XOR1

        #ifdef LFSR_ENBLD
        ( *mpShift )( mp_block_random.get() ) ;
        #endif // LFSR_ENBLD

        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
            
        #else
        for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
            mp_block_dest[i] = mp_block_source[i] ;
        #endif // XOR_ENBLD
        
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
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
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
        assert( mp_block_password != nullptr ) ;
    //Block_key
    auto block_key = make_unique< t_block >( m_n_quantum );
    //Key1
    m_ifs.read( (char*) block_key.get() , m_block_size ) ;
    if ( (unsigned) m_ifs.gcount() <  m_block_size )
        throw ("error: File too short, missing key block") ;

    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        mp_block_random[i] = block_key[i] ^ mp_block_password[i] ;

    //Key2
    m_ifs.read( (char*) block_key.get() , m_block_size ) ;
    if ( (unsigned) m_ifs.gcount() <  m_block_size )
        throw ("error: File too short, missing key2 block") ;

    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        mp_block_random[ i + offs_key2 / m_quantum_size ] = block_key[i] ^ mp_block_password[i] ;

    // Permutation Array 1
    m_ifs.read( (char*) m_Permutate.m_BIarray.m_array.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;
    if ( (unsigned) m_ifs.gcount() <  m_Permutate.m_BIarray.pma_size_bytes )
        throw ("error: File too short, missing permutation array block") ;
        
    unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / sizeof(unsigned) ;
    if ( m_Permutate.m_BIarray.pma_size_bytes % sizeof( unsigned ) != 0  )
        block_pma_size_words++ ;

    auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Password( (uint16_t *) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
    memcpy( m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;

    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray ) ;
    
    // build e_array, aka Expanded in the memory
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray.max_index ; ++i )
    {
        m_Permutate.e_array.get()[ i ] = m_Permutate.m_BIarray[ i ] ;
    }


    // Permutation Array 2
    m_ifs.read( (char*) m_Permutate.m_BIarray2.m_array.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;
    if ( (unsigned) m_ifs.gcount() <  m_Permutate.m_BIarray2.pma_size_bytes )
        throw ("error: File too short, missing permutation array block") ;
        
    block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / sizeof(unsigned) ;
    if ( m_Permutate.m_BIarray2.pma_size_bytes % sizeof( unsigned ) != 0  )
        block_pma_size_words++ ;

    //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Password( (uint16_t *) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
    memcpy( m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;

    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray2 ) ;
    
    // build e_array, aka Expanded in the memory
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray2.max_index ; ++i )
    {
        m_Permutate.e_array2.get()[ i ] = m_Permutate.m_BIarray2[ i ] ; // todo get out get()
    }

} ;

void GammaCrypt::WriteHead()
{
        assert( mp_block_random != nullptr ) ;
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
    //Key2
    for ( unsigned i = 0 ; i < m_n_quantum ; i++ )
        block_key[i] = mp_block_random[ i + offs_key2 / m_quantum_size ] ^ mp_block_password[i] ;
    m_ofs.write( (char*) block_key.get() , m_block_size ) ;

    
    //Permutation array 1
    // 1) XOR with password_block
    unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ;
    if ( m_Permutate.m_BIarray.pma_size_bytes % m_quantum_size != 0 )
        block_pma_size_words++ ;
    auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Password( ( uint16_t * ) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
    // 2) write to file
    m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
    
    //Permutation array 2
    // 1) XOR with password_block
    block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / m_quantum_size ;
    if ( m_Permutate.m_BIarray2.pma_size_bytes % m_quantum_size != 0 )
        block_pma_size_words++ ;
    //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Password( ( uint16_t * ) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
    // 2) write to file
    m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
}

void GammaCrypt::PMA_Xor_Password( uint16_t * p_pma_in , unsigned * p_pma_out )
{
    // массив перестановок должен быть выровнен по размеру блока, иначе block_password будет незаксоренный
    unsigned N_blocks = m_Permutate.m_BIarray.pma_size_bytes / m_block_size ;
    if ( m_Permutate.m_BIarray.pma_size_bytes % m_block_size != 0  )
        N_blocks++ ;
    const unsigned block_size_words = m_block_size / m_quantum_size ; // размер блока в словах unsigned

    for ( unsigned i = 0 ; i < N_blocks ; ++i )
    {
        for ( unsigned j = 0 ; j < block_size_words  ; j ++ )
        {
            const unsigned pmt2_index = ( i * block_size_words + j ) * 2 ; // *2 because permutation array is uint16_t
            p_pma_out[ i * block_size_words + j ] = mp_block_password[ j ] ^ 
                ( 
                  ( unsigned( p_pma_in[ pmt2_index + 1]) << 16 ) | 
                  unsigned( p_pma_in[ pmt2_index ] )
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


void GammaCrypt::mGenerateRandoms()
{
    std::atomic< bool > bstop ( false ) ;
    //timer
    std::atomic< uint64_t > ticks( 0 ) ;

    std::thread thr_tick( TickTimer , std::ref( ticks ) , std::ref( bstop ) ) ;
    //thr_tick.detach() ;
    
    unsigned hardware_concurrency = std::thread::hardware_concurrency() ;
    if ( hardware_concurrency == 0 )
        hardware_concurrency = 1 ;
        
    std::cout << "hardware_concurrency: " << hardware_concurrency << std::endl ;
    
    unsigned tail_size_bytes = m_header.source_file_size % m_block_size ;
    unsigned tail_size_words = tail_size_bytes / sizeof( unsigned ) ;
    if ( tail_size_bytes % sizeof( unsigned ) != 0 )
        tail_size_words ++ ;

    std::vector<std::thread> threads;
    for( unsigned i = 0; i < hardware_concurrency - 1 ; i++ ) 
    {
        std::thread thr_cfl( CPUFullLoad , std::ref( bstop ) ) ; // thread CPU Loading at 100%
        threads.emplace_back( std::move( thr_cfl ) ) ;
    }

    
    //std::thread thr_GenerateRandoms( GenerateRandoms , m_n_quantum * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + tail_size_words , mp_block_random.get()  ) ;
    GenerateRandoms( m_n_quantum * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + tail_size_words , mp_block_random.get() , std::ref( ticks ) ) ;
    
    bstop = true ;

    thr_tick.join() ;

    for ( auto & thr : threads ) 
    {
        thr.join();
    }

}

void GammaCrypt::DisplayInfo()
{
    ostringstream oss ;
    oss << "\n source file size (bytes): " << m_header.source_file_size ;
    oss << "\n block size (bytes): " << m_block_size ;
    oss << "\n permutation array size (bytes): " << m_Permutate.m_BIarray.pma_size_bytes ;
    oss << "\n number of permutation array elements: " << m_Permutate.m_BIarray.max_index ;
    oss << "\n size of one permutation array element (bits): " << m_Permutate.m_BIarray.index_size_bits ;
    oss << "\n" ;
    display_str( oss.str() ) ;
}
