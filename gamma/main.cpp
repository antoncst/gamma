
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>

#include "gamma.h"
#include "display.h"
#include "helper.h"

using namespace std ;

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    
    ifstream ifs ;
    ofstream ofs ;

    string in_filename;
    string out_filename ;

    CmdLnParser parser( in_filename , out_filename ) ;
    parser.ParseCommandLine( argc , argv ) ;
    if  (parser.m_b_error )
        return 1;
    
	//bool psw_input_twice ;

    
    if ( ! OpenFiles( in_filename , out_filename , ifs , ofs ) )
        return 3 ;
    
    string password ;
    password = "1234567890AB" ;
    
    if ( ! EnterPassword( password , parser.psw_input_twice ) )
        return 2 ;
    
    assert( ifs.is_open() ) ;
    assert( ofs.is_open() ) ;

    GammaCrypt gm( ifs , ofs , password ) ;

    if ( parser.action == parser.none || parser.action == parser.encrypt )
        gm.Encrypt() ;
    else
        gm.Decrypt() ;
        
    ifs.close() ;
    ofs.close() ;
    
    return 0;
}
