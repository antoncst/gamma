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

// transform random ( as a simple pseudorandom number generator)
//in:   n_quantum - size of block
//in:   m_block_random
//out:  m_block_random - transormed random
void TransformRandom( unsigned * block_random , const size_t n_quantum ) ;

void TransformRandomCycle( unsigned * block_random , const size_t n_quantum ) ;



//Перестановка битов или кусочков другого размера

class BitArray
{
    //all methods do not check bounds
public:
    //c-tor
    BitArray( unsigned char * in_array ) : array( in_array ) {} ;
    BitArray() {} ;
    //
    void Init( unsigned char * in_array ) ;
    bool operator[] ( uint16_t index ) ;
    void setbit( uint16_t index , bool value ) ;
    // get nbits bits , beginning from index(in bits)
    // nbits should be <= 16
    uint16_t get( uint16_t index , uint16_t nbits ) ;
    void set( uint16_t index , uint16_t nbits , uint16_t value ) ;
private:
    unsigned char * array ;
} ;

// Array of items
// where one item is a bit set ( nothing in common to std::bitset ) of specified number of bits
class BitsetItmesArray
{
public:
    void Init( size_t block_size ) ;
    void MakeRearrangeMatrix() ;
    void InverseRearrangeMatrix() ;
    void Rearrange( unsigned char * p_block_dest , uint16_t bytes_read ) ;

    uint16_t operator[] (uint16_t index) ;
    void set( uint16_t index , uint16_t val ) ;

    std::unique_ptr< unsigned char[] > m_array ;
    uint16_t max_index ; //  that is matrix length, the numbers of elements
    uint16_t matrix_size_bits ;
    uint16_t matrix_size_bytes ;
    uint16_t index_size_bits ;
private:
    uint16_t block_size_bytes ;
    BitArray m_matrixBA ;
} ;