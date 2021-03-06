
#include "../platform.h"
#include "randomroutines.h"
#include "../Display/ConsoleDisplay/include/display.h"

#include <chrono>
#include <cstring>
#include <cassert>
#include <exception>
#include <algorithm>
#include <memory>
#include <fstream>
#include <iostream>
#include <vector>

#include <thread> //for sleep

#ifdef WINDOWS
#include <windows.h>
#endif


//---------------------------------------------------------------------
//GENERATING RANDOM

/*
 * obsolete :

// отклонение от идеально равномерного распределения, процентов
const unsigned deviation = 40 ;

// make statistics distribution (статистическое распределение) lower two bits (shifting left on nshift bits)
// if distribution is even (равномерное, допускаю отклонение в deviation % ) returns true
// N - размер массива
// nshift - начиная от какого (с младшей стороны) бита брать 4 бита
bool MakeDistrbution_4lowbits( unsigned const N , t_block const rvals , unsigned nshift )
{
    const size_t dsize = 16 ;
    unsigned dvals[ dsize ] ;
    for ( unsigned i = 0 ; i < dsize ; i++ )
        dvals[i] = 0 ;

    unsigned umask = 0xf ;
    umask <<= nshift ;
    
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        unsigned index = rvals[ i ]  & umask ;
        index >>= nshift ;
        dvals[ index ] ++ ;
    }
    
    #ifdef DBG_INFO_ENBLD
    std::cout << "\n bit" << nshift << ' ' ;
    std::cout << "statistics:\n" ;
    for ( auto dval : dvals )
        std::cout << dval << ' ' ;
    #endif
    
    for ( auto dval : dvals )
        if ( dval < N * ( 100 - deviation ) / 100 / dsize  ||  dval > N * ( 100 + deviation ) / 100 / dsize  )
            return false ;

    return true ;
}

// make statistics distribution (статистическое распределение) lower two bits (shifting left on nshift bits)
// if distribution is even (равномерное, допускаю отклонение в deviation % ) returns true
// N - размер массива
// nshift - начиная от какого (с младшей стороны) бита брать 5 бит
bool MakeDistrbution_5lowbits( unsigned const N , t_block const rvals , unsigned nshift )
{
    const size_t dsize = 32 ;
    unsigned dvals[ dsize ] ;
    for ( unsigned i = 0 ; i < dsize ; i++ )
        dvals[i] = 0 ;

    unsigned umask = 0x1f ;
    umask <<= nshift ;
    
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        unsigned index = rvals[ i ]  & umask ;
        index >>= nshift ;
        dvals[ index ] ++ ;
    }
    
    #ifdef DBG_INFO_ENBLD
    std::cout << "\n bit" << nshift << ' ' ;
    std::cout << "statistics:\n" ;
    for ( auto dval : dvals )
        std::cout << dval << ' ' ;
    #endif

    // Вообще я полагаю отклонение представляет из себя нормальное распределение
    // Пусть четверть элементов может иметь отклонение в полтора раза больше допустимого
    int counter = 0 ;

    for ( auto dval : dvals )
    {
        if ( dval < N * ( 100 - deviation ) / 100 / dsize  ||  dval > N * ( 100 + deviation ) / 100 / dsize  )
        {
            if ( counter > 8 )
                return false ;
            else
            {
                counter++ ;
                if ( dval < N * ( 100 - deviation*3/2 ) / 100 / dsize  ||  dval > N * ( 100 + deviation*3/2 ) / 100 / dsize )
                    return false ;
            }
        }
    }

    return true ;
}
*/

//generate randoms using hdd operatios
// N - array size // размер массива
void GenRandByHdd( unsigned const N , t_block rvals , std::atomic< uint64_t > & ticks )
{
    unsigned sleep_drtn = 1 ;
    //rarray_t rvals ;
    uint64_t mcs1 = ticks ;
    uint64_t mcs2 ;
    for ( unsigned i = 0 ; i < N/2 ; ++i )
    {
        unsigned rval = 0 ; //random value
        for ( int j = 0 ; j < 1000 && rval == 0; ++j )
        {
            std::ofstream ofs ;
            ofs.open( "temp_1", std::ios::out | std::ios::trunc) ;
            ofs << "ttt" ;
            std::this_thread::sleep_for( std::chrono::microseconds( sleep_drtn ) ) ;
            ofs.flush() ;
            ofs.close() ;
            
            mcs2 = ticks ;
            rval = mcs2 - mcs1 ; 
            mcs1 = mcs2 ;
        
        }
        
        rvals[ i*2 ] = rval ;

        {
            rval = 0 ; //random value
            for ( int j = 0 ; j < 1000 && rval == 0; ++j )
            {
                std::ofstream ofs2 ;
                ofs2.open( "temp_2", std::ios::out | std::ios::trunc) ;
                ofs2 << "ttt" ;
                std::this_thread::sleep_for( std::chrono::microseconds( sleep_drtn ) ) ;
                ofs2.flush() ;
                ofs2.close() ;

                mcs2 = ticks ;
                rval = mcs2 - mcs1 ; //random value
                mcs1 = mcs2 ;
            }
            
            rvals[ i*2 +1 ] = rval ;
        }
    } ;
}

/*
 obsolete :
//generate randoms using hdd operatios
// N - array size // размер массива
void GenRandByHdd( unsigned const N , t_block rvals )
{
    unsigned sleep_drtn = 300 ;
    #ifdef WINDOWS
    sleep_drtn = 8000 ;
    #endif
    //rarray_t rvals ;
    auto mcs1 = std::chrono::high_resolution_clock::now() ;
    for ( unsigned i = 0 ; i < N/2 ; ++i )
    {
        
        std::ofstream ofs ;
        ofs.open( "temp_1", std::ios::out | std::ios::trunc) ;
        ofs << "ttt" ;
        std::this_thread::sleep_for( std::chrono::microseconds( sleep_drtn ) ) ;
        ofs.flush() ;
        ofs.close() ;
        
        auto mcs2 = std::chrono::high_resolution_clock::now() ;
        std::chrono::duration<double> mcs = mcs2 - mcs1 ;
        mcs1 = mcs2 ;
        unsigned rval ; //random value
        rval = mcs.count() * 1'000'000'000 ;
        
        rvals[ i*2 ] = rval ;

        {
            std::ofstream ofs2 ;
            ofs2.open( "temp_2", std::ios::out | std::ios::trunc) ;
            ofs2 << "ttt" ;
            std::this_thread::sleep_for( std::chrono::microseconds( sleep_drtn ) ) ;
            ofs2.flush() ;
            ofs2.close() ;

            mcs2 = std::chrono::high_resolution_clock::now() ;
            mcs = mcs2 - mcs1 ;
            mcs1 = mcs2 ;
            rval = mcs.count() * 1'000'000'000 ;
            
            rvals[ i*2 +1 ] = rval ;
        }
    }
}
*/

// определить старший, установленный в 1, бит
int DetermineHighSetBit( unsigned const val )
{
    if ( val > 0 )
    {
        unsigned umask = 0x8000'0000 ; //  1000 0000 0000 0000   0000 0000 0000 0000
        int n = 31 ;
        for ( n = 31 ; n >= 0 ; n-- , umask >>= 1 )
            if ( (val & umask) > 0 )
                return n ;
    }
    throw "error: too small random value to perform DetermineHighSetBit" ;
}

// Вычислить не среднее арифметическое, а "четверть арифметическое", то есть среднее африфметическое пополам
unsigned CalcQuadAverage( unsigned const N , t_block const rvals )
{
    unsigned result = 0 ;
    for ( unsigned i = 0 ; i < N ; ++i )
    {
        result += rvals[ i ] ;
    }
    return result / N / 2 ;
}

/*
//Определить количество младших незначащих битов
//Возвращает номер (считая от нуля) первого значащего бита
//возвращает -1, если вообще не получились случайные числа
int DetermineNonsignificantLowBits( unsigned const N , t_block const rvals )
{
    int HighSetBit = DetermineHighSetBit( CalcQuadAverage( N , rvals ) ) ;
    if ( HighSetBit < 1 )  // как минимум д.б. 2 значимых бита
        throw "error: too small random value to processing DetermineNonsignificantLowBits" ;
    int i = 0 ;
    for ( i = 0 ; i <= HighSetBit - 4 ; i++ )
        if ( MakeDistrbution_5lowbits( N , rvals , i ) == 1 )
            return i ;
    throw "error: fail to make statistic distribution" ;
}
*/

void CropRandoms( unsigned const N , t_block rvals )
{
    // вычтем минимальное время, чтобы получилось отклонение.
    unsigned * pmin = std::min_element( rvals , rvals + N ) ;
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        if (rvals[ i ] != *pmin)
        {
            rvals[ i ] -= *pmin * 2/3 ;
        }
    }

        #ifdef DBG_INFO_ENBLD
        std::cout << " Randoms after sub min : \n" ;
        for ( unsigned i = 0 ; i < N ; ++ i )
        {
            std::cout << rvals[ i ] << ' ' ;
        }
        #endif

/*    //уберём младшие "малозначащие" биты
    unsigned LowSignificantBit = DetermineNonsignificantLowBits( N ,rvals ) ;
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        rvals[ i ] >>= LowSignificantBit ;
    }
*/
}

// Взять из полученных случайных всё полезное и сформировать непрерывную кнаку
// Возвращает длину кнаки в словах (unsigned)
unsigned CompoundContiguousRandoms( unsigned const rsize , t_block block , unsigned const N , t_block const rvals )
{
    unsigned current_bit_offset = 0 ;
    unsigned current_block_index = 0 ;
    
    for ( unsigned i = 0 ; i < N ; ++i )
    {
        if ( rvals[ i ] < 2 ) // тогда и брать нечего
            continue ;
        int rsize_bits = DetermineHighSetBit( rvals[ i ] ) ;
        
        uint64_t rval = rvals[i] ;
        uint64_t umask = ~ ( 1 << rsize_bits ) ;
        rval &= umask ;
        rsize_bits-- ; // т.к. сбросили старшую единицу
        rval = rval << current_bit_offset ; //сдвигаю влево, может логичнее было бы двигать вправо, но т.к. у нас ГСЧ, то значения никакого не имеет
        
        block[ current_block_index ] |= unsigned ( rval & 0xFFFF'FFFF );
        if ( current_bit_offset + rsize_bits > 31 ) // got out of 32 bits / вылез за 32 бита
        {
            if ( current_block_index + 1 >= rsize )
            {
                current_block_index++ ;
                break ;
            }
            else
                block[ current_block_index +1 ] |= unsigned ( rval >> 32 ) ;
        }
        assert( current_block_index < rsize ) ;
        
        current_bit_offset += rsize_bits ;
        if ( current_bit_offset > 31 )
        {
            current_bit_offset -= 32 ;
            current_block_index++ ;
        }
        if ( current_block_index  >= rsize )
            break ;
    }
    
    return current_block_index ;
}

void TickTimer( std::atomic< uint64_t > & ticks , std::atomic< bool > & bstop )
{
    while ( true )
    {
        ++ ticks ;
        if ( bstop )
            break ;
    }
}

// looped function
void CPUFullLoad( std::atomic< bool > & bstop )
{
    //unsigned temp ;
    while ( true )
    {
        if ( bstop )
            break ;
        {
            std::ofstream ofs ;
            ofs.open( "temp_1", std::ios::out | std::ios::trunc) ;
            ofs << "ttt" ;
            std::this_thread::sleep_for( std::chrono::microseconds( 200 ) ) ;
            ofs.flush() ;
            ofs.close() ;
            
            std::ofstream ofs2 ;
            ofs2.open( "temp_2", std::ios::out | std::ios::trunc) ;
            ofs2 << "ttt" ;
            std::this_thread::sleep_for( std::chrono::microseconds( 200 ) ) ;
            ofs2.flush() ;
            ofs2.close() ;

        }
    } ;
}

//Генерировать блок СЧ заданной длины в словах (unsigned)
//rsize (random block size) - размер массива
void GenerateRandoms( unsigned const rsize , t_block randoms , std::atomic< uint64_t > & ticks )
{
    unsigned alignedsize = 192 ; // минимальный размер массива для ГСЧ
    if ( alignedsize < rsize)
        alignedsize = rsize ; 
    unsigned N = alignedsize * 4 ;
    while ( 1 )
    {
        auto rvals = std::make_unique< t_block >( N ) ;
        GenRandByHdd( N , rvals.get() , ticks ) ;

        #ifdef DBG_INFO_ENBLD
        std::cout << " Randoms by HDD : \n" ;
        for ( unsigned i = 0 ; i < N ; ++ i )
        {
            std::cout << rvals[ i ] << ' ' ;
        }
        /*std::cout << std::endl << " Randoms by HDD in hex : \n" ;
        for ( unsigned i = 0 ; i < N ; ++ i )
        {
            std::cout << std::hex << rvals[ i ] << ' ' ;
        }*/
        std::cout << std::dec << std::endl ;
        #endif
        
        //CropRandoms( N ,rvals.get() ) ; // it's ought not to do now 


        unsigned received_rsize = CompoundContiguousRandoms( rsize , randoms , N , rvals.get() ) ;
        if ( received_rsize == rsize ) 
            break;
        rvals = std::make_unique< t_block >( N ) ;
        N *= 2 ;
        display_str( "Too few random data, starting new cycle of generating..." ) ;
        // Ну уж слишком много тоже нельзя:
        if ( N > 1024*1024 ) // это как никак минимум 4 мегабайта
            throw( "error: fail to compound randoms, too big array" ) ;
    }
    
}

//END OF GENERATING RANDOM
//-----------------------------------------------


void TransformRandomCycle( unsigned * block_random , const size_t n_quantum )
{
        assert( block_random != nullptr ) ;
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
    *( block_random + n_quantum -1 ) = (( *(block_random + n_quantum -1) & 0x80000000 ) ^ ( first_bit << 31 )) | ( *(block_random+n_quantum -1) & 0x7FFFFFFF ) ;
}



//------------- REPOSITIONING --------------------


void Permutate::eRearrange( unsigned char * p8_block , unsigned char * temp8_block , uint16_t * p16_pm_earr , unsigned epma_size_elms , unsigned block_size_bytes , bool perm_bytes ) noexcept
{
    if ( perm_bytes )
    {
        RearrangeBytes( p8_block , p16_pm_earr , temp8_block ) ;
        return ;
    }

    memset( temp8_block , 0 , block_size_bytes ) ; // todo out away , нельзя убирать, т к биты задаются операцией |=, т е исходный бит д.б. сброшен

    for ( uint16_t i = 0 ; i < epma_size_elms ; ++i )
    {
        uint16_t byte_offset_src = p16_pm_earr[ i ] / 8 ;
        uint16_t bit_offset_src = p16_pm_earr[ i ] % 8 ;
        
        uint16_t byte_offset_res = i / 8 ;
        uint16_t bit_offset_res = i % 8 ;
        
        char mask_src = 1 << bit_offset_src ;
        
        bool res_bit = p8_block[byte_offset_src] & mask_src ;
        char mask_res = char ( res_bit ) << bit_offset_res ;
        
        temp8_block[ byte_offset_res ] |= mask_res ;
    }
    memcpy(  p8_block , temp8_block , block_size_bytes ) ; //todo realize move semantic
    
}


void Permutate::Init( unsigned block_size_bytes , bool perm_bytes )
{
    m_block_size_bytes = block_size_bytes ;
    m_perm_bytes = perm_bytes ;
    //epma:  Extended (in memory) Permutation Array size, elements
    // есть блок размером m_block_size байт

    if ( m_perm_bytes )
        //переставляем байты, тогда нужно адресовать :
        epma_size_elms = block_size_bytes ;
    else
        //переставляем биты, тогда нужно адресовать :
        epma_size_elms = block_size_bytes * 8 ; // block size bytes * 8 bits

    m_BIarray.Init( epma_size_elms ) ;
    m_BIarray2.Init( epma_size_elms ) ;
    e_array = std::make_unique< uint16_t[] >( m_BIarray.m_size_elms) ;
    m16e_arr = e_array.get() ;
    e_array2 = std::make_unique< uint16_t[] >( m_BIarray2.m_size_elms) ;
    m16e_arr2 = e_array2.get() ;
} 


void Permutate::InversePermutArr( BitsetItmesArray & bi_arr )
{
    BitsetItmesArray inverse_pma ; // BitsetItmesArray allocates memory, it must not be into cycle
    inverse_pma.Init( m_perm_bytes ) ;
    
    for ( unsigned i = 0 ; i < bi_arr.m_size_elms ;  ++i )
    {
        inverse_pma.set( bi_arr[ i ] , i ) ;
    }
    memcpy( bi_arr.m_array.get() , inverse_pma.m_array.get() , bi_arr.pma_size_bytes ) ; // todo move
} 

void Permutate::InverseExpPermutArr( uint16_t * p16_earr , uint16_t * p16_pm_earr ) noexcept
{
    for ( unsigned i = 0 ; i < epma_size_elms ;  ++i )
    {
        p16_earr[ p16_pm_earr[ i ] ] = i ;
    }
} 


void Permutate::MakePermutArr( uint16_t * e16arr , unsigned char * p8_randoms , BitsetItmesArray & bi_arr )
{
    //сгенерируем  список индексов
    std::vector< uint16_t > indexes_container ;
    indexes_container.reserve( bi_arr.m_size_elms ) ;
    for ( uint16_t i = 0 ; i < bi_arr.m_size_elms ; ++i )
        indexes_container.push_back( i ) ;

    BitsetItmesArray BI_rands ;
    BI_rands.Init( epma_size_elms ) ;

    assert( bi_arr.pma_size_bytes % 4 == 0 ) ;
    memcpy( BI_rands.m_array.get() , p8_randoms , bi_arr.pma_size_bytes ) ;
    
    //display_str("making pma...") ;
    for ( unsigned i = 0 ; i < bi_arr.m_size_elms ; ++i )
    {
        uint16_t r = BI_rands[ i ] ;
        uint16_t rr = ( r % ( bi_arr.m_size_elms - i ) ) ;
        
        bi_arr.set( i ,  indexes_container[ rr ] ) ;
        e16arr[ i ] = indexes_container[ rr ] ;
        indexes_container.erase( indexes_container.begin() + rr ) ;
    }
} ;


void Permutate::Rearrange( unsigned char * p_block , unsigned char * temp_block ) noexcept
{

    //memset( temp_block , 0 , m_BIarray.block_size_bytes ) ; // todo out away

    BitArray src_BA( p_block ) ;
    BitArray res_BA( temp_block ) ;
    
    for ( uint16_t i = 0 ; i < m_BIarray.m_size_elms ; ++i )
    {
        //uint16_t byte_offset_src = repostn_pma[ i ] / 8 ;
        //uint16_t bit_offset_src = repostn_pma[ i ] % 8 ;
        
        //uint16_t byte_offset_res = i / 8 ;
        //uint16_t bit_offset_res = i % 8 ;
        
        //char mask_src = 1 << bit_offset_src ;
        
        //bool res_bit = p_block[byte_offset_src] & mask_src ;
        //char mask_res = char ( res_bit ) << bit_offset_res ;
        
        //res_block[ byte_offset_res ] |= mask_res ;
        //res_BA.setbit( i , src_BA[ m_array[ i ] ] ) ;
        res_BA.setbit( i , src_BA[ m_BIarray[ i ] ] ) ;
    }
    memcpy(  p_block , temp_block , m_block_size_bytes ) ; //todo realize move semantic
    
}

void Permutate::RearrangeBytes( unsigned char * p_block , uint16_t * p_pm_earr , unsigned char * temp_block ) noexcept
{
    for ( uint16_t i = 0 ; i < m_BIarray.m_size_elms ; ++i )
        temp_block[ i ] = p_block[ p_pm_earr[ i ] ] ;
    memcpy(  p_block , temp_block , m_block_size_bytes ) ; //todo realize move semantic
}

void Permutate::eRearrangePMA1( uint16_t * temp_block , uint16_t * p_pm_earr ) noexcept
{
    for ( unsigned i = 0 ; i < epma_size_elms ; ++i )
    {
        temp_block[ p_pm_earr[ i ] ] = m16e_arr[ i ] ;
    }
    memcpy( m16e_arr , temp_block , epma_size_elms * sizeof( uint16_t ) ) ;
}


// N - epma_size_elms
void eTransformPMA2( uint16_t * e16_arr2 , const unsigned N , unsigned & op )
{

    if ( op == 0  )
    {
        op = 1 ;
        for ( unsigned i = 0 ; i < N / 2 ; ++i )
        {
            unsigned temp = e16_arr2[ i * 2 ] ;
            e16_arr2[ i * 2 ] = e16_arr2[ i * 2 + 1 ] ;
            e16_arr2[ i * 2 + 1 ] = temp ;
        }
    }
    else if ( op == 1  )
    {
        op = 0 ;

        unsigned temp = e16_arr2[ 0 ] ;
        e16_arr2[ 0 ] = e16_arr2[ 2 ] ;
        e16_arr2[ 2 ] = temp ;

        temp = e16_arr2[ 1 ] ;
        e16_arr2[ 1 ] = e16_arr2[ N-1 ] ;
        e16_arr2[ N-1 ] = temp ;

        for ( unsigned i = 0 ; i < N / 2 - 1 ; ++i )
        {
            unsigned temp = e16_arr2[ i * 2 + 1] ;
            e16_arr2[ i * 2 + 1] = e16_arr2[ i * 2 + 2 ] ;
            e16_arr2[ i * 2 + 2 ] = temp ;
        }
    }
}

// BitsetItmesArray has been written for packing the PMA (permutation array)
// In our case BitsetItmesArray stores the indexes of the permutation array
// For example, there is a set X of 64 (size_elms) numbers , and for each x: 0 <= x <= 63
// We will store them Packed, that is, for each x we will allocate not a byte, but 6 bits
// So BitsetItmesArray will contain 64 6-bit values
//      that is, an array of 6 bit values
// So the array itself will take up 64*6 = 384 bits, or 48 bytes

// BitsetItmesArray написан для упаковки массива перестановок PMA (permutation array)
// В нашем случае BitsetItmesArray хранит индексы массива перестановок
// К примеру имеется множество X из 64 (size_elms) чисел , при этом для всех x: 0 <= x <= 63
// Будем хранить их упакованно, то есть для каждого x выделим не байт, а 6 бит
// Таким образом BitsetItmesArray будет содержать 64 6-битовых значения
//      , то есть массив 6 битовых значений
// Т. о. непосредственно массив будет занимать 64*6 = 384 бита, или 48 байт

void BitsetItmesArray::Init( unsigned size_elms )
{
    m_size_elms = size_elms ;

    // Для хранения одного индекса нужно log2 ( size_elms ) бит ;
    uint16_t temp = m_size_elms ;
    index_size_bits = 0 ; 
    while ( temp >>= 1 ) index_size_bits ++ ;
    // Для хранения всех индексов нужно
    pma_size_bits = m_size_elms * index_size_bits ;
    pma_size_bytes = pma_size_bits / 8 ;
    if ( pma_size_bits % 8 != 0 ) // это вряд ли, но на всякий случай
        pma_size_bytes++ ;
        
    // выделим память и инициализируем BitArray
    m_array = std::make_unique<unsigned char[]>( pma_size_bytes ) ;

    m_pmaBA.Init( m_array.get() ) ;

}


/*inline*/ uint16_t BitsetItmesArray::operator []( uint16_t index ) noexcept
{
    return m_pmaBA.get( index * index_size_bits , index_size_bits ) ;
}

inline void BitsetItmesArray::set( uint16_t index , uint16_t val ) noexcept
{
    assert( index < m_size_elms ) ;
    m_pmaBA.set( index * index_size_bits , index_size_bits , val ) ;
}


inline void BitArray::setbit( unsigned index , bool value ) noexcept
{
    uint16_t byte_offset = index / 8 ;
    uint16_t bit_offset =  index % 8 ;
    unsigned char mask = 1 << bit_offset ;
    if ( value )
        array[byte_offset] |= mask ;
    else
        array[byte_offset] &= (unsigned char) (~mask) ;
}


inline bool BitArray::operator[] ( unsigned index ) noexcept
{
    uint16_t byte_offset = index / 8 ;
    uint16_t bit_offset =  index % 8 ;
    char mask = 1 << bit_offset ;
    return array[byte_offset] & mask ;
}

void BitArray::Init( unsigned char * in_array )
{
    array = in_array ;
}

inline uint16_t BitArray::get(unsigned index, uint16_t nbits) noexcept
{
    uint16_t byte_offset = index / 8 ;
    uint16_t bit_offset =  index % 8 ;
    
    uint16_t res = 0 ;

    
    uint16_t i = 0 ;
    while ( i < nbits )
    {
        uint16_t lnbits = 8 - bit_offset ;
        if ( i + lnbits > nbits )
            lnbits = nbits - i ;
        
        unsigned char mask = 1 ;
        mask <<= lnbits ;
        mask -- ;
        mask <<= bit_offset ;
        
        uint16_t lres = ( array[byte_offset] & mask ) >> bit_offset ;
        lres <<= i ;
        assert( i < 16 ) ;
        res |= lres ;
        
        i+= lnbits ;
        bit_offset += lnbits ;
        assert( bit_offset <= 8 ) ;
        if ( bit_offset == 8 )
        {
            bit_offset = 0 ;
            byte_offset ++ ;
        }

    }
    return res ;
}

inline void BitArray::set(unsigned index, uint16_t nbits, uint16_t value ) noexcept
{
    uint16_t byte_offset = index / 8 ;
    uint16_t bit_offset =  index % 8 ;
    
    uint16_t i = 0 ;
    while ( i < nbits )
    {
        uint16_t lnbits = 8 - bit_offset ;
        if ( i + lnbits > nbits )
            lnbits = nbits - i ;
        
        unsigned char mask = 1 ;
        mask <<= lnbits ;
        mask -- ;

        unsigned char lval = (value >> i) & mask ;
        lval <<= bit_offset ;

        mask <<= bit_offset ;
        
        array[ byte_offset ] &= ~mask ;
        array[ byte_offset ] |= lval ;
        
        assert( i < 16 ) ;
        
        i+= lnbits ;
        bit_offset += lnbits ;
        assert( bit_offset <= 8 ) ;
        if ( bit_offset == 8 )
        {
            bit_offset = 0 ;
            byte_offset ++ ;
        }

    }
} ;
