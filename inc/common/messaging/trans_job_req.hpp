/* 
 * File:   trans_job_req.hpp
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
 * Created on June 23, 2016, 3:36 PM
 */

#ifndef TRANS_JOB_REQ_HPP
#define TRANS_JOB_REQ_HPP

#include <common/messaging/request_msg.hpp>

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the base class for all the translation job request messages
                     */
                    class trans_job_req : public request_msg {
                    public:
                        //Stores the job id attribute name
                        static const char * JOB_ID_FIELD_NAME;
                        //Stores the priority name
                        static const char * PRIORITY_NAME;
                        //Stores the source language attribute name
                        static const char * SOURCE_LANG_FIELD_NAME;
                        //Stores the target language attribute name
                        static const char * TARGET_LANG_FIELD_NAME;
                        //Stores the translation info flag attribute name
                        static const char * IS_TRANS_INFO_FIELD_NAME;
                        //Stores the source sentences attribute name
                        static const char * SOURCE_SENTENCES_FIELD_NAME;

                        /**
                         * The basic constructor
                         */
                        trans_job_req() : request_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_req() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* TRANS_JOB_REQ_HPP */

