/* 
 * File:   post_proc_req_in.hpp
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
 * Created on July 26, 2016, 10:42 AM
 */

#ifndef POST_PROC_REQ_IN_HPP
#define	POST_PROC_REQ_IN_HPP

#include "processor/messaging/proc_req_in.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                namespace messaging {
                    
                    /**
                     * This class represents the incoming text post-process request.
                     */
                    class post_proc_req_in : public proc_req_in {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        post_proc_req_in(const incoming_msg * inc_msg)
                        : proc_req_in(inc_msg) {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~post_proc_req_in() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to get the processor job unique identifier
                         * @return the source text unique identifier
                         */
                        inline string get_proc_job_uid() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[PROC_JOB_UID_FIELD_NAME].GetString();
                        }
                    };
                }
            }
        }
    }
}

#endif	/* POST_PROC_REQ_IN_HPP */

