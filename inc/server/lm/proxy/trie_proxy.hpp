/* 
 * File:   trie_proxy.hpp
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
 * Created on February 5, 2016, 8:43 AM
 */

#ifndef TRIE_PROXY_INT_HPP
#define TRIE_PROXY_INT_HPP

#include "server/lm/proxy/lm_query_proxy.hpp"

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace proxy {
                        
                        /**
                         * This is the trie proxy interface class it allows to interact with templated tries in a uniform way.
                         */
                        class trie_proxy {
                        public:
                            
                            /**
                             * Allows to connect to the trie object based on the given parameters
                             * @param the parameters defining the trie model to connect to
                             */
                            virtual void connect(const lm_parameters & params) = 0;

                            /**
                             * Allows to disconnect from the trie
                             */
                            virtual void disconnect() = 0;
                            
                            /**
                             * The basic virtual destructor
                             */
                            virtual ~trie_proxy(){};

                            /**
                             * This method allows to get a query executor for the given trie
                             * @return the trie query proxy object
                             */
                            virtual lm_query_proxy * get_query_executor() = 0;
                            
                            /**
                             * Allows to log the trie type usage information
                             */
                            virtual void log_trie_type_usage_info() = 0; 
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_PROXY_INT_HPP */
