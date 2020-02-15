#include <check.h>
//#include <string>
#include "../../Display/ConsoleDisplay/include/display.h"
#include <sstream>

bool CheckPermutArr( uint16_t * arr , unsigned size )
{
    bool berr = false ;
    for ( unsigned i = 0 ; i < size - 1 ; i++ )
    {
        if ( arr[ i ] >= size )
        {
                berr = true ;
                std::stringstream sstream ;
                sstream << "CheckPermutArr error: too big value at " << i << std::endl ;
                display_err( sstream.str() ) ;
        }
        for ( unsigned j = i + 1 ; j < size ; j++ )
        {
            if ( arr[ i ] == arr[ j ] )
            {
                berr = true ;
                std::stringstream sstream ;
                sstream << "CheckPermutArr error: duples at " << i << " , " << j << std::endl ;
                display_err( sstream.str() ) ;
            }
        }
    }
    return ! berr ;
}