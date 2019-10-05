// We'll have to put everything in one file: DISPLAY, EDIT_STRING, GETCH
// In order to make it easy to include it all from other projects 

//#include " display.h"
#include "../Display/ConsoleDisplay/include/display.h"

//----- GETCH -----

#ifdef LINUX
#include <iostream>
#include <unistd.h>
#include <termios.h>

int getch()
{
    int ch;
    struct termios oldt, newt;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}
#endif

//----- END OF GETCH -----



#include <string>
#include <iostream>
#include <cassert>


//----- EDIT_STRING -----


void edit_string( std::string &str )
{
    EditString es( str ) ;
    es.Do() ;
}

void EditString::Symbol::PrintSymbol()
{
    if ( c1 == 9 ) 
        std::cout << tab_str ;
    else
        std::cout << c1 ;
    if ( memory_size > 1 )
        std::cout << c2 ;
}

void EditString::Symbol::GetCurr( const UTF8String & es  )
{
    assert( es.mem_poz < es.mr_str.size() ) ;
    c1 = es.mr_str[ es.mem_poz ] ;
    
    if ( c1 == 9 )
    {
        display_size = tab_str_size ;
        memory_size = 1 ;
        return ;
    }
    
    if ( c1 > 0 ) // то есть < 128 , т.е. один байт
    {
        display_size = 1 ;
        memory_size = 1 ;
    }
    else // т.е. два байта (или более)
    {
        assert( es.mem_poz + 1 < es.mr_str.size() ) ;
        c2 = es.mr_str[ es.mem_poz + 1 ] ;
        assert( c2 < 0 ) ;
        display_size = 1 ;
        memory_size = 2 ;
    }

}

bool EditString::Symbol::GetPrev( const UTF8String & es )
{
    assert( es.mem_poz <= es.mr_str.size() ) ;
    if ( es.mem_poz < 1 )
        return false ;
    
    c1 = es.mr_str[ es.mem_poz -1 ] ;
    
    if ( c1 == 9 )
    {
        display_size = tab_str_size ;
        memory_size = 1 ;
        return true ;
    }

    
    if ( c1 > 0 ) // то есть < 128 , т.е. один байт
    {
        display_size = 1 ;
        memory_size = 1 ;
        return true ;
    }
    else // т.е. два байта (или более)
    {
        if ( es.mem_poz - 2 < 0 )
            return false ;
        c2 = es.mr_str[ es.mem_poz - 2 ] ;
        assert( c2 < 0 ) ;
        display_size = 1 ;
        memory_size = 2 ;
        return true ;
    }
}

void EditString::UTF8String::Increment( Symbol s )
{
    mem_poz += s.memory_size ;
    display_poz += s.display_size ;
}

void EditString::UTF8String::Decrement( Symbol s )
{
    mem_poz -= s.memory_size ;
    display_poz -= s.display_size ;
    assert( mem_poz >= 0 ) ; 
    assert( display_poz >=0 ) ;
}

void EditString::print_str( UTF8String & es )
{
    //byte_number = 0 ;
    es.mem_poz = 0 ;
    while (  es.mem_poz < es.mr_str.size() )
    {
        //print_symbol( c ) ;
        Smbl.GetCurr( es ) ;
        Smbl.PrintSymbol() ;
        es.Increment( Smbl ) ;
    }    
}

void EditString::InputChar( char c )
{
    if ( c == 9 )
    {
        std::cout << tab_str ;
        EStr.mr_str.insert( EStr.mem_poz  , 1 , c ) ; 
        EStr.mem_poz ++ ;
        EStr.display_poz += tab_str_size ;
        OutRemainder() ;
        return ;
    }
        
    std::cout << c ;
    EStr.mr_str.insert( EStr.mem_poz  , 1 , c ) ; 
    EStr.mem_poz ++ ;
    if ( c > 0 ) // one byte symbol
    {
        EStr.display_poz ++ ;
        OutRemainder() ;
    }
    else
    {
        if ( byte_number == 0 )
            byte_number ++ ;
        else
        {
            EStr.display_poz ++ ;
            OutRemainder() ;
            byte_number = 0 ;
        }
        
    }
}

void EditString::Do()
{
    // UTF8
    // std::cout << mr_str ;
    // Никаких cout << string, будем выводить посимвольно, и считать смещение в памяти
    // Ибо считаем, что если char < 0x80 , то это однобайтовый, иначе двубайтовый
    // Проверку на 3-х и 4-х (и т.д.) байтовый символ проводить не будем
    // Расчитываем только на 2-байтовые символы
    
    print_str( EStr ) ;
    
    //поехали
    byte_number = 0 ; // for InputChar
    
    char c = getch() ;
    while ( c != '\n' ) 
    {                               //loop breaks if you press enter
        if ( c == 127 ) //for backspace(127) and delete(8) keys
            BackSpace() ;
        else if ( c == 27 )    //arrow
            Arrow() ;
        else 
        {
            InputChar( c ) ;
        }
        c = getch();
    }
}


void EditString::OutRemainder()
{
    //int n_remainder = EStr.mr_str.size() - EStr.mem_poz ;
    std::string remndr_str( EStr.mr_str.substr( EStr.mem_poz , EStr.mr_str.size() - EStr.mem_poz ) ) ;
    UTF8String es( remndr_str ) ;
    print_str( es ) ;
    std::cout << "   "    ; //for backspace , 3 spaces, because length of tab_symb = 3 , in our case
    for ( unsigned i = 0 ; i < es.display_poz + 3 ; ++i )
        std::cout << '\b' ;
}

void EditString::BackSpace()
{
    // ну, во-первых может быть нечего стирать
    if ( EStr.mem_poz == 0 )
    {
        return ;
    }
    
    if ( Smbl.GetPrev( EStr ) )
    {
        EStr.Decrement( Smbl ) ;
        EStr.mr_str.erase( EStr.mem_poz , Smbl.memory_size ) ;
        for ( unsigned i = 0 ; i < Smbl.display_size ; i++ )
        {
                std::cout << "\b" ;        //removes last character on the console
        }
        if ( EStr.mem_poz == EStr.mr_str.size() ) // забелим
        {
            for ( unsigned i = 0 ; i < Smbl.display_size ; i++ )
            {
                    std::cout << " " ;
            }
            for ( unsigned i = 0 ; i < Smbl.display_size ; i++ )
            {
                    std::cout << "\b" ;        //removes last character on the console
            }
        }
        else
        OutRemainder() ;
    }
}

void EditString::Delete()
{
    // ну, во-первых может быть нечего удалять
    if ( EStr.mem_poz + 1 > EStr.mr_str.size() )
        return ;
    // по хорошему надо было сделать проверку , что есть ещё два байта, если там 2-байтовый символ
    
    Smbl.GetCurr( EStr ) ;
    EStr.mr_str.erase( EStr.mem_poz , Smbl.memory_size ) ;
    OutRemainder() ;
}


void EditString::Arrow()
{
    char c = getch() ;
    if ( c == 91 )
    {
        char c = getch() ;
        if ( c == 68 ) // left arrow
        {
            if ( Smbl.GetPrev( EStr ) )
            {
                EStr.Decrement( Smbl ) ;
                int count = 1 ;
                if ( Smbl.c1 == 9 )
                    count = tab_str_size ;
                for ( int i=0 ; i < count ; ++i )
                    std::cout << "\b" ;
            }
        }
        else if ( c == 67 ) // right arrow
        {
            if ( EStr.mem_poz < EStr.mr_str.size() )
            {
                Smbl.GetCurr( EStr ) ;
                
                if ( EStr.mem_poz + Smbl.memory_size <= EStr.mr_str.size() )
                {
                    Smbl.PrintSymbol() ;
                    EStr.Increment( Smbl ) ;
                }
            }
        }
        else if ( c == 51 ) 
        {
            c = getch(); 
            if ( c == 126 )
                Delete() ;
        }
    }
}


//----- END OF EDIT_STRING -----




//----- DISPLAY------


void display_str( const std::string & str )
{
    std::cout << str << std::endl ;
}

void display_err( const std::string & str )
{
    std::cerr << str << std::endl ;
}

void do_progress_bar()
{
    std::cout << '.'; std::cout.flush();
}

void progress_bar_done()
{
    std::cout << std::endl ;
}

//----- END OF DISPLAY------

