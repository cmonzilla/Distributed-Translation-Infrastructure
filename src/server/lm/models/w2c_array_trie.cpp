/* 
 * File:   W2CArrayTrie.cpp
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
#include "server/lm/models/w2c_array_trie.hpp"

#include <inttypes.h>   // std::uint32_t

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;

using namespace __W2CArrayTrie;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<typename WordIndexType>
                    w2c_array_trie<WordIndexType>::w2c_array_trie(WordIndexType & word_index)
                    : layered_trie_base<w2c_array_trie<WordIndexType>, WordIndexType, __W2CArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_unk_data(NULL), m_num_word_ids(0), m_1_gram_data(NULL), m_n_gram_word_2_data(NULL) {
                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((LM_M_GRAM_LEVEL_MAX < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                        ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");
                        ASSERT_CONDITION_THROW((sizeof (uint32_t) != sizeof (word_uid)), string("Only works with a 32 bit word_uid!"));

                        //Memset the M/N grams reference and data arrays
                        memset(m_m_gram_word_2_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (T_M_GramWordEntry *));
                    }

                    template<typename WordIndexType>
                    void w2c_array_trie<WordIndexType>::pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //01) Pre-allocate the word index super class call
                        BASE::pre_allocate(counts);

                        //02) Pre-allocate the 1-Gram data
                        m_num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);
                        m_1_gram_data = new m_gram_payload[m_num_word_ids];
                        memset(m_1_gram_data, 0, m_num_word_ids * sizeof (m_gram_payload));

                        //03) Allocate data for the M-grams

                        for (phrase_length i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                            preAllocateWordsData<T_M_GramWordEntry>(m_m_gram_word_2_data[i], counts[i + 1], counts[0]);
                        }

                        //04) Allocate the data for the N-Grams 
                        preAllocateWordsData<T_N_GramWordEntry>(m_n_gram_word_2_data, counts[LM_M_GRAM_LEVEL_MAX - 1], counts[0]);
                    }

                    template<typename WordIndexType>
                    void w2c_array_trie<WordIndexType>::set_def_unk_word_prob(const prob_weight prob) {
                        //Insert the unknown word data into the allocated array
                        m_unk_data = &m_1_gram_data[UNKNOWN_WORD_ID];
                        m_unk_data->m_prob = prob;
                        m_unk_data->m_back = 0.0;
                    }

                    template<typename WordIndexType>
                    w2c_array_trie<WordIndexType>::~w2c_array_trie() {
                        //Check that the one grams were allocated, if yes then the rest must have been either
                        if (m_1_gram_data != NULL) {
                            delete[] m_1_gram_data;
                            for (phrase_length i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                                delete[] m_m_gram_word_2_data[i];
                            }
                            delete[] m_n_gram_word_2_data;
                        }
                    }

                    //Make sure that there will be templates instantiated, at least for the given parameter values
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_array_trie, basic_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_array_trie, counting_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_array_trie, hashing_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_array_trie, basic_optimizing_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_array_trie, counting_optimizing_word_index);
                }
            }
        }
    }
}