/* 
   Copyright (c) Marek Kurdej 2014.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#ifndef BOOST_ALGORITHM_NAIVE_SEARCH_HPP
#define BOOST_ALGORITHM_NAIVE_SEARCH_HPP

#include <iomanip>
#include <iterator>     // for std::iterator_traits

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/algorithm/searching/detail/debugging.hpp>

#define  BOOST_ALGORITHM_NAIVE_DEBUG_HPP

namespace boost { namespace algorithm {

/*
    A templated version of the naive (brute-force) searching algorithm.
    
    Requirements:
        * Random access iterators
        * The two iterator types (patIter and corpusIter) must 
            "point to" the same underlying type.
        * Additional requirements may be imposed buy the skip table, such as:
        ** Numeric type (array-based skip table)
        ** Hashable type (map-based skip table)

http://www-igm.univ-mlv.fr/~lecroq/string/node5.html

*/

    template <typename patIter>
    class naive {
        typedef typename std::iterator_traits<patIter>::difference_type difference_type;
    public:
        naive ( patIter first, patIter last ) 
                : pat_first ( first ), pat_last ( last ),
                  k_pattern_length ( std::distance ( pat_first, pat_last )) {
            }
            
        ~naive () {}
        
        /// \fn operator ( corpusIter corpus_first, corpusIter corpus_last, Pred p )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param p            A predicate used for the search comparisons.
        ///
        template <typename corpusIter>
        corpusIter operator () ( corpusIter corpus_first, corpusIter corpus_last ) const {
            BOOST_STATIC_ASSERT (( boost::is_same<
                typename std::iterator_traits<patIter>::value_type, 
                typename std::iterator_traits<corpusIter>::value_type>::value ));

            if ( corpus_first == corpus_last ) return corpus_last;  // if nothing to search, we didn't find it!
            if (    pat_first ==    pat_last ) return corpus_first; // empty pattern matches at start

            const difference_type k_corpus_length  = std::distance ( corpus_first, corpus_last );
        //  If the pattern is larger than the corpus, we can't find it!
            if ( k_corpus_length < k_pattern_length )
                return corpus_last;
    
        //  Do the search 
            return this->do_search ( corpus_first, corpus_last );
            }
            
        template <typename Range>
        typename boost::range_iterator<Range>::type operator () ( Range &r ) const {
            return (*this) (boost::begin(r), boost::end(r));
            }

    private:
/// \cond DOXYGEN_HIDE
        patIter pat_first, pat_last;
        const difference_type k_pattern_length;
        
        /// \fn do_search ( corpusIter corpus_first, corpusIter corpus_last )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param k_corpus_length The length of the corpus to search
        ///
        template <typename corpusIter>
        corpusIter do_search ( corpusIter corpus_first, corpusIter corpus_last ) const {
            corpusIter curPos = corpus_first;
            const corpusIter lastPos = corpus_last - k_pattern_length;
            while ( curPos <= lastPos ) {
            //  Do we match right where we are?
                std::size_t j = k_pattern_length - 1;
                while ( pat_first [j] == curPos [j] ) {
                //  We matched - we're done!
                    if ( j == 0 )
                        return curPos;
                    j--;
                    }
                ++curPos;
                }
            
            return corpus_last;
            }
// \endcond
        };

/*  Two ranges as inputs gives us four possibilities; with 2,3,3,4 parameters
    Use a bit of TMP to disambiguate the 3-argument templates */

/// \fn naive_search ( corpusIter corpus_first, corpusIter corpus_last, 
///       patIter pat_first, patIter pat_last )
/// \brief Searches the corpus for the pattern.
/// 
/// \param corpus_first The start of the data to search (Random Access Iterator)
/// \param corpus_last  One past the end of the data to search
/// \param pat_first    The start of the pattern to search for (Random Access Iterator)
/// \param pat_last     One past the end of the data to search for
///
    template <typename patIter, typename corpusIter>
    corpusIter naive_search ( 
                  corpusIter corpus_first, corpusIter corpus_last, 
                  patIter pat_first, patIter pat_last )
    {
        naive<patIter> rk ( pat_first, pat_last );
        return rk ( corpus_first, corpus_last );
    }

    template <typename PatternRange, typename corpusIter>
    corpusIter naive_search ( 
        corpusIter corpus_first, corpusIter corpus_last, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        naive<pattern_iterator> rk ( boost::begin(pattern), boost::end (pattern));
        return rk ( corpus_first, corpus_last );
    }
    
    template <typename patIter, typename CorpusRange>
    typename boost::lazy_disable_if_c<
        boost::is_same<CorpusRange, patIter>::value, typename boost::range_iterator<CorpusRange> >
    ::type
    naive_search ( CorpusRange &corpus, patIter pat_first, patIter pat_last )
    {
        naive<patIter> rk ( pat_first, pat_last );
        return rk (boost::begin (corpus), boost::end (corpus));
    }
    
    template <typename PatternRange, typename CorpusRange>
    typename boost::range_iterator<CorpusRange>::type
    naive_search ( CorpusRange &corpus, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        naive<pattern_iterator> rk ( boost::begin(pattern), boost::end (pattern));
        return rk (boost::begin (corpus), boost::end (corpus));
    }


    //  Creator functions -- take a pattern range, return an object
    template <typename Range>
    boost::algorithm::naive<typename boost::range_iterator<const Range>::type>
    make_naive ( const Range &r ) {
        return boost::algorithm::naive
            <typename boost::range_iterator<const Range>::type> (boost::begin(r), boost::end(r));
        }
    
    template <typename Range>
    boost::algorithm::naive<typename boost::range_iterator<Range>::type>
    make_naive ( Range &r ) {
        return boost::algorithm::naive
            <typename boost::range_iterator<Range>::type> (boost::begin(r), boost::end(r));
        }

}}

#endif  //  BOOST_ALGORITHM_NAIVE_SEARCH_HPP
