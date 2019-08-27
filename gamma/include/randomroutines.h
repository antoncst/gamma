#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <memory>

typedef unsigned t_block[] ;

// generate random
// in:  rsize - size of block 'in unsigned'
// out: randoms
void GenerateRandoms( const unsigned rsize , t_block randoms) ;

void TransformRandom( unsigned * block_random , const size_t n_quantum ) ;

void TransformRandomCycle( unsigned * block_random , const size_t n_quantum ) ;



// Перестановка битов или кусочков другого размера
// Does not have real array, the pointer to it only. Does not allocate memory.
class BitArray
{
    //all methods do not check bounds
public:
    //c-tor
    BitArray( unsigned char * in_array ) : array( in_array ) {} ;
    BitArray() {} ;
    //
    void Init( unsigned char * in_array ) ;
    inline bool operator[] ( unsigned index ) noexcept ;
    inline void setbit( unsigned index , bool value ) noexcept ;
    // get nbits bits , beginning from index(in bits)
    // nbits should be <= 16
    inline uint16_t get( unsigned index , uint16_t nbits ) noexcept ;
    inline void set( unsigned index , uint16_t nbits , uint16_t value ) noexcept ;
    unsigned char * array ;
private:
} ;

// Array of items
// where one item is a bit set ( nothing in common to std::bitset ) of specified number of bits
// has a real array, allocates memory 
class BitsetItmesArray
{
public:
    void Init( size_t block_size ) ;
    
    inline uint16_t operator[] (uint16_t index) noexcept ;
    inline void set( uint16_t index , uint16_t val ) noexcept ;

    std::unique_ptr< unsigned char[] > m_array ;
    unsigned max_index ; //  that is matrix length, the numbers of elements
    unsigned matrix_size_bits ;
    uint16_t matrix_size_bytes ;
    uint16_t index_size_bits ;
    uint16_t block_size_bytes ;
    BitArray m_matrixBA ;
} ;


class RearrangeSlices
{
public:
    BitsetItmesArray m_BIarray ;

    void Init( size_t block_size ) ;
    void MakeRearrangeMatrix() ;
    void InverseRearrangeMatrix() ;
    void Rearrange( unsigned char * p_block , uint16_t bytes_read , unsigned char * temp_block ) noexcept ;

} ;