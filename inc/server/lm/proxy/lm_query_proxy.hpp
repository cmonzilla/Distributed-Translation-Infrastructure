/* 
 * File:   lm_query_proxy.hpp
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
 * Created on February 5, 2016, 8:46 AM
 */

#ifndef TRIE_QUERY_HPP
#define TRIE_QUERY_HPP

#include "common/utils/file/text_piece_reader.hpp"

using namespace uva::utils::file;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This class represents a trie query proxy interface class.
                         * It allows to interact with templated trie queries in a uniform way.
                         * ToDo: Add methods/return values to extract the needed probability data.
                         */
                        class lm_query_proxy {
                        public:

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~lm_query_proxy() {
                            }

                            /**
                             * Allows to execute a query
                             * @param text the m-gram query to be executed
                             */
                            template<bool is_cumulative, bool is_log_result = false >
                            inline void execute(TextPieceReader &text) {
                                if (is_cumulative) {
                                    if (is_log_result) {
                                        execute_cum_yes_log_yes(text);
                                    } else {
                                        execute_cum_yes_log_no(text);
                                    }
                                } else {
                                    if (is_log_result) {
                                        execute_cum_no_log_yes(text);
                                    } else {
                                        execute_cum_no_log_no(text);
                                    }
                                }
                            };

                        protected:

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual void execute_cum_yes_log_yes(TextPieceReader &text) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual void execute_cum_yes_log_no(TextPieceReader &text) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual void execute_cum_no_log_yes(TextPieceReader &text) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual void execute_cum_no_log_no(TextPieceReader &text) = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_HPP */

