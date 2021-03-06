/* 
 * File:   trans_job_req_out.hpp
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
 * Created on June 23, 2016, 3:38 PM
 */

#ifndef TRANS_JOB_REQ_OUT_HPP
#define TRANS_JOB_REQ_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/trans_job_req.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents an outgoing translation
                     * job request, created on the client side.
                     */
                    class trans_job_req_out : public outgoing_msg, public trans_job_req {
                    public:

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation text and source
                         * and target language strings.
                         * @param job_id the translation job id
                         * @param priority the translation job priority
                         * @param source_lang the source language string
                         * @param source_text the text in the source language to translate
                         * @param target_lang the target language string
                         * @param is_trans_info true if the client should requests the translation info from the server
                         */
                        trans_job_req_out(const job_id_type job_id, const int32_t priority, const string & source_lang,
                                vector<string> & source_text, const string & target_lang, const bool is_trans_info)
                        : outgoing_msg(msg_type::MESSAGE_TRANS_JOB_REQ), trans_job_req(), m_job_id(job_id) {
                            m_writer.String(JOB_ID_FIELD_NAME);
                            m_writer.Uint64(job_id);
                            m_writer.String(PRIORITY_NAME);
                            m_writer.Int(priority);
                            m_writer.String(SOURCE_LANG_FIELD_NAME);
                            m_writer.String(source_lang.c_str());
                            m_writer.String(TARGET_LANG_FIELD_NAME);
                            m_writer.String(target_lang.c_str());
                            m_writer.String(IS_TRANS_INFO_FIELD_NAME);
                            m_writer.Bool(is_trans_info);
                            
                            //Add the source sentences one by one in an array
                            m_writer.String(SOURCE_SENTENCES_FIELD_NAME);
                            m_writer.StartArray();
                            for (auto iter = source_text.begin(); iter != source_text.end(); ++iter) {
                                m_writer.String(iter->c_str());
                            }
                            m_writer.EndArray();

                            LOG_DEBUG << "Translation job request, job id: " << m_job_id
                                    << " source language: " << source_lang
                                    << " target language: " << target_lang
                                    << " translation info flag: " << is_trans_info << END_LOG;
                        }

                        /**
                         * Allows to get the job id type
                         * @return the job id type
                         */
                        job_id_type get_job_id() const {
                            return m_job_id;
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_req_out() {
                            //Nothing to be done here
                        }
                        
                    private:
                        //Stores the job id
                        job_id_type m_job_id;
                        
                    };

                }
            }
        }
    }
}

#endif /* TRANS_JOB_REQ_OUT_HPP */

