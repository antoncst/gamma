
#include "gamma.h"

#include <string>
#include <iostream>

#ifdef GAMMA_LINUX
#include <termios.h>
#include <unistd.h>
#endif

#ifdef GAMMA_WINDOWS
#include <windows.h>
#include <conio.h>
#endif

using namespace std ;

int EnterPassword( string & password , bool twice = true )
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

    int result = 0 ; //success

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
				result = 1 ;  // fail
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
    //tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return result ;  
}

int main(int argc, char **argv)
{
    CmdLnParser parser ;
    parser.ParseCommandLine( argc , argv ) ;
    if  (parser.m_b_error )
        return 1;
    
    string in_file;
    string out_file ;
	bool psw_input_twice ;
    if ( parser.action == parser.encrypt)
    {
        in_file = "source" ;
        out_file = "encrypted" ;
		psw_input_twice = true ;
    }
    else
    {
        in_file = "encrypted" ;
        out_file = "source" ;
		psw_input_twice = false ;
    }
    string password ;
    //password = "1234567890AB" ;
    
    if ( EnterPassword( password , psw_input_twice ) == 1 )
        return 2 ;
    
    if ( parser.infile )
        in_file = argv[ parser.infile ] ;
    if ( parser.outfile )
        out_file = argv[ parser.outfile ] ;

    GammaCrypt gm( in_file , out_file , password ) ;

    if ( parser.action == parser.none || parser.action == parser.encrypt )
        gm.Encrypt() ;
    else
        gm.Decrypt() ;
    
    return 0;
}
