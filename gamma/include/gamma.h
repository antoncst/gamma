#ifndef GAMMA_H_INCLUDED
#define GAMMA_H_INCLUDED

#include "platform.h"
#include "randomroutines.h"

#include <string>
#include <fstream>
#include <cstring>

class GammaCrypt
{
    static const size_t block_size = 64 ;    // in bytes
    static const size_t quantum_size = sizeof ( unsigned) ; // block size of one calculating operation (probably 32 or 64 bit) 
                                            // размер блока одной вычислительной операции (наверняка 32 или 64 бита)
    static const size_t n_quantum = block_size / quantum_size ;
    typedef unsigned t_block[ n_quantum ] ;
public:     // for random routines
    typedef unsigned t_block_random[ n_quantum + 1 ] ; // to transform Random block it is required one quantum more (therefore "+1")

private:    

    bool OpenFiles() ;  // input and output files

    //write header to outpuy file (i.e. Key) 
    void WriteHead() ;
    //read header from input file
    void ReadHead() ;

    // actually crypt algorythm
    // called from Encrypt(), Decrypt()
    void Crypt() ;
    
    void CloseFiles() ;
    
    t_block_random m_block_random ; 
    t_block m_block_password ;
    t_block m_block_source ;
    t_block m_block_dest ;

    const std::string m_in_filename ;
    const std::string m_out_filename ;
    const std::string m_password ;
    
    std::ifstream m_ifs ;
    std::ofstream m_ofs ;
    
public:
    GammaCrypt( const std::string in_filename, const std::string out_filename, const std::string password ) ;
    void Encrypt() ;
    void Decrypt() ;
    
} ;

static const std::string help_string  = 
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
/*    int password ; // 0 - password is not specified
*/                         // otherwise number of argv
    int infile ;  // the index of the argv argument that points to the file name
    int outfile ; // similarly

    void ParseCommandLine( int argc , char **argv ) ;
    
private:

} ;

#endif // GAMMA_H_INCLUDED
