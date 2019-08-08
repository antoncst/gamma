
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>

#include "gamma.h"
#include "../../Display/ConsoleDisplay/include/display.h"
#include "helper.h"
#include <cstdio>

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

    try
    {
        OpenFiles( in_filename , out_filename , ifs , ofs ) ;
    }
    catch ( const char * s )
    {
        display_err( s ) ;
        return 3 ;
    }
    
    
    string password ;
    password = "1234567890AB" ;
    
    if ( ! EnterPassword( password , parser.psw_input_twice ) )
        return 2 ;
    
    assert( ifs.is_open() ) ;
    assert( ofs.is_open() ) ;

    GammaCrypt gm( ifs , ofs , password ) ;

    try 
    {
        if ( parser.action == parser.none || parser.action == parser.encrypt )
            gm.Encrypt() ;
        else
            gm.Decrypt() ;
    }
    catch( const char * s) 
    {
        display_err( string( s ) ) ;
        return 1 ;
    }
        
    ifs.close() ;
    ofs.close() ;
    
    #ifdef DEBUG
        std::cout << " Press Enter" << std::endl ;
        getchar() ;
    #endif
    
    return 0;
}
