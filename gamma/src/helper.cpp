
#include "helper.h"
#include "display.h"
#include "platform.h"

using namespace std ;

#ifdef GAMMA_LINUX
#include <termios.h>
#include <unistd.h>
#endif

#ifdef GAMMA_WINDOWS
#include <windows.h>
#include <conio.h>
#endif

#include <iostream> // for EnterPassword

bool EnterPassword( string & password , bool twice )
{
    //make console input invisible

#ifdef GAMMA_LINUX
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif
#ifdef GAMMA_WINDOWS
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
#ifdef GAMMA_LINUX
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
#ifdef GAMMA_WINDOWS
    SetConsoleMode(hStdin, mode);
#endif

    return result ;  
}


// OpenFile
// in:  string in_filename, string out_filename
// out: ifstream & ifs, ofstream & ofs
bool OpenFiles(const std::string in_filename, const std::string out_filename /* input and output files */
                , ifstream & ifs , ofstream & ofs )
{
    ifs.open( in_filename.c_str() , ifstream::in | ifstream::binary ) ;
    if ( ! ifs.is_open() )
    {
        display_err( " Error opening input file for reading" ) ;
        return false ;
    }

    ofs.open( out_filename.c_str(), ifstream::out | ifstream::binary | ifstream::trunc ) ;

    if ( ! ofs.is_open() )
    {
        display_err( " Error opening output file for writing" ) ;
        return false ;
    }

    return true ;
}



CmdLnParser::CmdLnParser( std::string & in_filename , std::string & out_filename )
    : m_in_filename( in_filename) , m_out_filename( out_filename ) 
    {} ;

void CmdLnParser::ParseCommandLine( int argc , char **argv )
{
    m_b_error = false ;
    int arg_counter = 0;
    
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
        m_in_filename = "source" ;
        m_out_filename = "encrypted" ;
		psw_input_twice = true ;
    }
    else
    {
        m_in_filename = "encrypted" ;
        m_out_filename = "source" ;
		psw_input_twice = false ;
    }
    
    if ( n_infile )
        m_in_filename = argv[ n_infile ] ;
    if ( n_outfile )
        m_out_filename = argv[ n_outfile ] ;

}
