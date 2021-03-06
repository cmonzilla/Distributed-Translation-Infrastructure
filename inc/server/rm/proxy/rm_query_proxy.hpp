/* 
 * File:   rm_query_proxy.hpp
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
 * Created on February 8, 2016, 9:58 AM
 */

#ifndef RM_QUERY_PROXY_HPP
#define RM_QUERY_PROXY_HPP

#include <vector>

#include "server/common/models/phrase_uid.hpp"

#include "server/rm/models/rm_entry.hpp"

using namespace std;

using namespace uva::smt::bpbd::server::common::models;

using namespace uva::smt::bpbd::server::rm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This class represents a reordering query proxy interface class.
                         * It allows to interact with reordering model queries in a uniform way.
                         */
                        class rm_query_proxy {
                        public: 
                            
                            /**
                             * Allows to execute the query, for the given source/target phrase ids
                             * @param st_ids is the list of the source/target phrase ids
                             *               for which the reordering data is needed
                             */
                            virtual void execute(const vector<phrase_uid> & st_ids) = 0;
                            
                            /**
                             * Allows to retrieve the begin tag reordering entry from the reordering model
                             * @return the start tag reordering entry
                             */
                            virtual const rm_entry & get_begin_tag_reordering() const = 0;
                            
                            /**
                             * Allows to retrieve the end tag reordering entry from the reordering model
                             * @return the start tag reordering entry
                             */
                            virtual const rm_entry & get_end_tag_reordering() const = 0;

                            /**
                             * Allows to get the source/target reordering data from the reordering model
                             * @param uid the source/target phrase uid
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the reordering was not found.
                             */
                            virtual const rm_entry & get_reordering(const phrase_uid uid) const = 0;

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~rm_query_proxy() {
                            }
                         };
                    }
                }
            }
        }
    }
}

#endif /* RM_QUERY_PROXY_HPP */

