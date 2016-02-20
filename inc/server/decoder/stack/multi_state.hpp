/* 
 * File:   trans_stack_state.hpp
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
 * Created on February 16, 2016, 4:20 PM
 */

#ifndef TRANS_STACK_STATE_HPP
#define TRANS_STACK_STATE_HPP

#include <vector>
#include <bitset>

#include "common/utils/containers/circular_queue.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

using namespace std;

using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Forward declaration of the class
                        template<size_t NUM_WORDS_PER_SENTENCE>
                        class multi_state_templ;

                        //Typedef the multi state and instantiate it with the maximum number of words per sentence
                        typedef multi_state_templ<MAX_WORDS_PER_SENTENCE> multi_state;

                        //Define the multi state pointer
                        typedef multi_state * multi_state_ptr;

                        /**
                         * This is the translation stack state class that is responsible for the sentence translation
                         */
                        template<size_t NUM_WORDS_PER_SENTENCE>
                        class multi_state_templ {
                        public:

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor for the root stack state
                             * @param max_target_phrase_len the maximum target phrase length
                             */
                            multi_state_templ(const de_parameters & params)
                            : m_parent(NULL), m_next(NULL), m_recomb_from(), m_recomb_to(NULL),
                            m_covered(), m_last_covered(ZERRO_WORD_IDX),
                            m_history(params.m_max_t_phrase_len - 1),
                            m_partial_score(ZERO_LOG_PROB_WEIGHT),
                            m_future_cost(ZERO_LOG_PROB_WEIGHT) {
                                LOG_DEBUG2 << "multi_state create: " << params << END_LOG;

                                //Mark the zero word as covered
                                m_covered.set(ZERRO_WORD_IDX);
                                
                                //Add the sentence start to the target
                                m_history.push_back(BEGIN_SENTENCE_TAG_STR);
                            }

                            /**
                             * The basic constructor for the non-root stack state
                             * @param parent the pointer to the parent element
                             */
                            multi_state_templ(const de_parameters & params, multi_state_ptr parent)
                            : m_parent(NULL), m_next(NULL), m_recomb_from(), m_recomb_to(NULL),
                            m_covered(), m_last_covered(UNDEFINED_WORD_IDX),
                            m_history(params.m_max_t_phrase_len - 1),
                            m_partial_score(ZERO_LOG_PROB_WEIGHT),
                            m_future_cost(ZERO_LOG_PROB_WEIGHT) {
                                LOG_DEBUG2 << "multi_state create, with parent: " << params << END_LOG;
                                
                                //Compute the partial score;
                                compute_partial_score();
                                
                                //Compute the future costs;
                                compute_future_cost();
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_state_templ() {
                            }

                            /**
                             * Allows to get the next multi-state
                             * @return the poniter to the next multi-state in the list
                             */
                            inline multi_state_ptr get_next() {
                                return m_next;
                            }

                            /**
                             * Allows to set the next multi-state
                             * @param next the poniter to the next multi-state in the list
                             */
                            inline void set_next(multi_state_ptr next) {
                                return m_next = next;
                            }

                            /**
                             * Allows to compare two states
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const multi_state & other) const {
                                //ToDo: Implement the state compare
                                return true;
                            }

                            /**
                             * Allows to compare two states
                             * @param other the other state to compare with
                             * @return true if this state is equal to the other one
                             */
                            inline bool operator==(const multi_state & other) const {
                                //ToDo: Implement the state compare
                                return false;
                            }

                            inline float get_partial_score() {
                                return m_partial_score;
                            }

                            inline float get_future_cost() {
                                return m_future_cost;
                            }

                        private:

                            /**
                             * Allows to compute the partial score of the current hypothesis
                             */
                            inline void compute_partial_score() {
                                //ToDo: Implement
                            }

                            /**
                             * Allows to compute the future score of the current hypothesis
                             */
                            inline void compute_future_cost() {
                                //ToDo: Implement
                            }

                        protected:
                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            multi_state_ptr m_parent;

                            //This variable stores the pointer to the next state in the stack or NULL if it is the last one
                            multi_state_ptr m_next;

                            //This vector stores the list of states recombined into this state
                            vector<multi_state_ptr> m_recomb_from;

                            //This variable stores to which state this state was recombined or NULL
                            multi_state_ptr m_recomb_to;

                            //Stores the bitset of covered words indexes
                            bitset<NUM_WORDS_PER_SENTENCE> m_covered;

                            //Stores the last translated word index
                            int32_t m_last_covered;

                            //Stores the N-1 previously translated words
                            circular_queue<const string> m_history;

                            //Stores the logarithmic partial score of the current hypothesis
                            float m_partial_score;

                            //Stores the logarithmic future cost of the current hypothesis
                            float m_future_cost;
                        };

                        template<size_t NUM_WORDS_PER_SENTENCE>
                        constexpr int32_t multi_state_templ<NUM_WORDS_PER_SENTENCE>::UNDEFINED_WORD_IDX;

                        template<size_t NUM_WORDS_PER_SENTENCE>
                        constexpr int32_t multi_state_templ<NUM_WORDS_PER_SENTENCE>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */
