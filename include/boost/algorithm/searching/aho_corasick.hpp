/* 
   Copyright (c) Marek Kurdej 2014.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#ifndef BOOST_ALGORITHM_AHO_CORASICK_SEARCH_HPP
#define BOOST_ALGORITHM_AHO_CORASICK_SEARCH_HPP

#include <iomanip>
#include <iterator>     // for std::iterator_traits

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/algorithm/searching/detail/debugging.hpp>

#define  BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP

namespace boost { namespace algorithm {

namespace detail {
    template <typename T>
    struct ac_trie {
        typedef T value_type;
        typedef std::list<ac_trie*> Container;
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;
        
        ac_trie(value_type const& v, bool endOfWord = false)
            : value(v),
              end_of_word(endOfWord) {
        }
        
        // TODO: begin, end, cbegin, cend

        const_iterator find_child(value_type const& v) {
            return children.find(v);
        }
        
        bool is_end_of_word() const {
            return end_of_word;
        }
        
        ac_trie* suffix() const {
            return suffix;
        }

    private:
        Container children; // TODO: use a map or sth else
        value_type value;
        bool end_of_word;
        ac_trie* suffix;
    };
} // namespace detail

/*
    A templated version of the Aho-Corasick searching algorithm.
    
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
    class aho_corasick {
        typedef typename std::iterator_traits<patIter>::difference_type difference_type;
        typedef ac_trie node;
    public:
        aho_corasick ( patIter first, patIter last ) 
                : pat_first ( first ), pat_last ( last ),
                  k_pattern_length ( std::distance ( pat_first, pat_last )) {
            }
            
        ~aho_corasick () {}
        
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
        node* root;
        
        /// \fn do_search ( corpusIter corpus_first, corpusIter corpus_last )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param k_corpus_length The length of the corpus to search
        ///
        template <typename corpusIter>
        corpusIter do_search ( corpusIter corpus_first, corpusIter corpus_last ) const {
            typedef node::iterator iterator;
            typedef node::const_iterator const_iterator;

            corpusIter curPos = corpus_first;
            const corpusIter lastPos = corpus_last;
            asssert(root != nullptr);
            node* curNode = root;
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
#endif
            while ( curPos < corpus_last ) {
                const_iterator child = curNode->find_child(*curPos);
                if (child != curNode->cend()) {
                    // there is a child node with this value
                    curNode = child;
                    if (curNode->is_end_of_word()) {
                        // output
                        return curPos - depth(curNode);
                    }
                } else {
                    // no child with the searched value
                    // let's look at the suffix node
                    node* suffix = curNode->suffix();
                    curNode = suffix;
                    continue; // and if root?
                }
                // std::size_t j = k_pattern_length - 1;
                // while ( pat_first [j] == curPos [j] ) {
                // //  We matched - we're done!
                    // if ( j == 0 )
                        // return curPos;
                    // j--;
                    // }
                ++curPos;
                }
            
            return corpus_last;
            }
// \endcond
        };

/*  Two ranges as inputs gives us four possibilities; with 2,3,3,4 parameters
    Use a bit of TMP to disambiguate the 3-argument templates */

/// \fn aho_corasick_search ( corpusIter corpus_first, corpusIter corpus_last, 
///       patIter pat_first, patIter pat_last )
/// \brief Searches the corpus for the pattern.
/// 
/// \param corpus_first The start of the data to search (Random Access Iterator)
/// \param corpus_last  One past the end of the data to search
/// \param pat_first    The start of the pattern to search for (Random Access Iterator)
/// \param pat_last     One past the end of the data to search for
///
    template <typename patIter, typename corpusIter>
    corpusIter aho_corasick_search ( 
                  corpusIter corpus_first, corpusIter corpus_last, 
                  patIter pat_first, patIter pat_last )
    {
        aho_corasick<patIter> rk ( pat_first, pat_last );
        return rk ( corpus_first, corpus_last );
    }

    template <typename PatternRange, typename corpusIter>
    corpusIter aho_corasick_search ( 
        corpusIter corpus_first, corpusIter corpus_last, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        aho_corasick<pattern_iterator> rk ( boost::begin(pattern), boost::end (pattern));
        return rk ( corpus_first, corpus_last );
    }
    
    template <typename patIter, typename CorpusRange>
    typename boost::lazy_disable_if_c<
        boost::is_same<CorpusRange, patIter>::value, typename boost::range_iterator<CorpusRange> >
    ::type
    aho_corasick_search ( CorpusRange &corpus, patIter pat_first, patIter pat_last )
    {
        aho_corasick<patIter> rk ( pat_first, pat_last );
        return rk (boost::begin (corpus), boost::end (corpus));
    }
    
    template <typename PatternRange, typename CorpusRange>
    typename boost::range_iterator<CorpusRange>::type
    aho_corasick_search ( CorpusRange &corpus, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        aho_corasick<pattern_iterator> rk ( boost::begin(pattern), boost::end (pattern));
        return rk (boost::begin (corpus), boost::end (corpus));
    }


    //  Creator functions -- take a pattern range, return an object
    template <typename Range>
    boost::algorithm::aho_corasick<typename boost::range_iterator<const Range>::type>
    make_aho_corasick ( const Range &r ) {
        return boost::algorithm::aho_corasick
            <typename boost::range_iterator<const Range>::type> (boost::begin(r), boost::end(r));
        }
    
    template <typename Range>
    boost::algorithm::aho_corasick<typename boost::range_iterator<Range>::type>
    make_aho_corasick ( Range &r ) {
        return boost::algorithm::aho_corasick
            <typename boost::range_iterator<Range>::type> (boost::begin(r), boost::end(r));
        }

}}

#endif  //  BOOST_ALGORITHM_AHO_CORASICK_SEARCH_HPP
