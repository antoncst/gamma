#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "platform.h"
#include "randomroutines.h"

#include <string>
#include <iostream>
#include <cstring>

class GammaCrypt
{
    static const size_t block_size = 64 ;    // in bytes
    static const size_t quantum_size = sizeof ( unsigned) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    static const size_t n_quantum = block_size / quantum_size ;
    typedef unsigned t_block[ n_quantum ] ;
    typedef unsigned t_block_random[ n_quantum + 1 ] ; // to transform Random block it is required one quantum more (therefore "+1")

    //write header into output file (i.e. Key) 
    void WriteHead() ;
    //read header from input file
    void ReadHead() ;

    // actually crypt algorythm
    // called from Encrypt(), Decrypt()
    void Crypt() ;
    
    t_block_random m_block_random ; 
    t_block m_block_password ;
    t_block m_block_source ;
    t_block m_block_dest ;

    std::istream & m_ifs ;
    std::ostream & m_ofs ;
    
    const std::string & m_password ;
    
public:
    GammaCrypt( std::istream & ifs , std::ostream & ofs , const std::string & password ) ;

    void Encrypt() ;
    void Decrypt() ;
    
} ;

#endif // GAMMA_H_INCLUDED
