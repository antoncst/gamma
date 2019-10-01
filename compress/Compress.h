#pragma once
#include <string>
#include <array>
#include <map>

class Compress
{
    struct AuxData  // auxiliary data
    {
        int idx ;    // index of arr
        int count ;  // number of repeats
    } ;
    
    typedef std::array< char , 16 > t_arr16 ;
    typedef std::array< char , 14 > t_arr14 ;
    typedef std::array< char , 13 > t_arr13 ;
    typedef std::array< char , 10 > t_arr10 ;
    typedef std::array< char , 8 > t_arr8 ;
    typedef std::array< char , 6 > t_arr6 ;

    typedef std::array< char , 5 > t_arr5 ;
    typedef std::array< char , 4 > t_arr4 ;
    typedef std::array< char , 3 > t_arr3 ;
    typedef std::array< char , 2 > t_arr2 ;
    typedef std::array< char , 1 > t_arr1 ;


    typedef std::map< t_arr16 , AuxData > t_map16 ;
    typedef std::map< t_arr14 , AuxData > t_map14 ;
    typedef std::map< t_arr13 , AuxData > t_map13 ;
    typedef std::map< t_arr10 , AuxData > t_map10 ;
    typedef std::map< t_arr8 , AuxData > t_map8 ;
    typedef std::map< t_arr6 , AuxData > t_map6 ;

    typedef std::map< t_arr5 , AuxData > t_map5 ;
    typedef std::map< t_arr4 , AuxData > t_map4 ;
    typedef std::map< t_arr3 , AuxData > t_map3 ;
    typedef std::map< t_arr2 , AuxData > t_map2 ;
    typedef std::map< t_arr1 , AuxData > t_map1 ;


    t_map16 map16 ;
    t_map14 map14 ;
    t_map13 map13 ;
    t_map10 map10 ;
    t_map8 map8 ;
    t_map6 map6 ;

    t_map5 map5 ;
    t_map4 map4 ;
    t_map3 map3 ;
    t_map2 map2 ;
    t_map1 map1 ;

    unsigned curr_idx13 ;
    unsigned curr_idx10 ;
    unsigned curr_idx8 ;
    unsigned curr_idx6 ;
    unsigned curr_idx5 ;
    unsigned curr_idx4 ;
    unsigned curr_idx3 ;
    unsigned curr_idx2 ;
    unsigned curr_idx1 ;
    
    // количество уже просмотренных вперёд байт, которые non consecutive and equal
    int n_forward_eqbytes_searched ;

    template< typename Tarr, typename Tmap >
    bool Search( char * in_data, int data_size , Tmap & mapN,  int N , int & i , unsigned & curr_idx ) noexcept ;
    
    template< typename Tarr, typename Tmap >
    void Reorganize( Tmap & mapN ) noexcept ;

    // Search consecutive equal bytes
    // returns number of repeatitions :
    unsigned SearchEqBytes( char * in_data, int data_size ) noexcept ;

    void SearchForwardEqBytes( char * in_data , int data_size , int N , int & i ) noexcept ;
    
public:
    void Do(  char * in_data , int data_size ) noexcept ; 

};

