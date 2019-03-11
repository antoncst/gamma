#pragma once
#include <stdio.h>

// generate random
// in:  n_quantum - size of block 'in unsigned'
// out: m_block_random
void GenerateRandom( unsigned * block_random , const size_t n_quantum ) ;

// transform random ( as a simple pseudorandom number generator)
//in:   n_quantum - size of block
//in:   m_block_random
//out:  m_block_random - transormed random
void TransformRandom( unsigned * block_random , const size_t n_quantum ) ;

void TransformRandomCycle( unsigned * block_random , const size_t n_quantum ) ;

