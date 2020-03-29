#pragma once

#include <string>
#include <fstream>

bool EnterPassword( std::string & password , bool twice ) ;


void OpenInFile(const std::string in_filename , std::ifstream & ifs ) ;

void OpenOutFile( const std::string out_filename , std::ofstream & ofs ) ;


class CmdLnParser
{

public:
    std::string m_in_filename ;
    std::string m_out_filename ;
    std::string m_keyfilename ;

    CmdLnParser() ;
    void ParseCommandLine( int argc , char **argv ) ;

   	bool psw_input_twice ;
    bool m_b_error ;

    enum e_action
    {
        none ,
        encrypt ,
        decrypt ,
        genkey
    } action ;
    
    bool mb_blocksize_specified = false ;
    bool mb_use_keyfile = false ;
    bool mb_perm_bits = false ;
    unsigned m_block_size ;

/*    int password ; // 0 - password is not specified
*/                         // otherwise number of argv

} ;

static const std::string help_string  = 
    "Usage:\n" 
    "gamma [-command] [-s N] [file1] [file2]\n"
    "\n"
    "commands:\n"
    "-e --encrypt [source_file] [encrypted_file]\n"
    "-d --decrypt [encrypted_file] [source_file]\n"
    "-gk --genkey    generate keys to keyfile\n"
    "\n"
    "default command is --encrypt\n"
    "default source_file is 'source'\n"
    "default encrypted_file is 'encrypted'\n"
    "\n"
    "-s N  :  block size. N should be a power of 2.\n"
    "-k --keyfile :  use keyfile instead of generating keys.\n"
    "-pb --permutate_bits :  permutate bits, default is perumtate bytes."
    "\n"
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
