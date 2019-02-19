
#include <string>
#include <iostream>

void display_str( const std::string & str )
{
    std::cout << str << "\n" ;
}

void display_err( const std::string & str )
{
    std::cerr << str << "\n" ;
}

void do_progress_bar()
{
    std::cout << '.'; std::cout.flush();
}

void progress_bar_done()
{
    std::cout << '\n';
}