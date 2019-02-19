#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "platform.h"

#include <string>
#include <fstream>

using namespace std ;

class GammaCrypt
{
    static const size_t block_size = 32 ;
    static const size_t quantum_size = sizeof ( unsigned) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    static const size_t n_quantum = block_size / quantum_size ;
    typedef unsigned t_block[ n_quantum ] ;
    typedef unsigned t_block_random[ n_quantum + 1 ] ;
    
    void GenerateRandom() ;
    bool OpenFiles() ;  // in and out files
    void WriteHead() ;
    void Crypt() ;
    void CloseFiles() ;
    void ReadHead() ;
    
    t_block_random m_block_random ;
    t_block m_block_password ;
    t_block m_block_source ;
    t_block m_block_dest ;

    const string m_in_filename ;
    const string m_out_filename ;
    const string m_password ;
    
    ifstream m_ifs ;
    ofstream m_ofs ;
    
public:
    GammaCrypt( const string in_filename, const string out_filename, const string password ) ;
    void Encrypt() ;
    void Decrypt() ;
    
} ;

static const string help_string  = 
    "Usage:\n" 
    "gamma [-command] [file1] [file2]\n"
    "commands:\n"
    "-e --encrypt [source_file] [encrypted_file]\n"
    "-d --decrypt [encrypted_file] [source_file]\n"
    "default command is --encrypt\n"
    "default source_file is 'source'\n"
    "default encrypted_file is'encrypted'\n"
    "Examples:\n"
    "gamma   \n"
    "       encrypt file 'source' to file 'encrypted'\n"
    "gamma -d\n"
    "       decrypt file 'encrypted' to file 'source'\n"
    "gamma -e example.txt example.crypted\n"
    "       encrypt file 'example.txt' to file 'example.crypted'\n"
    "gamma -d example.crypted\n"
    "       decrypt file 'example.crypted' to file 'source'\n"
    ;


class CmdLnParser
{
public:
    bool m_b_error ;
    enum e_action
    {
        none,
        encrypt,
        decrypt
    } action ;
    int password ; // 0 - password is not specified
                         // otherwise number of argv
    int infile ;
    int outfile ;

    void ParseCommandLine( int argc , char **argv ) ;
    
private:

} ;

#endif // GAMMA_H_INCLUDED
