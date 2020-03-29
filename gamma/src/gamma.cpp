#include <string>
#include <fstream>
#include <cassert>
#include "check.h"

#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <memory>
#include <chrono>
#include <sstream>
#include <thread>
#include <atomic>
#include <random>
#include <functional>
#include <algorithm>
#include <fstream>
//#include <array> // for debug only
//#include <iostream>  // for debug only

#include "gamma.h"
#include "LFSR.h"
#include "../../Display/ConsoleDisplay/include/display.h"

#include "../platform.h"


std::mutex g_lockprint; // dbg for debug


void Threading::thread_proc( int nthr ) 
{
	while ( true )
	{
		std::shared_lock< std::shared_timed_mutex > locker( m_mutex ) ;

		// проверка прохода:
		if ( m_pass_processed[ m_n_pass_thr ] )
		{
			std::cout << " \n Fatal error m_pass_processed in thread, nItThr: " << mnItTh << "\n" ;
			return ;
		}
		
		// Выполнение:
		mpGC->EncryptBlockOneThr( nthr , m_n_pass_thr ) ;
//#ifndef PERMUTATE_ENBLD
		//std::this_thread::sleep_for(std::chrono::milliseconds(5)); // todo remove
//#endif


		locker.unlock() ;
		
		// дождёмся завершения итерации главного потока:
		locker.lock() ;
		while ( ! m_thr_ntfd[ nthr ] )
			m_notifier.wait( locker ) ;
		m_thr_ntfd[ nthr ] = false;
		locker.unlock() ;
		
		if ( m_bexit )
			break ;
	}
}


void Threading::Process( GammaCryptImpl * pGC , const unsigned Nthr )
{
	mpGC = pGC ;
	m_Nthr = Nthr ;
	
	for ( unsigned i = 0 ; i < m_Nthr ; ++i )
		m_thr_ntfd.push_back( false ) ;
	
	m_pass_processed[ 0 ] = true ;
	m_pass_processed[ 1 ] = true ;
	m_n_pass_thr =  0 ;

	std::unique_lock< std::shared_timed_mutex > locker( m_mutex ) ;
	
    std::vector< std::thread > threads;
    for( unsigned i = 0; i < m_Nthr ; i++) 
	{
        std::thread thr( &Threading::thread_proc, this , i);
        threads.emplace_back(std::move(thr));
    }

	unsigned n_pass = 0 ; // номер прохода 0/1
	
	// Выполнение: Предварительный рассчёт ключей
	mpGC->PreCalc( n_pass ) ;

	m_pass_processed[ n_pass ] = false ;
	n_pass = 1 ;

	mnItTh = 0 ;

	unsigned bytes_to_write = 0 ;
	
	// Читаем файл:
	mpGC->m_ifs.read(  mpGC->mpbuffer , mpGC->mbuf_size ) ;
	unsigned bytes_read = mpGC->m_ifs.gcount() ;
	bytes_to_write = bytes_read ;
	unsigned tail_size_read ;
	uint64_t bytes_left = mpGC->m_header.source_file_size - mpGC->m_ifs.tellg() ;

   //    main cycle
    while ( ! mpGC->m_ifs.eof() )
    {

		// Запуск потоков:
		locker.unlock() ;
		// проверка прохода:
		if ( ! m_pass_processed[ n_pass ] )
		{
			display_err( " \n Fatal error m_pass_processed \n" );
			return ;
		}
		m_pass_processed[ n_pass ] = false ;
		
//--------------Выполнение:-------------------------------------------------------------------------------
		
		if ( bytes_read < mpGC->mbuf_size )
		{
            assert( false ) ;
			tail_size_read = bytes_read % mpGC->m_block_size_bytes ;
			bytes_to_write = bytes_read - tail_size_read + mpGC->m_block_size_bytes ;

			if ( tail_size_read != 0 )
			{
				//assert( tail_size_read == tail_size_bytes ) ;
				//mNblocks++ ;
				// тогда остаток блока надо добить randoms[m_blk_sz_words*2] .. randoms[m_blk_sz_words*2 + tail_size_bytes]
				memcpy( (char*) mpGC->params.p_source + tail_size_read, /*reinterpret_cast< unsigned char * >*/ reinterpret_cast< unsigned char * >( mpGC->m32_block_random ) + mpGC->offs_ktail , mpGC->m_block_size_bytes - tail_size_read ) ;
			}
			
		}
        
        // calculate all key1, key2 , pma1 , pma2
		mpGC->PreCalc( n_pass ) ;

		// а это уже не сюда:
        //mpGC->EncryptBlockMT( mce_temp_block  ) ;
        

//--------------КонецВыполнения---------------------------------------------------------------------------
		
		//Дождёмся завершения всех потоков:
		locker.lock() ;

        // ------------ OUT ------------
        mpGC->m_ofs.write( (char*) mpGC->mpbuffout , bytes_to_write ) ;
		
		// только один поток должен менять эти значения:
		m_pass_processed[ m_n_pass_thr ] = true ;
		m_n_pass_thr = ( m_n_pass_thr == 0 ) ? 1 : 0 ;
		++ mnItTh ;
		// сбросим счётчик для потоков:
		//counter = 0 ;

            // Читаем файл: -------- IN --------
        mpGC->m_ifs.read(  mpGC->mpbuffer , mpGC->mbuf_size ) ;
        bytes_read = mpGC->m_ifs.gcount() ;
        bytes_to_write = bytes_read ;
        bytes_left = mpGC->m_header.source_file_size - mpGC->m_ifs.tellg() ;
			
		// оповестим потоки, что готовы выполняться дальше, пусть ждут unlock()
		for ( unsigned i = 0 ; i < m_Nthr ; ++i )
			m_thr_ntfd[ i ] = true ;
		m_notifier.notify_all();
			
		n_pass = ( n_pass == 0 )  ? 1 : 0 ;
		
		if ( bytes_left < mpGC->mbuf_size )
		{
			// Завершаем все потоки, продолжаем однопоточно
			mpGC->mbuf_size = mpGC->m_block_size_bytes ;
			mpGC->mb_multithread = false ;
			break ;
		}

    } // end of main cycle
	
	// Запуск потоков:
	locker.unlock() ;

	//Дадим возможность потокам запуститься
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

			
	//Дождёмся завершения всех потоков:
	locker.lock() ;
	// только один поток должен менять эти значения:
	m_pass_processed[ m_n_pass_thr ] = true ;
	m_n_pass_thr = ( m_n_pass_thr == 0 ) ? 1 : 0 ;
	++ mnItTh ;

	// оповестим потоки, что готовы выполняться дальше, пусть ждут unlock()
	for ( unsigned i = 0 ; i < m_Nthr ; ++i )
		m_thr_ntfd[ i ] = true ;
	m_notifier.notify_all();

	// Запуск потоков:
	locker.unlock() ;
	
	m_bexit = true ;

	// разморозим потоки:
	for ( unsigned i = 0 ; i < m_Nthr ; ++i )
		m_thr_ntfd[ i ] = true ;
	m_notifier.notify_all();
		
    for(auto& thr : threads) 
	{
        thr.join();
    }

    // ------------ OUT ------------

    mpGC->m_ofs.write( (char*) mpGC->mpbuffout , bytes_to_write ) ;
		
}


GammaCryptImpl::GammaCryptImpl( std::istream & ifs , std::ostream & ofs ,  const std::string & password ) 
    : m_ifs( ifs ) , m_ofs( ofs ) , mu32_block_random( nullptr ) ,  m_password( password )  
    , mu32_block_password( nullptr ) , mu32_block_source( nullptr ) , mu32_block_dest( nullptr )
	
{
	m_Threading.mpGC = this  ;
}

void GammaCryptImpl::Initialize()
{
    m_hrdw_concr = std::thread::hardware_concurrency() ;
    if ( m_hrdw_concr == 0 )
        m_hrdw_concr = 1 ;
//    else if ( m_hrdw_concr > 3 )
//        m_hrdw_concr = 3 ;
    
    //m_hrdw_concr = 1 ; // todo FOR DEBUG!!! REMOVE IT
    
    // calculate our 'constants'
    
//    if ( mb_need_init_blocksize )
//    {
//        if ( m_header.source_file_size >= 131072 )     // 128K -
//            m_block_size_bytes = 128 ;
//        else if ( m_header.source_file_size >= 65536 ) // 64K - 128K
//            m_block_size_bytes = 64 ;
//        else if ( m_header.source_file_size >= 16384 ) // 16K - 64K
//            m_block_size_bytes = 32 ;
//        else if ( m_header.source_file_size >= 2048 )  // 2K - 16K
//            m_block_size_bytes = 16 ;
//        else                                           // 0 - 2K
//            m_block_size_bytes = 8 ;        
//    }

    if ( mb_need_init_blocksize )
    {
        if ( m_header.source_file_size >= 65536 )     // 64 -
            m_block_size_bytes = 128 ;
        else  // 0 - 64K
            m_block_size_bytes = 64 ;
    }

    m_blk_sz_words = m_block_size_bytes / m_quantum_size ;
    params.blk_sz_bytes = m_block_size_bytes ;

    // init mpShift
    if ( m_block_size_bytes == 128 ) // 64M -
        mpShift = Shift1024 ;
    else if ( m_block_size_bytes == 64 ) // 16M - 
        mpShift = Shift512 ;
    else if ( m_block_size_bytes == 32 ) // 1M - 16M
        mpShift = Shift256 ;
    else if ( m_block_size_bytes == 16 ) // 4K - 1M
        mpShift = Shift128 ;
    else //   m_block_size_bytes == 8     // 0 - 4K
        mpShift = Shift64 ;
    
    // m_Permutate Initializing
    m_Permutate.Init( m_block_size_bytes , m_perm_bytes ) ;

    // allocate memory
    m8_block_psw_pma = std::make_unique< unsigned char[] >( m_Permutate.m_BIarray.pma_size_bytes ) ;

    // allocate memory
    mu32_block_password  = std::make_unique< t_block >( m_blk_sz_words ) ;
    m32_block_password = mu32_block_password.get() ;
    // obsolete: mp_block_random3   = make_unique< t_block >( m_blk_sz_words + 1 ) ; // obsolete: to transform Random block it is required one quantum more (therefore "+1")
    mu32_block_source    = std::make_unique< t_block >( m_blk_sz_words ) ;
    m32_block_source = mu32_block_source.get() ;
    mu32_block_dest      = std::make_unique< t_block >( m_blk_sz_words ) ;
    m32_block_dest = mu32_block_dest.get() ;

    // block random
    // N = m_blk_sz_words / 4 
    // offset in bytes:
    //    KEY1           KEY2                PMA1                             PMA2                         tail KEY
    // 0  ... N-1   N  ...  N*2-1   N*2...N*2+PMAsizeBytes-1   N*2+PMAsizeBytes...N*2+PMAsizeBytes*2-1    N*2+PMAsizeBytes*2 ... N*2+PMAsizeBytes*2+tailSize-1

    m_tail_size_bytes = m_header.source_file_size % m_block_size_bytes ;
    m_tail_size_words = m_tail_size_bytes / sizeof( unsigned ) ;
    if ( m_tail_size_bytes % sizeof( unsigned ) != 0 )
        m_tail_size_words ++ ;

    m_rnd_size_words = m_blk_sz_words * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + m_blk_sz_words - m_tail_size_words + 1 ;

    mu32_block_random = std::make_unique< t_block >( m_rnd_size_words ) ;
    m32_block_random = mu32_block_random.get() ;

    // m_Permutate Initializing (continue)
    m_Permutate.mp8_randoms = reinterpret_cast< unsigned char * >( m32_block_random ) ;

    offs_key2 = m_block_size_bytes ;
    offs_pma1 = m_block_size_bytes * 2 ;
    offs_pma2 = m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes ;
    offs_ktail = m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 ;

    m_Threading.m_Nthr = 1 ;

	uint64_t min_multithread_filesize = m_blks_per_thr * m_block_size_bytes * m_hrdw_concr * 2 ; // todo check(debug) it (near bound)
	if ( m_hrdw_concr > 1 && m_header.source_file_size < min_multithread_filesize )
	{
		m_hrdw_concr = 1 ;
	}
    if ( m_hrdw_concr > 1 ) 
    {
        mb_multithread = true ;
        m_Threading.m_Nthr = m_hrdw_concr - 1 ; // кол-во фоновых потоков (не считая основного)
    }

}

void GammaCryptImpl::MakePswBlock()
{
    // password block initialization
        // expand password to block_size
    unsigned i = 0 ;
    unsigned psw_size = m_password.size() ;
    //trunc psw_size to block_size
    if ( psw_size > m_block_size_bytes )
        psw_size = m_block_size_bytes ;
        
    unsigned bytes_to_copy ;
    while ( i < m_block_size_bytes )
    {
        bytes_to_copy =  i + psw_size  <= m_block_size_bytes  ?  psw_size : m_block_size_bytes - i ;
        memcpy( reinterpret_cast<char*>( m32_block_password ) + i , m_password.c_str(), bytes_to_copy ) ;
        i += bytes_to_copy ;
    }
    assert( i == m_block_size_bytes ) ;
            // time mesaure
            std::cout << "\n making psw_block... \n" ;
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    //return ;
    // transform psw block in cycle
    // This results in a delay that is usefull against 'dictionary' atack
    
    const unsigned N = 3091 ; // must be odd - нечётное кол-во
    const unsigned Neven = N + 1 ; // четное кол-во

    // 1.
    for ( unsigned i = 0 ; i < N ; ++i )
        ( *mpShift )( m32_block_password ) ;

    const unsigned block_size_bytes = m_block_size_bytes ;
    Permutate Pmt ;
    Pmt.Init( block_size_bytes , false ) ;

    const unsigned & pma_size_bytes = Pmt.m_BIarray.pma_size_bytes ; 

    auto up_block_rnd = std::make_unique< unsigned char[] >( pma_size_bytes ) ;

    // 2. psw_block_rnd1
    for ( unsigned i = 0 ; i < pma_size_bytes ; i += block_size_bytes )
    {
        for ( unsigned j = 0 ; j < N ; ++j )
            ( *mpShift )( m32_block_password ) ;

        bytes_to_copy = ( i + block_size_bytes > pma_size_bytes ) ? (i + block_size_bytes) - pma_size_bytes  : block_size_bytes ;
        std::memcpy( up_block_rnd.get() + i , m32_block_password , block_size_bytes ) ;
    }

    auto up_block_rnd2 = std::make_unique< unsigned char[] >( pma_size_bytes ) ;

    // 3. psw_blocl_rnd2
    for ( unsigned i = 0 ; i < pma_size_bytes ; i += block_size_bytes )
    {
        for ( unsigned j = 0 ; j < N ; ++j )
            ( *mpShift )( m32_block_password ) ;
        bytes_to_copy = ( i + block_size_bytes > pma_size_bytes ) ? (i + block_size_bytes) - pma_size_bytes  : block_size_bytes ;

        std::memcpy( up_block_rnd2.get() + i , m32_block_password , block_size_bytes ) ;
    }

    // 4. PMA2
    Pmt.mp8_randoms = up_block_rnd2.get() ;
    Pmt.MakePermutArr( Pmt.m16e_arr2 , Pmt.mp8_randoms , Pmt.m_BIarray2 ) ; //
    
    // 5. PMA1
    Pmt.mp8_randoms = up_block_rnd.get() ;
    Pmt.MakePermutArr( Pmt.m16e_arr , Pmt.mp8_randoms , Pmt.m_BIarray ) ; //
    
    auto temp_block = std::make_unique< unsigned char[] >( block_size_bytes ) ; 
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( Pmt.epma_size_elms ) ; // for ePMA2


    //gamma crypt (for crypt, not for MakePswBlock) pma_size_bytes :
    const unsigned & GC_pma_size_bytes = m_Permutate.m_BIarray.pma_size_bytes ;
    // при perm_bytes (перестановка байтов, а не битов pma_size_bytes < block_size_bytes ) , поэтому
    //приращение в циклах (чтобы доходить до конца блока)
    unsigned increment =  GC_pma_size_bytes < block_size_bytes ? GC_pma_size_bytes : block_size_bytes ;

    if ( GC_pma_size_bytes > block_size_bytes )
        assert( GC_pma_size_bytes % block_size_bytes == 0 ) ;
    // 6. GammaCryptImpl::m8_block_psw_pma
    for ( unsigned i = 0 ; i < GC_pma_size_bytes  ; i += increment )
    {
        unsigned op = 0 ; 
        for ( unsigned j = 0 ; j < Neven ; ++ j )
        {
            ( *mpShift )( m32_block_password ) ;


            eTransformPMA2( Pmt.m16e_arr2 , Pmt.epma_size_elms , op ) ;
            Pmt.eRearrangePMA1( e_temp_block_pma.get() , Pmt.m16e_arr2 ) ;

            Pmt.eRearrange( ( ( unsigned char*) m32_block_password ) , temp_block.get() , Pmt.m16e_arr , Pmt.epma_size_elms , block_size_bytes , false ) ;
            
        }
        memcpy( m8_block_psw_pma.get() + i , ( unsigned char*) m32_block_password , increment ) ;
    }
    
    // 7. GammaCryptImpl::m32_block_password
    unsigned op = 0 ;
    for ( unsigned i = 0 ; i < N ; ++ i )
    {
        ( *mpShift )( m32_block_password ) ;


        eTransformPMA2( Pmt.m16e_arr2 , Pmt.epma_size_elms , op ) ;
        Pmt.eRearrangePMA1( e_temp_block_pma.get() , Pmt.m16e_arr2 ) ;

        Pmt.eRearrange( ( ( unsigned char*) m32_block_password ) , temp_block.get() , Pmt.m16e_arr , Pmt.epma_size_elms , block_size_bytes , false ) ;
        
    }
    
    
   
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
    
}


inline void GammaCryptImpl::MakeDiffusion( unsigned * p32src) noexcept
{

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

	
/*    for ( unsigned j = 0 ; j <= 9 ; j++ )
    {
        //arithmetic manipulations
        for ( unsigned i = 1 ; i < m_blk_sz_words ; i++ )
        {
            p32src[ i ] += p32src[ i - 1 ] ;
        }
        p32src[ 0 ] += p32src[ m_blk_sz_words - 1 ] ;

        // left circular shift
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 32 bit rolls
            if ( j < 4 )
                p32src[i] = ( p32src[i] << 1 ) | ( p32src[i] >> 31 ) ; // 6 rolls
            else if ( j == 4 )
                p32src[i] = ( p32src[i] << 4 ) | ( p32src[i] >> 28 ) ; // 4 rolls
            else if ( j == 5 )
                p32src[i] = ( p32src[i] << 2 ) | ( p32src[i] >> 30 ) ; // 2 rolls
            else if ( j == 7 )
                p32src[i] = ( p32src[i] << 5 ) | ( p32src[i] >> 27 ) ; // 5 rolls
            else if ( j == 8 &&  ( i % 2 == 0 ) )
                p32src[i] = ( p32src[i] << 16 ) | ( p32src[i] >> 16 ) ; // 15 rolls
    }
*/
    
}

void GammaCryptImpl::EncryptBlock() noexcept
{

    // ----------- XOR1 -----------

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_source[ i ] ^ mp32keys1[ i ] ;
    #else
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_source[i] ;
    #endif

    #ifdef LFSR_ENBLD
    ( *mpShift )( mp32keys1 ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( mp32keys1 ) ; // this is CONFUSION
        MakeDiffusion( params.p_dest ) ; 
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_dest[ i ] ^ mp32keys1[ i ] ;
    #endif  // XOR_ENBLD

    // ---------- REPOSITIONING ----------

    #ifdef PERMUTATE_ENBLD
        eTransformPMA2( mp16pma2 , m_Permutate.epma_size_elms , m_op ) ;

        #ifdef DEBUG
            CheckPermutArr( mp16pma2 , m_Permutate.epma_size_elms ) ;
        #endif

        m_Permutate.eRearrangePMA1( m16e_temp_block_pma , mp16pma2 ) ;
        #ifdef DEBUG
            CheckPermutArr( m_Permutate.m16e_arr , m_Permutate.epma_size_elms ) ;
        #endif


        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n EncryptBlock : pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << mp16pma1[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << mp16pma2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD


        m_Permutate.eRearrange( (unsigned char*) params.p_dest , m8_temp_block , mp16pma1 , m_Permutate.epma_size_elms , m_block_size_bytes , m_perm_bytes ) ;
    #endif // PERMUTATE_ENBLD
    
    // ----------- XOR2 ------------

// todo1 uncomment it
    #ifdef LFSR_ENBLD
    ( *mpShift )( mp32keys2 ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( params.p_dest ) ; 
        MakeDiffusion( mp32keys2 ) ; // this is CONFUSION
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[ i ] ^= mp32keys2[ i ] ;
    #endif // XOR_ENBLD

}

// nthr - thread number
void GammaCryptImpl::EncryptBlockOneThr( unsigned nthr , unsigned n_pass_thr  ) noexcept
{

    for ( unsigned j = 0 ; j < m_blks_per_thr ; ++ j )
    {
        //uidx idx type: unsigneds
        unsigned uidx = nthr * m_blks_per_thr * m_blk_sz_words + j * m_blk_sz_words ;
        // ----------- XOR1 -----------
        #ifdef XOR_ENBLD
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_source[ uidx + i ] 
                                             ^ mp32keys1[ n_pass_thr * mNblocks * m_blk_sz_words + uidx + i]  ;
		#else
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_source[ uidx + i ] ;
        #endif

        #ifdef DIFFUSION_ENBLD
            //MakeDiffusion( mp32keys1 + n_pass_thr * mNblocks * m_blk_sz_words + uidx ) ; // this is CONFUSION
            MakeDiffusion( params.p_dest + uidx ) ;
        #endif
        
        #ifdef XOR_ENBLD
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_dest[ uidx + i ] 
                                             ^ mp32keys1[ n_pass_thr * mNblocks * m_blk_sz_words + m_blk_sz_words + uidx + i]  ;
		#endif // XOR_ENBLD


        // ---------- REPOSITIONING ----------

        #ifdef PERMUTATE_ENBLD
            m_Permutate.eRearrange( reinterpret_cast< unsigned char * >( params.p_dest + uidx )  
                    , m8_temp_block + nthr * m_block_size_bytes
                    , mp16pma1 + n_pass_thr * mNblocks * m_Permutate.epma_size_elms + nthr * m_blks_per_thr * params.epma_sz_elmnts + j * params.epma_sz_elmnts 
                    , params.epma_sz_elmnts , m_blk_sz_words * sizeof( unsigned) , m_perm_bytes ) ;

//                // cout pma
//            {
//                std::unique_lock<std::mutex> locker(g_lockprint);
//                m_dbg_ofs2 << "\n pma1  thread: " << nthr << " thr_n_pass: " << n_pass_thr << "\n" ;
//                for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//                    m_dbg_ofs2 << *(i + mp16pma1 + n_pass_thr * mNblocks * m_Permutate.epma_size_elms
//                                 + nthr * m_blks_per_thr * params.epma_sz_elmnts + j * params.epma_sz_elmnts)
//                        << ' ' ;
//                m_dbg_ofs2 << std::endl ;
//            }
//    
		#endif // PERMUTATE_ENBLD

        // ----------- XOR2 -----------
        #ifdef DIFFUSION_ENBLD
            MakeDiffusion( params.p_dest + uidx ) ; 
        #endif
        #ifdef XOR_ENBLD
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] ^=  mp32keys2[ n_pass_thr * mNblocks * m_blk_sz_words + uidx + i]  ;
		#endif // XOR_ENBLD
    }

}


void GammaCryptImpl::PreCalc( const unsigned n_pass )
{
            for ( unsigned i = 0 ; i < mNblocks ; ++i )
            {
                // ----------- KEY1 -----------
                #ifdef XOR_ENBLD
                    if ( n_pass == 0 && i == 0 )
                        memcpy( mp32keys1 , m32_block_random , m_block_size_bytes ) ;
                #endif

                #ifdef LFSR_ENBLD
                    ( *mpShift )( m32_block_random ) ;
                #endif

                #ifdef DIFFUSION_ENBLD
                    MakeDiffusion( m32_block_random ) ; // this is CONFUSION
                #endif

                #ifdef XOR_ENBLD
                    memcpy( mp32keys1 + n_pass * mNblocks * m_blk_sz_words + (i + 1) * m_blk_sz_words , m32_block_random , m_block_size_bytes ) ;
                #endif
            
                // ---------- PMA2, PMA1 ----------
                #ifdef PERMUTATE_ENBLD
                    eTransformPMA2( m_Permutate.m16e_arr2 , m_Permutate.epma_size_elms , m_op ) ;
                    memcpy( mp16pma2 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms , m_Permutate.m16e_arr2 , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                    m_Permutate.eRearrangePMA1( m16e_temp_block_pma , m_Permutate.m16e_arr2 ) ;
                    memcpy( mp16pma1 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms , m_Permutate.m16e_arr , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                #endif
//				// cout pma
//				//m_dbg_ofs1 << "\n pma1 n_pass: " << n_pass << "\n" ;
//                m_dbg_ofs1 << "\n pma1  _________" << " ____n_pass: " << n_pass << "\n" ;
//				for ( unsigned j = 0 ; j < m_Permutate.epma_size_elms ; j ++)
//					m_dbg_ofs1 << *( j + mp16pma1 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms)
//						<< ' ' ;
//				m_dbg_ofs1 << std::endl;

                
                // ----------- KEY2 ------------

                #ifdef LFSR_ENBLD
                    ( *mpShift )( m32_block_random + offs_key2 / m_quantum_size ) ;
                #endif

                #ifdef DIFFUSION_ENBLD
                    MakeDiffusion( m32_block_random + offs_key2 / m_quantum_size ) ; // this is CONFUSION
                #endif
                
                #ifdef XOR_ENBLD
                    memcpy( mp32keys2 + n_pass * mNblocks * m_blk_sz_words + i * m_blk_sz_words , m32_block_random + offs_key2 / m_quantum_size , m_block_size_bytes ) ;
                #endif

            }
}

void GammaCryptImpl::GenKeyToFile()
{
    m_header.source_file_size = 131072 ;
    Initialize() ;
    MakePswBlock() ;

    display_str("Generating randoms...") ;
    mGenerateRandoms() ;

    display_str("making permutation array...") ;
    // m_Permutate initializing is inside GammaCryptImpl::Initialize
    m_Permutate.MakePermutArr( m_Permutate.m16e_arr , m_Permutate.mp8_randoms + m_block_size_bytes * 2 , m_Permutate.m_BIarray ) ;
    m_Permutate.MakePermutArr( m_Permutate.m16e_arr2 , m_Permutate.mp8_randoms + m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes , m_Permutate.m_BIarray2 ) ;

    #ifdef DEBUG
        CheckPermutArr( m_Permutate.m16e_arr , m_Permutate.epma_size_elms ) ;
        CheckPermutArr( m_Permutate.m16e_arr2 , m_Permutate.epma_size_elms ) ;
    #endif

    m_header.source_file_size = 0 ;
    WriteHead() ; 
    DisplayInfo() ;
    
}



void GammaCryptImpl::Encrypt()  // вот это и будет main_multithread
{
    static_assert( sizeof(int) == 4 , " int size is not equal to 4");
	//m_dbg_ofs1.open( "out1.txt" ) ;
	//m_dbg_ofs2.open( "out2.txt" ) ;

    if ( mb_use_keyfile )
    {
        m_ifs_keyfile.open( m_keyfilename.c_str() , std::ifstream::in | std::ifstream::binary ) ;
        if ( ! m_ifs_keyfile.is_open() )
        {
            //display_str( help_string ) ;
            throw( "Error opening key file" ) ;
        }
        ReadHead( m_ifs_keyfile ) ;
        mb_need_init_blocksize = false ;
        m_block_size_bytes = m_header.h_block_size ;
    }

    //calculate size of input file
    //определим размер входного файла
    m_ifs.seekg( 0 , std::ios::end ) ;
    m_header.source_file_size = m_ifs.tellg() ;
    m_ifs.seekg( 0 , std::ios::beg ) ;

    Initialize() ;
        assert( m32_block_random != nullptr ) ;
        
    //
    unsigned tail_size_bytes = m_header.source_file_size % m_block_size_bytes ;
    unsigned tail_size_words = tail_size_bytes / sizeof( unsigned ) ;
    if ( tail_size_bytes % sizeof( unsigned ) != 0 )
        tail_size_words ++ ;
    
    MakePswBlock() ;
    
    if ( mb_use_keyfile )
    {
        try 
		{
			ReadOverheadData( m_ifs_keyfile ) ;
		}
		catch( const char * s )
		{
			display_err( s ) ;
			return ;
		}
    }
    else
    {
        display_str("Generating randoms...") ;
                // time mesaure
                auto mcs1 = std::chrono::high_resolution_clock::now() ;
                //
        mGenerateRandoms() ;
        //GenerateRandoms( m_blk_sz_words * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + tail_size_words , mu32_block_random.get()  ) ;
                // time mesaure
                auto mcs2 = std::chrono::high_resolution_clock::now() ;
                std::chrono::duration<double> mcs = mcs2 - mcs1 ;
                uint64_t drtn = mcs.count() * 1000000 ;
                std::cout << "Duration: " << drtn << std::endl ;
                //
        display_str("making permutation array...") ;
                // time mesaure
                //mcs1 = std::chrono::high_resolution_clock::now() ;
                //

        // m_Permutate initializing is inside GammaCryptImpl::Initialize
        if ( ! mb_decrypting )
        {
            m_Permutate.MakePermutArr( m_Permutate.m16e_arr , m_Permutate.mp8_randoms + m_block_size_bytes * 2 , m_Permutate.m_BIarray ) ;
            m_Permutate.MakePermutArr( m_Permutate.m16e_arr2 , m_Permutate.mp8_randoms + m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes , m_Permutate.m_BIarray2 ) ;
        }
    }
    

        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n pma1 \n " ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n " ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD

            // time mesaure
            //mcs2 = std::chrono::high_resolution_clock::now() ;
            //mcs = mcs2 - mcs1 ;
            //drtn = mcs.count() * 1000000 ;
            //std::cout << "Duration: " << drtn << std::endl ;
            //

    WriteHead() ; 
    DisplayInfo() ;

    //CRYPT ;
        assert( m32_block_random != nullptr ) ;
        assert( m32_block_source != nullptr ) ;
        assert( m32_block_dest != nullptr ) ;

    display_str("crypting...") ;

    assert( m_hrdw_concr != 0 ) ;
    auto ce_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes * m_Threading.m_Nthr * 2 ) ; // for ePMA, c-char // * 2 для разных проходов многопоточности todo remove
	m8_temp_block = ce_temp_block.get() ;
	
    auto u16e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms * m_Threading.m_Nthr ) ; // for ePMA2
	m16e_temp_block_pma = u16e_temp_block_pma.get() ;

    unsigned bytes_read ;
    
    // prepair to calculate all key1, key2 , pma1 , pma2
    mNblocks = m_Threading.m_Nthr * m_blks_per_thr ;
    mbuf_size = m_block_size_bytes * mNblocks ;
    
    std::unique_ptr< t_block > upkeys1 , upkeys2 ;
    std::unique_ptr< uint16_t[] > uppma1 , uppma2 ;

    if ( mb_multithread )
    {
        upkeys1 = std::make_unique< t_block >( (mNblocks + 1 ) * m_blk_sz_words * 2 ) ; // * 2 для разных проходов многопоточности
        mp32keys1 = upkeys1.get() ;
        
        upkeys2 = std::make_unique< t_block >( mNblocks * m_blk_sz_words * 2 ) ;
        mp32keys2 = upkeys2.get() ;
        
        uppma1 = std::make_unique< uint16_t[] >( mNblocks * m_Permutate.epma_size_elms * 2 ) ;
        mp16pma1 = uppma1.get() ;
        
        uppma2 = std::make_unique< uint16_t[] >( mNblocks * m_Permutate.epma_size_elms * 2 ) ;
        mp16pma2 = uppma2.get() ;
        
    }
   
    std::unique_ptr< char[] > upbuffer ; // up - Unique Ptr

    std::unique_ptr< char[] > upbuffout ; // up - Unique Ptr
    
    if ( ! mb_multithread )
    {
        mpbuffer = reinterpret_cast< char * > ( m32_block_source ) ;
        mpbuffout = reinterpret_cast< char * > ( m32_block_dest ) ;
        mbuf_size = m_block_size_bytes ;
		params.p_source = m32_block_source ;
		params.p_dest = m32_block_dest ;
    }
    else
    {
        upbuffer = std::make_unique< char[] >( mbuf_size ) ;
        mpbuffer = upbuffer.get() ;

        upbuffout = std::make_unique< char[] >( mbuf_size ) ;
        mpbuffout = upbuffout.get() ;
        
        params.p_source = reinterpret_cast< unsigned * >( mpbuffer ) ;
        params.p_dest = reinterpret_cast< unsigned * >( mpbuffout ) ;
    }

        // time mesaure
        auto mcs1 = std::chrono::high_resolution_clock::now() ;
        //

    // operation for eTransformPMA2
    m_op = 0 ;
    
    mNblocks = mbuf_size / m_block_size_bytes ;
	
    params.e16_arr = m_Permutate.m16e_arr ; // todo get out 
    params.e16_arr2 = m_Permutate.m16e_arr2 ; // todo get out
    params.epma_sz_elmnts = m_Permutate.epma_size_elms ;

	if ( mb_multithread )
	{
		m_Threading.Process( this , m_Threading.m_Nthr ) ;
	}
	//даже если был multithread, заканчиваем однопоточно:
    mp32keys1 = m32_block_random ;
    mp32keys2 = m32_block_random + offs_key2 / m_quantum_size ;
    mp16pma1 = m_Permutate.m16e_arr ;
    mp16pma2 = m_Permutate.m16e_arr2 ;

    // GO!

    while ( ! m_ifs.eof() )
    {
        
        m_ifs.read(  mpbuffer , mbuf_size ) ;
        bytes_read = m_ifs.gcount() ;
        if ( bytes_read == 0 )
            continue ;
		unsigned bytes_to_write = bytes_read ;
		unsigned tail_size_read ;
		
		//uint64_t bytes_left = m_header.source_file_size - m_ifs.tellg() ;
		
		if ( bytes_read < mbuf_size )
		{
			tail_size_read = bytes_read % m_block_size_bytes ;
			bytes_to_write = bytes_read - tail_size_read + m_block_size_bytes ;

			if ( tail_size_read != 0 )
			{
				assert( tail_size_read == tail_size_bytes ) ;
				//mNblocks++ ;
				// тогда остаток блока надо добить randoms[m_blk_sz_words*2] .. randoms[m_blk_sz_words*2 + tail_size_bytes]
				memcpy( (char*) params.p_source + tail_size_read, reinterpret_cast< unsigned char * >( m32_block_random ) + offs_ktail , m_block_size_bytes - tail_size_read ) ;
			}
			
		}
        
		EncryptBlock() ;
        
        // ------------ OUT ------------

        m_ofs.write( (char*) mpbuffout , bytes_to_write ) ;
		
    } // end of main cycle
    
        // time mesaure
        auto mcs2 = std::chrono::high_resolution_clock::now() ;
        std::chrono::duration<double> mcs = mcs2 - mcs1 ;
        uint64_t drtn = mcs.count() * 1'000'000 ;
        std::cout << "Speed: " << float(m_header.source_file_size) / 1024 / 1024 / drtn * 1'000'000 << " MB/sec" << std::endl ;
        //
    
}


inline void GammaCryptImpl::RemoveDiffusion( unsigned * p32dst ) noexcept
{
	uint64_t * p64dst = reinterpret_cast< uint64_t * >( p32dst ) ; 

	for ( int j = 8 ; j >= 0 ; j-- )
    {
        // cycled shift right
        for ( unsigned i = 0 ; i < m_blk_sz_words/2 ; i++ ) //
            if ( j == 0 || j == 2 )
                p64dst[i] = ( p64dst[i] >> 1 ) | ( p64dst[i] << 63 ) ;
            else if ( j == 1 )
                p64dst[i] = ( p64dst[i] >> 2 ) | ( p64dst[i] << 62 ) ;
            else if ( j == 3 )
                p64dst[i] = ( p64dst[i] >> 3 ) | ( p64dst[i] << 61 ) ;
            else if ( j == 4 )
                p64dst[i] = ( p64dst[i] >> 7 ) | ( p64dst[i] << 57 ) ;
            else if ( j == 5 &&  ( i % 2 == 0 ) )
                p64dst[i] = ( p64dst[i] >> 16 ) | ( p64dst[i] << 48 ) ;
            else if ( j == 6 &&  ( i % 2 == 0 ) )
                p64dst[i] = ( p64dst[i] >> 31 ) | ( p64dst[i] << 33 ) ;
            else if ( j == 7 &&  ( i % 2 == 0 ) )
                p64dst[i] = ( p64dst[i] >> 58 ) | ( p64dst[i] << 6 ) ;
        //arithmetic manipulations
        p64dst[ 0 ] -= p64dst[ m_blk_sz_words/2 - 1 ] ;
        for ( unsigned i = m_blk_sz_words/2 -1 ; i > 0 ; i-- )
        {
            p64dst[ i ] -= p64dst[ i - 1 ] ;
        }
    }


//    for ( unsigned j = 0 ; j < m_quantum_size * 8 ; j++ )
//    {
//        // cycled shift right
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            p32dst[ i ] = ( p32dst[ i ] >> 1 ) | ( p32dst[ i ] << 31 ) ;
//        //arithmetic manipulations
//        p32dst[ 0 ] -= p32dst[ m_blk_sz_words - 1 ] ;
//        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
//        {
//            p32dst[ i ] -= p32dst[ i - 1 ] ;
//        }
//    }
}


void GammaCryptImpl::DecryptBlock( uint16_t * t16_invs_pma1 ) noexcept
{
    // -------------- XOR2 --------------
    #ifdef LFSR_ENBLD
    ( *mpShift )( m32_block_random + m_blk_sz_words ) ;
    #endif
    
    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( m32_block_random + m_blk_sz_words ) ; // this is CONFUSION
    #endif
    
    //todo1 Uncomment !
    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            m32_block_source[i] = m32_block_source[i] ^ m32_block_random[ i + offs_key2 / m_quantum_size ] ;
    #endif // XOR_ENBLD
        
    #ifdef DIFFUSION_ENBLD
        RemoveDiffusion( m32_block_source ) ;
    #endif
    

    // -------------- REPOSITIONING --------------
    #ifdef PERMUTATE_ENBLD
        // transform pma2
        eTransformPMA2( m_Permutate.m16e_arr2 , m_Permutate.epma_size_elms , m_op ) ;

        //rearrange pma1
        m_Permutate.eRearrangePMA1( m16e_temp_block_pma , m_Permutate.m16e_arr2 ) ;


        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n Decrypt block: pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD


        //inverse pma1
        m_Permutate.InverseExpPermutArr( t16_invs_pma1 , m_Permutate.m16e_arr ) ;

        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n After inverse: pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << t16_invs_pma1[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif
        //permutate
        m_Permutate.eRearrange( ( unsigned char*) m32_block_source , m8_temp_block , t16_invs_pma1
                , m_Permutate.epma_size_elms , m_block_size_bytes , m_perm_bytes ) ;
    #endif // PERMUTATE_ENBLD
    
    // -------------- XOR1 --------------
    #ifdef XOR_ENBLD
        memcpy( m8_temp_block , m32_block_random , m_block_size_bytes ) ;
    #endif
    
    #ifdef LFSR_ENBLD
        ( *mpShift )( m32_block_random ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( m32_block_random ) ; // this is CONFUSION
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            m32_block_dest[i] = m32_block_source[i] ^ m32_block_random[i] ; // todo1 Uncomment it!!!!
    #else
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            m32_block_dest[i] = m32_block_source[i] ;
    #endif // XOR_ENBLD
    
    #ifdef DIFFUSION_ENBLD
        RemoveDiffusion( m32_block_dest ) ;
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            m32_block_dest[i] = m32_block_dest[i] ^ ( (unsigned * ) (m8_temp_block) )[ i ] ; // todo1 Uncomment it!!!!
    #endif
    
}



void GammaCryptImpl::Decrypt()
{
    static_assert( sizeof(int) == 4 , " int size is not equal to 4");
    mb_use_keyfile = false ;
    ReadHead( m_ifs ) ;
    if ( m_header.major_ver & 0x80 )
        mb_use_keyfile = true ;
    
    
    if ( mb_use_keyfile )
    {
        m_ifs_keyfile.open( m_keyfilename.c_str() , std::ifstream::in | std::ifstream::binary ) ;
        if ( ! m_ifs_keyfile.is_open() )
        {
            //display_str( help_string ) ;
            throw( "Error opening key file" ) ;
        }
        uint64_t temp_source_file_size = m_header.source_file_size ; // save source_file_size
        ReadHead( m_ifs_keyfile ) ;
        m_header.source_file_size = temp_source_file_size ;
    }

    m_block_size_bytes = m_header.h_block_size ;
    mb_decrypting = true ;
    mb_need_init_blocksize = false ;
    
    Initialize() ;

    MakePswBlock() ;

    if ( mb_use_keyfile )
	{
        try 
		{
			ReadOverheadData( m_ifs_keyfile ) ;
		}
		catch( const char * s )
		{
			display_err( s ) ;
			return ;
		}
	}	
    else
	{
        try 
		{
			ReadOverheadData( m_ifs ) ;
		}
		catch( const char * s )
		{
			display_err( s ) ;
			return ;
		}
	}	
    
        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.m16e_arr2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD

    DisplayInfo() ;
    
    // CRYPT  Crypt() ;
        assert( m32_block_random != nullptr ) ;
        assert( m32_block_source != nullptr ) ;
        assert( m32_block_dest != nullptr ) ;

    auto temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for Rearrange with PMA
    m8_temp_block = temp_block.get() ;
    //auto e8_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for ePMA
    auto u16e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
    m16e_temp_block_pma = u16e_temp_block_pma.get() ;
    
    auto t_invs_pma1 = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ;

            // time mesaure
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    // operation for eTransformPMA2
    m_op = 0 ;
    
    int64_t i = m_header.source_file_size ;
    while ( i > 0 )
    {
        // READING
        m_ifs.read( (char*) m32_block_source, m_block_size_bytes ) ;
        unsigned bytes_read = m_ifs.gcount() ;
        if ( bytes_read == 0 )
            break ;
            
        DecryptBlock( t_invs_pma1.get() ) ;
        
        //it should not be the tail now
        // checking tail size
        unsigned bytes_to_write ;
        i -= m_block_size_bytes ;
        if ( i >=0 )
            bytes_to_write = bytes_read ;
        else
        {
            //assert( false ) ;
            bytes_to_write = i + m_block_size_bytes ;
        }
        
/*        m_ifs.peek() ; 
        if ( ! m_ifs.good() ) // Проверяем конец файла
            bytes_to_write = m_header.tail_size ;
             */
        //OUT
        m_ofs.write( reinterpret_cast< char* >( m32_block_dest ) , bytes_to_write ) ;

    }
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Speed: " << float(m_header.source_file_size) / 1024 / 1024 / drtn * 1'000'000 << " MB/sec" << std::endl ;
            //
}


void GammaCryptImpl::ReadHead( std::istream & ifs )
{
    //Header
    ifs.read( (char*) &m_header , sizeof( t_header ) ) ;
    if ( (unsigned) ifs.gcount() <  sizeof( t_header ) )
        throw ("error: File too short, missing header") ;
    if ( m_header.major_ver & 0x40 )
        m_perm_bytes = false ;
}

void GammaCryptImpl::ReadOverheadData( std::istream & ifs )
{
        assert( m32_block_random != nullptr ) ;
        assert( m32_block_password != nullptr ) ;
    //Block_key
    auto block_key = std::make_unique< t_block >( m_blk_sz_words );
    //Key1
    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
        throw ("error: File too short, missing key block") ;

    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        m32_block_random[i] = block_key[i] ^ m32_block_password[i] ; // todo1 Uncomment it!!!!


    //Key2
    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
        throw ("error: File too short, missing key2 block") ;

    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        m32_block_random[ i + offs_key2 / m_quantum_size ] = block_key[i] ^ m32_block_password[i] ;

    // Permutation Array 1
    ifs.read( (char*) m_Permutate.m_BIarray.m_array.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_Permutate.m_BIarray.pma_size_bytes )
        throw ("error: File too short, missing permutation array block") ;
        
    unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / sizeof(unsigned) ;
    if ( m_Permutate.m_BIarray.pma_size_bytes % sizeof( unsigned ) != 0  )
        block_pma_size_words++ ;

    auto block_pma_xored = std::make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Psw( (unsigned *) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
    memcpy( m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;

    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray ) ;
    
    // build e_array, aka Expanded in the memory
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray.m_size_elms ; ++i )
    {
        m_Permutate.m16e_arr[ i ] = m_Permutate.m_BIarray[ i ] ;
    }
	if ( !CheckPermutArr( m_Permutate.m16e_arr , m_Permutate.epma_size_elms ) )
		throw( "wrong password" ) ;
		


    // Permutation Array 2
    ifs.read( (char*) m_Permutate.m_BIarray2.m_array.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_Permutate.m_BIarray2.pma_size_bytes )
        throw ("error: File too short, missing permutation array block") ;
        
    block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / sizeof(unsigned) ;
    if ( m_Permutate.m_BIarray2.pma_size_bytes % sizeof( unsigned ) != 0  )
        block_pma_size_words++ ;

    //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
    PMA_Xor_Psw( (unsigned *) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
    memcpy( m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;

    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray2 ) ;
    
    // build e_array2, aka Expanded in the memory
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray2.m_size_elms ; ++i )
    {
        m_Permutate.m16e_arr2[ i ] = m_Permutate.m_BIarray2[ i ] ; // todo get out get()
    }

	if ( !CheckPermutArr( m_Permutate.m16e_arr2 , m_Permutate.epma_size_elms ) )
		throw( "wrong password" ) ;

} ;

void GammaCryptImpl::WriteHead()
{
        assert( m32_block_random != nullptr ) ;
        assert( m32_block_password != nullptr ) ;
    if ( mb_use_keyfile )
    {
        // set high bit of m_header.major_ver :
        m_header.major_ver |= 0x80 ;
    }
    if ( ! m_perm_bytes )
    {
        // set bit6 of m_header.major_ver :
        m_header.major_ver |= 0x40 ;
    }
    //Header
    m_header.data_offset = sizeof( t_header ) ;
    m_header.h_block_size = m_block_size_bytes ;
    m_ofs.write( (char*) &m_header , sizeof( t_header ) ) ;
    

    if ( ! mb_use_keyfile )
    {
        //Block_key
        auto block_key = std::make_unique< t_block >( m_blk_sz_words );
        //Key1
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            block_key[i] = m32_block_random[i] ^ m32_block_password[i] ;
        m_ofs.write( (char*) block_key.get() , m_block_size_bytes ) ;
        //Key2
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            block_key[i] = m32_block_random[ i + offs_key2 / m_quantum_size ] ^ m32_block_password[i] ;
        m_ofs.write( (char*) block_key.get() , m_block_size_bytes ) ;

        
        //Permutation array 1
        // 1) XOR with password_block
        unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ;
        if ( m_Permutate.m_BIarray.pma_size_bytes % m_quantum_size != 0 )
            block_pma_size_words++ ;
        auto block_pma_xored = std::make_unique< unsigned[] >( block_pma_size_words ) ;
        PMA_Xor_Psw( ( unsigned * ) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
        // 2) write to file
        m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
        
        //Permutation array 2
        // 1) XOR with password_block
        block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / m_quantum_size ;
        if ( m_Permutate.m_BIarray2.pma_size_bytes % m_quantum_size != 0 )
            block_pma_size_words++ ;
        //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
        PMA_Xor_Psw( ( unsigned * ) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
        // 2) write to file
        m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
    }
}

void GammaCryptImpl::PMA_Xor_Psw( unsigned * p32_pma_in , unsigned * p32_pma_out )
{
    for ( unsigned i = 0 ; i < m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ; ++i  )
    {
        p32_pma_out[ i ] = p32_pma_in[ i ] ^ reinterpret_cast< unsigned * >( m8_block_psw_pma.get() )[ i ]   ;
    }
    
} ;


void GammaCryptImpl::SetBlockSize( unsigned block_size )
{
    if ( block_size > 128 )
        block_size = 128 ;
    mb_need_init_blocksize = false ;
    unsigned log2 = 0 ; 
    while ( block_size >>= 1 ) log2 ++ ;
    m_block_size_bytes = 1 ;
    for ( unsigned i = 0 ; i < log2 ; ++i )
        m_block_size_bytes <<= 1 ;
    if ( m_block_size_bytes < 8)
        m_block_size_bytes = 8 ;
}


void GammaCryptImpl::mGenerateRandoms()
{
    std::atomic< bool > bstop ( false ) ;
    //timer
    std::atomic< uint64_t > ticks( 0 ) ;

    std::thread thr_tick( TickTimer , std::ref( ticks ) , std::ref( bstop ) ) ;
    
    std::vector<std::thread> threads;
    for( unsigned i = 0; i < m_hrdw_concr - 1 ; i++ ) 
    {
        std::thread thr_cfl( CPUFullLoad , std::ref( bstop ) ) ; // thread CPU Loading at 100%
        threads.emplace_back( std::move( thr_cfl ) ) ;
    }

    try
    {
        GenerateRandoms( m_rnd_size_words , m32_block_random , std::ref( ticks ) ) ;
    }
    catch( const char * s)
    {
        bstop = true ;
        thr_tick.join() ;
        for ( auto & thr : threads ) 
            thr.join();

        throw s ;
    }
    
    bstop = true ;
    thr_tick.join() ;
    for ( auto & thr : threads ) 
        thr.join();

    // XOR std::random
    std::random_device rndDevice;
    std::mt19937 eng(rndDevice());
    std::uniform_int_distribution< unsigned > dist( 0 , 0xFFFF'FFFF );
    auto gen = std::bind(dist, eng);
    
    std::vector< unsigned > vec( m_rnd_size_words );
    std::generate(begin(vec), end(vec), gen);
    
    for ( unsigned i = 0 ; i < m_rnd_size_words ; ++ i )
        m32_block_random[ i ] ^= vec[ i ] ;
}

void GammaCryptImpl::DisplayInfo()
{
    std::ostringstream oss ;
    oss << "\n source file size (bytes): " << m_header.source_file_size ;
    oss << "\n block size (bytes): " << m_block_size_bytes ;
    oss << "\n permutation array size (bytes): " << m_Permutate.m_BIarray.pma_size_bytes ;
    oss << "\n number of permutation array elements: " << m_Permutate.m_BIarray.m_size_elms ;
    oss << "\n size of one permutation array element (bits): " << m_Permutate.m_BIarray.index_size_bits ;
    oss << "\n hardware_concurrency: " << m_hrdw_concr ;
    oss << "\n" ;
    display_str( oss.str() ) ;
}

