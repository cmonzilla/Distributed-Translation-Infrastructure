/* 
 * File:   C2WArrayTrie.hpp
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
 * Created on August 25, 2015, 11:10 AM
 */

#ifndef C2WORDEREDARRAYTRIE_HPP
#define C2WORDEREDARRAYTRIE_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"

#include "layered_trie_base.hpp"

#include "server/lm/dictionaries/aword_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"
#include "common/utils/containers/array_utils.hpp"

using namespace std;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::utils::containers::utils;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace __C2WArrayTrie {

                        /**
                         * This structure stores two things the word id
                         * and the corresponding probability/back-off data.
                         * It is used to store the M-gram data for levels 1 < M < N.
                         * @param id the word id
                         * @param payload the back-off and probability data
                         */
                        typedef struct {
                            TShortId id;
                            m_gram_payload payload;
                        } TWordIdPBData;

                        /**
                         * This is the less operator implementation
                         * @param one the first object to compare
                         * @param two the second object to compare
                         * @return true one.id < two.id
                         */
                        inline bool operator<(const TWordIdPBData & one, const TWordIdPBData & two) {
                            return (one.id < two.id);
                        }

                        /**
                         * Stores the information about the context id, word id and corresponding probability
                         * This data structure is to be used for the N-Gram data, as there are no back-offs
                         * It is used to store the N-gram data for the last Trie level N.
                         * @param ctx_id the context id
                         * @param word_id the word id
                         * @param prob the probability data
                         */
                        typedef struct {
                            TShortId word_id;
                            TShortId ctx_id;
                            prob_weight prob;
                        } TCtxIdProbData;

                        /**
                         * This is the compare operator implementation
                         * @param one the first object to compare
                         * @param two the second object to compare
                         * @return -1 if (word_id,ctx_id) < (word_id,ctx_id)
                         *          0 if (word_id,ctx_id) == (word_id,ctx_id)
                         *         +1 if (word_id,ctx_id) > (word_id,ctx_id)
                         */
                        inline int8_t compare(const TCtxIdProbData & one, const TCtxIdProbData & two) {
                            if (one.word_id < two.word_id) {
                                return -1;
                            } else {
                                if (one.word_id == two.word_id) {
                                    if (one.ctx_id < two.ctx_id) {
                                        return -1;
                                    } else {
                                        if (one.ctx_id == two.ctx_id) {
                                            return 0;
                                        } else {
                                            return +1;
                                        }
                                    }
                                } else {
                                    return +1;
                                }
                            }
                        };

                        //An alternative check: Is a tiny biut slower
                        //const TLongId key1 = TShortId_TShortId_2_TLongId(one.word_id, one.ctx_id);
                        //const TLongId key2 = TShortId_TShortId_2_TLongId(two.word_id, two.ctx_id);
                        //return (key1 < key2);

                        inline bool operator<(const TCtxIdProbData & one, const TCtxIdProbData & two) {
                            return (compare(one, two) < 0);
                        };

                        inline bool operator>(const TCtxIdProbData & one, const TCtxIdProbData & two) {
                            return (compare(one, two) > 0);
                        };

                        inline bool operator==(const TCtxIdProbData & one, const TCtxIdProbData & two) {
                            return (compare(one, two) == 0);
                        };
                    }

                    /**
                     * This is the Context to word array memory trie implementation class.
                     * 
                     * WARNING: This trie assumes that the M-grams (1 <= M < N) are added
                     * to the Trie in an ordered way and there are no duplicates in the
                     * 1-Grams. The order is assumed to be lexicographical as in the ARPA
                     * files! This is also checked if the sanity checks are on see Globals.hpp!
                     * 
                     * @param N the maximum number of levels in the trie.
                     */
                    template<typename WordIndexType>
                    class c2w_array_trie : public layered_trie_base<c2w_array_trie<WordIndexType>, WordIndexType, __C2WArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef layered_trie_base<c2w_array_trie<WordIndexType>, WordIndexType, __C2WArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

                        /**
                         * The basic constructor
                         * @param p_word_index the word index (dictionary) container
                         */
                        explicit c2w_array_trie(WordIndexType & p_word_index);

                        /**
                         * Allows to retrieve the unknown target word log probability penalty 
                         * @return the target source word log probability penalty
                         */
                        inline float get_unk_word_prob() const {
                            return m_unk_data->m_prob;
                        }

                        /**
                         * Computes the M-Gram context using the previous context and the current word id
                         * @see LayeredTrieBese
                         */
                        inline bool get_ctx_id(const phrase_length level_idx, const TShortId word_id, TLongId & ctx_id) const {
                            //Compute back the current level pure for debug purposes.
                            const phrase_length curr_level = level_idx + BASE::MGRAM_IDX_OFFSET;

                            //Perform sanity checks if needed
                            ASSERT_SANITY_THROW(((curr_level == LM_M_GRAM_LEVEL_MAX) || (curr_level < M_GRAM_LEVEL_2)), string("Unsupported level id: ") + std::to_string(curr_level));

                            LOG_DEBUG2 << "Searching for the next ctx_id of " << SSTR(curr_level)
                                    << "-gram with word_id: " << SSTR(word_id) << ", ctx_id: "
                                    << SSTR(ctx_id) << END_LOG;

                            //First get the sub-array reference. Note that, even if it is the 2-Gram
                            //case and the previous word is unknown (ctx_id == 0) we still can use
                            //the ctx_id to get the data entry. The reason is that we allocated memory
                            //for it but being for an unknown word context it should have no data!
                            TSubArrReference & ref = m_m_gram_ctx_2_data[level_idx][ctx_id];

                            LOG_DEBUG2 << "Got context mapping for ctx_id: " << SSTR(ctx_id)
                                    << ", with beginIdx: " << SSTR(ref.begin_idx) << ", endIdx: "
                                    << SSTR(ref.end_idx) << END_LOG;

                            //Check that there is data for the given context available
                            if (ref.begin_idx != BASE::UNDEFINED_ARR_IDX) {
                                //The data is available search for the word index in the array
                                //WARNING: The linear search here is much slower!!!
                                if (my_bsearch_id<TWordIdPBEntry>(m_m_gram_data[level_idx], ref.begin_idx, ref.end_idx, word_id, ctx_id)) {
                                    LOG_DEBUG2 << "The next ctx_id for word_id: " << SSTR(word_id) << ", is: " << SSTR(ctx_id) << END_LOG;
                                    return true;
                                }
                            }
                            return false;
                        }

                        /**
                         * Allows to log the information about the instantiated trie type
                         */
                        inline void log_model_type_info() const {
                            LOG_USAGE << "Using the <" << __FILENAME__ << "> model." << END_LOG;
                        }

                        /**
                         * This method can be used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * For more details @see LayeredTrieBase
                         */
                        virtual void pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * @see word_index_trie_base
                         */
                        void set_def_unk_word_prob(const prob_weight prob);

                        /**
                         * This method allows to check if post processing should be called after
                         * all the X level grams are read. This method is virtual.
                         * For more details @see WordIndexTrieBase
                         */
                        template<phrase_length level>
                        bool is_post_grams() const {
                            //Check the base class and we need to do post actions
                            //for the N-grams. The N-grams level data has to be
                            //sorted see post_N_Grams method implementation below.
                            return (level > M_GRAM_LEVEL_1) || BASE::template is_post_grams<level>();
                        };

                        /**
                         * This method should be called after all the X level grams are read.
                         * For more details @see WordIndexTrieBase
                         */
                        template<phrase_length CURR_LEVEL>
                        inline void post_grams() {
                            //Call the base class method first
                            if (BASE::template is_post_grams<CURR_LEVEL>()) {
                                BASE::template post_grams<CURR_LEVEL>();
                            }

                            //Do the post actions here
                            if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                post_n_grams();
                            } else {
                                if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                                    post_m_grams<CURR_LEVEL>();
                                }
                            }
                        };

                        /**
                         * Allows to retrieve the data storage structure for the M gram
                         * with the given M-gram level Id. M-gram context and last word Id.
                         * If the storage structure does not exist, return a new one.
                         * For more details @see LayeredTrieBase
                         */
                        template<phrase_length CURR_LEVEL>
                        inline void add_m_gram(const model_m_gram & gram) {
                            const TShortId word_id = gram.get_last_word_id();
                            if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                                //Store the payload
                                m_1_gram_data[word_id] = gram.m_payload;
                            } else {
                                //Register the m-gram in the hash cache
                                this->register_m_gram_cache(gram);

                                //Define the context id variable
                                TLongId ctx_id = UNKNOWN_WORD_ID;
                                //Obtain the m-gram context id
                                __LayeredTrieBase::get_context_id<c2w_array_trie<WordIndexType>, CURR_LEVEL, debug_levels_enum::DEBUG2>(*this, gram, ctx_id);

                                if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                    //Get the new n-gram index
                                    const TShortId n_gram_idx = m_m_n_gram_next_ctx_id[BASE::N_GRAM_IDX_IN_M_N_ARR]++;

                                    //Store the context and word ids
                                    m_n_gram_data[n_gram_idx].ctx_id = ctx_id;
                                    m_n_gram_data[n_gram_idx].word_id = word_id;

                                    //Store the payload
                                    m_n_gram_data[n_gram_idx].prob = gram.m_payload.m_prob;
                                } else {
                                    //Compute the m-gram index
                                    const phrase_length m_gram_idx = CURR_LEVEL - BASE::MGRAM_IDX_OFFSET;

                                    //First get the sub-array reference. 
                                    TSubArrReference & ref = m_m_gram_ctx_2_data[m_gram_idx][ctx_id];

                                    //Get the new index and increment - this will be the new end index
                                    ref.end_idx = m_m_n_gram_next_ctx_id[m_gram_idx]++;

                                    //Check if there are yet no elements for this context
                                    if (ref.begin_idx == BASE::UNDEFINED_ARR_IDX) {
                                        //There was no elements put into this context, the begin index is then equal to the end index
                                        ref.begin_idx = ref.end_idx;
                                    }

                                    //Store the word id
                                    m_m_gram_data[m_gram_idx][ref.end_idx].id = word_id;

                                    //Store the payload
                                    m_m_gram_data[m_gram_idx][ref.end_idx].payload = gram.m_payload;
                                }
                            }
                        }

                        /**
                         * Allows to attempt the sub-m-gram payload retrieval for m==1.
                         * The retrieval of a uni-gram data is always a success
                         * @see GenericTrieBase
                         */
                        inline void get_unigram_payload(m_gram_query & query) const {
                            //Get the uni-gram word id
                            const word_uid word_id = query.get_curr_uni_gram_word_id();

                            //The data is always present.
                            query.set_curr_payload(m_1_gram_data[word_id]);

                            LOG_DEBUG << "The uni-gram word id " << SSTR(word_id) << " payload : "
                                    << m_1_gram_data[word_id] << END_LOG;
                        };

                        /**
                         * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                         * For more details @see LayeredTrieBase
                         */
                        inline void get_m_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            LOG_DEBUG << "Getting the payload for sub-m-gram : " << query << END_LOG;

                            //First ensure the context of the given sub-m-gram
                            LAYERED_BASE_ENSURE_CONTEXT(query, status);

                            //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                //Store the shorthand for the context and end word id
                                TLongId & ctx_id = query.get_curr_ctx_ref();
                                const TShortId & word_id = query.get_curr_end_word_id();

                                //Compute the distance between words
                                const phrase_length & curr_level = query.get_curr_level();
                                LOG_DEBUG << "curr_level: " << SSTR(curr_level) << ", ctx_id: " << ctx_id << ", m_end_word_idx: "
                                        << SSTR(query.m_curr_end_word_idx) << ", end word id: " << word_id << END_LOG;

                                //Get the next context id
                                const phrase_length & level_idx = query.get_curr_level_m2();
                                if (get_ctx_id(level_idx, word_id, ctx_id)) {
                                    LOG_DEBUG << "level_idx: " << SSTR(level_idx) << ", ctx_id: " << ctx_id << END_LOG;
                                    //There is data found under this context
                                    query.set_curr_payload(m_m_gram_data[level_idx][ctx_id].payload);
                                    LOG_DEBUG << "The payload is retrieved: " << m_m_gram_data[level_idx][ctx_id].payload << END_LOG;
                                } else {
                                    //The payload could not be found
                                    LOG_DEBUG1 << "Unable to find m-gram data for ctx_id: " << SSTR(ctx_id)
                                            << ", word_id: " << SSTR(word_id) << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                                LOG_DEBUG << "Context ensure status is: " << status_to_string(status) << END_LOG;
                            }
                        }

                        /**
                         * Allows to attempt the sub-m-gram payload retrieval for m==n
                         * @see GenericTrieBase
                         */
                        inline void get_n_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            //First ensure the context of the given sub-m-gram
                            LAYERED_BASE_ENSURE_CONTEXT(query, status);

                            //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                //Store the shorthand for the context and end word id
                                TLongId & ctx_id = query.get_curr_ctx_ref();
                                const TShortId & word_id = query.get_curr_end_word_id();

                                LOG_DEBUG2 << "Getting " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-gram with word_id: "
                                        << SSTR(word_id) << ", ctx_id: " << SSTR(ctx_id) << END_LOG;

                                //Create the search key by combining ctx and word ids, see TCtxIdProbEntryPair
                                const TLongId key = put_32_32_in_64(word_id, ctx_id);
                                LOG_DEBUG4 << "Searching N-Gram: TShortId_TShortId_2_TLongId(word_id = " << SSTR(word_id)
                                        << ", ctx_id = " << SSTR(ctx_id) << ") = " << SSTR(key) << END_LOG;

                                //Search for the index using binary search
                                TShortId idx = BASE::UNDEFINED_ARR_IDX;
                                if (my_bsearch_wordId_ctxId<TCtxIdProbEntry>(m_n_gram_data, BASE::FIRST_VALID_CTX_ID,
                                        m_m_n_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR] - 1, word_id, ctx_id, idx)) {
                                    //Return the data
                                    query.set_curr_payload(m_n_gram_data[idx].prob);
                                    LOG_DEBUG << "The payload is retrieved: " << m_n_gram_data[idx].prob << END_LOG;
                                } else {
                                    //The payload could not be found
                                    LOG_DEBUG1 << "Unable to find " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-gram data for ctx_id: " << SSTR(ctx_id)
                                            << ", word_id: " << SSTR(word_id) << ", key " << SSTR(key) << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                            }
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~c2w_array_trie();

                    protected:

                        /**
                         * This structure is needed to store begin and end index to reference pieces of an array
                         * It is used to reference sub-array ranges for the M-gram data for levels 1 < M < N.
                         * 
                         * WARNING: It is not possible to get rid of this structure as the contexts are not ordered.
                         * It is only true that the contexts will be filled one after another, but the context id 
                         * will not be increased all the time.
                         * 
                         * @param beginIdx the begin index
                         * @param endIdx the end index
                         */
                        typedef struct {
                            TShortId begin_idx;
                            TShortId end_idx;
                        } TSubArrReference;

                        typedef __C2WArrayTrie::TWordIdPBData TWordIdPBEntry;
                        typedef __C2WArrayTrie::TCtxIdProbData TCtxIdProbEntry;

                        template<phrase_length CURR_LEVEL>
                        inline void post_m_grams() {
                            //Compute the m-gram index
                            constexpr phrase_length mgram_idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                            LOG_DEBUG2 << "Running post actions on " << CURR_LEVEL << "-grams, m-gram array index: " << mgram_idx << END_LOG;

                            //Sort the entries per context with respect to the word index
                            //the order could be arbitrary if a non-basic word index is used!
                            const size_t num_prev_ctx = (CURR_LEVEL == M_GRAM_LEVEL_2) ? m_one_gram_arr_size : m_m_n_gram_num_ctx_ids[mgram_idx - 1];
                            LOG_DEBUG2 << "Number of previous contexts: " << num_prev_ctx << END_LOG;
                            for (size_t ctx_id = 0; ctx_id < num_prev_ctx; ++ctx_id) {
                                const TSubArrReference & info = m_m_gram_ctx_2_data[mgram_idx][ctx_id];
                                if (info.begin_idx != BASE::UNDEFINED_ARR_IDX) {
                                    LOG_DEBUG3 << "Sorting for context id: " << ctx_id << ", info.beginIdx: "
                                            << info.begin_idx << ", info.endIdx: " << info.end_idx << END_LOG;
                                    my_sort<TWordIdPBEntry>(&m_m_gram_data[mgram_idx][info.begin_idx], (info.end_idx - info.begin_idx) + 1);
                                }
                            }
                        }

                        inline void post_n_grams() {
                            LOG_DEBUG2 << "Sorting the N-gram's data: ptr: " << m_n_gram_data
                                    << ", size: " << m_m_n_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR] << END_LOG;

                            //Order the N-gram array as it is unordered and we will binary search it later!
                            //Note: We dot not use Q-sort as it needs quite a lot of extra memory!
                            //Also, I did not yet see any performance advantages compared to sort!
                            //Actually the qsort provided here was 50% slower on a 20 Gb language
                            //model when compared to the str::sort!
                            my_sort<TCtxIdProbEntry>(m_n_gram_data, m_m_n_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR]);
                        };

                    private:
                        //Stores the pointer to the UNK word payload
                        m_gram_payload * m_unk_data;

                        //Stores the 1-gram data
                        m_gram_payload * m_1_gram_data;

                        //Stores the M-gram context to data mappings for: 1 < M < N
                        //This is a two dimensional array
                        TSubArrReference * m_m_gram_ctx_2_data[BASE::NUM_M_GRAM_LEVELS];
                        //Stores the M-gram data for the M levels: 1 < M < N
                        //This is a two dimensional array
                        TWordIdPBEntry * m_m_gram_data[BASE::NUM_M_GRAM_LEVELS];

                        //Stores the N-gram data
                        TCtxIdProbEntry * m_n_gram_data;

                        //Stores the size of the One-gram
                        TShortId m_one_gram_arr_size;
                        //Stores the maximum number of context id  per M-gram level: 1 < M <= N
                        TShortId m_m_n_gram_num_ctx_ids[BASE::NUM_M_N_GRAM_LEVELS];
                        //Stores the context id counters per M-gram level: 1 < M <= N
                        TShortId m_m_n_gram_next_ctx_id[BASE::NUM_M_N_GRAM_LEVELS];
                    };

                    typedef c2w_array_trie<basic_word_index > TC2WArrayTrieBasic;
                    typedef c2w_array_trie<counting_word_index > TC2WArrayTrieCount;
                    typedef c2w_array_trie<basic_optimizing_word_index > TC2WArrayTrieOptBasic;
                    typedef c2w_array_trie<counting_optimizing_word_index > TC2WArrayTrieOptCount;
                    typedef c2w_array_trie<hashing_word_index > TC2WArrayTrieHashing;
                }
            }
        }
    }
}
#endif /* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

