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


void Threading::Process( GammaCrypt * pGC , unsigned Nthr )
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
				memcpy( (char*) mpGC->params.p_source + tail_size_read, /*reinterpret_cast< unsigned char * >*/ (unsigned char *) ( mpGC->mp_block_random.get() ) + mpGC->offs_ktail , mpGC->m_block_size_bytes - tail_size_read ) ;
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


GammaCrypt::GammaCrypt( std::istream & ifs , std::ostream & ofs ,  const std::string & password ) 
    : m_ifs( ifs ) , m_ofs( ofs ) , mp_block_random( nullptr ) ,  m_password( password )  
    , mp_block_password( nullptr ) , mp_block_source( nullptr ) , mp_block_dest( nullptr )
	
{
	m_Threading.mpGC = this  ;
}

void GammaCrypt::Initialize()
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
    mpc_block_psw_pma = std::make_unique< unsigned char[] >( m_Permutate.m_BIarray.pma_size_bytes ) ;

    // allocate memory
    mp_block_password  = std::make_unique< t_block >( m_blk_sz_words ) ;
    // obsolete: mp_block_random3   = make_unique< t_block >( m_blk_sz_words + 1 ) ; // obsolete: to transform Random block it is required one quantum more (therefore "+1")
    mp_block_source    = std::make_unique< t_block >( m_blk_sz_words ) ;
    mp_block_dest      = std::make_unique< t_block >( m_blk_sz_words ) ;

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

    mp_block_random = std::make_unique< t_block >( m_rnd_size_words ) ; 

    // m_Permutate Initializing (continue)
    m_Permutate.mpc_randoms = reinterpret_cast< unsigned char * >( mp_block_random.get() ) ;

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

void GammaCrypt::MakePswBlock()
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
        memcpy( reinterpret_cast<char*>( mp_block_password.get() ) + i , m_password.c_str(), bytes_to_copy ) ;
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
        ( *mpShift )( mp_block_password.get() ) ;

    const unsigned block_size_bytes = m_block_size_bytes ;
    Permutate Pmt ;
    Pmt.Init( block_size_bytes , false ) ;

    const unsigned & pma_size_bytes = Pmt.m_BIarray.pma_size_bytes ; 

    auto up_block_rnd = std::make_unique< unsigned char[] >( pma_size_bytes ) ;

    // 2. psw_block_rnd1
    for ( unsigned i = 0 ; i < pma_size_bytes ; i += block_size_bytes )
    {
        for ( unsigned j = 0 ; j < N ; ++j )
            ( *mpShift )( mp_block_password.get() ) ;

        bytes_to_copy = ( i + block_size_bytes > pma_size_bytes ) ? (i + block_size_bytes) - pma_size_bytes  : block_size_bytes ;
        std::memcpy( up_block_rnd.get() + i , mp_block_password.get() , block_size_bytes ) ;
    }

    auto up_block_rnd2 = std::make_unique< unsigned char[] >( pma_size_bytes ) ;

    // 3. psw_blocl_rnd2
    for ( unsigned i = 0 ; i < pma_size_bytes ; i += block_size_bytes )
    {
        for ( unsigned j = 0 ; j < N ; ++j )
            ( *mpShift )( mp_block_password.get() ) ;
        bytes_to_copy = ( i + block_size_bytes > pma_size_bytes ) ? (i + block_size_bytes) - pma_size_bytes  : block_size_bytes ;

        std::memcpy( up_block_rnd2.get() + i , mp_block_password.get() , block_size_bytes ) ;
    }

    // 4. PMA2
    Pmt.mpc_randoms = up_block_rnd2.get() ;
    Pmt.MakePermutArr( Pmt.e_array2.get() , Pmt.mpc_randoms , Pmt.m_BIarray2 ) ; //
    
    // 5. PMA1
    Pmt.mpc_randoms = up_block_rnd.get() ;
    Pmt.MakePermutArr( Pmt.e_array.get() , Pmt.mpc_randoms , Pmt.m_BIarray ) ; //
    
    auto temp_block = std::make_unique< unsigned char[] >( block_size_bytes ) ; 
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( Pmt.epma_size_elms ) ; // for ePMA2


    //gamma crypt (for crypt, not for MakePswBlock) pma_size_bytes :
    const unsigned & GC_pma_size_bytes = m_Permutate.m_BIarray.pma_size_bytes ;
    // при perm_bytes (перестановка байтов, а не битов pma_size_bytes < block_size_bytes ) , поэтому
    //приращение в циклах (чтобы доходить до конца блока)
    unsigned increment =  GC_pma_size_bytes < block_size_bytes ? GC_pma_size_bytes : block_size_bytes ;

    if ( GC_pma_size_bytes > block_size_bytes )
        assert( GC_pma_size_bytes % block_size_bytes == 0 ) ;
    // 6. GammaCrypt::mpc_block_psw_pma
    for ( unsigned i = 0 ; i < GC_pma_size_bytes  ; i += increment )
    {
        unsigned op = 0 ; 
        for ( unsigned j = 0 ; j < Neven ; ++ j )
        {
            ( *mpShift )( mp_block_password.get() ) ;


            eTransformPMA2( Pmt.e_array2.get() , Pmt.epma_size_elms , op ) ;
            Pmt.eRearrangePMA1( e_temp_block_pma.get() , Pmt.e_array2.get() ) ;

            Pmt.eRearrange( ( ( unsigned char*) mp_block_password.get() ) , temp_block.get() , Pmt.e_array.get() , Pmt.epma_size_elms , block_size_bytes , false ) ;
            
        }
        memcpy( mpc_block_psw_pma.get() + i , ( unsigned char*) mp_block_password.get() , increment ) ;
    }
    
    // 7. GammaCrypt::mp_block_password
    unsigned op = 0 ;
    for ( unsigned i = 0 ; i < N ; ++ i )
    {
        ( *mpShift )( mp_block_password.get() ) ;


        eTransformPMA2( Pmt.e_array2.get() , Pmt.epma_size_elms , op ) ;
        Pmt.eRearrangePMA1( e_temp_block_pma.get() , Pmt.e_array2.get() ) ;

        Pmt.eRearrange( ( ( unsigned char*) mp_block_password.get() ) , temp_block.get() , Pmt.e_array.get() , Pmt.epma_size_elms , block_size_bytes , false ) ;
        
    }
    
    
   
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
    
}


/*
void GammaCrypt::DecryptBlock( uint16_t * e_temp_block_pma , unsigned char * e_temp_block , unsigned & op ) noexcept
{
    #ifdef XOR_ENBLD
    // XOR2
    
    #ifdef LFSR_ENBLD
    ( *mpShift )( mp_block_random.get() + m_blk_sz_words ) ;
    #endif
    
    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        mp_block_source[i] = mp_block_source[i] ^ mp_block_random[ i + offs_key2 / m_quantum_size ] ;
    
    #endif // XOR_ENBLD

    #ifdef PERMUTATE_ENBLD
    //REPOSITIONING
    
    // transform pma2
    eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , op ) ;

    //rearrange pma1
    m_Permutate.eRearrangePMA1( e_temp_block_pma.get() , m_Permutate.e_array2.get() ) ;
    //inverse pma1
    m_Permutate.InverseExpPermutArr( t_invs_pma1.get() , m_Permutate.e_array.get() ) ;
    eRearrange( ( unsigned char*) mp_block_source.get() , temp_block.get() , t_invs_pma1.get() , m_Permutate.epma_size_elms , m_block_size_bytes ) ;

    #endif // PERMUTATE_ENBLD
    
    #ifdef XOR_ENBLD
    // XOR1

    #ifdef LFSR_ENBLD
    ( *mpShift )( mp_block_random.get() ) ;
    #endif // LFSR_ENBLD

    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
        
    #else
    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        mp_block_dest[i] = mp_block_source[i] ;
    #endif // XOR_ENBLD
    
}
*/


inline void GammaCrypt::MakeDiffusion( unsigned * psrc) noexcept
{
//    memcpy( mc_temp_block , psrc , m_block_size_bytes ) ;
//    //для удобства:
//    psrc = mc_temp_block ;
    
    for ( unsigned j = 0 ; j <= 9 /*m_quantum_size * 8*/ ; j++ )
    {
        //arithmetic manipulations
        for ( unsigned i = 1 ; i < m_blk_sz_words ; i++ )
        {
            psrc[ i ] += psrc[ i - 1 ] ;
        }
        psrc[ 0 ] += psrc[ m_blk_sz_words - 1 ] ;

        // left circular shift
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 32 bit rolls
            if ( j < 4 )
                psrc[i] = ( psrc[i] << 1 ) | ( psrc[i] >> 31 ) ; // 6 rolls
            else if ( j == 4 )
                psrc[i] = ( psrc[i] << 4 ) | ( psrc[i] >> 28 ) ; // 4 rolls
            else if ( j == 5 )
                psrc[i] = ( psrc[i] << 2 ) | ( psrc[i] >> 30 ) ; // 2 rolls
            else if ( j == 7 )
                psrc[i] = ( psrc[i] << 5 ) | ( psrc[i] >> 27 ) ; // 5 rolls
            else if ( j == 8 &&  ( i % 2 == 0 ) )
                psrc[i] = ( psrc[i] << 16 ) | ( psrc[i] >> 16 ) ; // 15 rolls
    }

//    for ( unsigned j = 0 ; j < m_quantum_size * 8 ; j++ )
//    {
//        for ( unsigned i = 1 ; i < m_blk_sz_words ; i++ )
//        {
//            params.p_source[ i ] += params.p_source[ i - 1 ] ;
//            //x = (x >> k) | (x << (32 - k))
//    //        unsigned * tsrc = & params.p_source[i] ;
//    //        asm(  
//    //            "rol %[arg_a] , 1 \n\t" 
//    //            :[arg_a]"=r"( *tsrc )
//    //        ) ;
//        }
//        params.p_source[ 0 ] += params.p_source[ m_blk_sz_words - 1 ] ;
//
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            params.p_source[i] = (params.p_source[i] << 1 ) | ( params.p_source[i] >> 31 ) ;
//    }
    
}

void GammaCrypt::EncryptBlock() noexcept
{

    // ----------- XOR1 -----------

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_source[ i ] ^ mpkeys1[ i ] ;
    #else
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_source[i] ;
    #endif

    #ifdef LFSR_ENBLD
    ( *mpShift )( mpkeys1 ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( mpkeys1 ) ; // this is CONFUSION
        MakeDiffusion( params.p_dest ) ; 
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[i] = params.p_dest[ i ] ^ mpkeys1[ i ] ;
    #endif  // XOR_ENBLD

    // ---------- REPOSITIONING ----------

    #ifdef PERMUTATE_ENBLD
        eTransformPMA2( mppma2 , m_Permutate.epma_size_elms , m_op ) ;

        #ifdef DEBUG
            CheckPermutArr( mppma2 , m_Permutate.epma_size_elms ) ;
        #endif

        m_Permutate.eRearrangePMA1( mpu16e_temp_block_pma , mppma2 ) ;
        #ifdef DEBUG
            CheckPermutArr( m_Permutate.e_array.get() , m_Permutate.epma_size_elms ) ;
        #endif


        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n EncryptBlock : pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << mppma1[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << mppma2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD


        m_Permutate.eRearrange( (unsigned char*) params.p_dest , mc_temp_block , mppma1 , m_Permutate.epma_size_elms , m_block_size_bytes , m_perm_bytes ) ;
    #endif // PERMUTATE_ENBLD
    
    // ----------- XOR2 ------------

// todo1 uncomment it
    #ifdef LFSR_ENBLD
    ( *mpShift )( mpkeys2 ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( params.p_dest ) ; 
        MakeDiffusion( mpkeys2 ) ; // this is CONFUSION
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            params.p_dest[ i ] ^= mpkeys2[ i ] ;
    #endif // XOR_ENBLD

}

// nthr - thread number
void GammaCrypt::EncryptBlockOneThr( unsigned nthr , unsigned n_pass_thr  ) noexcept
{

    for ( unsigned j = 0 ; j < m_blks_per_thr ; ++ j )
    {
        //uidx idx type: unsigneds
        unsigned uidx = nthr * m_blks_per_thr * m_blk_sz_words + j * m_blk_sz_words ;
        // ----------- XOR1 -----------
        #ifdef XOR_ENBLD
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_source[ uidx + i ] 
                                             ^ mpkeys1[ n_pass_thr * mNblocks * m_blk_sz_words + uidx + i]  ;
		#else
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_source[ uidx + i ] ;
        #endif

        #ifdef DIFFUSION_ENBLD
            //MakeDiffusion( mpkeys1 + n_pass_thr * mNblocks * m_blk_sz_words + uidx ) ; // this is CONFUSION
            MakeDiffusion( params.p_dest + uidx ) ;
        #endif
        
        #ifdef XOR_ENBLD
            for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
                params.p_dest[ uidx + i ] =  params.p_dest[ uidx + i ] 
                                             ^ mpkeys1[ n_pass_thr * mNblocks * m_blk_sz_words + m_blk_sz_words + uidx + i]  ;
		#endif // XOR_ENBLD


        // ---------- REPOSITIONING ----------

        #ifdef PERMUTATE_ENBLD
            m_Permutate.eRearrange( reinterpret_cast< unsigned char * >( params.p_dest + uidx )  
                    , mc_temp_block + nthr * m_block_size_bytes
                    , mppma1 + n_pass_thr * mNblocks * m_Permutate.epma_size_elms + nthr * m_blks_per_thr * params.epma_sz_elmnts + j * params.epma_sz_elmnts 
                    , params.epma_sz_elmnts , m_blk_sz_words * sizeof( unsigned) , m_perm_bytes ) ;

//                // cout pma
//            {
//                std::unique_lock<std::mutex> locker(g_lockprint);
//                m_dbg_ofs2 << "\n pma1  thread: " << nthr << " thr_n_pass: " << n_pass_thr << "\n" ;
//                for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//                    m_dbg_ofs2 << *(i + mppma1 + n_pass_thr * mNblocks * m_Permutate.epma_size_elms
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
                params.p_dest[ uidx + i ] ^=  mpkeys2[ n_pass_thr * mNblocks * m_blk_sz_words + uidx + i]  ;
		#endif // XOR_ENBLD
    }

}


// MT - multithreading
void GammaCrypt::EncryptBlockMT( unsigned char * e_temp_block ) noexcept
{
//    unsigned n_cycles = mNblocks / m_hrdw_concr ;
    
//    params.e_temp_block = e_temp_block ;
//    params.e_array = m_Permutate.e_array.get() ;
//    params.e_array2 = m_Permutate.e_array2.get() ;
//    params.epma_sz_elmnts = m_Permutate.epma_size_elms ;
    
//	EncryptBlockOneThr( params , nthr ) ;
	
//    if ( n_cycles )
//    {
//        std::vector<std::thread> threads;
//        for ( unsigned j = 0 ; j < m_hrdw_concr ; ++j )
//        {
//            std::thread thr( &GammaCrypt::EncryptBlockOneThr , this , std::ref( params ) , j ) ; 
//            threads.emplace_back( std::move( thr ) ) ;
//        }
//        for ( auto & thr : threads ) 
//            thr.join();
//    }
//    else
//    {
//        ;
//    }

}

void GammaCrypt::PreCalc( unsigned n_pass )
{
            for ( unsigned i = 0 ; i < mNblocks ; ++i )
            {
                // ----------- KEY1 -----------
                #ifdef XOR_ENBLD
                    if ( n_pass == 0 && i == 0 )
                        memcpy( mpkeys1 , mp_block_random.get() , m_block_size_bytes ) ;
                #endif

                #ifdef LFSR_ENBLD
                    ( *mpShift )( mp_block_random.get() ) ;
                #endif

                #ifdef DIFFUSION_ENBLD
                    MakeDiffusion( mp_block_random.get() ) ; // this is CONFUSION
                #endif

                #ifdef XOR_ENBLD
                    memcpy( mpkeys1 + n_pass * mNblocks * m_blk_sz_words + (i + 1) * m_blk_sz_words , mp_block_random.get() , m_block_size_bytes ) ;
                #endif
            
                // ---------- PMA2, PMA1 ----------
                #ifdef PERMUTATE_ENBLD
                    eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , m_op ) ;
                    memcpy( mppma2 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms , m_Permutate.e_array2.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                    m_Permutate.eRearrangePMA1( mpu16e_temp_block_pma , m_Permutate.e_array2.get() ) ;
                    memcpy( mppma1 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms , m_Permutate.e_array.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                #endif
//				// cout pma
//				//m_dbg_ofs1 << "\n pma1 n_pass: " << n_pass << "\n" ;
//                m_dbg_ofs1 << "\n pma1  _________" << " ____n_pass: " << n_pass << "\n" ;
//				for ( unsigned j = 0 ; j < m_Permutate.epma_size_elms ; j ++)
//					m_dbg_ofs1 << *( j + mppma1 + n_pass * mNblocks * m_Permutate.epma_size_elms + i * m_Permutate.epma_size_elms)
//						<< ' ' ;
//				m_dbg_ofs1 << std::endl;

                
                // ----------- KEY2 ------------

                #ifdef LFSR_ENBLD
                    ( *mpShift )( mp_block_random.get() + offs_key2 / m_quantum_size ) ;
                #endif

                #ifdef DIFFUSION_ENBLD
                    MakeDiffusion( mp_block_random.get() + offs_key2 / m_quantum_size ) ; // this is CONFUSION
                #endif
                
                #ifdef XOR_ENBLD
                    memcpy( mpkeys2 + n_pass * mNblocks * m_blk_sz_words + i * m_blk_sz_words , mp_block_random.get() + offs_key2 / m_quantum_size , m_block_size_bytes ) ;
                #endif

            }
}

void GammaCrypt::GenKeyToFile()
{
    m_header.source_file_size = 131072 ;
    Initialize() ;
    MakePswBlock() ;

    display_str("Generating randoms...") ;
    mGenerateRandoms() ;

    display_str("making permutation array...") ;
    // m_Permutate initializing is inside GammaCrypt::Initialize
    m_Permutate.MakePermutArr( m_Permutate.e_array.get() , m_Permutate.mpc_randoms + m_block_size_bytes * 2 , m_Permutate.m_BIarray ) ;
    m_Permutate.MakePermutArr( m_Permutate.e_array2.get() , m_Permutate.mpc_randoms + m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes , m_Permutate.m_BIarray2 ) ;

    #ifdef DEBUG
        CheckPermutArr( m_Permutate.e_array.get() , m_Permutate.epma_size_elms ) ;
        CheckPermutArr( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms ) ;
    #endif

    m_header.source_file_size = 0 ;
    WriteHead() ; 
    DisplayInfo() ;
    
}



void GammaCrypt::Encrypt()  // вот это и будет main_multithread
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
        assert( mp_block_random != nullptr ) ;
        
    //
    unsigned tail_size_bytes = m_header.source_file_size % m_block_size_bytes ;
    unsigned tail_size_words = tail_size_bytes / sizeof( unsigned ) ;
    if ( tail_size_bytes % sizeof( unsigned ) != 0 )
        tail_size_words ++ ;
    
    MakePswBlock() ;
    
    if ( mb_use_keyfile )
    {
        ReadOverheadData( m_ifs_keyfile ) ;
    }
    else
    {
        display_str("Generating randoms...") ;
                // time mesaure
                auto mcs1 = std::chrono::high_resolution_clock::now() ;
                //
        mGenerateRandoms() ;
        //GenerateRandoms( m_blk_sz_words * 2 + m_Permutate.m_BIarray.pma_size_bytes * 2 / m_quantum_size + tail_size_words , mp_block_random.get()  ) ;
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

        // m_Permutate initializing is inside GammaCrypt::Initialize
        if ( ! mb_decrypting )
        {
            m_Permutate.MakePermutArr( m_Permutate.e_array.get() , m_Permutate.mpc_randoms + m_block_size_bytes * 2 , m_Permutate.m_BIarray ) ;
            m_Permutate.MakePermutArr( m_Permutate.e_array2.get() , m_Permutate.mpc_randoms + m_block_size_bytes * 2 + m_Permutate.m_BIarray.pma_size_bytes , m_Permutate.m_BIarray2 ) ;
        }
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
            //mcs2 = std::chrono::high_resolution_clock::now() ;
            //mcs = mcs2 - mcs1 ;
            //drtn = mcs.count() * 1000000 ;
            //std::cout << "Duration: " << drtn << std::endl ;
            //

    WriteHead() ; 
    DisplayInfo() ;

    //CRYPT ;
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_source != nullptr ) ;
        assert( mp_block_dest != nullptr ) ;

    display_str("crypting...") ;

    assert( m_hrdw_concr != 0 ) ;
    auto ce_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes * m_Threading.m_Nthr * 2 ) ; // for ePMA, c-char // * 2 для разных проходов многопоточности todo remove
	mc_temp_block = ce_temp_block.get() ;
	
    auto u16e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms * m_Threading.m_Nthr ) ; // for ePMA2
	mpu16e_temp_block_pma = u16e_temp_block_pma.get() ;

    unsigned bytes_read ;
    
    // prepair to calculate all key1, key2 , pma1 , pma2
    mNblocks = m_Threading.m_Nthr * m_blks_per_thr ;
    mbuf_size = m_block_size_bytes * mNblocks ;
    
    std::unique_ptr< t_block > upkeys1 , upkeys2 ;
    std::unique_ptr< uint16_t[] > uppma1 , uppma2 ;

    if ( mb_multithread )
    {
        upkeys1 = std::make_unique< t_block >( (mNblocks + 1 ) * m_blk_sz_words * 2 ) ; // * 2 для разных проходов многопоточности
        mpkeys1 = upkeys1.get() ;
        
        upkeys2 = std::make_unique< t_block >( mNblocks * m_blk_sz_words * 2 ) ;
        mpkeys2 = upkeys2.get() ;
        
        uppma1 = std::make_unique< uint16_t[] >( mNblocks * m_Permutate.epma_size_elms * 2 ) ;
        mppma1 = uppma1.get() ;
        
        uppma2 = std::make_unique< uint16_t[] >( mNblocks * m_Permutate.epma_size_elms * 2 ) ;
        mppma2 = uppma2.get() ;
        
    }
   
    std::unique_ptr< char[] > upbuffer ; // up - Unique Ptr

    std::unique_ptr< char[] > upbuffout ; // up - Unique Ptr
    
    if ( ! mb_multithread )
    {
        mpbuffer = reinterpret_cast< char * > ( mp_block_source.get() ) ;
        mpbuffout = reinterpret_cast< char * > ( mp_block_dest.get() ) ;
        mbuf_size = m_block_size_bytes ;
		params.p_source = mp_block_source.get() ;
		params.p_dest = mp_block_dest.get() ;
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
	
    params.e_array = m_Permutate.e_array.get() ; // todo get out 
    params.e_array2 = m_Permutate.e_array2.get() ; // todo get out
    params.epma_sz_elmnts = m_Permutate.epma_size_elms ;

	if ( mb_multithread )
	{
		m_Threading.Process( this , m_Threading.m_Nthr ) ;
	}
	//даже если был multithread, заканчиваем однопоточно:
    mpkeys1 = mp_block_random.get() ;
    mpkeys2 = mp_block_random.get() + offs_key2 / m_quantum_size ;
    mppma1 = m_Permutate.e_array.get() ;
    mppma2 = m_Permutate.e_array2.get() ;

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
				memcpy( (char*) params.p_source + tail_size_read, /*reinterpret_cast< unsigned char * >*/ (unsigned char *) ( mp_block_random.get() ) + offs_ktail , m_block_size_bytes - tail_size_read ) ;
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


inline void GammaCrypt::RemoveDiffusion( unsigned * pdst ) noexcept
{
    for ( int j = 9 ; j >= 0 ; j-- )
    {
        // cycled shift right
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 38 bit rolls
            if ( j < 4 )
                pdst[i] = ( pdst[i] >> 1 ) | ( pdst[i] << 31 ) ; // 6 rolls
            else if ( j == 4 )
                pdst[i] = ( pdst[i] >> 4 ) | ( pdst[i] << 28 ) ; // 4 rolls
            else if ( j == 5 )
                pdst[i] = ( pdst[i] >> 2 ) | ( pdst[i] << 30 ) ; // 2 rolls
            else if ( j == 7 )
                pdst[i] = ( pdst[i] >> 5 ) | ( pdst[i] << 27 ) ; // 5 rolls
            else if ( j == 8 &&  ( i % 2 == 0 ) )
                pdst[i] = ( pdst[i] << 16 ) | ( pdst[i] >> 16 ) ; // 15 rolls
        //arithmetic manipulations
        pdst[ 0 ] -= pdst[ m_blk_sz_words - 1 ] ;
        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
        {
            pdst[ i ] -= pdst[ i - 1 ] ;
        }
    }


//    for ( unsigned j = 0 ; j < m_quantum_size * 8 ; j++ )
//    {
//        // cycled shift right
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            pdst[ i ] = ( pdst[ i ] >> 1 ) | ( pdst[ i ] << 31 ) ;
//        //arithmetic manipulations
//        pdst[ 0 ] -= pdst[ m_blk_sz_words - 1 ] ;
//        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
//        {
//            pdst[ i ] -= pdst[ i - 1 ] ;
//        }
//    }
}



void GammaCrypt::DecryptBlock( uint16_t * t_invs_pma1 ) noexcept
{
    // -------------- XOR2 --------------
    #ifdef LFSR_ENBLD
    ( *mpShift )( mp_block_random.get() + m_blk_sz_words ) ;
    #endif
    
    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( mp_block_random.get() + m_blk_sz_words ) ; // this is CONFUSION
    #endif
    
    //todo1 Uncomment !
    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            mp_block_source[i] = mp_block_source[i] ^ mp_block_random[ i + offs_key2 / m_quantum_size ] ;
    #endif // XOR_ENBLD
        
    #ifdef DIFFUSION_ENBLD
        RemoveDiffusion( mp_block_source.get() ) ;
    #endif
    

    // -------------- REPOSITIONING --------------
    #ifdef PERMUTATE_ENBLD
        // transform pma2
        eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , m_op ) ;

        //rearrange pma1
        m_Permutate.eRearrangePMA1( mpu16e_temp_block_pma , m_Permutate.e_array2.get() ) ;


        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n Decrypt block: pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array[ i ] << ' ' ;
        std::cout << std::endl ;

        std::cout << "\n pma2 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << m_Permutate.e_array2[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif // DBG_INFO_ENBLD


        //inverse pma1
        m_Permutate.InverseExpPermutArr( t_invs_pma1 , m_Permutate.e_array.get() ) ;

        #ifdef DBG_INFO_ENBLD
        // cout pma
        std::cout << "\n After inverse: pma1 \n" ;
        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
            std::cout << t_invs_pma1[ i ] << ' ' ;
        std::cout << std::endl ;
        #endif
        //permutate
        m_Permutate.eRearrange( ( unsigned char*) mp_block_source.get() , mc_temp_block , t_invs_pma1
                , m_Permutate.epma_size_elms , m_block_size_bytes , m_perm_bytes ) ;
    #endif // PERMUTATE_ENBLD
    
    // -------------- XOR1 --------------
    #ifdef XOR_ENBLD
        memcpy( mc_temp_block , mp_block_random.get() , m_block_size_bytes ) ;
    #endif
    
    #ifdef LFSR_ENBLD
        ( *mpShift )( mp_block_random.get() ) ;
    #endif // LFSR_ENBLD

    #ifdef DIFFUSION_ENBLD
        MakeDiffusion( mp_block_random.get() ) ; // this is CONFUSION
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ; // todo1 Uncomment it!!!!
    #else
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            mp_block_dest[i] = mp_block_source[i] ;
    #endif // XOR_ENBLD
    
    #ifdef DIFFUSION_ENBLD
        RemoveDiffusion( mp_block_dest.get() ) ;
    #endif

    #ifdef XOR_ENBLD
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            mp_block_dest[i] = mp_block_dest[i] ^ ( (unsigned * ) (mc_temp_block) )[ i ] ; // todo1 Uncomment it!!!!
    #endif
    
}


void GammaCrypt::Decrypt()
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
        ReadOverheadData( m_ifs_keyfile ) ;
    else
        ReadOverheadData( m_ifs ) ;
    
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

    auto temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for PMA
    mc_temp_block = temp_block.get() ;
    //auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for ePMA
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
    mpu16e_temp_block_pma = e_temp_block_pma.get() ;
    
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
        m_ifs.read( (char*) mp_block_source.get(), m_block_size_bytes ) ;
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
        m_ofs.write( (char*) mp_block_dest.get() , bytes_to_write ) ;

    }
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Speed: " << float(m_header.source_file_size) / 1024 / 1024 / drtn * 1'000'000 << " MB/sec" << std::endl ;
            //
}



// в разработке многопоточная:
/*
void GammaCrypt::Decrypt()
{
    ReadHead( m_ifs ) ;
    
    m_block_size_bytes = m_header.h_block_size ;
    mb_decrypting = true ;
    mb_need_init_blocksize = false ;
    
    Initialize() ;

    MakePswBlock() ;

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

    auto temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for PMA
    //auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for ePMA
    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
    
    auto t_invs_pma1 = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ;

            // time mesaure
            auto mcs1 = std::chrono::high_resolution_clock::now() ;
            //

    auto ce_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes * m_Threading.m_Nthr) ; // for ePMA
	mce_temp_block = ce_temp_block.get() ;
    auto u16e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms * m_Threading.m_Nthr ) ; // for ePMA2

    // prepair to calculate all key1, key2 , pma1 , pma2
    unsigned n_blocks = m_Threading.m_Nthr * m_blks_per_thr ;
    mbuf_size = m_block_size_bytes * n_blocks ;
    
    unsigned * pkeys1 = nullptr , * pkeys2 = nullptr ;
    uint16_t * ppma1 = nullptr , * ppma2 = nullptr ;
    
    if ( mb_multithread )
    {
        m_upkeys1 = std::make_unique< t_block >( n_blocks * m_blk_sz_words ) ;
        pkeys1 = m_upkeys1.get() ;
        
        m_upkeys2 = std::make_unique< t_block >( n_blocks * m_blk_sz_words ) ;
        pkeys2 = m_upkeys2.get() ;
        
        m_uppma1 = std::make_unique< uint16_t[] >( n_blocks * m_Permutate.epma_size_elms ) ;
        ppma1 = m_uppma1.get() ;
        
        m_uppma2 = std::make_unique< uint16_t[] >( n_blocks * m_Permutate.epma_size_elms ) ;
        ppma2 = m_uppma2.get() ;
        
    }

   
    std::unique_ptr< char[] > upbuffer ; // up - Unique Ptr

    std::unique_ptr< char[] > upbuffout ; // up - Unique Ptr
    char * mpbuffout ;
    
    if ( ! mb_multithread )
    {
        mpbuffer = reinterpret_cast< char * > ( mp_block_source.get() ) ;
        mpbuffout = reinterpret_cast< char * > ( mp_block_dest.get() ) ;
        mbuf_size = m_block_size_bytes ;
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


    // operation for eTransformPMA2
    unsigned op = 0 ;
    
    int64_t i = m_header.source_file_size ;
    while ( i > 0 )
    {
        // READING
        m_ifs.read(  mpbuffer , mbuf_size ) ;
        unsigned bytes_read = m_ifs.gcount() ;
        if ( bytes_read == 0 )
            break ;
        
        n_blocks = bytes_read / m_block_size_bytes ;


        // calculate all key1, key2 , pma1 , pma2
        if ( mb_multithread )
        {
            for ( unsigned i = 0 ; i < n_blocks ; ++i )
            {
                // ----------- KEY1 -----------
                ( *mpShift )( mp_block_random.get() ) ;
                //memcpy( m_vkey1[ i ] , mp_block_random.get() , m_block_size_bytes ) ;
                memcpy( pkeys1 + i * m_blk_sz_words , mp_block_random.get() , m_block_size_bytes ) ;
            
                // ---------- PMA2, PMA1 ----------
                eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , op ) ;
                memcpy( ppma2 + i * m_Permutate.epma_size_elms , m_Permutate.e_array2.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                m_Permutate.eRearrangePMA1( u16e_temp_block_pma.get() , m_Permutate.e_array2.get() ) ;
                memcpy( ppma1 + i * m_Permutate.epma_size_elms , m_Permutate.e_array.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
                
                // ----------- KEY2 ------------
                ( *mpShift )( mp_block_random.get() + offs_key2 / m_quantum_size ) ;
                memcpy( pkeys2 + i * m_blk_sz_words , mp_block_random.get() + offs_key2 / m_quantum_size , m_block_size_bytes ) ;

            }
        }


        if ( mb_multithread )
            EncryptBlockMT( u16e_temp_block_pma.get() , mce_temp_block , n_blocks ) ;
        else
            EncryptBlock( u16e_temp_block_pma.get() , ce_temp_block.get() , op ) ;
        
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

        //OUT
        //m_ofs.write( (char*) mp_block_dest.get() , bytes_to_write ) ;
        m_ofs.write( (char*) mpbuffout , mbuf_size ) ;

    }
            // time mesaure
            auto mcs2 = std::chrono::high_resolution_clock::now() ;
            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
            uint64_t drtn = mcs.count() * 1000000 ;
            std::cout << "Duration: " << drtn << std::endl ;
            //
}
*/

void GammaCrypt::ReadHead( std::istream & ifs )
{
    //Header
    ifs.read( (char*) &m_header , sizeof( t_header ) ) ;
    if ( (unsigned) ifs.gcount() <  sizeof( t_header ) )
        throw ("error: File too short, missing header") ;
    if ( m_header.major_ver & 0x40 )
        m_perm_bytes = false ;
}

void GammaCrypt::ReadOverheadData( std::istream & ifs )
{
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_password != nullptr ) ;
    //Block_key
    auto block_key = std::make_unique< t_block >( m_blk_sz_words );
    //Key1
    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
        throw ("error: File too short, missing key block") ;

    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        mp_block_random[i] = block_key[i] ^ mp_block_password[i] ; // todo1 Uncomment it!!!!


    //Key2
    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
        throw ("error: File too short, missing key2 block") ;

    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
        mp_block_random[ i + offs_key2 / m_quantum_size ] = block_key[i] ^ mp_block_password[i] ;

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
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray.max_index ; ++i )
    {
        m_Permutate.e_array.get()[ i ] = m_Permutate.m_BIarray[ i ] ;
    }
	#ifdef DEBUG
		CheckPermutArr( m_Permutate.e_array.get() , m_Permutate.epma_size_elms ) ;
	#endif


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
    
    // build e_array, aka Expanded in the memory
    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray2.max_index ; ++i )
    {
        m_Permutate.e_array2.get()[ i ] = m_Permutate.m_BIarray2[ i ] ; // todo get out get()
    }
	#ifdef DEBUG
		CheckPermutArr( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms ) ;
	#endif

} ;

void GammaCrypt::WriteHead()
{
        assert( mp_block_random != nullptr ) ;
        assert( mp_block_password != nullptr ) ;
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
            block_key[i] = mp_block_random[i] ^ mp_block_password[i] ;
        m_ofs.write( (char*) block_key.get() , m_block_size_bytes ) ;
        //Key2
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
            block_key[i] = mp_block_random[ i + offs_key2 / m_quantum_size ] ^ mp_block_password[i] ;
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

void GammaCrypt::PMA_Xor_Psw( unsigned * p_pma_in , unsigned * p_pma_out )
{
    for ( unsigned i = 0 ; i < m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ; ++i  )
    {
        p_pma_out[ i ] = p_pma_in[ i ] ^ reinterpret_cast< unsigned * >( mpc_block_psw_pma.get() )[ i ]   ;
    }
    
} ;

//obsolete:
void GammaCrypt::Crypt()
{ ;
} ;


void GammaCrypt::SetBlockSize( unsigned block_size )
{
    if ( block_size > 128 )
        block_size = 128 ;
    mb_need_init_blocksize = false ;
    unsigned temp = block_size ;
    unsigned log2 = 0 ; 
    while ( temp >>= 1 ) log2 ++ ;
    m_block_size_bytes = 1 ;
    for ( unsigned i = 0 ; i < log2 ; ++i )
        m_block_size_bytes <<= 1 ;
    if ( m_block_size_bytes < 8)
        m_block_size_bytes = 8 ;
}


void GammaCrypt::mGenerateRandoms()
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
        GenerateRandoms( m_rnd_size_words , mp_block_random.get() , std::ref( ticks ) ) ;
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
        mp_block_random[ i ] ^= vec[ i ] ;
}

void GammaCrypt::DisplayInfo()
{
    std::ostringstream oss ;
    oss << "\n source file size (bytes): " << m_header.source_file_size ;
    oss << "\n block size (bytes): " << m_block_size_bytes ;
    oss << "\n permutation array size (bytes): " << m_Permutate.m_BIarray.pma_size_bytes ;
    oss << "\n number of permutation array elements: " << m_Permutate.m_BIarray.max_index ;
    oss << "\n size of one permutation array element (bits): " << m_Permutate.m_BIarray.index_size_bits ;
    oss << "\n hardware_concurrency: " << m_hrdw_concr ;
    oss << "\n" ;
    display_str( oss.str() ) ;
}

//
//
//
//void GammaCrypt::DecryptBlock( uint16_t * e_temp_block_pma , unsigned char * e_temp_block , uint16_t * t_invs_pma1 ) noexcept
//{
//    #ifdef XOR_ENBLD
//    // XOR2
//    #ifdef LFSR_ENBLD
//    ( *mpShift )( mp_block_random.get() + m_blk_sz_words ) ;
//    #endif
//    
//    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//        mp_block_source[i] = mp_block_source[i] ^ mp_block_random[ i + offs_key2 / m_quantum_size ] ;
//    
//    #endif // XOR_ENBLD
//
//    #ifdef PERMUTATE_ENBLD
//    //REPOSITIONING
//    
//    // transform pma2
//    eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , m_op ) ;
//
//    //rearrange pma1
//    m_Permutate.eRearrangePMA1( e_temp_block_pma , m_Permutate.e_array2.get() ) ;
//    //inverse pma1
//    m_Permutate.InverseExpPermutArr( t_invs_pma1 , m_Permutate.e_array.get() ) ;
//    eRearrange( ( unsigned char*) mp_block_source.get() , e_temp_block , t_invs_pma1
//            , m_Permutate.epma_size_elms , m_block_size_bytes ) ;
//
//    #endif // PERMUTATE_ENBLD
//    
//    #ifdef XOR_ENBLD
//    // XOR1
//
//    #ifdef LFSR_ENBLD
//    ( *mpShift )( mp_block_random.get() ) ;
//    #endif // LFSR_ENBLD
//
//    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//        mp_block_dest[i] = mp_block_source[i] ^ mp_block_random[i] ;
//
//    RemoveDiffusion( mp_block_dest.get() ) ;
//        
//    #else
//    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//        mp_block_dest[i] = mp_block_source[i] ;
//    #endif // XOR_ENBLD
//    
//    
//}
//
//
//void GammaCrypt::Decrypt()
//{
//    mb_use_keyfile = false ;
//    ReadHead( m_ifs ) ;
//    if ( m_header.major_ver & 0x80 )
//        mb_use_keyfile = true ;
//    
//    
//    if ( mb_use_keyfile )
//    {
//        m_ifs_keyfile.open( m_keyfilename.c_str() , std::ifstream::in | std::ifstream::binary ) ;
//        if ( ! m_ifs_keyfile.is_open() )
//        {
//            //display_str( help_string ) ;
//            throw( "Error opening key file" ) ;
//        }
//        uint64_t temp_source_file_size = m_header.source_file_size ; // save source_file_size
//        ReadHead( m_ifs_keyfile ) ;
//        m_header.source_file_size = temp_source_file_size ;
//    }
//
//    m_block_size_bytes = m_header.h_block_size ;
//    mb_decrypting = true ;
//    mb_need_init_blocksize = false ;
//    
//    Initialize() ;
//
//    MakePswBlock() ;
//
//    if ( mb_use_keyfile )
//        ReadOverheadData( m_ifs_keyfile ) ;
//    else
//        ReadOverheadData( m_ifs ) ;
//    
//        #ifdef DBG_INFO_ENBLD
//        // cout pma
//        std::cout << "\n pma1 \n" ;
//        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//            std::cout << m_Permutate.e_array[ i ] << ' ' ;
//        std::cout << std::endl ;
//
//        std::cout << "\n pma2 \n" ;
//        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//            std::cout << m_Permutate.e_array2[ i ] << ' ' ;
//        std::cout << std::endl ;
//        #endif // DBG_INFO_ENBLD
//
//    DisplayInfo() ;
//    
//    // CRYPT  Crypt() ;
//        assert( mp_block_random != nullptr ) ;
//        assert( mp_block_source != nullptr ) ;
//        assert( mp_block_dest != nullptr ) ;
//
//    auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for PMA
//    //auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for ePMA
//    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
//    
//    auto t_invs_pma1 = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ;
//
//            // time mesaure
//            auto mcs1 = std::chrono::high_resolution_clock::now() ;
//            //
//
//    // operation for eTransformPMA2
//    m_op = 0 ;
//    
//    int64_t i = m_header.source_file_size ;
//    while ( i > 0 )
//    {
//        // READING
//        m_ifs.read( (char*) mp_block_source.get(), m_block_size_bytes ) ;
//        unsigned bytes_read = m_ifs.gcount() ;
//        if ( bytes_read == 0 )
//            break ;
//            
//        DecryptBlock( e_temp_block_pma.get() , e_temp_block.get() , t_invs_pma1.get() ) ;
//        
//        //it should not be the tail now
//        // checking tail size
//        unsigned bytes_to_write ;
//        i -= m_block_size_bytes ;
//        if ( i >=0 )
//            bytes_to_write = bytes_read ;
//        else
//        {
//            //assert( false ) ;
//            bytes_to_write = i + m_block_size_bytes ;
//        }
//        
///*        m_ifs.peek() ; 
//        if ( ! m_ifs.good() ) // Проверяем конец файла
//            bytes_to_write = m_header.tail_size ;
//             */
//        //OUT
//        m_ofs.write( (char*) mp_block_dest.get() , bytes_to_write ) ;
//
//    }
//            // time mesaure
//            auto mcs2 = std::chrono::high_resolution_clock::now() ;
//            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
//            uint64_t drtn = mcs.count() * 1000000 ;
//            std::cout << "Speed: " << float(m_header.source_file_size) / 1024 / 1024 / drtn * 1'000'000 << " MB/sec" << std::endl ;
//            //
//}
//
//
//
//// в разработке многопоточная:
///*
//void GammaCrypt::Decrypt()
//{
//    ReadHead( m_ifs ) ;
//    
//    m_block_size_bytes = m_header.h_block_size ;
//    mb_decrypting = true ;
//    mb_need_init_blocksize = false ;
//    
//    Initialize() ;
//
//    MakePswBlock() ;
//
//    ReadOverheadData() ;
//    
//        #ifdef DBG_INFO_ENBLD
//        // cout pma
//        std::cout << "\n pma1 \n" ;
//        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//            std::cout << m_Permutate.e_array[ i ] << ' ' ;
//        std::cout << std::endl ;
//
//        std::cout << "\n pma2 \n" ;
//        for ( unsigned i = 0 ; i < m_Permutate.epma_size_elms ; i ++)
//            std::cout << m_Permutate.e_array2[ i ] << ' ' ;
//        std::cout << std::endl ;
//        #endif // DBG_INFO_ENBLD
//
//    DisplayInfo() ;
//    
//    // CRYPT  Crypt() ;
//        assert( mp_block_random != nullptr ) ;
//        assert( mp_block_source != nullptr ) ;
//        assert( mp_block_dest != nullptr ) ;
//
//    auto temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for PMA
//    //auto e_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes ) ; // for ePMA
//    auto e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ; // for ePMA2
//    
//    auto t_invs_pma1 = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms ) ;
//
//            // time mesaure
//            auto mcs1 = std::chrono::high_resolution_clock::now() ;
//            //
//
//    auto ce_temp_block = std::make_unique< unsigned char[] >( m_block_size_bytes * m_Threading.m_Nthr) ; // for ePMA
//	mce_temp_block = ce_temp_block.get() ;
//    auto u16e_temp_block_pma = std::make_unique< uint16_t[] >( m_Permutate.epma_size_elms * m_Threading.m_Nthr ) ; // for ePMA2
//
//    // prepair to calculate all key1, key2 , pma1 , pma2
//    unsigned n_blocks = m_Threading.m_Nthr * m_blks_per_thr ;
//    mbuf_size = m_block_size_bytes * n_blocks ;
//    
//    unsigned * pkeys1 = nullptr , * pkeys2 = nullptr ;
//    uint16_t * ppma1 = nullptr , * ppma2 = nullptr ;
//    
//    if ( mb_multithread )
//    {
//        m_upkeys1 = std::make_unique< t_block >( n_blocks * m_blk_sz_words ) ;
//        pkeys1 = m_upkeys1.get() ;
//        
//        m_upkeys2 = std::make_unique< t_block >( n_blocks * m_blk_sz_words ) ;
//        pkeys2 = m_upkeys2.get() ;
//        
//        m_uppma1 = std::make_unique< uint16_t[] >( n_blocks * m_Permutate.epma_size_elms ) ;
//        ppma1 = m_uppma1.get() ;
//        
//        m_uppma2 = std::make_unique< uint16_t[] >( n_blocks * m_Permutate.epma_size_elms ) ;
//        ppma2 = m_uppma2.get() ;
//        
//    }
//
//   
//    std::unique_ptr< char[] > upbuffer ; // up - Unique Ptr
//
//    std::unique_ptr< char[] > upbuffout ; // up - Unique Ptr
//    char * mpbuffout ;
//    
//    if ( ! mb_multithread )
//    {
//        mpbuffer = reinterpret_cast< char * > ( mp_block_source.get() ) ;
//        mpbuffout = reinterpret_cast< char * > ( mp_block_dest.get() ) ;
//        mbuf_size = m_block_size_bytes ;
//    }
//    else
//    {
//        upbuffer = std::make_unique< char[] >( mbuf_size ) ;
//        mpbuffer = upbuffer.get() ;
//
//        upbuffout = std::make_unique< char[] >( mbuf_size ) ;
//        mpbuffout = upbuffout.get() ;
//        
//        params.p_source = reinterpret_cast< unsigned * >( mpbuffer ) ;
//        params.p_dest = reinterpret_cast< unsigned * >( mpbuffout ) ;
//    }
//
//
//    // operation for eTransformPMA2
//    unsigned op = 0 ;
//    
//    int64_t i = m_header.source_file_size ;
//    while ( i > 0 )
//    {
//        // READING
//        m_ifs.read(  mpbuffer , mbuf_size ) ;
//        unsigned bytes_read = m_ifs.gcount() ;
//        if ( bytes_read == 0 )
//            break ;
//        
//        n_blocks = bytes_read / m_block_size_bytes ;
//
//
//        // calculate all key1, key2 , pma1 , pma2
//        if ( mb_multithread )
//        {
//            for ( unsigned i = 0 ; i < n_blocks ; ++i )
//            {
//                // ----------- KEY1 -----------
//                ( *mpShift )( mp_block_random.get() ) ;
//                //memcpy( m_vkey1[ i ] , mp_block_random.get() , m_block_size_bytes ) ;
//                memcpy( pkeys1 + i * m_blk_sz_words , mp_block_random.get() , m_block_size_bytes ) ;
//            
//                // ---------- PMA2, PMA1 ----------
//                eTransformPMA2( m_Permutate.e_array2.get() , m_Permutate.epma_size_elms , op ) ;
//                memcpy( ppma2 + i * m_Permutate.epma_size_elms , m_Permutate.e_array2.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
//                m_Permutate.eRearrangePMA1( u16e_temp_block_pma.get() , m_Permutate.e_array2.get() ) ;
//                memcpy( ppma1 + i * m_Permutate.epma_size_elms , m_Permutate.e_array.get() , m_Permutate.epma_size_elms * sizeof( uint16_t ) ) ;
//                
//                // ----------- KEY2 ------------
//                ( *mpShift )( mp_block_random.get() + offs_key2 / m_quantum_size ) ;
//                memcpy( pkeys2 + i * m_blk_sz_words , mp_block_random.get() + offs_key2 / m_quantum_size , m_block_size_bytes ) ;
//
//            }
//        }
//
//
//        if ( mb_multithread )
//            EncryptBlockMT( u16e_temp_block_pma.get() , mce_temp_block , n_blocks ) ;
//        else
//            EncryptBlock( u16e_temp_block_pma.get() , ce_temp_block.get() , op ) ;
//        
//        //it should not be the tail now
//        // checking tail size
//        unsigned bytes_to_write ;
//        i -= m_block_size_bytes ;
//        if ( i >=0 )
//            bytes_to_write = bytes_read ;
//        else
//        {
//            //assert( false ) ;
//            bytes_to_write = i + m_block_size_bytes ;
//        }
//
//        //OUT
//        //m_ofs.write( (char*) mp_block_dest.get() , bytes_to_write ) ;
//        m_ofs.write( (char*) mpbuffout , mbuf_size ) ;
//
//    }
//            // time mesaure
//            auto mcs2 = std::chrono::high_resolution_clock::now() ;
//            std::chrono::duration<double> mcs = mcs2 - mcs1 ;
//            uint64_t drtn = mcs.count() * 1000000 ;
//            std::cout << "Duration: " << drtn << std::endl ;
//            //
//}
//*/
//
//void GammaCrypt::ReadHead( std::istream & ifs )
//{
//    //Header
//    ifs.read( (char*) &m_header , sizeof( t_header ) ) ;
//    if ( (unsigned) ifs.gcount() <  sizeof( t_header ) )
//        throw ("error: File too short, missing header") ;
//}
//
//void GammaCrypt::ReadOverheadData( std::istream & ifs )
//{
//        assert( mp_block_random != nullptr ) ;
//        assert( mp_block_password != nullptr ) ;
//    //Block_key
//    auto block_key = std::make_unique< t_block >( m_blk_sz_words );
//    //Key1
//    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
//    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
//        throw ("error: File too short, missing key block") ;
//
//    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//        mp_block_random[i] = block_key[i] ^ mp_block_password[i] ;
//
//
//    //Key2
//    ifs.read( (char*) block_key.get() , m_block_size_bytes ) ;
//    if ( (unsigned) ifs.gcount() <  m_block_size_bytes )
//        throw ("error: File too short, missing key2 block") ;
//
//    for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//        mp_block_random[ i + offs_key2 / m_quantum_size ] = block_key[i] ^ mp_block_password[i] ;
//
//    // Permutation Array 1
//    ifs.read( (char*) m_Permutate.m_BIarray.m_array.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;
//    if ( (unsigned) ifs.gcount() <  m_Permutate.m_BIarray.pma_size_bytes )
//        throw ("error: File too short, missing permutation array block") ;
//        
//    unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / sizeof(unsigned) ;
//    if ( m_Permutate.m_BIarray.pma_size_bytes % sizeof( unsigned ) != 0  )
//        block_pma_size_words++ ;
//
//    auto block_pma_xored = std::make_unique< unsigned[] >( block_pma_size_words ) ;
//    PMA_Xor_Psw( (unsigned *) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
//    memcpy( m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray.pma_size_bytes ) ;
//
//    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray ) ;
//    
//    // build e_array, aka Expanded in the memory
//    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray.max_index ; ++i )
//    {
//        m_Permutate.e_array.get()[ i ] = m_Permutate.m_BIarray[ i ] ;
//    }
//
//
//    // Permutation Array 2
//    ifs.read( (char*) m_Permutate.m_BIarray2.m_array.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;
//    if ( (unsigned) ifs.gcount() <  m_Permutate.m_BIarray2.pma_size_bytes )
//        throw ("error: File too short, missing permutation array block") ;
//        
//    block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / sizeof(unsigned) ;
//    if ( m_Permutate.m_BIarray2.pma_size_bytes % sizeof( unsigned ) != 0  )
//        block_pma_size_words++ ;
//
//    //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
//    PMA_Xor_Psw( (unsigned *) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
//    memcpy( m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() , m_Permutate.m_BIarray2.pma_size_bytes ) ;
//
//    //m_Permutate.InversePermutArr( m_Permutate.m_BIarray2 ) ;
//    
//    // build e_array, aka Expanded in the memory
//    for ( uint16_t i = 0 ; i < m_Permutate.m_BIarray2.max_index ; ++i )
//    {
//        m_Permutate.e_array2.get()[ i ] = m_Permutate.m_BIarray2[ i ] ; // todo get out get()
//    }
//
//} ;
//
//void GammaCrypt::WriteHead()
//{
//        assert( mp_block_random != nullptr ) ;
//        assert( mp_block_password != nullptr ) ;
//    if ( mb_use_keyfile )
//    {
//        // set high bit of m_header.major_ver :
//        m_header.major_ver |= 0x80 ;
//    }
//    //Header
//    m_header.data_offset = sizeof( t_header ) ;
//    m_header.h_block_size = m_block_size_bytes ;
//    m_ofs.write( (char*) &m_header , sizeof( t_header ) ) ;
//    
//
//    if ( ! mb_use_keyfile )
//    {
//        //Block_key
//        auto block_key = std::make_unique< t_block >( m_blk_sz_words );
//        //Key1
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            block_key[i] = mp_block_random[i] ^ mp_block_password[i] ;
//        m_ofs.write( (char*) block_key.get() , m_block_size_bytes ) ;
//        //Key2
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            block_key[i] = mp_block_random[ i + offs_key2 / m_quantum_size ] ^ mp_block_password[i] ;
//        m_ofs.write( (char*) block_key.get() , m_block_size_bytes ) ;
//
//        
//        //Permutation array 1
//        // 1) XOR with password_block
//        unsigned block_pma_size_words = m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ;
//        if ( m_Permutate.m_BIarray.pma_size_bytes % m_quantum_size != 0 )
//            block_pma_size_words++ ;
//        auto block_pma_xored = std::make_unique< unsigned[] >( block_pma_size_words ) ;
//        PMA_Xor_Psw( ( unsigned * ) m_Permutate.m_BIarray.m_array.get() , block_pma_xored.get() ) ;
//        // 2) write to file
//        m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
//        
//        //Permutation array 2
//        // 1) XOR with password_block
//        block_pma_size_words = m_Permutate.m_BIarray2.pma_size_bytes / m_quantum_size ;
//        if ( m_Permutate.m_BIarray2.pma_size_bytes % m_quantum_size != 0 )
//            block_pma_size_words++ ;
//        //auto block_pma_xored = make_unique< unsigned[] >( block_pma_size_words ) ;
//        PMA_Xor_Psw( ( unsigned * ) m_Permutate.m_BIarray2.m_array.get() , block_pma_xored.get() ) ;
//        // 2) write to file
//        m_ofs.write( (char*) block_pma_xored.get() , block_pma_size_words * m_quantum_size ) ;
//    }
//}
//
//void GammaCrypt::PMA_Xor_Psw( unsigned * p_pma_in , unsigned * p_pma_out )
//{
//    assert( m_Permutate.m_BIarray.pma_size_bytes % m_block_size_bytes == 0 ) ;
//    
//    for ( unsigned i = 0 ; i < m_Permutate.m_BIarray.pma_size_bytes / m_quantum_size ; ++i  )
//    {
//        p_pma_out[ i ] = p_pma_in[ i ] ^ reinterpret_cast< unsigned * >( mpc_block_psw_pma.get() )[ i ]   ;
//    }
//    
//} ;
//
//void GammaCrypt::Crypt()
//{ ;
//} ;
//
//
//void GammaCrypt::SetBlockSize( unsigned block_size )
//{
//    if ( block_size > 128 )
//        block_size = 128 ;
//    mb_need_init_blocksize = false ;
//    unsigned temp = block_size ;
//    unsigned log2 = 0 ; 
//    while ( temp >>= 1 ) log2 ++ ;
//    m_block_size_bytes = 1 ;
//    for ( unsigned i = 0 ; i < log2 ; ++i )
//        m_block_size_bytes <<= 1 ;
//    if ( m_block_size_bytes < 8)
//        m_block_size_bytes = 8 ;
//}
//
//
//void GammaCrypt::mGenerateRandoms()
//{
//    std::atomic< bool > bstop ( false ) ;
//    //timer
//    std::atomic< uint64_t > ticks( 0 ) ;
//
//    std::thread thr_tick( TickTimer , std::ref( ticks ) , std::ref( bstop ) ) ;
//    
//    std::vector<std::thread> threads;
//    for( unsigned i = 0; i < m_hrdw_concr - 1 ; i++ ) 
//    {
//        std::thread thr_cfl( CPUFullLoad , std::ref( bstop ) ) ; // thread CPU Loading at 100%
//        threads.emplace_back( std::move( thr_cfl ) ) ;
//    }
//
//    try
//    {
//        GenerateRandoms( m_rnd_size_words , mp_block_random.get() , std::ref( ticks ) ) ;
//    }
//    catch( const char * s)
//    {
//        bstop = true ;
//        thr_tick.join() ;
//        for ( auto & thr : threads ) 
//            thr.join();
//
//        throw s ;
//    }
//    
//    bstop = true ;
//    thr_tick.join() ;
//    for ( auto & thr : threads ) 
//        thr.join();
//
//    // XOR std::random
//    std::random_device rndDevice;
//    std::mt19937 eng(rndDevice());
//    std::uniform_int_distribution< unsigned > dist( 0 , 0xFFFF'FFFF );
//    auto gen = std::bind(dist, eng);
//    
//    std::vector< unsigned > vec( m_rnd_size_words );
//    std::generate(begin(vec), end(vec), gen);
//    
//    for ( unsigned i = 0 ; i < m_rnd_size_words ; ++ i )
//        mp_block_random[ i ] ^= vec[ i ] ;
//}
//
//void GammaCrypt::DisplayInfo()
//{
//    std::ostringstream oss ;
//    oss << "\n source file size (bytes): " << m_header.source_file_size ;
//    oss << "\n block size (bytes): " << m_block_size_bytes ;
//    oss << "\n permutation array size (bytes): " << m_Permutate.m_BIarray.pma_size_bytes ;
//    oss << "\n number of permutation array elements: " << m_Permutate.m_BIarray.max_index ;
//    oss << "\n size of one permutation array element (bits): " << m_Permutate.m_BIarray.index_size_bits ;
//    oss << "\n hardware_concurrency: " << m_hrdw_concr ;
//    oss << "\n" ;
//    display_str( oss.str() ) ;
//}
