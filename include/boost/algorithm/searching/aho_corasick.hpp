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

#include <boost/shared_ptr.hpp>

#include <map>      // std::map
#include <queue>      // std::queue
#include <utility>  // std::make_pair, std::pair

// #define  BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP

namespace boost { namespace algorithm {

namespace detail {
    template <typename T>
    class ac_trie {
    public:
        typedef std::size_t size_type;
        typedef T value_type;
    private:
        typedef ac_trie* pointer_type;
        typedef ac_trie const* const_pointer_type;
        // typedef std::unique_ptr<ac_trie> unique_pointer_type;
        // typedef boost::scoped_ptr<ac_trie> unique_pointer_type;
        typedef boost::shared_ptr<ac_trie> shared_pointer_type;

        typedef std::map<value_type, shared_pointer_type> Container;
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;

    public:
        ac_trie() 
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
            : node_value(value_type()),
#else
            :
#endif
              end_of_word(false),
              suffix_node(this), // or NULL
              node_depth(0) {
            }
        
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
        ac_trie(value_type const& v, bool endOfWord, pointer_type suffixNode = NULL, size_type nodeDepth = 0)
            : node_value(v),
              end_of_word(endOfWord),
              suffix_node(suffixNode),
              node_depth(nodeDepth) {
            }
#else
        ac_trie(bool endOfWord, pointer_type suffixNode = NULL, size_type nodeDepth = 0)
            : end_of_word(endOfWord),
              suffix_node(suffixNode),
              node_depth(nodeDepth) {
            }
#endif
        
        /// @returns a pair consisting of an iterator (pointer) to the last inserted node (or to the node that prevented the insertion) and a bool denoting whether the insertion took place.
        template <typename patIter>
        std::pair<pointer_type, bool> insert(patIter first, patIter last) {
            BOOST_STATIC_ASSERT (( boost::is_same<typename std::iterator_traits<patIter>::value_type, value_type>::value ));
            pointer_type prev = NULL;
            pointer_type cur = this;
            patIter it = first;
            while (cur) {
                if (it == last) {
                    cur->set_end_of_word(true);
                    return std::make_pair(cur, false);
                    }
                prev = cur;
                cur = cur->find_child(*it++);
                }
            // prev is the node being the prefix [first, it)
            // we have to add the remaining [it, last) nodes
            cur = prev;
            --it; // go back to the place that wasn't found
            for (; it != last; ++it) {
                cur = cur->add_child(*it);
                }
            cur->set_end_of_word(true);
            
            finalize_trie(); // TODO: lazy execution?
            
            return std::make_pair(cur, true);
            }
        
        /// Finalizes the trie by updating suffix and output pointers
        /// E.g. if the trie contains sequences {abc, bca, ca}, then
        /// ab will point to b, abc to bc, bc to c, bca to ca, ca to a
        /// because b is the longest suffix of ab existing in the trie
        // TODO: output
        void finalize_trie() {
            pointer_type root = this;
            root->set_suffix(root);
            std::queue<pointer_type> Q;

#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
            std::cout << "Root:   '" << root->value() << "' - " << root->depth() << " (" << root << ")\n";
#endif
            // Set suffixes of all root's children to root
            // And add children to the queue
            for (iterator it = children.begin(); it != children.end(); ++it) {
                pointer_type child = it->second.get();
                assert(child);
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                std::cout << "Child:  '" << child->value() << "' - " << child->depth() << " (" << child << ")\n";
#endif
                child->set_suffix(root);
                Q.push(child);
                }

            // Update suffixes of all nodes through a breadth-first search
            while (Q.size() > 0) {
                // Pop an element from the queue and process it
                pointer_type current = Q.front();
                Q.pop();
                assert(current);
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                std::cout << "Current: " << current->value() << "\n";
#endif

                // Update suffixes for all children of the currently process node
                iterator itend = current->children.end();
                for (iterator it = current->children.begin(); it != itend; ++it) {
                    // Enqueue child
                    pointer_type child = it->second.get();
                    typename iterator::value_type::first_type const& value = it->first;
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                    assert(child->value() == value);
#endif
                    Q.push(child);
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                    std::cout << "Child:  '" << child->value() << "' - " << child->depth() << " (" << child << ")\n";
#endif

                    // Child's suffix will be a child of current's suffix if possible,
                    // Otherwise, it will be a child of currents' (suffix's)+ suffix if possible.
                    // Othwerise, root.
                    pointer_type v = current->suffix();
                    pointer_type found;
                    // Find v with a child having a value 'value'
                    while ((found = v->find_child(value)) == NULL) {
                        v = v->suffix();
                        assert(v);
                        if (v == root) {
                            found = root;
                            break;
                            }
                        }
                    assert(found);
                    assert(found != child);
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                    std::cout << "Suffix:  '" << found->value() << "' - " << found->depth() << " (" << found << ")\n";
#endif
                    child->set_suffix(found);
                    // out(child) := out(child) \cup out(fail(child));
                    // child->out = child->out + child->suffix->out;
                    }
                }
            }

        template <typename patIter>
        const_pointer_type find(patIter first, patIter last) const {
            BOOST_STATIC_ASSERT (( boost::is_same<typename std::iterator_traits<patIter>::value_type, value_type>::value ));
            const_pointer_type cur = this;
            while (cur) {
                if (first == last) {
                    break;
                    }
                cur = cur->find_child(*first++);
                }
            return cur;
            }

        pointer_type add_child(value_type const& v) {
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
            shared_pointer_type newChild(new ac_trie(v, false, suffix_node, depth() + 1));
            std::cout << "New child of '" << this->value() << "' is '" << newChild->value() << "'\n";
#else
            shared_pointer_type newChild(new ac_trie(false, suffix_node, depth() + 1));
#endif
            children[v] = newChild;
            return newChild.get();
            }

        pointer_type find_child(value_type const& v) const {
            const_iterator found = children.find(v);
            if (found != children.end()) {
                return found->second.get();
                }
            return NULL;
            }

        void set_end_of_word(bool endOfWord) {
            end_of_word = endOfWord;
            }

        bool is_end_of_word() const {
            return end_of_word;
            }
        
        pointer_type suffix() const {
            return suffix_node;
            }
        
        void set_suffix(pointer_type suffixNode) {
            suffix_node = suffixNode;
            }
        
        size_type depth() const {
            return node_depth;
            }

#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
        value_type value() const {
            return node_value;
            }
#endif

    private:
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
        value_type node_value;
#endif
        Container children;
        bool end_of_word;
        pointer_type suffix_node;
        // ac_trie* parent_node;
        size_type node_depth;
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
        typedef typename std::iterator_traits<patIter>::value_type value_type;
        typedef detail::ac_trie<value_type> node;
    public:
        aho_corasick ( patIter first, patIter last ) 
                : pat_first ( first ), pat_last ( last ),
                  k_pattern_length ( std::distance ( pat_first, pat_last )),
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                  root(' ', false, &root) {
#else
                  root(false, &root) {
#endif
            root.insert(pat_first, pat_last);
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
        node root;
        
        /// \fn do_search ( corpusIter corpus_first, corpusIter corpus_last )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param k_corpus_length The length of the corpus to search
        ///
        template <typename corpusIter>
        corpusIter do_search ( corpusIter corpus_first, corpusIter corpus_last ) const {
            // typedef typename node::iterator iterator;
            // typedef typename node* iterator;
            // typedef typename node::const_iterator const_iterator;
            // typedef typename node const* const_iterator;

            corpusIter curPos = corpus_first;
            // const corpusIter lastPos = corpus_last;
            node const* curNode = &root;
            while ( curPos < corpus_last ) {
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                std::cout << "At '" << *curPos << "'" << "... ";
#endif
                node const* child = curNode->find_child(*curPos);
                if (child) {
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                    std::cout << "Found." << " From '" << curNode->value() << "' to '" << child->value() << "'" << "\n";
#endif
                    // there is a child node with this value
                    curNode = child;
                    if (curNode->is_end_of_word()) {
                        // output
                        return curPos - curNode->depth() + 1;
                    }
                } else {
                    // no child with the searched value
                    // let's look at the suffix node
                    node* suffix = curNode->suffix();
                    curNode = suffix;
#ifdef BOOST_ALGORITHM_AHO_CORASICK_DEBUG_HPP
                    std::cout << "Not found. Back to '" << curNode->value() << "'" << "\n";
#endif
                    // continue; // and if root?
                }
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
