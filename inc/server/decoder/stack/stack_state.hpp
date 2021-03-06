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

#ifndef STACK_STATE_HPP
#define STACK_STATE_HPP

#include <vector>
#include <algorithm>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/state_data.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Forward declaration of the stack level to be used as a state friend
                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class stack_level_templ;

                        /**
                         * This is the translation stack state class that is responsible for the sentence translation
                         * @param is_dist the flag indicating whether there is a left distortion limit or not
                         * @param NUM_WORDS_PER_SENTENCE the maximum allowed number of words per sentence
                         * @param MAX_HISTORY_LENGTH the maximum allowed length of the target translation hystory
                         * @param MAX_M_GRAM_QUERY_LENGTH the maximum length of the m-gram query
                         */
                        template<bool is_dist, size_t NUM_WORDS_PER_SENTENCE, size_t MAX_HISTORY_LENGTH, size_t MAX_M_GRAM_QUERY_LENGTH>
                        class stack_state_templ {
                        public:
                            //Typedef the state data template for a shorter name
                            typedef state_data_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH> state_data;
                            //Typedef the stack data
                            typedef typename state_data::stack_data stack_data;
                            //Typedef the state pointer
                            typedef typename stack_data::stack_state_ptr stack_state_ptr;
                            //Typedef the state pointer
                            typedef typename stack_data::const_stack_state_ptr const_stack_state_ptr;
                            //Typedef the state
                            typedef typename stack_data::stack_state stack_state;

                            //Stores the undefined and initial ids for the state 
                            static constexpr int32_t UNDEFINED_STATE_ID = -1;
                            static constexpr int32_t INITIAL_STATE_ID = 0;

#if IS_SERVER_TUNING_MODE
#define INIT_STACK_STATE_TUNING_DATA , m_state_id(UNDEFINED_STATE_ID), m_is_not_dumped(true)
#else
#define INIT_STACK_STATE_TUNING_DATA
#endif                        

                            /**
                             * The basic constructor for the BEGIN stack state, corresponding to the &lt;s&gt; tag.
                             * @param data the shared data container
                             */
                            stack_state_templ(const stack_data & data)
                            : m_parent(NULL), m_state_data(data), m_prev(NULL), m_next(NULL),
                            m_fncs_pos(m_state_data.m_stack_data.m_sent_data.m_min_idx),
                            m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New BEGIN state: " << this << ", parent: " << m_parent << END_LOG;
                            }

                            /**
                             * The basic constructor for the END stack state, corresponding to the &lt;/s&gt; tag.
                             * @param parent the parent state pointer, NOT NULL!
                             */
                            stack_state_templ(stack_state_ptr parent) :
                            m_parent(parent), m_state_data(parent->m_state_data), m_prev(NULL), m_next(NULL),
                            m_fncs_pos(m_state_data.m_stack_data.m_sent_data.m_max_idx),
                            m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New END state: " << this << ", parent: " << m_parent << END_LOG;
                            }

                            /**
                             * The basic constructor for the non-begin/end (&lt;s&gt;/&lt;/s&gt;) stack state.
                             * @param parent the parent state pointer, NOT NULL!
                             * @param fncs_pos the position which will be used for searching for the 
                             *                 first non-covered words in this new state expansions.
                             * @param begin_pos this state translated source phrase begin position
                             * @param end_pos this state translated source phrase end position
                             * @param covered the pre-cooked covered vector, for efficiency reasons.
                             * @param target the new translation target
                             */
                            stack_state_templ(stack_state_ptr parent, const int32_t fncs_pos,
                                    const int32_t begin_pos, const int32_t end_pos,
                                    const typename state_data::covered_info & covered,
                                    tm_const_target_entry* target)
                            : m_parent(parent), m_state_data(parent->m_state_data, begin_pos, end_pos, covered, target),
                            m_prev(NULL), m_next(NULL), m_fncs_pos(fncs_pos), m_recomb_from(NULL) INIT_STACK_STATE_TUNING_DATA{
                                LOG_DEBUG1 << "New state: " << this << ", parent: " << m_parent
                                << ", source[" << begin_pos << "," << end_pos << "], target ___"
                                << target->get_target_phrase() << "___" << END_LOG;
                            }
                            
                            /**
                             * The basic destructor, should free all the allocated
                             * resources. Deletes the states that are recombined
                             * into this state as they are not in any stack level
                             */
                            ~stack_state_templ() {
                                //We need two pointers one to point to the current state to
                                //be deleted and another one for the next state to move to.
                                stack_state_ptr curr_state = m_recomb_from;
                                stack_state_ptr next_state = NULL;

                                LOG_DEBUG1 << "Destructing state " << this << ", recombined-"
                                        << "from states ptr: " << curr_state << END_LOG;

                                //While there is states to delete
                                while (curr_state != NULL) {
                                    //Save the next state
                                    next_state = curr_state->m_next;

                                    LOG_DEBUG1 << "Deleting recombined from state: " << curr_state << END_LOG;

                                    //Delete the revious state
                                    delete curr_state;

                                    //Move on to the next state;
                                    curr_state = next_state;
                                }
                            }

#if IS_SERVER_TUNING_MODE

                            /**
                             * Allows to set the state id for the case of decoder
                             * tuning, i.e. search lattice generation.
                             * @param state_id the state id as issued by the stack
                             */
                            inline void set_state_id(const int32_t & state_id) {
                                //Check the sanity, that the id is not set yet
                                ASSERT_SANITY_THROW((m_state_id != UNDEFINED_STATE_ID),
                                        string("Re-initializing state id, old value: ") +
                                        to_string(m_state_id) + string(" new value: ") +
                                        to_string(state_id));

                                //Set the new state id
                                m_state_id = state_id;
                            }

                            /**
                             * Allows to dump the end-state (&lt;/s&gt;) data to the lattice as a FROM state.
                             * Here we go through all the states coming into this &lt;/s&gt; state and dump 
                             * them as the FROM states with zero score instead. This is to match
                             * the way Oister dumps its lattice, there the &lt;/s&gt; state is not dumped
                             * and is combined with the last actual translation state.
                             * @param this_dump the stream to dump the from state into
                             */
                            inline void dump_from_end_state_state_data(ostream & this_dump) const {
                                LOG_DEBUG1 << "Dumping the END STATE AS FROM state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //If the state does not have a parent then it is the
                                //root of translation tree, so no need to dump it
                                if (m_parent != NULL) {
                                    //Dump the parent as the from state into the lattice
                                    this_dump << to_string(m_parent->m_state_id) << "||||||0";

                                    //Dump the parents of the recombined from states, if any
                                    stack_state_ptr rec_from = m_recomb_from;
                                    while (rec_from != NULL) {
                                        //Add an extra space before the new element in the lattice dump
                                        this_dump << " ";
                                        //Dump the parent of the recombined from state
                                        this_dump << to_string(rec_from->m_parent->m_state_id) << "||||||0";
                                        //Move to the next recombined from state
                                        rec_from = rec_from->m_next;
                                    }
                                }

                                LOG_DEBUG1 << "Dumping the END STATE AS FROM state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }

                            /**
                             * Allows to dump the stack state data to the lattice
                             * @param this_dump the lattice output stream to dump the data into
                             * @param covers_dump the dump stream to dump the to-from covers
                             * @param to_state the to state
                             * @param main_to_state the main to state, i.e. in case the to_state
                             * was recombined into another state, we shall use that as a main
                             * state-carrying the state id for the lattice
                             * @param score_delta the score delta to log, in case this state is
                             * recombined into another one this is the delta for that other state,
                             * as required for oister tuning algorithm.
                             */
                            inline void dump_from_state_data(ostream & this_dump, ostream & covers_dump,
                                    const stack_state & to_state, const stack_state & main_to_state,
                                    const prob_weight score_delta) const {
                                LOG_DEBUG1 << "Dumping the FROM state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //Extract the target translation string.
                                string target = "";
                                to_state.m_state_data.template get_target_phrase<true>(target);

                                //Dump the data into the lattice
                                this_dump << to_string(m_state_id) << "|||" << target << "|||" << to_string(score_delta);

                                //Dump the cover vector for the to state
                                covers_dump << to_string(main_to_state.m_state_id) << "-" << to_string(m_state_id)
                                        << ":" << to_string(to_state.m_state_data.m_s_begin_word_idx)
                                        << ":" << to_string(to_state.m_state_data.m_s_end_word_idx) << " ";

                                LOG_DEBUG1 << "Dumping the FROM state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }

                            /**
                             * Allows to dump the end-state (&lt;/s&gt;) data to the lattice as a TO state.
                             * Here we go through all the states coming into this &lt;/s&gt; state and dump 
                             * them as the FROM states with zero score instead. This is to match
                             * the way Oister dumps its lattice, there the &lt;/s&gt; state is not dumped
                             * and is combined with the last actual translation state.
                             * @param this_dump the stream where the current state is to be dumped right away
                             * @param scores_dump the stream for dumping the used model features per state
                             * @param covers_dump the stream for dumping the cover vectors
                             */
                            inline void dump_to_end_state_state_data(ostream & this_dump, ostream & scores_dump, ostream & covers_dump) const {
                                LOG_DEBUG1 << "Dumping the END STATE AS TO state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //If the state does not have a parent then it is the
                                //root of translation tree, so no need to dump it
                                if ((m_parent != NULL) && m_is_not_dumped) {
                                    //Declare the stream to store the parent's data
                                    stringstream parents_dump;

                                    //Dump the parent state as to state
                                    //Pass this state as we will need its feature scores and partial score
                                    m_parent->template dump_to_state_data<true>(parents_dump, scores_dump, covers_dump, this, this);

                                    //Dump the parents of the recombined from states, if any
                                    stack_state_ptr rec_from = m_recomb_from;
                                    while (rec_from != NULL) {
                                        //Dump the recombined state parent
                                        //Pass this state as we will need its feature scores and partial score
                                        rec_from->m_parent->template dump_to_state_data<true>(parents_dump, scores_dump, covers_dump, rec_from, this);
                                        //Move to the next recombined from state
                                        rec_from = rec_from->m_next;
                                    }

                                    //Finish this entry and add the parents
                                    this_dump << parents_dump.str();

                                    //Mark the state as dumped 
                                    const_cast<bool &> (m_is_not_dumped) = false;
                                }

                                LOG_DEBUG1 << "Dumping the END STATE AS TO state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }

                            /**
                             * Allows to dump the stack state data to the lattice as a TO state
                             * @param this_dump the stream where the current state is to be dumped right away
                             * @param scores_dump the stream for dumping the used model features per state
                             * @param covers_dump the stream for dumping the cover vectors
                             * @param ce_state the end state (&lt;/s&gt;) in case it is the direct child of this state, the default is NULL
                             * @param main_ce_state the end state (&lt;/s&gt;) in case it is the direct child of this state, the default is NULL
                             *        the main_pe_state is different from pe_state in case pe_state was recombined into main_pe_state
                             */
                            template<bool is_end_state = false >
                            inline void dump_to_state_data(ostream & this_dump, ostream & scores_dump, ostream & covers_dump,
                                    const stack_state * end_state = NULL, const stack_state * main_end_state = NULL) const {
                                LOG_DEBUG1 << "Dumping the TO state " << this << " ("
                                        << m_state_id << ") to the search lattice" << END_LOG;

                                //Assert sanity that the only state with no parent is the rood one with the zero id.
                                ASSERT_SANITY_THROW((m_parent != NULL)&&(m_state_id == INITIAL_STATE_ID),
                                        string("The parent is present but the root state id is ") + to_string(INITIAL_STATE_ID));
                                ASSERT_SANITY_THROW((m_parent == NULL)&&(m_state_id != INITIAL_STATE_ID),
                                        string("The parent is NOT present but the root state id is NOT ") + to_string(INITIAL_STATE_ID));

                                //If the state does not have a parent then it is the
                                //root of translation tree, so no need to dump it
                                if ((m_parent != NULL) && m_is_not_dumped) {
                                    //Dump the state info
                                    this_dump << to_string(m_state_id) << "\t";

                                    //Dump the scores
                                    dump_state_scores<is_end_state>(scores_dump, main_end_state);

                                    //Declare the stream to store the parent's data
                                    stringstream parents_dump;

                                    LOG_DEBUG1 << "Dumping the PARENT state of state "
                                            << this << " (" << m_state_id << ")" << END_LOG;

                                    //Compute the partial score delta for the state
                                    const prob_weight score_delta = m_parent->template compute_partial_score_delta<is_end_state>(*this, end_state);

                                    //Dump the state's parent as its from state 
                                    m_parent->dump_from_state_data(this_dump, covers_dump, *this, *this, score_delta);
                                    //Dump as a to state into the parent dump
                                    m_parent->dump_to_state_data(parents_dump, scores_dump, covers_dump);

                                    LOG_DEBUG1 << "Dumping the RECOPMBINED FROM states of the TO state "
                                            << this << " (" << m_state_id << ")" << END_LOG;

                                    //Dump the parents of the recombined from states, if any
                                    stack_state_ptr rec_from = m_recomb_from;
                                    while (rec_from != NULL) {
                                        //Add an extra space before the new element in the lattice dump
                                        this_dump << " ";
                                        //Dump as a from state
                                        rec_from->m_parent->dump_from_state_data(this_dump, covers_dump, *rec_from, *this, score_delta);
                                        //Dump as a to state into the parent dump
                                        rec_from->m_parent->dump_to_state_data(parents_dump, scores_dump, covers_dump);
                                        //Move to the next recombined from state
                                        rec_from = rec_from->m_next;
                                    }

                                    //Finish this entry and add the parents
                                    this_dump << std::endl << parents_dump.str();

                                    //Mark the state as dumped 
                                    const_cast<bool &> (m_is_not_dumped) = false;
                                }

                                LOG_DEBUG1 << "Dumping the TO state " << this << " ("
                                        << m_state_id << ") to the lattice is done" << END_LOG;
                            }
#endif

                            /**
                             * Allows to get the stack level, the latter is equal
                             * to the number of so far translated words.
                             * @return the stack level
                             */
                            inline uint32_t get_stack_level() const {
                                //Get the stack level as it was computed during the state creation.
                                return m_state_data.m_stack_level;
                            }

                            /**
                             * Allows the state to expand itself, it will
                             * add itself to the proper stack.
                             */
                            inline void expand() {
                                //Create shorthands for the data to compare and log
                                const size_t curr_count = m_state_data.m_covered.count();
                                const size_t & num_words = m_state_data.m_stack_data.m_sent_data.get_dim();

                                LOG_DEBUG << "Num words = " << num_words << ", covered: "
                                        << m_state_data.covered_to_string() << ", curr_count = "
                                        << curr_count << END_LOG;

                                //Check if this is the last state, i.e. we translated everything
                                if (curr_count == num_words) {
                                    //All of the words have been translated, add the end state
                                    stack_state_ptr end_state = new stack_state(this);
                                    m_state_data.m_stack_data.m_add_state(end_state);
                                } else {
                                    //Do the "from first not-covered" expansion - the Oister style.
                                    expand_from_first_non_covered();
                                }
                            }

                            /**
                             * Allows to get the translation ending in this state.
                             * @param target_sent [out] the variable to store the translation
                             */
                            inline void get_translation(string & target_sent) const {
                                //Go through parents until the root state and append translations when going back!
                                if (m_parent != NULL) {
                                    //Go recursively to the parent
                                    m_parent->get_translation(target_sent);
                                }

                                //Allows to add the target phrase to the given string
                                m_state_data.get_target_phrase(target_sent);
                            }

                            /**
                             * Allows to compare two states, the comparison is based on the state total score.
                             * The state with the bigger total score is considered to be bigger, i.e. more probable.
                             * The state with the smalle total score is considered to be smaller, i.e. less probable.
                             * @param other the other state to compare with
                             * @return true if this state is smaller than the other one
                             */
                            inline bool operator<(const stack_state & other) const {
                                //Get the shorthand for the other state data
                                const state_data & other_data = other.m_state_data;

                                LOG_DEBUG3 << "Checking states: " << this << " <? " << &other << END_LOG;
                                LOG_DEBUG3 << "Total scores: " << m_state_data.m_total_score << " <? "
                                        << other_data.m_total_score << END_LOG;

                                //Compute the comparison result
                                const bool is_less = (m_state_data.m_total_score < other_data.m_total_score);

                                //Log the comparison result
                                LOG_DEBUG3 << "Result, state: " << this << (is_less ? " < " : " >= ") << &other << END_LOG;

                                //Return the comparison result
                                return is_less;
                            }

                            /**
                             * Allows to compare two states, the states are equal if the solve
                             * the same sub-problem i.e. are eligible for recombination.
                             * The states are equal if and only if:
                             *    1. They have the same last translated word
                             *    2. They have the same history of target words
                             *    3. They cover the same source words
                             * @param other the other state to compare with
                             * @return true if this state is equal to the other one, otherwise false.
                             */
                            inline bool operator==(const stack_state & other) const {
                                //Get the shorthand for the other state data
                                const state_data & other_data = other.m_state_data;

                                //Log the state data that will be compared
                                LOG_DEBUG1 << "--- State recombination check: " << this << " =?= " << &other << END_LOG;
                                LOG_DEBUG1 << "--- " << m_state_data.m_s_end_word_idx << " =?= " << other_data.m_s_end_word_idx << END_LOG;
                                LOG_DEBUG1 << "--- Checking tail history of " << MAX_HISTORY_LENGTH << " elements" << END_LOG;
                                LOG_DEBUG1 << "--- " << m_state_data.m_trans_frame.to_string() << " =?= " << other_data.m_trans_frame.to_string() << END_LOG;
                                LOG_DEBUG1 << "--- " << m_state_data.covered_to_string() << " =?= " << other_data.covered_to_string() << END_LOG;
                                LOG_DEBUG1 << "--- " << m_state_data.rm_entry_data << " =(second 1/2)?= " << other_data.rm_entry_data << END_LOG;

                                //Compute the comparison result
                                const bool is_equal = (m_state_data.m_s_end_word_idx == other_data.m_s_end_word_idx) &&
                                        m_state_data.m_trans_frame.is_equal_last(other_data.m_trans_frame, MAX_HISTORY_LENGTH) &&
                                        (m_state_data.m_covered == other_data.m_covered) &&
                                        m_state_data.rm_entry_data.is_equal_from_weights(other_data.rm_entry_data);

                                //Log the equality comparison result
                                LOG_DEBUG1 << "--- State: " << this << (is_equal ? " == " : " != ") << &other << END_LOG;

                                //Return the comparison result
                                return is_equal;
                            }

                            /**
                             * Allows to compare two states for not being equal,
                             * this is an inverse of the == operator.
                             * @param other the other state to compare with
                             * @return true if this state is not equal to the other one, otherwise false.
                             */
                            inline bool operator!=(const stack_state & other) const {
                                return !(*this == other);
                            }

                            /**
                             * Allows to check if the given new state is within the
                             * threshold limit.
                             * @param score_bound the bound to compare with
                             * @return true if the state's totl cost is &gt;= score_bound, otherwise false
                             */
                            inline bool is_above_threshold(const prob_weight & score_bound) const {
                                LOG_DEBUG1 << "checking state " << this << " score: " << m_state_data.m_total_score
                                        << ", threshold: " << score_bound << END_LOG;
                                return ( m_state_data.m_total_score >= score_bound);
                            }

                            /**
                             * Allows to add a state recombined into this one,
                             * i.e. the one equivalent to this one but having
                             * the lower value of the total cost. In case this state
                             * already has too many states recombined into this one
                             * and the new state probability is lower than that of
                             * the others, then we just delete it. Also, if there
                             * were states recombined into the other one, then they
                             * have lower costs, so proper merging of them is to
                             * be done as well. Eventually the states recombined into
                             * this one must have their m_recomb_from arrays empty.
                             * @param other_state the state to recombine into this one.
                             */
                            inline void recombine_from(stack_state_ptr other_state) {
                                LOG_DEBUG2 << "====================================================================" << END_LOG;
                                LOG_DEBUG1 << "Recombining " << other_state << " into " << this << END_LOG;

                                //Combine the new state with its recombined from states
                                //Put the state and its recombine from list together
                                other_state->m_prev = NULL;
                                other_state->m_next = other_state->m_recomb_from;
                                LOG_DEBUG << "State " << other_state << " recombined from "
                                        << "ptr = " << other_state->m_recomb_from << END_LOG;
                                //If the other's state recombine from list is not empty then link-back the list
                                if (other_state->m_next != NULL) {
                                    other_state->m_next->m_prev = other_state;
                                    //Clean the recombined from data
                                    other_state->m_recomb_from = NULL;
                                }

                                //Check if this is the first state we recombine into the current one
                                if (m_recomb_from == NULL) {
                                    LOG_DEBUG << "State " << other_state << " is the first"
                                            << " state recombined into " << this << END_LOG;
                                    //If this state has no recombined-from states
                                    //then just set the other list into this state
                                    m_recomb_from = other_state;
                                } else {
                                    LOG_DEBUG << "State " << this << " already has a "
                                            << " recombined from state " << m_recomb_from << END_LOG;
                                    //If this state already has some states recombined into this one then
                                    //we need to append the two lists together, the order is not important
                                    //We assume that the other state we are recombining into this one has
                                    //fewer recombined-from states, so we put it as a first one

                                    //First skip forward until the last one
                                    stack_state_ptr cursor = other_state;
                                    while (cursor->m_next != NULL) {
                                        cursor = cursor->m_next;
                                    }

                                    //Set the current recombined from as the last one
                                    cursor->m_next = m_recomb_from;
                                    m_recomb_from->m_prev = cursor;

                                    //Set the other state as the first in the list
                                    m_recomb_from = other_state;
                                }

                                LOG_DEBUG1 << "Recombining " << other_state << " into " << this << " is done!" << END_LOG;
                                LOG_DEBUG2 << "====================================================================" << END_LOG;
                            }

                        protected:

                            /**
                             * Allows to cut the tail of states starting from this one.
                             * The states present in the cut tail are to be deleted.
                             * @param tail the tails of staits to delete
                             */
                            inline void cut_the_tail(stack_state_ptr tail) {
                                //If the tail is not empty
                                if (tail != NULL) {
                                    //If the tail has a preceeding state
                                    if (tail->m_prev != NULL) {
                                        //Cut from the preceeding state
                                        tail->m_prev->m_next = NULL;
                                    }

                                    //Iterate through the states and delete them
                                    stack_state_ptr next = NULL;
                                    while (tail != NULL) {
                                        //Store the next element
                                        next = tail->m_next;
                                        //Delete the tail first element
                                        delete tail;
                                        //Move to the next tail element
                                        tail = next;
                                    }
                                }
                            }

                            /**
                             * This method implements the way oister does reordering when expanding hypothesis.
                             * In essence it searches for the first non-covered position from the left of the
                             * cover vector and goes on with expanding from it for the number of positions defined
                             * by the distortion limit. In other words, with this method we can do about a half of
                             * the expansions that can be done by just expanding to the left and right of the current
                             * position with the given distortion limit to each side. This first implementation is
                             * sub optimal and is for now only made to see what happens to the BLEU scores. If the
                             * result will be better than with the our version of expansions, we will optimize speed.
                             */
                            inline void expand_from_first_non_covered() {
                                LOG_DEBUG1 << ">>>>> starting expansions" << END_LOG;

                                //Store the shorthand to the minimum possible word index
                                const int32_t & MAX_WORD_IDX = m_state_data.m_stack_data.m_sent_data.m_max_idx;

                                LOG_DEBUG1 << "Searching for the first uncovered position between ["
                                        << m_fncs_pos << ", " << MAX_WORD_IDX << "]" << END_LOG;

                                //Search for the first not covered element
                                for (int32_t first_nc_pos = m_fncs_pos; first_nc_pos <= MAX_WORD_IDX; ++first_nc_pos) {
                                    LOG_DEBUG2 << "Considering " << m_state_data.covered_to_string() << " @ "
                                            << first_nc_pos << " = " << m_state_data.m_covered[first_nc_pos] << END_LOG;
                                    //Check if the last position is not covered,
                                    //if not then we can expand starting from here.
                                    if (!m_state_data.m_covered[first_nc_pos]) {
                                        LOG_DEBUG1 << "Found an uncovered position @ " << first_nc_pos << END_LOG;

                                        //We shall not expand in two direction to the left and to the right
                                        //of the last phrase, but the expansion to the left is only limited
                                        //by the number of uncovered positions on the left and the expansions
                                        //to the right is limited by the first non-covered position and the
                                        //distortion limit given in the parameters So here we shall first compute
                                        //the left and right boundaries for the expansions we are to make

                                        //The maximum position to consider to the left is the
                                        //first non-covered word position in the sentence.
                                        const int32_t max_left_pos = first_nc_pos;
                                        //Get the distortion limit and store it locally
                                        const int32_t & d_limit = m_state_data.m_stack_data.m_params.m_dist_limit;
                                        //The maximum position to consider to the right is the
                                        //first non-covered word position in the sentence plus
                                        //the distortion limit, if it is set, otherwise the
                                        //end position of the sentence.
                                        const int32_t max_right_pos = (is_dist) ? min(MAX_WORD_IDX, first_nc_pos + d_limit) : MAX_WORD_IDX;

                                        LOG_DEBUG1 << "Max [Left, Right] = [" << max_left_pos << ", " << max_right_pos << "]" << END_LOG;
                                        LOG_DEBUG1 << "Last phrase [Left, Right] = [" << m_state_data.m_s_begin_word_idx
                                                << ", " << m_state_data.m_s_end_word_idx << "]" << END_LOG;

                                        //Expand the states to the right first and then to the left
                                        expand_states_right(first_nc_pos, m_state_data.m_s_end_word_idx + 1, max_right_pos);
                                        expand_states_left(first_nc_pos, m_state_data.m_s_begin_word_idx - 1, max_left_pos);

                                        LOG_DEBUG1 << "Finished all possible expansions in the state." << END_LOG;
                                        //Once we finish the expansions we shall break from the outer cycle
                                        break;
                                    }
                                }

                                LOG_DEBUG1 << "<<<<< finished expansions" << END_LOG;
                            }

                            /**
                             * Allows to expand to the right from the last expanded phrase, if possible.
                             * We expand from the positions closest to the last phrase and then move away.
                             * @param first_nc_pos the first non-covered word index in the current hypothesis
                             * @param begin_pos the first position to attempt an expansion for
                             * @param end_pos the last position to attempt an expansion for
                             */
                            inline void expand_states_right(const int32_t & first_nc_pos, int32_t begin_pos, const int32_t end_pos) {
                                //Iterate through the allowed positions and try the length expansions
                                // begin_pos ----- > ---- end_pos
                                while (begin_pos <= end_pos) {
                                    //If the position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[begin_pos]) {
                                        //Expand the lengths from the last position
                                        expand_length(first_nc_pos, begin_pos);
                                    }
                                    //Increment the current position - moving to the right
                                    ++begin_pos;
                                }
                            }

                            /**
                             * Allows to expand to the left from the last expanded phrase, if possible
                             * We expand from the positions closest to the last phrase and then move away.
                             * @param first_nc_pos the first non-covered word index in the current hypothesis
                             * @param begin_pos the first position to attempt an expansion for
                             * @param end_pos the last position to attempt an expansion for
                             */
                            inline void expand_states_left(const int32_t & first_nc_pos, int32_t begin_pos, const int32_t end_pos) {
                                //Iterate through the allowed positions and try the length expansions
                                // end_pos ----- < ---- begin_pos
                                while (begin_pos >= end_pos) {
                                    //If the position is not covered then expand the lengths
                                    if (!m_state_data.m_covered[begin_pos]) {
                                        //Expand the lengths from the last position
                                        expand_length(first_nc_pos, begin_pos);
                                    }
                                    //Decrement the current position - moving to the left
                                    begin_pos--;
                                }
                            }

                            /**
                             * Allows to expand for all the possible phrase lengths
                             * @param first_nc_pos the index of the first non-covered word in the given hypothesis
                             * @param start_pos the position to start the word expansions from.
                             */
                            inline void expand_length(const int32_t first_nc_pos, const int32_t start_pos) {
                                LOG_DEBUG1 << ">>>>> [start_pos] = [" << start_pos << "]" << END_LOG;

                                //Always take the one word translation even if
                                //It is an unknown entry.
                                int32_t end_pos = start_pos;
                                //Just expand the single word, this is always needed and possible
                                expand_trans<true>(first_nc_pos, start_pos, end_pos);

                                LOG_DEBUG1 << "Expanded the single word @ [" << start_pos << ", " << end_pos << "]" << END_LOG;

                                //Get the maximum position
                                const int32_t & MAX_WORD_IDX = m_state_data.m_stack_data.m_sent_data.m_max_idx;

                                //Iterate through lengths > 1 until the maximum possible source phrase length is 
                                //reached or we hit the end of the sentence or the end of the uncovered region
                                for (size_t len = 2; len <= m_state_data.m_stack_data.m_params.m_max_s_phrase_len; ++len) {
                                    //Move to the next end position
                                    ++end_pos;
                                    //Check if we are in a good state
                                    if ((end_pos <= MAX_WORD_IDX) && (!m_state_data.m_covered[end_pos])) {
                                        //Expand the source phrase which is not a single word
                                        expand_trans<false>(first_nc_pos, start_pos, end_pos);
                                    } else {
                                        //We've hit the last possible length here, time to stop
                                        break;
                                    }
                                }

                                LOG_DEBUG1 << "<<<<< [start_pos] = [" << start_pos << "]" << END_LOG;
                            }

                            /**
                             * Allows to expand for all the possible translations
                             */
                            template<bool single_word>
                            inline void expand_trans(const int32_t first_nc_pos, const int32_t start_pos, const int32_t end_pos) {
                                LOG_DEBUG1 << ">>>>> [start_pos, end_pos] = [" << start_pos << ", " << end_pos << "]" << END_LOG;

                                //Obtain the source entry for the currently considered source phrase
                                tm_const_source_entry_ptr entry = m_state_data.m_stack_data.m_sent_data[start_pos][end_pos].m_source_entry;

                                ASSERT_SANITY_THROW((entry == NULL),
                                        string("The source entry [") + to_string(start_pos) +
                                        string(", ") + to_string(end_pos) + string("] is NULL!"));

                                //Check if we are in the situation of a single word or in the
                                //situation when we actually have translations for a phrase
                                if (single_word || entry->has_translations()) {
                                    //Get the targets
                                    tm_const_target_entry* targets = entry->get_targets();

                                    //Initialize the new covered vector, take the old one plus enable the new states
                                    typename state_data::covered_info covered(m_state_data.m_covered);
                                    for (int32_t idx = start_pos; idx <= end_pos; ++idx) {
                                        covered.set(idx);
                                    }

                                    //Compute the first non-covered word start search position for the subsequent states
                                    //In case the first not-covered position is the same as the start position of the
                                    //phrase we expand then the next not-covered position will be after the end of this phrase.
                                    //Otherwise, there is a possible gap of not-covered positions starting from the given one.
                                    const int32_t fncs_pos = ((first_nc_pos == start_pos) ? (end_pos + 1) : first_nc_pos);

                                    //Iterate through all the available target translations
                                    for (size_t idx = 0; idx < entry->num_targets(); ++idx) {
                                        //Add a new hypothesis state to the multi-stack
                                        m_state_data.m_stack_data.m_add_state(new stack_state(this, fncs_pos, start_pos, end_pos, covered, &targets[idx]));
                                    }
                                } else {
                                    //Do nothing we have an unknown phrase of length > 1
                                    LOG_DEBUG << "The source phrase [" << start_pos << ", " << end_pos << "] has no translations, ignoring!" << END_LOG;
                                }

                                LOG_DEBUG1 << "<<<<< [start_pos, end_pos] = [" << start_pos << ", " << end_pos << "]" << END_LOG;
                            }

                        private:
                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            stack_state_ptr m_parent;

                            //Stores the state data that is to be passed on the the children
                            state_data m_state_data;

                            //This variable stores the pointer to the previous state:
                            //1. In the stack level or NULL if it is the first one
                            //2. In the list of recombined from states
                            stack_state_ptr m_prev;

                            //This variable stores the pointer to the next state:
                            //1. In the stack level or NULL if it is the last one
                            //2. In the list of recombined from states
                            stack_state_ptr m_next;

                            //This variable stores the first non-covered word search from position
                            //This is an optimization parameter that allows to track the fully
                            //covered/translated prefix of the sentence.
                            const int32_t m_fncs_pos;

                            //This double-linked list stores the list of states recombined into this state
                            stack_state_ptr m_recomb_from;

#if IS_SERVER_TUNING_MODE
                            //Stores the state id unique within the multi-stack
                            //In case the software is compiled for the tuning mode.
                            int32_t m_state_id;

                            //Stores the flag indicating whether the state
                            //has been dumped to the lattice or not.
                            const bool m_is_not_dumped;

                            /**
                             * Allows to dump the feature scores to the scores output stream
                             * @param scores_dump the output stream for the feature scores
                             * @param add_state the additional state to add scores from
                             */
                            template<bool is_add_state = false >
                            inline void dump_state_scores(ostream & scores_dump, const stack_state * add_state = NULL) const {
                                scores_dump << to_string(m_state_id);

                                //Dump the lattice scores, adding the additional state scores if needed
                                prob_weight lattice_score = 0.0;
                                for (size_t idx = 0; idx < m_state_data.m_stack_data.get_num_features(); ++idx) {
                                    //The this state's score
                                    lattice_score = m_state_data.m_lattice_scores[idx];
                                    //Add the additional state score if needed
                                    if (is_add_state) {
                                        lattice_score += add_state->m_state_data.m_lattice_scores[idx];
                                    }
                                    //If the resulting score is not zero then log it
                                    if (lattice_score != 0.0) {
                                        scores_dump << " " << to_string(idx) << "=" << lattice_score;
                                    }
                                }
                                scores_dump << std::endl;
                            }

                            /**
                             * Allows to compute the partial score delta between this state, as the parent state and the to_state.
                             * In case the end state is given, the to_state is the &lt;/s&gt; tag state which is to be eliminated from the
                             * lattice and this is why then we compute the partial score delta relative to the given end_state.
                             * @param is_end_state if true indicates the presence of the end state
                             * @param to_state the child state of this state
                             * @param end_state the end state following the child state in case the to_state is an &lt;/s&gt; state
                             * @return the partial score delta, should ne null if and only if is_end_state is false
                             */
                            template<bool is_end_state = false >
                            inline prob_weight compute_partial_score_delta(const stack_state & to_state, const stack_state * end_state = NULL) {
                                //If the end state is present then use it to compute the delta
                                //score relative to it, instead of the actual to state.
                                const prob_weight to_state_partial_score = (is_end_state ?
                                        end_state->m_state_data.m_partial_score : to_state.m_state_data.m_partial_score);

                                //Return the partial score delta
                                return (to_state_partial_score - m_state_data.m_partial_score);
                            }
#endif

                            //Make the stack level the friend of this class
                            friend class stack_level_templ<is_dist, NUM_WORDS_PER_SENTENCE, MAX_HISTORY_LENGTH, MAX_M_GRAM_QUERY_LENGTH>;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

