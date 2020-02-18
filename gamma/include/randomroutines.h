#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>

typedef uint32_t t_block[] ;

void CPUFullLoad( std::atomic< bool > & bstop ) ;
void TickTimer( std::atomic< uint64_t > & ticks , std::atomic< bool > & bstop ) ;

// generate random
// in:  rsize - size of block 'in unsigned'
// out: randoms
void GenerateRandoms( const unsigned rsize , t_block randoms , std::atomic< uint64_t > & ticks ) ;

//void TransformRandom( unsigned * block_random , const size_t n_quantum ) ;

//void TransformRandomCycle( unsigned * block_random , const size_t n_quantum ) ;



// Манипуляция битами массива битов
// Bit manipulation in bit array
// Does not have real array, the pointer to it only. Does not allocate memory.
class BitArray
{
    //all methods do not check bounds !
    // the size of bit array is undefined
    // The responsibility falls on the caller
    // размер массива битов не определён
    // Ответственность ложится на вызывающего
public:
    //c-tor
    BitArray( unsigned char * in_array ) : array( in_array ) {} ;
    BitArray() {} ;
    //
    void Init( unsigned char * in_array ) ;

    // get the value of one bit 
    inline bool operator[] ( unsigned index ) noexcept ;

    // set the value of one bit 
    inline void setbit( unsigned index , bool value ) noexcept ;
    
    // get nbits bits , beginning from index(in bits)
    // nbits must be <= 16
    inline uint16_t get( unsigned index , uint16_t nbits ) noexcept ;

    // set the value of nbits bits , beginning from index(in bits)
    // nbits must be <= 16
    inline void set( unsigned index , uint16_t nbits , uint16_t value ) noexcept ;
    
    friend class BitsetItmesArray ;
private:
    // pointer to bit array
    unsigned char * array ;
} ;


// Array of items
// where one item is a bit set (bit field) ( nothing in common to std::bitset ) of specified number of bits
// size of an element in bits = log2( size_elms ) 
// see the description in the implementation
// has a real array, allocates memory 
class BitsetItmesArray
{
public:
    void Init( unsigned size_elms ) ;
    
    /*inline*/ uint16_t operator[] (uint16_t index) noexcept ; // to do inline
    inline void set( uint16_t index , uint16_t val ) noexcept ;

    friend class Permutate ;
    friend class GammaCryptImpl ;

private:
    std::unique_ptr< unsigned char[] > m_array ;

    // permutation array size bytes
    uint16_t pma_size_bytes ;

    // permutation array size bits
    unsigned pma_size_bits ;

    unsigned m_size_elms ; //  that is permutation array length, the numbers of elements

    uint16_t index_size_bits ;

    // BitsetArray as continuous bit array
    // BitsetArray как непрерывный массив битов
    BitArray m_pmaBA ;
} ;

// transform PMA2 via invented simple algorithms
// e16_arr2 - permutate array to be transformed
// N = epma_size_elms : size (elements) of array 
// op - 0..1 ,  operation within eTransformPMA2 , two different algorihms
// op is changed 0 <-> 1 on every eTransformPMA2 calling (inside this method) , so op must be global within the class
void eTransformPMA2( uint16_t * e16_arr2 , const unsigned N , unsigned & op) ;


class Permutate
{
public:

    void Init( unsigned block_size_bytes , bool perm_bytes ) ;
    // Make Permutation Array
    void MakePermutArr( uint16_t * e16arr , unsigned char * p8_randoms , BitsetItmesArray & bi_arr ) ;
    
    void InversePermutArr( BitsetItmesArray & bi_arr ) ;
    // p_pm_earr  -Pointer to _ PurMutation _ Expanded ARRay
    void InverseExpPermutArr( uint16_t * p16_earr, uint16_t * p16_pm_earr ) noexcept ; 
    
    // p_block - data to Rearrange
    // temp_block - block for temprorary storing
    // p_pm_earr - PerMutate Extended (in memory) Array , aka PMA
    void eRearrange( unsigned char * p8_block , unsigned char * temp8_block , uint16_t * p16_pm_earr , unsigned epma_size_elms , unsigned block_size_bytes , bool perm_bytes ) noexcept ;

    friend class GammaCryptImpl ;
private:
    unsigned char *  mp8_randoms ; // mpc - Member, Pointer to unsigned Char
    bool m_perm_bytes ;
    unsigned epma_size_elms ;
    unsigned m_block_size_bytes ;
    // массив перестановок (развернутый): 
    // expanded permutation array:
    std::unique_ptr< uint16_t[] > e_array ; // expanded array
    std::unique_ptr< uint16_t[] > e_array2 ; // expanded array2
    uint16_t * m16e_arr ;   // Member pointer to 16 bit elements Expanded ARRay
    uint16_t * m16e_arr2 ;

    // здесь хранятся упакованные массивы перестановок
    // here stores packed permutation arrays :
    BitsetItmesArray m_BIarray ;
    BitsetItmesArray m_BIarray2 ;

    // permutate bits
    void Rearrange( unsigned char * p_block , unsigned char * temp_block ) noexcept ;

    // permutate bytes (8 bit blocks)
    // p_block - data to rearrange
    // p_pm_arr - permtation array
    void RearrangeBytes( unsigned char * p_block , uint16_t * p_pm_earr , unsigned char * temp_block ) noexcept ;

    // rearrange PMA1 via PMA2
    void eRearrangePMA1( uint16_t * temp_block , uint16_t * p_pm_earr ) noexcept ;
} ;