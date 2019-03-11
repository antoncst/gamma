
#include <string>
#include <iostream>

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
