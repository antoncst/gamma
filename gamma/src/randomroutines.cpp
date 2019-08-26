
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


//---------------------------------------------------------------------
//GENERATING RANDOM

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
    
    #ifdef DEBUG
    std::cout << "bit" << nshift << std::endl ;
    std::cout << "statistics:\n" ;
    for ( auto dval : dvals )
        std::cout << dval << std::endl ;
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
    
    #ifdef DEBUG
    std::cout << "bit" << nshift << std::endl ;
    std::cout << "statistics:\n" ;
    for ( auto dval : dvals )
        std::cout << dval << std::endl ;
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

//generate randoms using hdd operatios
// N - array size // размер массива
void GenRandByHdd( unsigned const N , t_block rvals )
{
    //rarray_t rvals ;
    auto mcs1 = std::chrono::high_resolution_clock::now() ;
    for ( unsigned i = 0 ; i < N/2 ; ++i )
    {
        
        std::ofstream ofs ;
        ofs.open( "/home/anton/ramdisk/temp_1", std::ios::out | std::ios::trunc) ;
        ofs << "ttt" ;
        std::this_thread::sleep_for( std::chrono::microseconds( 10 ) ) ;
        ofs.flush() ;
        ofs.close() ;
        
        auto mcs2 = std::chrono::high_resolution_clock::now() ;
        std::chrono::duration<double> mcs = mcs2 - mcs1 ;
        mcs1 = mcs2 ;
        /*uint64_t*/ unsigned rval ; //random value
        rval = mcs.count() * 1000000000 ;
        
        rvals[ i*2 ] = rval ;

        {
            std::ofstream ofs2 ;
            ofs2.open( "/home/anton/ramdisk/temp_2", std::ios::out | std::ios::trunc) ;
            ofs2 << "ttt" ;
            std::this_thread::sleep_for( std::chrono::microseconds( 10 ) ) ;
            ofs2.flush() ;
            ofs2.close() ;

            mcs2 = std::chrono::high_resolution_clock::now() ;
            mcs = mcs2 - mcs1 ;
            mcs1 = mcs2 ;
            rval = mcs.count() * 1000000000 ;
            
            rvals[ i*2 +1 ] = rval ;
        }
    }
}


// определить старший, установленный в 1, бит
int DetermineHighSetBit( unsigned const val )
{
    if ( val > 0 )
    {
                                      // 31                             8 7  4    0
        unsigned umask = 0x80000000 ; //  1000 0000 0000 0000   0000 0000 0000 0000
        int n = 31 ;
        for ( n = 31 ; n >= 0 ; n-- , umask >>= 1 )
            if ( (val & umask) > 0 )
                return n ;
    }
    throw "error: too small random value to processing DetermineHighSetBit" ;
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

void CropRandoms( unsigned const N , t_block rvals )
{
    // вычтем минимальное время, чтобы получилось отклонение.
    unsigned * pmin = std::min_element( rvals , rvals + N ) ;
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        rvals[ i ] -= * pmin ;
    }

    //уберём младшие "малозначащие" биты
    unsigned LowSignificantBit = DetermineNonsignificantLowBits( N ,rvals ) ;
    for ( unsigned i = 0 ; i < N ; i++ )
    {
        rvals[ i ] >>= LowSignificantBit ;
    }
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
        
        block[ current_block_index ] |= unsigned ( rval & 0xFFFFFFFF );
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

//Генерировать блок СЧ заданной длины в словах (unsigned)
//rsize (random block size) - размер массива
void GenerateRandoms( unsigned const rsize , t_block randoms )
{
    unsigned alignedsize = 192 ; // минимальный размер массива для ГСЧ
    if ( alignedsize < rsize)
        alignedsize = rsize ; 
    unsigned N = alignedsize * 4 ;
    while ( 1 )
    {
        auto rvals = std::make_unique< t_block >( N ) ;
        GenRandByHdd( N , rvals.get() ) ;
        CropRandoms( N ,rvals.get() ) ;
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


void TransformRandom( unsigned * block_random , const size_t n_quantum )
{
        assert( block_random != nullptr ) ;
    unsigned first_bit = block_random[0] & 0x1 ;

    for ( size_t i = 0 ; i < n_quantum ; i++ )
    {
        //                          1st bit                     2nd bit                             reset 1st bit
        block_random[i] = ( (block_random[i] & 0x1 ) ^ (( block_random[i] & 0x2 ) >>1) ) | ( block_random[i] & 0xFFFFFFFE ) ;
        //                          2st bit                     3d bit                              reset 2st bit
        block_random[i] = ( (block_random[i] & 0x2 ) ^ (( block_random[i] & 0x4 ) >>1)) | ( block_random[i] & 0xFFFFFFFD ) ;
        //                          3d bit                      4th bit                             reset 3d bit
        block_random[i] = ( (block_random[i] & 0x4 ) ^ (( block_random[i] & 0x8 ) >>1)) | ( block_random[i] & 0xFFFFFFFB ) ;
        //                          4th bit                     5th bit                             reset 4th bit
        block_random[i] = ( (block_random[i] & 0x8 ) ^ (( block_random[i] & 0x10 ) >>1)) | ( block_random[i] & 0xFFFFFFF7 ) ;
        //                          5th bit                     6th bit                             reset 5th bit
        block_random[i] = ( (block_random[i] & 0x10 ) ^ (( block_random[i] & 0x20 ) >>1)) | ( block_random[i] & 0xFFFFFFEF ) ;
        //                          6th bit                     7th bit                             reset 6th bit
        block_random[i] = ( (block_random[i] & 0x20 ) ^ (( block_random[i] & 0x40 ) >>1)) | ( block_random[i] & 0xFFFFFFDF ) ;
        //                          7th bit                     8th bit                             reset 7th bit
        block_random[i] = ( (block_random[i] & 0x40 ) ^ (( block_random[i] & 0x80 ) >>1)) | ( block_random[i] & 0xFFFFFFBF ) ;
        //                          8th bit                     9th bit                             reset 8th bit
        block_random[i] = ( (block_random[i] & 0x80 ) ^ (( block_random[i] & 0x100 ) >>1)) | ( block_random[i] & 0xFFFFFF7F ) ;
        // etc
        block_random[i] = ((block_random[i] & 0x100 ) ^ (( block_random[i] & 0x200 )>>1)) | ( block_random[i] &    0xFFFFFEFF ) ;
        block_random[i] = ((block_random[i] & 0x200 ) ^ (( block_random[i] & 0x400 )>>1)) | ( block_random[i] &    0xFFFFFDFF ) ;
        block_random[i] = ((block_random[i] & 0x400 ) ^ (( block_random[i] & 0x800 )>>1)) | ( block_random[i] &    0xFFFFFBFF ) ;
        block_random[i] = ((block_random[i] & 0x800 ) ^ (( block_random[i] & 0x1000 )>>1)) | ( block_random[i] &   0xFFFFF7FF ) ;
        block_random[i] = ((block_random[i] & 0x1000 ) ^ (( block_random[i] & 0x2000 )>>1)) | ( block_random[i] &  0xFFFFEFFF ) ;
        block_random[i] = ((block_random[i] & 0x2000 ) ^ (( block_random[i] & 0x4000 )>>1)) | ( block_random[i] &  0xFFFFDFFF ) ;
        block_random[i] = ((block_random[i] & 0x4000 ) ^ (( block_random[i] & 0x8000 )>>1)) | ( block_random[i] &  0xFFFFBFFF ) ;
        block_random[i] = ((block_random[i] & 0x8000 ) ^ (( block_random[i] & 0x10000 )>>1)) | ( block_random[i] & 0xFFFF7FFF ) ;

        block_random[i] = ((block_random[i] & 0x10000 ) ^ (( block_random[i] &  0x20000 )>>1)) | ( block_random[i] &   0xFFFEFFFF ) ;
        block_random[i] = ((block_random[i] & 0x20000 ) ^ (( block_random[i] &  0x40000 )>>1)) | ( block_random[i] &   0xFFFDFFFF ) ;
        block_random[i] = ((block_random[i] & 0x40000 ) ^ (( block_random[i] &  0x80000 )>>1)) | ( block_random[i] &   0xFFFBFFFF ) ;
        block_random[i] = ((block_random[i] & 0x80000 ) ^ (( block_random[i] &  0x100000 )>>1)) | ( block_random[i] &  0xFFF7FFFF ) ;
        block_random[i] = ((block_random[i] & 0x100000 ) ^ (( block_random[i] & 0x200000 )>>1)) | ( block_random[i] &  0xFFEFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x200000 ) ^ ((block_random[i] &  0x400000 )>>1)) | ( block_random[i] &  0xFFDFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x400000 ) ^ (( block_random[i] & 0x800000 )>>1)) | ( block_random[i] &  0xFFBFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x800000 ) ^ (( block_random[i] & 0x1000000 )>>1)) | ( block_random[i] & 0xFF7FFFFF ) ;

        block_random[i] = ((block_random[i] & 0x1000000 ) ^ (( block_random[i] &  0x2000000 )>>1)) | ( block_random[i] &   0xFEFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x2000000 ) ^ (( block_random[i] &  0x4000000 )>>1)) | ( block_random[i] &   0xFDFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x4000000 ) ^ (( block_random[i] &  0x8000000 )>>1)) | ( block_random[i] &   0xFBFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x8000000 ) ^ (( block_random[i] &  0x10000000 )>>1)) | ( block_random[i] &  0xF7FFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x10000000 ) ^ (( block_random[i] & 0x20000000 )>>1)) | ( block_random[i] &  0xEFFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x20000000 ) ^ (( block_random[i] & 0x40000000 )>>1)) | ( block_random[i] &  0xDFFFFFFF ) ;
        block_random[i] = ((block_random[i] & 0x40000000 ) ^ (( block_random[i] & 0x80000000 )>>1)) | ( block_random[i] &  0xBFFFFFFF ) ;
        if ( i != n_quantum -1 )
            block_random[i] = ((block_random[i] & 0x80000000 ) ^ (( block_random[i+1] & 0x1 ) << 31)) | ( block_random[i] & 0x7FFFFFFF ) ;
    }
    block_random[n_quantum -1] = ((block_random[n_quantum -1] & 0x80000000 ) ^ ( first_bit <<31 )) | ( block_random[n_quantum -1] & 0x7FFFFFFF ) ;
}


//------------- REPOSITIONING --------------------


void BitsetItmesArray::Init( size_t block_size )
{
    block_size_bytes = block_size ;
    // есть блок размером m_block_size байт
    //допустим переставляем биты, тогда нужно адресовать :
    max_index = block_size_bytes * 8 ;
    // Для хранения одного индекса нужно log2 ( m_max_index ) бит ;
    uint16_t temp = max_index ;
    index_size_bits = 0 ; 
    while ( temp >>= 1 ) index_size_bits ++ ;
    // Для хранения всех индексов нужно
    matrix_size_bits = max_index * index_size_bits ;
    matrix_size_bytes = matrix_size_bits / 8 ;
    if ( matrix_size_bits % 8 != 0 ) // это вряд ли, но на всякий случай
        matrix_size_bytes++ ;
        
    // выделим память и инициализируем BitArray
    m_array = std::make_unique<unsigned char[]>( matrix_size_bytes ) ;

    m_matrixBA.Init( m_array.get() ) ;

}

void RearrangeSlices::InverseRearrangeMatrix()
{
    //std::vector< uint16_t > inverse_matrix ;
    BitsetItmesArray inverse_matrix ;
    inverse_matrix.Init( m_BIarray.block_size_bytes  ) ;
    
    //inverse_matrix.reserve( max_index ) ;
    for ( unsigned i = 0 ; i < m_BIarray.max_index ;  ++i )
    {
        inverse_matrix.set( m_BIarray[ i ] , i ) ;
    }
    memcpy( m_BIarray.m_array.get() , inverse_matrix.m_array.get() , m_BIarray.matrix_size_bytes ) ; // todo move
} 

void RearrangeSlices::MakeRearrangeMatrix()
{
    //сгенерируем  список индексов
    std::vector< uint16_t > indexes_container ;
    indexes_container.reserve( m_BIarray.max_index ) ;
    for ( uint16_t i = 0 ; i < m_BIarray.max_index ; ++i )
        indexes_container.push_back( i ) ;

    //display_str("Generating randoms for matrix ...") ;

    BitsetItmesArray BI_rands ;
    BI_rands.Init( /*N_bytes*/ m_BIarray.block_size_bytes ) ;

    assert( m_BIarray.matrix_size_bytes % 4 == 0 ) ;
    GenerateRandoms( m_BIarray.matrix_size_bytes / 4 , ( unsigned * ) BI_rands.m_array.get() ) ;
    
    //display_str("making matrix...") ;
    for ( unsigned i = 0 ; i < m_BIarray.max_index ; ++i )
    {
        // вместо rands - unique_ptr< t_block > 
        // нужен BitsetItmesArray
        
        uint16_t r = BI_rands[ i ] ;
        /*if ( i % 2 == 0)
            r = rands [ i/2 ] & 0xffff ;
        else
            r = rands[ i/2 ] >> 16 ; */
        // m_array[ i ] =  indexes_deque[ rr ] ; // 
        
        uint16_t rr = ( r % ( m_BIarray.max_index - i ) ) ;
        //repostn_matrix[ i ] = indexes[ rr ] ;
        
        m_BIarray.set( i ,  indexes_container[ rr ] ) ;
        indexes_container.erase( indexes_container.begin() + rr ) ;
    }
} ;

void RearrangeSlices::Rearrange( unsigned char * p_block , uint16_t bytes_read , unsigned char * temp_block ) noexcept
{
    
    if ( bytes_read < m_BIarray.block_size_bytes )
    {
        // тогда остаток блока надо добить нулями
        memset( p_block + bytes_read, 0 , m_BIarray.block_size_bytes - bytes_read ) ;
    }
    
    memset( temp_block , 0 , m_BIarray.block_size_bytes ) ; // todo out away

    BitArray src_BA( p_block ) ;
    BitArray res_BA( temp_block ) ;
    
    for ( uint16_t i = 0 ; i < m_BIarray.max_index ; ++i )
    {
        //uint16_t byte_offset_src = repostn_matrix[ i ] / 8 ;
        //uint16_t bit_offset_src = repostn_matrix[ i ] % 8 ;
        
        //uint16_t byte_offset_res = i / 8 ;
        //uint16_t bit_offset_res = i % 8 ;
        
        //char mask_src = 1 << bit_offset_src ;
        
        //bool res_bit = p_block[byte_offset_src] & mask_src ;
        //char mask_res = char ( res_bit ) << bit_offset_res ;
        
        //res_block[ byte_offset_res ] |= mask_res ;
        //res_BA.setbit( i , src_BA[ m_array[ i ] ] ) ;
        res_BA.setbit( i , src_BA[ m_BIarray[ i ] ] ) ;
    }
    memcpy(  p_block , temp_block , m_BIarray.block_size_bytes ) ; //todo realize move semantic
    
}

inline uint16_t BitsetItmesArray::operator []( uint16_t index ) noexcept
{
    return m_matrixBA.get( index * index_size_bits , index_size_bits ) ;
}

inline void BitsetItmesArray::set( uint16_t index , uint16_t val ) noexcept
{
    assert( index < max_index ) ;
    m_matrixBA.set( index * index_size_bits , index_size_bits , val ) ;
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

void RearrangeSlices::Init( size_t block_size )
{
    m_BIarray.Init( block_size ) ;
} ;