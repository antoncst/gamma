#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "../platform.h"
#include "randomroutines.h"

#include <string>
#include <iostream>
#include <cstring>
#include <memory>

class GammaCrypt
{
public:
    GammaCrypt( std::istream & ifs , std::ostream & ofs , const std::string & password ) ;

    void Encrypt() ;
    void Decrypt() ;
    void SetBlockSize( unsigned block_size ) ;

private:    
    size_t m_block_size = 64 ;    // in bytes , further calculated in Initialize method, deponds on file size
    bool mb_need_init_blocksize = true ; // for if block_size specified in command line

    static const size_t m_quantum_size = sizeof ( unsigned) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    size_t m_n_quantum = m_block_size / m_quantum_size ;
    
    void Initialize() ;

    //write header into output file (i.e. Key) 
    void WriteHead() ;
    //read header from input file
    void ReadHead() ;
    
    //read keys, matrix etc.
    void ReadOverheadData() ;

    // actually crypt algorythm
    // called from Encrypt(), Decrypt()
    void Crypt() ;
    
    std::istream & m_ifs ;
    std::ostream & m_ofs ;
    
    const std::string & m_password ;
    
    std::unique_ptr< t_block > mp_block_random ; // mp_...   m - member, p - pointer
    //obsolete: std::unique_ptr< t_block > mp_block_random3 ;
    std::unique_ptr< t_block > mp_block_password ;
    std::unique_ptr< t_block > mp_block_source ;
    std::unique_ptr< t_block > mp_block_dest ;
    
    RearrangeSlices m_Reposition ;

    void Matrix_Xor_Password( uint16_t * pmatrix_in , unsigned * pmatrix_out ) ;
    
    void DisplayInfo() ;
    
    void ( * mpShift )( unsigned * p_block ) ;
    
    struct t_header
    {
        // 0x00
        uint8_t major_ver = 0x00 ;
        uint8_t minor_ver = 0x00 ;
        // 0x2
        uint16_t data_offset ;  // начало блока ключа.
                                // относительно начала файла (заголовка) , то есть размер заголовка
        // 0x4
        uint64_t source_file_size ; // размер исходного файла
        uint16_t h_block_size ; // размер блока (ключа и пр.). h_ (header) - чтобы не путаться с другими "block_size"
        uint16_t reserved2 = 0 ;
    } m_header ; 
} ;

#endif // GAMMA_H_INCLUDED
