/* 
 * File:   lm_trie_query_proxy_local.hpp
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
 * Created on February 5, 2016, 8:47 AM
 */

#ifndef LM_TRIE_QUERY_PROXY_LOCAL_HPP
#define LM_TRIE_QUERY_PROXY_LOCAL_HPP

#include "server/lm/proxy/lm_query_proxy.hpp"
#include "server/lm/models/sliding_m_gram_query.hpp"
#include "server/lm/models/simple_m_gram_query.hpp"

using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a local implementation of the language model query
                         * This implementation works with the local trie
                         */
                        template<typename trie_type>
                        class lm_trie_query_proxy_local : public lm_query_proxy {
                        public:

                            /**
                             * The basic constructor that accepts the trie reference to query to
                             * @param trie the trie to query
                             */
                            lm_trie_query_proxy_local(const trie_type & trie)
                            : m_sliding_query(trie), m_simple_query(trie) {
                            }
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual float get_unk_word_prob() {
                                return m_simple_query.get_unk_word_prob();
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            ~lm_trie_query_proxy_local() {
                                //Nothing to free, all the resources are allocated on the stack.
                            }

                        protected:
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_yes_log_yes(const word_uid * word_ids) {
                                return m_sliding_query.template execute<true, true>(word_ids);
                            };
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_yes_log_no(const word_uid * word_ids) {
                                return m_sliding_query.template execute<true, false>(word_ids);
                            };
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_no_log_yes(const word_uid * word_ids) {
                                return m_sliding_query.template execute<false, true>(word_ids);
                            };
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_no_log_no(const word_uid * word_ids) {
                                return m_sliding_query.template execute<false, false>(word_ids);
                            };
                            
                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_yes_log_yes(TextPieceReader &text) {
                                return m_simple_query.template execute<true, true>(text);
                            };

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_yes_log_no(TextPieceReader &text) {
                                return m_simple_query.template execute<true, false>(text);
                            };

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_no_log_yes(TextPieceReader &text) {
                                return m_simple_query.template execute<false, true>(text);
                            };

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute_cum_no_log_no(TextPieceReader &text) {
                                return m_simple_query.template execute<false, false>(text);
                            };

                        private:
                            //Stores the reference to the sliding query
                            sliding_m_gram_query<trie_type> m_sliding_query;
                            //Stores the reference to the simple query
                            simple_m_gram_query<trie_type> m_simple_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_PROXY_IMPL_HPP */

