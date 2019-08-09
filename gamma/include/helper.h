#pragma once

#include <string>
#include <fstream>

bool EnterPassword( std::string & password , bool twice ) ;


void OpenFiles(const std::string in_filename, const std::string out_filename /* input and output files */
                , std::ifstream & ifs , std::ofstream & ofs ) ;


class CmdLnParser
{
    std::string & m_in_filename ;
    std::string & m_out_filename ;

public:

    CmdLnParser( std::string & in_filename , std::string & out_filename ) ;
    void ParseCommandLine( int argc , char **argv ) ;

   	bool psw_input_twice ;
    bool m_b_error ;

    enum e_action
    {
        none,
        encrypt,
        decrypt
    } action ;

/*    int password ; // 0 - password is not specified
*/                         // otherwise number of argv

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
