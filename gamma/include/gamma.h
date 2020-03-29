#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "../platform.h"
#include "gamma_intfc.h"
#include "randomroutines.h"

#include <string>
#include <iostream>
#include <cstring>
#include <memory>
#include <fstream>

#include <condition_variable>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>


// Parameters for CryptBlock
struct ParamsForCryBl
{
    ParamsForCryBl() {} ;
    //uint16_t * e_temp_block_pma ;
    //unsigned char * e8_temp_block ;
    //unsigned hardware_concurrency ;
    //unsigned n_blocks ;
    unsigned * p_source ;
    unsigned * p_dest ;
    //unsigned offset ; // in p_block_source and p_block_dest

    //unsigned * pkeys1 , * pkeys2 ;
    //uint16_t * ppma1 , * ppma2 ;

    //unsigned blk_sz_words ;
    //unsigned blks_per_thr ;
    uint16_t * e16_arr ;
    uint16_t * e16_arr2 ;
    unsigned blk_sz_bytes ;
    unsigned epma_sz_elmnts ;
} ;

class GammaCryptImpl ;

class Threading
{
public:
	void Process( GammaCryptImpl * GC , const unsigned Nthr ) ;
	GammaCryptImpl * mpGC ; 
	unsigned m_Nthr = 0 ;
private:
	std::shared_timed_mutex m_mutex ;
	std::condition_variable_any m_notifier ;
	std::vector< bool > m_thr_ntfd ; //std::atomic_bool
	unsigned m_n_pass_thr ; //std::atomic_uint
	unsigned mnItTh ;
	bool m_pass_processed[ 2 ] ; //std::atomic_bool
	bool m_bexit = false ;

	void thread_proc( int nthr ) ;
} ;


class GammaCryptImpl : public GammaCrypt
{
public:
    GammaCryptImpl( std::istream & ifs , std::ostream & ofs , const std::string & password ) ;

	//std::ofstream m_dbg_ofs1 ;
	//std::ofstream m_dbg_ofs2 ;

    virtual void Encrypt() override ;
    virtual void Decrypt() override ;
    virtual void GenKeyToFile() override ;
    virtual void SetBlockSize( unsigned block_size ) override ;

    // mb_use_keyfile == true    use keyfile
    // mb_use_keyfile == false   generate keys and stores them into encrypted file (default)
    virtual void UseKeyfile( bool use_keyfile ) override { mb_use_keyfile = use_keyfile ; } ;
    
    virtual void SetKeyFilename( const std::string & keyfilename ) override { m_keyfilename = keyfilename ; } ;

    virtual const std::string & GetKeyFilename() override { return m_keyfilename ; } ;
    
    // m_perm_bytes == true    permutate bytes (default)
    // m_perm_bytes == false   permutate bits
    virtual void PermutateBytes( bool perm_bytes ) override { m_perm_bytes = perm_bytes ; } ;

    bool mb_use_keyfile = false ;
    std::string m_keyfilename ;
    // m_perm_bytes == true    permutate bytes
    // m_perm_bytes == false   permutate bits
    bool m_perm_bytes = true ;
    
    friend class Threading ;
private:
	// Предварительные расчёты keys, pma's
	void PreCalc( const unsigned n_pass ) ;
    
    void mGenerateRandoms() ;
    void MakePswBlock() ;
    
    // parameters for crypting block:
    ParamsForCryBl params ;
    
    std::istream & m_ifs ;
    std::fstream m_ifs_keyfile ;
    std::ostream & m_ofs ;
    
    char * mpbuffer ;
    char * mpbuffout ;
	unsigned mbuf_size ;

	void EncryptBlockOneThr( unsigned Nthr , unsigned n_pass_thr ) noexcept ;

    unsigned m_rnd_size_words ; // word - sizeof( unsigned )
    unsigned m_tail_size_words ;
    unsigned m_tail_size_bytes ;

	unsigned char * m8_temp_block ;
	
    struct t_header
    {
        // 0x00
        // bit7 (high bit) == 0 - file contains keys (and pma's)
        // bit7 (high bit) == 1 - keys are in the keyfile
        // bit6 == 0 - permutate bytes
        // bit6 == 1 - permutate bits
        uint8_t major_ver = 0x00 ;
        uint8_t minor_ver = 0x00 ;
        // 0x02
        uint8_t data_offset ;  // начало блока ключа.
                                // относительно начала файла (заголовка) , то есть размер заголовка
        uint8_t reserved = 0 ;
        // 0x04
        uint16_t h_block_size ; // размер блока (ключа и пр.). h_ (header) - чтобы не путаться с другими "block_size"
        uint16_t reserved2 = 0 ;
        // 0x0C
        uint64_t source_file_size ; // размер исходного файла
    } m_header ; 

    bool mb_multithread = false ;
    unsigned m_block_size_bytes = 64 ;    // in bytes , further calculated in Initialize method, deponds on file size

    // block random
    // N = m_blk_sz_words / 4 
    // offset in bytes:
    //    KEY1           KEY2                PMA1                             PMA2                         tail KEY
    // 0  ... N-1   N  ...  N*2-1   N*2...N*2+PMAsizeBytes-1   N*2+PMAsizeBytes...N*2+PMAsizeBytes*2-1    N*2+PMAsizeBytes*2 ... N*2+PMAsizeBytes*2+tailSize-1
    std::unique_ptr< t_block > mu32_block_random ; // mp_...   m - member, uqp - unique ptr , 32 - of 4 bytes elements
    uint32_t * m32_block_random ;
    
    //offsets bytes:
    unsigned offs_key2 ;
    unsigned offs_pma1 ;
    unsigned offs_pma2 ;
    unsigned offs_ktail ;

    bool mb_need_init_blocksize = true ; // for if block_size specified in command line or for decrypt
    bool mb_decrypting = false ;
    const unsigned m_blks_per_thr = 1*8*1024 ; // if the block size is 128 bytes, this is 1 MB per thread, not counting memory for PreCalc()
    
    unsigned * mp32keys1 = nullptr , * mp32keys2 = nullptr ; // m-member, p-pointer
    uint16_t * mp16pma1 = nullptr , * mp16pma2 = nullptr ; // m-member, p-pointer
	uint16_t * m16e_temp_block_pma ; // m-member, p-pointer to u16-uint16_t, e - expanded
	unsigned mNblocks ; // number of blocks
	unsigned m_op ; // operation for TransformPMA2
	
	Threading m_Threading ;

    inline void MakeDiffusion( unsigned * p32src ) noexcept ;
    inline void RemoveDiffusion( unsigned * p32src ) noexcept ;
    //inline void RemoveConfusion( unsigned * psrc ) noexcept ;
            
    void EncryptBlock() noexcept ;

    void DecryptBlock( uint16_t * t16_invs_pma1 ) noexcept ;


    unsigned m_hrdw_concr ;

    static const size_t m_quantum_size = sizeof ( uint32_t ) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    size_t m_blk_sz_words = m_block_size_bytes / m_quantum_size ;
    
    void Initialize() ;

    //write header into output file (i.e. Key) 
    void WriteHead() ;
    //read header from input file
    void ReadHead( std::istream & ifs ) ;
    
    //read keys, pma etc.
    void ReadOverheadData( std::istream & ifs ) ;

    const std::string & m_password ;
    
    std::unique_ptr< t_block > mu32_block_password ;
    std::unique_ptr< unsigned char[] > m8_block_psw_pma ; // c - char
    std::unique_ptr< t_block > mu32_block_source ;
    std::unique_ptr< t_block > mu32_block_dest ;
    
    uint32_t * m32_block_password = nullptr ;
    uint32_t * m32_block_source = nullptr ;
    uint32_t * m32_block_dest = nullptr ;
    
    Permutate m_Permutate ;

    void PMA_Xor_Psw( unsigned * p32_pma_in , unsigned * p32_pma_out ) ;
    
    void DisplayInfo() ;
    
    // Pointer to LFSR function
    void ( * mpShift )( unsigned * p_block ) ;
    
} ;


#endif // GAMMA_H_INCLUDED
