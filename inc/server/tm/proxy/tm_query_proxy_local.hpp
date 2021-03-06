/* 
 * File:   tm_query_proxy_local.hpp
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
 * Created on February 8, 2016, 9:59 AM
 */

#ifndef TM_QUERY_PROXY_LOCAL_HPP
#define TM_QUERY_PROXY_LOCAL_HPP

#include "server/tm/proxy/tm_query_proxy.hpp"
#include "server/tm/models/tm_query.hpp"

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace proxy {

                        /**
                         * This is a local implementation of the translation model query
                         * This implementation works with the local translation model
                         */
                        template<typename model_type>
                        class tm_query_proxy_local : public tm_query_proxy {
                        public:

                            /**
                             * The basic constructor that accepts the translation model reference to query to
                             * @param model the translation model to query
                             */
                            tm_query_proxy_local(const model_type & model) : m_query(model) {
                            }

                            /**
                             * @see tm_query_proxy
                             */
                            virtual void execute(const phrase_uid uid, tm_const_source_entry_ptr & entry_ptr) {
                                m_query.execute(uid, entry_ptr);
                            }

                            /**
                             * @see tm_query_proxy
                             */
                            virtual tm_const_source_entry * get_source_entry(const phrase_uid uid) {
                                return m_query.get_source_entry(uid);
                            }

                            /**
                             * @see tm_query_proxy
                             */
                            virtual void get_st_uids(vector<phrase_uid> & st_uids) const {
                                m_query.get_st_uids(st_uids);
                            }

                            /**
                             * @see tm_query_proxy
                             */
                            virtual ~tm_query_proxy_local() {
                                //Nothing to be done, no dynamically allocated resources
                            }

                        private:
                            //Stores the actual query
                            tm_query<model_type> m_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_QUERY_PROXY_LOCAL_HPP */

