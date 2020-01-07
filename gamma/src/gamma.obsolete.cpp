inline void GammaCrypt::MakeConfusion( unsigned * psrc) noexcept
{
    for ( unsigned j = 0 ; j <= 9 /*m_quantum_size * 8*/ ; j++ )
    {
        //arithmetic manipulations
        for ( unsigned i = 1 ; i < m_blk_sz_words ; i++ )
        {
            psrc[ i ] -= psrc[ i - 1 ] ;
        }
        psrc[ 0 ] -= psrc[ m_blk_sz_words - 1 ] ;

        // left circular shift
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 32 bit rolls
            if ( j < 4 )
                psrc[i] = ( psrc[i] >> 1 ) | ( psrc[i] << 31 ) ; // 6 rolls
            else if ( j == 4 )
                psrc[i] = ( psrc[i] >> 4 ) | ( psrc[i] << 28 ) ; // 4 rolls
            else if ( j == 5 )
                psrc[i] = ( psrc[i] >> 2 ) | ( psrc[i] << 30 ) ; // 2 rolls
            else if ( j == 7 )
                psrc[i] = ( psrc[i] >> 5 ) | ( psrc[i] << 27 ) ; // 5 rolls
            else if ( j == 8 &&  ( i % 2 == 0 ) )
                psrc[i] = ( psrc[i] >> 16 ) | ( psrc[i] << 16 ) ; // 15 rolls
    }
}

inline void GammaCrypt::RemoveConfusion( unsigned * pdst ) noexcept
{
    for ( int j = 9 ; j >= 0 ; j-- )
    {
        // cycled shift right
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 38 bit rolls
            if ( j < 4 )
                pdst[i] = ( pdst[i] << 1 ) | ( pdst[i] >> 31 ) ; // 6 rolls
            else if ( j == 4 )
                pdst[i] = ( pdst[i] << 4 ) | ( pdst[i] >> 28 ) ; // 4 rolls
            else if ( j == 5 )
                pdst[i] = ( pdst[i] << 2 ) | ( pdst[i] >> 30 ) ; // 2 rolls
            else if ( j == 7 )
                pdst[i] = ( pdst[i] << 5 ) | ( pdst[i] >> 27 ) ; // 5 rolls
            else if ( j == 8 &&  ( i % 2 == 0 ) )
                pdst[i] = ( pdst[i] << 16 ) | ( pdst[i] >> 16 ) ; // 15 rolls
        //arithmetic manipulations
        pdst[ 0 ] += pdst[ m_blk_sz_words - 1 ] ;
        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
        {
            pdst[ i ] += pdst[ i - 1 ] ;
        }
    }
}




inline void MakeDiffusion( unsigned * psrc , unsigned m_blk_sz_words ) noexcept
{
    for ( unsigned j = 0 ; j <= 9 /*m_quantum_size * 8*/ ; j++ )
    {
        //arithmetic manipulations
        for ( unsigned i = 1 ; i < m_blk_sz_words ; i++ )
        {
            psrc[ i ] += psrc[ i - 1 ] ;
//            pkey[ i ] -= pkey[ i - 1 ] ;
        }
        psrc[ 0 ] += psrc[ m_blk_sz_words - 1 ] ;
//        pkey[ 0 ] -= pkey[ m_blk_sz_words - 1 ] ;

        // left circular shift
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 31 bit rolls
            if ( j < 4 )
            {
                psrc[i] = ( psrc[i] << 1 ) | ( psrc[i] >> 31 ) ; // 4 rolls
//                pkey[i] = ( pkey[i] >> 1 ) | ( pkey[i] << 31 ) ; // 4 rolls
            }
            else if ( j == 4 )
            {
                psrc[i] = ( psrc[i] << 4 ) | ( psrc[i] >> 28 ) ; // 4 rolls
//                pkey[i] = ( pkey[i] >> 4 ) | ( pkey[i] << 28 ) ; // 4 rolls
            }
            else if ( j == 5 )
            {
                psrc[i] = ( psrc[i] << 2 ) | ( psrc[i] >> 30 ) ; // 2 rolls
//                pkey[i] = ( pkey[i] >> 2 ) | ( pkey[i] << 30 ) ; // 2 rolls
            }
            else if ( j == 7 )
            {
                psrc[i] = ( psrc[i] << 5 ) | ( psrc[i] >> 27 ) ; // 5 rolls
//                pkey[i] = ( pkey[i] >> 5 ) | ( pkey[i] << 27 ) ; // 5 rolls
            }
            else if ( j == 8 &&  ( i % 2 == 0 ) )
            {
                psrc[i] = ( psrc[i] << 16 ) | ( psrc[i] >> 16 ) ; // 16 rolls
//                pkey[i] = ( pkey[i] >> 16 ) | ( pkey[i] << 16 ) ; // 16 rolls
            }
        //psrc[ i ] ^= pkey[ i ] ;
    }


inline void RemoveDiffusion( unsigned * pdst , unsigned m_blk_sz_words ) noexcept
{
    for ( int j = 9 ; j >= 0 ; j-- )
    {
        // cycled shift right
        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ ) // 38 bit rolls
            if ( j < 4 )
            {
                pdst[i] = ( pdst[i] >> 1 ) | ( pdst[i] << 31 ) ; // 6 rolls
//                pkey[i] = ( pkey[i] << 1 ) | ( pkey[i] >> 31 ) ; // 6 rolls
            }
            else if ( j == 4 )
            {
                pdst[i] = ( pdst[i] >> 4 ) | ( pdst[i] << 28 ) ; // 4 rolls
//                pkey[i] = ( pkey[i] << 4 ) | ( pkey[i] >> 28 ) ; // 4 rolls
            }
            else if ( j == 5 )
            {
                pdst[i] = ( pdst[i] >> 2 ) | ( pdst[i] << 30 ) ; // 2 rolls
//                pkey[i] = ( pkey[i] << 2 ) | ( pkey[i] >> 30 ) ; // 2 rolls
            }
            else if ( j == 7 )
            {
                pdst[i] = ( pdst[i] >> 5 ) | ( pdst[i] << 27 ) ; // 5 rolls
//                pkey[i] = ( pkey[i] << 5 ) | ( pkey[i] >> 27 ) ; // 5 rolls
            }
            else if ( j == 8 &&  ( i % 2 == 0 ) )
            {
                pdst[i] = ( pdst[i] << 16 ) | ( pdst[i] >> 16 ) ; // 15 rolls
//                pkey[i] = ( pkey[i] >> 16 ) | ( pkey[i] << 16 ) ; // 15 rolls
            }
        //arithmetic manipulations
        pdst[ 0 ] -= pdst[ m_blk_sz_words - 1 ] ;
//        pkey[ 0 ] += pkey[ m_blk_sz_words - 1 ] ;
        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
        {
            pdst[ i ] -= pdst[ i - 1 ] ;
//            pkey[ i ] += pkey[ i - 1 ] ;
        }
    }


//    for ( unsigned j = 0 ; j < m_quantum_size * 8 ; j++ )
//    {
//        // cycled shift right
//        for ( unsigned i = 0 ; i < m_blk_sz_words ; i++ )
//            pdst[ i ] = ( pdst[ i ] >> 1 ) | ( pdst[ i ] << 31 ) ;
//        //arithmetic manipulations
//        pdst[ 0 ] -= pdst[ m_blk_sz_words - 1 ] ;
//        for ( unsigned i = m_blk_sz_words -1 ; i > 0 ; i-- )
//        {
//            pdst[ i ] -= pdst[ i - 1 ] ;
//        }
//    }
}


