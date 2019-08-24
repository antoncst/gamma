
#include "helper.h"
#include "../Display/ConsoleDisplay/include/display.h"
#include "../platform.h"

using namespace std ;

#ifdef LINUX
#include <termios.h>
#include <unistd.h>
#endif

#ifdef WINDOWS
#include <windows.h>
#include <conio.h>
#endif

#include <iostream> // for EnterPassword

bool EnterPassword( string & password , bool twice )
{
    //make console input invisible

#ifdef LINUX
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
#ifdef WINDOWS
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#endif

    bool result = true ; //success

    cout << "Enter password:";
    string s1,s2;
    getline(cin, s1);

    if ( s1.size() > 0 )
    {
		if ( twice )
		{
			cout << "\nConfirm password:";
			getline(cin, s2);
	        
			if ( s1 != s2 )
			{
				cout << "\nPasswords do not match" << endl ;
				result = false ;  // fail
			}
			else
				password = s1 ;
		}
		else
			password = s1 ;
    }

    //restore console visibility
#ifdef LINUX
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
#ifdef WINDOWS
    SetConsoleMode(hStdin, mode);
#endif

    return result ;  
}


// OpenFile
// in:  string in_filename, string out_filename
// out: ifstream & ifs, ofstream & ofs
void OpenFiles(const std::string in_filename, const std::string out_filename /* input and output files */
                , ifstream & ifs , ofstream & ofs )
{
    ifs.open( in_filename.c_str() , ifstream::in | ifstream::binary ) ;
    if ( ! ifs.is_open() )
    {
        display_str( help_string ) ;
        throw( " Error opening input file for reading" ) ;
    }

    ofs.open( out_filename.c_str(), ifstream::out | ifstream::binary | ifstream::trunc ) ;

    if ( ! ofs.is_open() )
        throw( " Error opening output file for writing" ) ;

}



CmdLnParser::CmdLnParser( std::string & in_filename , std::string & out_filename )
    : m_in_filename( in_filename) , m_out_filename( out_filename ) 
    {} ;

void CmdLnParser::ParseCommandLine( int argc , char **argv )
{
    m_b_error = false ;
    int arg_counter = 0;
    mb_blocksize_specified = false ;
   
    action = none ;
    int n_infile = 0 ;  // the index of the argv argument that points to the file name
    int n_outfile = 0 ; // similarly
    
    bool now_keys_coming = true ;
    for ( int i = 1 ; i < argc ; i++ )
    {
        string s( argv[i] ) ;
        if ( now_keys_coming )
        {
            if ( s[0] == '-')
            {
                if ( s == "-e" || s == "--encrypt" )
                    if ( action == none )
                        action = encrypt ;
                    else
                        { m_b_error = true ; break ; } 
                else if ( s == "-d" || s == "--decrypt" )
                    if ( action == none )
                        action = decrypt ;
                    else
                        { m_b_error = true ; break ; } 
                else if ( s == "-s" || s == "--size" )
                    if ( i + 1 < argc  )
                    {
                        m_block_size = std::stoul( argv[ i+1 ] ) ;
                        i++ ;
                        mb_blocksize_specified = true ;
                    }
                    else
                        { m_b_error = true ; break ; } 
                else if ( s == "-h" || s == "--help" )
                    { m_b_error = true ; break ; } 
                else
                    { m_b_error = true ; break ; } 
            }
            else
                now_keys_coming = false ;
        }
        if ( !now_keys_coming )
        {
            if ( s[0] == '-')
                { m_b_error = true ; break ; } 
            else
            {
                arg_counter++ ;
                if ( arg_counter >2 )
                    { m_b_error = true ; break ; } 
                if ( arg_counter == 1 )
                    n_infile = i ;
                if ( arg_counter == 2 )
                    n_outfile = i ;
                    
                
            }
        }
    }
    if ( m_b_error )
    {
        display_str( help_string ) ;
        return ;
    }
    else
        if ( action == none)
            action = encrypt ;

    if ( action == encrypt)
    {
        m_in_filename ="source" ; // "/home/anton/ramdisk/source" ; //
        m_out_filename = "encrypted" ; // "/home/anton/ramdisk/encrypted" ; //
		psw_input_twice = true ;
    }
    else
    {
        m_in_filename = "encrypted" ; // "/home/anton/ramdisk/encrypted" ; //
        m_out_filename = "source" ; // "/home/anton/ramdisk/source" ; //
		psw_input_twice = false ;
    }
    
    if ( n_infile )
        m_in_filename = argv[ n_infile ] ;
    if ( n_outfile )
        m_out_filename = argv[ n_outfile ] ;

}
