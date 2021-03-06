/* 
 * File:   W2CHybridTrie.cpp
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
 * Created on August 21, 2015, 4:18 PM
 */
#include "server/lm/models/w2c_hybrid_trie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "server/lm/lm_consts.hpp"

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<typename WordIndexType, template<phrase_length > class StorageFactory, class StorageContainer>
                    w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>::w2c_hybrid_trie(WordIndexType & word_index)
                    : layered_trie_base<w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>, WordIndexType, __W2CHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_unk_data(NULL), m_storage_factory(NULL) {
                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((LM_M_GRAM_LEVEL_MAX < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                        ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");
                        ASSERT_CONDITION_THROW((sizeof (uint32_t) != sizeof (word_uid)), string("Only works with a 32 bit word_uid!"));

                        //Check for the storage memory sized. This one is needed to be able to store
                        //N-gram probabilities in the C type container as its value! See description
                        //of the m_mgram_mapping data member. We missuse the mapping container for
                        //the last trie level by storing there probabilities instead of ids! 
                        const size_t float_size = sizeof (prob_weight);
                        const size_t idx_size = sizeof (TShortId);

                        //Assert on sizes
                        ASSERT_CONDITION_THROW((float_size > idx_size), string("Broken tries, sizeof(TLogProbBackOff) = ") +
                                to_string(float_size) + string(" is not equal to sizeof(TIndexSize) = ") + to_string(idx_size) +
                                string("! The implementation has been altered!"));

                        //Initialize the array of counters
                        memset(next_ctx_id, 0, NUM_IDX_COUNTERS * sizeof (TShortId));
                    }

                    template<typename WordIndexType, template<phrase_length > class StorageFactory, class StorageContainer>
                    void w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>::pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //01) Pre-allocate the word index super class call
                        BASE::pre_allocate(counts);

                        //Compute the number of words to be stored
                        m_word_arr_size = BASE::get_word_index().get_number_of_words(counts[0]);

                        //02) Allocate the factory
                        m_storage_factory = new StorageFactory<LM_M_GRAM_LEVEL_MAX>(counts);

                        //03) Allocate the main arrays of pointers where probs/back-offs will be stored

                        //First allocate the memory for the One-grams, add an extra
                        //element for the unknown word and initialize it!
                        m_mgram_data[0] = new m_gram_payload[m_word_arr_size];
                        memset(m_mgram_data[0], 0, m_word_arr_size * sizeof (m_gram_payload));

                        //Allocate more memory for probabilities and back off weight for
                        //the remaining M-gram levels until M < N. For M==N there is no
                        //back-off weights and thus we will store the probabilities just
                        //Inside the C container class values.
                        for (int idx = 1; idx < (LM_M_GRAM_LEVEL_MAX - 1); idx++) {
                            m_mgram_data[idx] = new m_gram_payload[counts[idx]];
                            memset(m_mgram_data[idx], 0, counts[idx] * sizeof (m_gram_payload));
                        }

                        //04) Allocate the word map arrays per level There is N-1 levels to have 
                        //as the for M == 0 - the One Grams, we do not need this mappings
                        for (int idx = 0; idx < (LM_M_GRAM_LEVEL_MAX - 1); idx++) {
                            m_mgram_mapping[idx] = new StorageContainer*[m_word_arr_size];
                            memset(m_mgram_mapping[idx], 0, m_word_arr_size * sizeof (StorageContainer*));
                        }
                    }

                    template<typename WordIndexType, template<phrase_length > class StorageFactory, class StorageContainer>
                    void w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>::set_def_unk_word_prob(const prob_weight prob) {
                        //Record the dummy probability and back-off values for the unknown word
                        m_unk_data = &m_mgram_data[0][UNKNOWN_WORD_ID];
                        m_unk_data->m_prob = prob;
                        m_unk_data->m_back = 0.0;

                    }

                    template<typename WordIndexType, template<phrase_length > class StorageFactory, class StorageContainer>
                    w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>::~w2c_hybrid_trie() {
                        //Delete the probability and back-off data
                        for (phrase_length idx = 0; idx < (LM_M_GRAM_LEVEL_MAX - 1); idx++) {
                            //Delete the prob/back-off arrays per level
                            if (m_mgram_data[idx] != NULL) {
                                delete[] m_mgram_data[idx];
                            }
                        }
                        //Delete the mapping data
                        for (phrase_length idx = 0; idx < (LM_M_GRAM_LEVEL_MAX - 1); idx++) {
                            //Delete the word arrays per level
                            if (m_mgram_mapping[idx] != NULL) {
                                for (TShortId widx = 0; widx < m_word_arr_size; widx++) {
                                    //Delete the C containers per word index
                                    if (m_mgram_mapping[idx][widx] != NULL) {
                                        delete m_mgram_mapping[idx][widx];
                                    }
                                }
                                delete[] m_mgram_mapping[idx];
                            }
                        }
                        if (m_storage_factory != NULL) {
                            delete m_storage_factory;
                        }
                    }

                    //Make sure that there will be templates instantiated, at least for the given parameter values
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_hybrid_trie, basic_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_hybrid_trie, counting_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_hybrid_trie, hashing_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_hybrid_trie, basic_optimizing_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(w2c_hybrid_trie, counting_optimizing_word_index);
                }
            }
        }
    }
}