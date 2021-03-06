/* 
 * File:   trans_job_resp_out.hpp
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
 * Created on June 23, 2016, 3:37 PM
 */

#ifndef TRANS_JOB_RESP_OUT_HPP
#define TRANS_JOB_RESP_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/trans_job_resp.hpp"

#include "server/messaging/trans_sent_data_out.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    /**
                     * This class represents a translation job response message to be sent to the client
                     */
                    class trans_job_resp_out : public outgoing_msg, public trans_job_resp {
                    public:

                        /**
                         * The basic class constructor
                         */
                        trans_job_resp_out()
                        : outgoing_msg(msg_type::MESSAGE_TRANS_JOB_RESP),
                        trans_job_resp(), m_sent_data(m_writer) {
                            //Nothing to be done here
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation result code. This
                         * constructor is to be used in case of error situations.
                         * @param job_id the client-issued id of the translation job 
                         * @param code the translation job result code
                         * @param msg the translation job status message
                         */
                        trans_job_resp_out(const job_id_type job_id,
                                const status_code code,
                                const string & msg)
                        : outgoing_msg(msg_type::MESSAGE_TRANS_JOB_RESP), trans_job_resp(),
                        m_sent_data(m_writer) {
                            //Set the values using the setter methods
                            set_job_id(job_id);
                            set_status(code, msg);
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~trans_job_resp_out() {
                            //Nothing to be done here
                        }

                        /**
                         * Begin the sentence data array section
                         */
                        inline void begin_sent_data_arr() {
                            m_writer.String(TARGET_DATA_FIELD_NAME);
                            m_writer.StartArray();
                        }

                        /**
                         * End the sentence data array section
                         */
                        inline void end_sent_data_arr() {
                            m_writer.EndArray();
                        }

                        /**
                         * Allows to set the message status: code and message
                         * @param code the status code
                         * @param msg the status message
                         */
                        inline void set_status(const status_code & code, const string & msg) {
                            m_writer.String(STAT_CODE_FIELD_NAME);
                            m_writer.Int(code.val());
                            m_writer.String(STAT_MSG_FIELD_NAME);
                            m_writer.String(msg.c_str());
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline void set_job_id(const job_id_type job_id) {
                            m_writer.String(JOB_ID_FIELD_NAME);
                            m_writer.Uint64(job_id);
                        }

                        /**
                         * Allows to add a new sentence data object into the list and
                         * return it in a wrapped around translation sentence data object.
                         * This method always return the same object, it only changes the
                         * object wrapped inside it.
                         * @return the same sentence data wrapper wrapping around different sentence data objects.
                         */
                        inline trans_sent_data_out & get_sent_data_writer() {
                            //Return the reference to the data
                            return m_sent_data;
                        }

                    private:
                        //Stores the pointer to the sentence data
                        trans_sent_data_out m_sent_data;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_RESP_OUT_HPP */

