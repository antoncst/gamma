#pragma once

#include <string>

struct GammaCrypt
{
    
    virtual void Encrypt() = 0 ;
    virtual void Decrypt() = 0 ;
    //Generate keys to file:
    virtual void GenKeyToFile() = 0 ;
    virtual void SetBlockSize( unsigned block_size ) = 0 ;

    // mb_use_keyfile == true    use keyfile
    // mb_use_keyfile == false   generate keys and stores them into encrypted file (default)
    virtual void UseKeyfile( bool use_keyfile ) = 0 ;
    
    virtual void SetKeyFilename( const std::string & keyfilename ) = 0;
    virtual const std::string & GetKeyFilename() = 0;
    
    // m_perm_bytes == true    permutate bytes (default)
    // m_perm_bytes == false   permutate bits
    virtual void PermutateBytes( bool perm_bytes ) = 0;

    virtual ~GammaCrypt() {} ;
} ;