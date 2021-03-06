/* 
 * File:   proc_req_in.hpp
 * Author: zapreevis
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
 * Created on July 26, 2016, 2:04 PM
 */

#ifndef PROC_REQ_IN_HPP
#define	PROC_REQ_IN_HPP

#include "common/messaging/proc_req.hpp"
#include "common/messaging/incoming_msg.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                namespace messaging {

                    /**
                     * This class represents the incoming text processor request.
                     */
                    class proc_req_in : public proc_req {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        proc_req_in(const incoming_msg * inc_msg)
                        : proc_req(), m_inc_msg(inc_msg) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~proc_req_in() {
                            //Destroy the incoming message, the pointer must not be NULL
                            delete m_inc_msg;
                        }

                        /**
                         * Allows to get the job token
                         * @return the job token
                         */
                        inline string get_job_token() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[JOB_TOKEN_FIELD_NAME].GetString();
                        }
                        
                        /**
                         * Allows to get the job priority as specified by the client
                         * @return the job priority as specified by the client
                         */
                        inline int32_t get_priority() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[PRIORITY_NAME].GetInt();
                        }

                        /**
                         * Allows to get this text piece index, the index starts with
                         * zero and correspond to the text piece index in the job.
                         * @return the task index represented by this request
                         */
                        inline size_t get_chunk_idx() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[CHUNK_IDX_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the number of job's text pieces
                         * @return the number of job's text pieces
                         */
                        inline size_t get_num_chunks() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[NUM_CHUNKS_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the language for the task,
                         * it should be equal for all the job's tasks
                         * @return the processor job language, lowercased
                         */
                        inline string get_language() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[LANG_FIELD_NAME].GetString();
                        }

                        /**
                         * Allows to get the processor task text
                         * @return the processor task text
                         */
                        inline string get_chunk() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[CHUNK_FIELD_NAME].GetString();
                        }

                    protected:
                        //Stores the pointer to the incoming message
                        const incoming_msg * m_inc_msg;

                    };
                    
                    //Typedef the pointer to the request
                    typedef proc_req_in * proc_req_in_ptr;
                }
            }
        }
    }
}

#endif	/* PROC_REQ_IN_HPP */

