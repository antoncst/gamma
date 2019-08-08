// We'll have to put everything in one file: DISPLAY, EDIT_STRING, GETCH
// In order to make it easy to include it all from other projects 

#pragma once

#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include "../platform.h"

#include <string>



int getch() ;


//----- EDIT_STRING -----

void edit_string( std::string &str ) ;

class EditString
{
public:
    EditString( std::string & str ) : EStr( str )  {} ;
    void Do() ;
private:

    struct UTF8String ;

    struct Symbol
    {
        size_t display_size ;
        size_t memory_size ;
        char c1 ;
        char c2 ;
        void GetCurr( const UTF8String & es ) ;
        bool GetPrev( const UTF8String & es ) ;
        void PrintSymbol() ;
    } ;

    struct UTF8String
    {
        std::string & mr_str ; // r - reference
        size_t display_poz ;
        size_t mem_poz ;
        //c-tor
        UTF8String( std::string & str ) : mr_str( str ) , display_poz( 0 ) , mem_poz( 0 ) {} ;
        void Increment( Symbol s ) ;
        void Decrement( Symbol s ) ;
    } ;

    UTF8String EStr ;

    Symbol Smbl ;


    void print_str( UTF8String & es ) ;

    void BackSpace() ;
    void Delete() ;
    void Arrow() ;
    void OutRemainder() ;
    void InputChar( char c ) ;
    
    unsigned byte_number ; // for InputChar, which byte (of multibyte sumbol) inputed ;

} ;

    //constans
    static const std::string tab_str = " · " ; 
    static const size_t tab_str_size  = 3 ;

//----- END OF EDIT_STRING -----


//----- DISPLAY------

void display_str( const std::string & str ) ;
void display_err( const std::string & str ) ;
void do_progress_bar() ;
void progress_bar_done() ;

//----- END OF DISPLAY------


#endif // DISPLAY_H_INCLUDED
