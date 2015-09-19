/* 
 * File:   W2COrderedArrayTrie.cpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on August 27, 2015, 08:33 PM
 */
#include "W2COrderedArrayTrie.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;

using namespace __W2CArrayTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, typename WordIndexType>
            W2CArrayTrie<N, WordIndexType>::W2CArrayTrie(WordIndexType & word_index)
            : ALayeredTrie<N, WordIndexType>(word_index,
            [&] (const TShortId wordId, TLongId & ctxId, const TModelLevel level) -> bool {

                return this->getContextId(wordId, ctxId, level); }, __W2CArrayTrie::DO_BITMAP_HASH_CACHE),
            m_num_word_ids(0), m_1_gram_data(NULL), m_N_gram_word_2_data(NULL) {

                //Memset the M/N grams reference and data arrays
                memset(m_M_gram_word_2_data, 0, ALayeredTrie<N, WordIndexType>::NUM_M_GRAM_LEVELS * sizeof (T_M_GramWordEntry *));
            }

            template<TModelLevel N, typename WordIndexType>
            void W2CArrayTrie<N, WordIndexType>::pre_allocate(const size_t counts[N]) {
                //01) Pre-allocate the word index super class call
                ALayeredTrie<N, WordIndexType>::pre_allocate(counts);

                //02) Pre-allocate the 1-Gram data
                m_num_word_ids = ATrie<N, WordIndexType>::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new TProbBackOffEntry[m_num_word_ids];
                memset(m_1_gram_data, 0, m_num_word_ids * sizeof (TProbBackOffEntry));

                //03) Insert the unknown word data into the allocated array
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                //04) Allocate data for the M-grams

                for (TModelLevel i = 0; i < ALayeredTrie<N, WordIndexType>::NUM_M_GRAM_LEVELS; i++) {
                    preAllocateWordsData<T_M_GramWordEntry>(m_M_gram_word_2_data[i], counts[i + 1], counts[0]);
                }

                //05) Allocate the data for the N-Grams 
                preAllocateWordsData<T_N_GramWordEntry>(m_N_gram_word_2_data, counts[N - 1], counts[0]);
            }

            template<TModelLevel N, typename WordIndexType>
            W2CArrayTrie<N, WordIndexType>::~W2CArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < ALayeredTrie<N, WordIndexType>::NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_M_gram_word_2_data[i];
                    }
                    delete[] m_N_gram_word_2_data;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class W2CArrayTrie<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class W2CArrayTrie<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template class W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template class W2CArrayTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;
        }
    }
}

