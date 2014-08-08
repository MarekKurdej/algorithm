/* 
   Copyright (c) Marek Kurdej 2014.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#include <boost/algorithm/searching/aho_corasick.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp> // for 'list_of()', tuple_list_of
using namespace boost::assign; // bring 'list_of()' into scope

#include <iostream>
#include <string>
#include <vector>


namespace ba = boost::algorithm;

namespace {

    typedef ba::detail::ac_trie<char> trie;
    typedef trie* ptrie;
    typedef trie const* cptrie;

    void check_suffix ( const std::string &needle, std::vector<boost::tuple<int, int> > &suffixes) {
        trie trie1;
        std::pair<ptrie, bool> insert_result1 = trie1.insert(needle.begin(), needle.end());
        ptrie ptrie1 = insert_result1.first;
        BOOST_REQUIRE(ptrie1);

        std::size_t suffixes_size = suffixes.size();
        for (std::size_t i = 0; i < suffixes_size; ++i) {
            int from = suffixes[i].get<0>();
            int to = suffixes[i].get<1>();

            cptrie cptrie2 = trie1.find(needle.begin(), needle.begin() + from + 1);
            BOOST_REQUIRE(cptrie2);
            BOOST_CHECK_EQUAL(cptrie2->depth(), from + 1);
            BOOST_REQUIRE(cptrie2->suffix() != NULL);
            // BOOST_CHECK_EQUAL(cptrie2->value(), *(needle.begin() + from - 1));
            BOOST_CHECK_EQUAL(cptrie2->suffix()->depth(), to);
            }
        }
    
// //  Check using iterators
    void check_one_insert_iter ( const std::string &needle, int expected_depth, char expected_last_value, bool expected_inserted = true ) {
        trie trie1;
        std::pair<ptrie, bool> insert_result1 = trie1.insert(needle.begin(), needle.end());
        BOOST_CHECK_EQUAL(insert_result1.second, expected_inserted);
        ptrie ptrie1 = insert_result1.first;
        BOOST_REQUIRE(ptrie1);
        BOOST_CHECK_EQUAL(ptrie1->is_end_of_word(), true);
        BOOST_CHECK_EQUAL(ptrie1->depth(), expected_depth);
        BOOST_CHECK_EQUAL(ptrie1->value(), expected_last_value);
        
        if (expected_depth >= 2) {
            int half_length = expected_depth / 2;
            cptrie cptrie2 = trie1.find(needle.begin(), needle.begin() + half_length);
            BOOST_REQUIRE(cptrie2);
            BOOST_CHECK_EQUAL(cptrie2->is_end_of_word(), false);
            BOOST_CHECK_EQUAL(cptrie2->depth(), half_length);
            BOOST_CHECK_EQUAL(cptrie2->value(), *(needle.begin() + half_length - 1));
            }
        }

    void check_one_insert ( const std::string &needle, int expected_depth, char expected_last_value, bool expected_inserted = true ) {
        check_one_insert_iter ( needle, expected_depth, expected_last_value, expected_inserted );
        // check_one_find_iter ( needle, expected_depth, expected_last_value, expected_inserted );
        // check_one_pointer ( haystack, needle, expected );
        // check_one_object ( haystack, needle, expected );
        }
    void check_one ( const std::string &needle, int expected_depth, char expected_last_value, bool expected_inserted = true ) {
        check_one_insert ( needle, expected_depth, expected_last_value, expected_inserted );
        }
    }


BOOST_AUTO_TEST_CASE( test_main )
{
    std::string needle1   ( "ANPANMAN" );
    std::string needle2   ( "MAN THE" );
    std::string needle3   ( "WE\220ER" );
    std::string needle4   ( "NOW " );   //  At the beginning
    std::string needle5   ( "NEND" );   //  At the end
    std::string needle6   ( "NOT FOUND" );  // Nowhere
    std::string needle7   ( "NOT FO\340ND" );   // Nowhere

    std::string needle11  ( "ABCDABD" );

    std::string needle12  ( "abracadabra" );

    std::string needle13   ( "" );

    check_one ( needle1, 8, 'N' );
    check_one ( needle2, 7, 'E' );
    check_one ( needle3, 5, 'R' );
    check_one ( needle4, 4, ' ' );
    check_one ( needle5, 4, 'D' );
    check_one ( needle6, 9, 'D' );
    check_one ( needle7, 9, 'D' );
    check_one ( needle11, 7, 'D' );
    check_one ( needle12, 11, 'a' );
    check_one ( needle13, 0, '\0', false );

    // ANPANMAN
    //    || ||
    //    12 12
    std::vector<boost::tuple<int, int> > suffixes1 = tuple_list_of(0, 0) (1, 0) (2, 0) (3, 1) (4, 2) (5, 0) (6, 1) (7, 2);
    check_suffix ( needle1, suffixes1);

    // ABCDABD
    //     ||
    //     12
    std::vector<boost::tuple<int, int> > suffixes11 = tuple_list_of(0, 0) (1, 0) (2, 0) (3, 0) (4, 1) (5, 2) (6, 0);
    check_suffix ( needle11, suffixes11);

    // abracadabra
    //    | | ||||
    //    1 1 1234
    std::vector<boost::tuple<int, int> > suffixes12 = tuple_list_of(0, 0) (1, 0) (2, 0) (3, 1) (4, 0) (5, 1) (6, 0) (7, 1) (8, 2) (9, 3) (10, 4);
    check_suffix ( needle12, suffixes12);

    // ""
    // std::vector<boost::tuple<int, int> > suffixes13 = tuple_list_of(0, 0);
    // check_suffix ( needle13, suffixes13);
    // TODO: check no nodes created, any find should fail

    // check_one ( needle1, 26 );
    // check_one ( needle2, 18 );
    // check_one ( needle3,  9 );
    // check_one ( needle4,  0 );
    // check_one ( needle5, 33 );
    // check_one ( needle6, -1 );
    // check_one ( needle7, -1 );

    // check_one ( needle1, -1 );   // cant find long pattern in short corpus

    // check_one ( needle11, 15 );
    // check_one ( needle12, 13 );

    // check_one ( needle13, 0 );   // find the empty string 
    // check_one ( needle1, -1 );  // can't find in an empty haystack

//  Mikhail Levin <svarneticist@gmail.com> found a problem, and this was the test
//  that triggered it.

  const std::string mikhail_pattern =   
"GATACACCTACCTTCACCAGTTACTCTATGCACTAGGTGCGCCAGGCCCATGCACAAGGGCTTGAGTGGATGGGAAGGA"
"TGTGCCCTAGTGATGGCAGCATAAGCTACGCAGAGAAGTTCCAGGGCAGAGTCACCATGACCAGGGACACATCCACGAG"
"CACAGCCTACATGGAGCTGAGCAGCCTGAGATCTGAAGACACGGCCATGTATTACTGTGGGAGAGATGTCTGGAGTGGT"
"TATTATTGCCCCGGTAATATTACTACTACTACTACTACATGGACGTCTGGGGCAAAGGGACCACG"
;
    const std::string mikhail_corpus = std::string (8, 'a') + mikhail_pattern;

    // check_one ( mikhail_corpus, mikhail_pattern, 8 );
    }
