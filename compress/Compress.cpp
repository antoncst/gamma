#include "Compress.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <cstring>

unsigned weight ;
unsigned k ;


template< typename Tarr, typename Tmap >
bool Compress::Search( char * in_data, int data_size , Tmap & mapN , int N , int & i , unsigned & curr_idx ) noexcept
{
    SearchForwardEqBytes( in_data , data_size , N , i ) ;

    if ( data_size - i >=  N * 2  /*&& curr_idx < 32*/ )
    {
        char * res = std::search( in_data + i + N , in_data + data_size , in_data + i , in_data + i + N ) ;
        if ( res != in_data + data_size ) // found in data
        {
            Tarr arr ;
            memcpy ( &arr[0] , in_data + i , N ) ;
            //{ *( in_data ) , *( in_data + 1 ) , *( in_data + 2 ) , *( in_data + 3 ) } ;
            
            auto itf = mapN.find( arr ) ;
            if ( itf != mapN.end() ) // found in map
            {
                ++ ( itf->second.count ) ;
                if ( itf->second.count == 2 )
                {
                    itf->second.idx = curr_idx ;
                    curr_idx ++ ;
                }
            }
            else
            { 
                AuxData auxd ;
                auxd.count = 1 ;
                auxd.idx = -1 ; 
                mapN.insert( std::pair< Tarr , AuxData >( arr , auxd ) ) ;
            }
            
            i += N ;
            n_forward_eqbytes_searched -= N ;
            if ( n_forward_eqbytes_searched < 0 )
                n_forward_eqbytes_searched = 0 ;
            return true ;
        }
    }
    
    if ( N == 13 )
    {
        return Search< t_arr10, t_map10 >( in_data , data_size , map10 , 10 , i , curr_idx10 ) ;
    }
    else if ( N == 10 )
    {
        return Search< t_arr8, t_map8 >( in_data , data_size , map8 , 8 , i , curr_idx8 ) ;
    }
    else if ( N == 8 )
    {
        return Search< t_arr6, t_map6 >( in_data , data_size , map6 , 6 , i , curr_idx6 ) ;
    }
    else if ( N == 6 )
    {
        return Search< t_arr5, t_map5 >( in_data , data_size , map5 , 5 , i , curr_idx5) ;
    }
    else if ( N == 5 )
    {
        return Search< t_arr4, t_map4 >( in_data , data_size , map4 , 4 , i , curr_idx4 ) ;
    }
    else if ( N == 4 )
    {
        return Search< t_arr3, t_map3 >( in_data , data_size , map3 , 3 , i , curr_idx3 ) ;
    }
    else if ( N == 3 )
    {
        return Search< t_arr2, t_map2 >( in_data , data_size , map2 , 2 , i , curr_idx2 ) ;
    }
    else if ( N == 2 )
    {
        return Search< t_arr1, t_map1 >( in_data , data_size , map1 , 1 , i , curr_idx1 ) ;
    }

        ++ i ;
        n_forward_eqbytes_searched -- ;
        if ( n_forward_eqbytes_searched < 0 )
            n_forward_eqbytes_searched = 0 ;
    return false ;
}

unsigned Compress::SearchEqBytes(char* in_data, int data_size) noexcept
{
    int i = 0 ;
    while ( true )
    {
        if ( data_size < 3 )
        {
            //n_forward_eqbytes_searched = 3 ;
            break ;
        }
        if ( *( in_data ) != *( in_data + 1 ) )
        {
            break ;
            n_forward_eqbytes_searched = 2 ;
        }
        if ( *( in_data ) != *( in_data + 2 ) )
        {
            break ;
            n_forward_eqbytes_searched = 3 ;
        }
        
        i += 3 ;
        
        bool bend = false ;
        for ( int j = 0 ; i < (data_size - i ) || j < 18 ; ++i , ++j )
        {
            if ( *( in_data ) != *( in_data + i ) )
            {
                bend = true ;
                break ;
            }
        }
        if ( bend )
            break ;
    }
    return i ;
}

void Compress::SearchForwardEqBytes( char * in_data , int data_size , int N , int & i ) noexcept
{
    // Search consecutive equal bytes
    // Поиск последовательных одинаковых байт
    int neqb = 0 ; // Number of consecutive EQual Bytes
    //if ( n_forward_eqbytes_searched == 0 )
    {
        do
        {
            int j = 0 ;
            //bool bSuccess ;
            for ( ; j < N - n_forward_eqbytes_searched ; j++ ) // cycle for Search ;
            {
                neqb = SearchEqBytes( in_data + i + j , data_size - i - j ) ;
                if ( neqb ) 
                {
                    // out [in_data .. in_data + j)

                    // out repeating bytes
                    std::cout << " out repeating : " << int ( * (in_data + i + j ) ) << " n: " << neqb << std::endl ;
                    k += neqb - 2 ;

                    //bSuccess = true ;
                    break ;
                }
            }
            if ( neqb ) // found
            {
                // if ( j > 2 )
                //      Search3 ;
                //      Search2 ;
                i += neqb + j ;
            }
            else
                n_forward_eqbytes_searched += j ;
        } while ( neqb != 0 ) ;
    }
}

void Compress::Do(  char * in_data, int data_size ) noexcept
{
    k = 0 ;
    int i = 0 ; // index in in_data

    n_forward_eqbytes_searched = 0 ;

    curr_idx13 = 0 ;
    curr_idx10 = 0 ;
    curr_idx8 = 0 ;
    curr_idx6 = 0 ;
    curr_idx5 = 0 ;
    curr_idx4 = 0 ;
    curr_idx3 = 0 ;
    curr_idx2 = 0 ;
    curr_idx1 = 0 ;
    
    while ( i < data_size - 3)
    {
        Search< t_arr13, t_map13 >( in_data , data_size , map13 , 13 , i , curr_idx13 ) ;
/*        else 
        {
            bsuccess = Search< t_arr4, t_map4 >( in_data + i , data_size - i , map4 , 4 ) ;
            if ( bsuccess )
                i += 3 ;
            else 
            {
                bsuccess = Search< t_arr3, t_map3 >( in_data + i , data_size - i , map3 , 3 ) ;
                if ( bsuccess )
                    i += 2 ;
            }
        }
*/
    }

    std::cout << std::endl << "Data size: " << data_size << std::endl ;
    
    weight = 13 ;
    std::cout << std::endl << " ------------ arr 13 --------------" << std::endl ;
    Reorganize<t_arr13 ,  t_map13 >( map13 ) ;

    weight = 10 ;
    std::cout << std::endl << " ------------ arr 10 --------------" << std::endl ;
    Reorganize<t_arr10 ,  t_map10 >( map10 ) ;

    weight = 8 ;
    std::cout << std::endl << " ------------ arr 8 ---------------" << std::endl ;
    Reorganize<t_arr8 ,  t_map8 >( map8 ) ;

    weight = 6 ;
    std::cout << std::endl << " ------------ arr 6 ---------------" << std::endl ;
    Reorganize<t_arr6 ,  t_map6 >( map6 ) ;

    weight = 5 ;
    std::cout << std::endl << " ------------ arr 5 ---------------" << std::endl ;
    Reorganize<t_arr5 ,  t_map5 >( map5 ) ;

    weight = 4 ;
    std::cout << std::endl << " ------------ arr 4 ---------------" << std::endl ;
    Reorganize<t_arr4 ,  t_map4 >( map4 ) ;

    weight = 3 ;
    std::cout << std::endl << " ------------ arr 3 ---------------" << std::endl ;
    Reorganize<t_arr3 ,  t_map3 >( map3 ) ;
    
    weight = 2 ;
    std::cout << std::endl << " ------------ arr 2 ---------------" << std::endl ;
    Reorganize<t_arr2 ,  t_map2 >( map2 ) ;
    
    weight = 1 ;
    std::cout << std::endl << " ------------ arr 1 ---------------" << std::endl ;
    Reorganize<t_arr1 ,  t_map1 >( map1 ) ;
    
    std::cout << " k =  " << k ;

//    std::cout << std::endl << "arr 4" << std::endl ;
//    Reorganize<t_arr4 ,  t_map4 >( map4 ) ;

//    std::cout << std::endl << "arr 3" << std::endl ;
//    Reorganize<t_arr3 ,  t_map3 >( map3 ) ;
   
}

template< typename Tarr, typename Tmap >
void Compress::Reorganize( Tmap & mapN ) noexcept
{
    // Теперь надо взять map и отсортировать по count
    //                 arr      idx
    typedef std::pair< Tarr , int > t_pai ; // pair of arr and idx
    //              count                          
    std::multimap< int , t_pai > cmap ; // count map
    
    for ( auto val : mapN)
    {
        //                       count                                             arr 
        cmap.insert( std::pair< unsigned , t_pai > (val.second.count , t_pai( val.first , val.second.idx ) ) ) ;
    }
    
    //Удалим все элементы с count = 1 ;
//    cmap.erase( 1 ) ;
/*    
    //cout
    for ( auto & val : cmap)
    {
        for ( unsigned i = 0 ; i < 8 ; ++i ) 
        {
            std::cout << (int) val.second.first[i] << '\t';
        }
            std::cout << std::string
            { 
              val.second.first[0]  , val.second.first[1] , val.second.first[2] , val.second.first[3] ,
              val.second.first[4]  , val.second.first[5] , val.second.first[6] , val.second.first[7] 
            } 
                << " n: " << val.first 
                << "  index: " << val.second.second << std::endl ;
    }
*/
     
    //        count  QuantityOfArrs
    std::map<  int ,  int        > aux_map ;
    
    for ( auto & val : cmap )
            ++ aux_map[ val.first ] ;
    
    for ( auto & val : aux_map )
    {
            std::cout << "count: " << val.first << "  |  quantity of arrays: " << val.second << std::endl ;
            k += val.first * val.second * weight /*- val.second * weight*/ ;
    }
    
    std::cout << "cmap size: " << cmap.size() << std::endl ;

}