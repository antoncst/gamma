#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "../platform.h"
#include "randomroutines.h"

#include <string>
#include <iostream>
#include <cstring>
#include <memory>


// Parameters for CryptBlock
struct ParamsForCryBl
{
    ParamsForCryBl() {} ;
    uint16_t * e_temp_block_pma ;
    unsigned char * e_temp_block ;
    unsigned hardware_concurrency ;
    unsigned n_blocks ;
    unsigned * p_source ;
    unsigned * p_dest ;
    //unsigned offset ; // in p_block_source and p_block_dest

    unsigned * pkeys1 , * pkeys2 ;
    uint16_t * ppma1 , * ppma2 ;

    unsigned blk_sz_words ;
    unsigned blks_per_thr ;
    uint16_t * e_array ;
    uint16_t * e_array2 ;
    unsigned epma_sz_elmnts ;
} ;

static ParamsForCryBl params ;


class GammaCrypt
{
public:
    GammaCrypt( std::istream & ifs , std::ostream & ofs , const std::string & password ) ;

    void Encrypt() ;
    void Decrypt() ;
    
    void SetBlockSize( unsigned block_size ) ;
    void mGenerateRandoms() ;
    void MakePswBlock() ;
    
    unsigned m_rnd_size_words ; // word - sizeof( unsigned )
    unsigned m_tail_size_words ;
    unsigned m_tail_size_bytes ;
    
private:    
    size_t m_block_size_bytes = 64 ;    // in bytes , further calculated in Initialize method, deponds on file size
    bool mb_need_init_blocksize = true ; // for if block_size specified in command line or for decrypt
    bool mb_decrypting = false ;
    bool mb_multithread = false ;
    static const unsigned m_blks_per_thr = 16 ;
    
    std::unique_ptr< t_block >   m_upkeys1 , m_upkeys2 ;
    std::unique_ptr< uint16_t[] > m_uppma1 , m_uppma2 ;

    // MT- multithread
    void EncryptBlockMT( uint16_t * e_temp_block_pma , unsigned char * e_temp_block , unsigned n_blocks ) noexcept ;

//    void EncryptBlockOneThr( uint16_t * e_temp_block_pma , unsigned char * e_temp_block , unsigned n_blocks , unsigned Nthr) noexcept ;
            
    void EncryptBlock( uint16_t * e_temp_block_pma , unsigned char * e_temp_block , unsigned & op ) noexcept ;

    unsigned m_hrdw_concr ;

    static const size_t m_quantum_size = sizeof ( unsigned) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    size_t m_blk_sz_words = m_block_size_bytes / m_quantum_size ;
    
    void Initialize() ;

    //write header into output file (i.e. Key) 
    void WriteHead() ;
    //read header from input file
    void ReadHead() ;
    
    //read keys, pma etc.
    void ReadOverheadData() ;

    //obsolete:
    // actually crypt algorythm
    // called from Encrypt(), Decrypt()
    void Crypt() ;
    
    std::istream & m_ifs ;
    std::ostream & m_ofs ;
    
    const std::string & m_password ;
    
    // block random
    // N = m_blk_sz_words / 4 
    // offset in bytes:
    //    KEY1           KEY2                PMA1                             PMA2                         tail KEY
    // 0  ... N-1   N  ...  N*2-1   N*2...N*2+PMAsizeBytes-1   N*2+PMAsizeBytes...N*2+PMAsizeBytes*2-1    N*2+PMAsizeBytes*2 ... N*2+PMAsizeBytes*2+tailSize-1
    std::unique_ptr< t_block > mp_block_random ; // mp_...   m - member, p - pointer
    //obsolete: std::unique_ptr< t_block > mp_block_random3 ;
    
    //offsets bytes:
    unsigned offs_key2 ;
    unsigned offs_pma1 ;
    unsigned offs_pma2 ;
    unsigned offs_ktail ;
    


    std::unique_ptr< t_block > mp_block_password ;
    std::unique_ptr< unsigned char[] > mpc_block_psw_pma ; // c - char
    std::unique_ptr< t_block > mp_block_source ;
    std::unique_ptr< t_block > mp_block_dest ;
    
    Permutate m_Permutate ;

    void PMA_Xor_Psw( unsigned * p_pma_in , unsigned * p_pma_out ) ;
    
    void DisplayInfo() ;
    
    // Pointer to LFSR function
    void ( * mpShift )( unsigned * p_block ) ;
    
    struct t_header
    {
        // 0x00
        uint8_t major_ver = 0x00 ;
        uint8_t minor_ver = 0x00 ;
        // 0x02
        uint16_t data_offset ;  // начало блока ключа.
                                // относительно начала файла (заголовка) , то есть размер заголовка
        // 0x04
        uint16_t h_block_size ; // размер блока (ключа и пр.). h_ (header) - чтобы не путаться с другими "block_size"
        uint16_t reserved = 0 ;
        // 0x08
        uint64_t source_file_size ; // размер исходного файла
    } m_header ; 
} ;

#endif // GAMMA_H_INCLUDED
