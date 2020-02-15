
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>

#include "gamma.h"
#include "../../Display/ConsoleDisplay/include/display.h"
#include "helper.h"
#include <cstdio>

int main(int argc, char **argv)
{
    //std::ios::sync_with_stdio(false);
    
    std::ifstream ifs ; 
    std::ifstream ifs_keyfile ;
    std::ofstream ofs ; // when generating keyfile , using this stream for keyfile

    CmdLnParser parser ;
    parser.ParseCommandLine( argc , argv ) ;
    if  (parser.m_b_error )
        return 1;
    
    // open in file
    if ( parser.action != parser.genkey )
    {
        try
        {
            OpenInFile( parser.m_in_filename , ifs ) ;
        }
        catch ( const char * s )
        {
            display_err( s ) ;
            return 3 ;
        }
    }
    
    std::string password ;
    password = "1234567890AB" ;
    
    GammaCrypt gm( ifs , ofs , password ) ;
    
    gm.m_keyfilename = "keyfile" ;
    gm.mb_use_keyfile = parser.mb_use_keyfile ;
    gm.m_perm_bytes = ! parser.mb_perm_bits ;

    
    if ( ! EnterPassword( password , parser.psw_input_twice ) )
        return 2 ;
    
    // open out file
    try
    {
        if ( parser.action != parser.genkey )
            OpenOutFile( parser.m_out_filename , ofs ) ;
        else // genkey
            OpenOutFile( gm.m_keyfilename , ofs ) ;
    }
    catch ( const char * s )
    {
        display_err( s ) ;
        return 4 ;
    }


    try 
    {
        if ( parser.action == parser.none || parser.action == parser.encrypt || parser.action == parser.genkey )
        {
            if ( parser.mb_blocksize_specified )
                gm.SetBlockSize( parser.m_block_size ) ;

            if ( parser.action == parser.none || parser.action == parser.encrypt )
                gm.Encrypt() ;
            else
                gm.GenKeyToFile() ;
        }
            else
                gm.Decrypt() ;
    }
    catch( const char * s) 
    {
        display_err( std::string( s ) ) ;
        return 1 ;
    }
        
    ifs.close() ;
    ofs.close() ;
    
    #ifdef DEBUG
        //std::cout << " Press Enter" << std::endl ;
        //getchar() ;
    #endif
    
    return 0;
} ;
