/* 
 * File:   processor_job.hpp
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
 * Created on August 23, 2016, 13:30 AM
 */
#include "processor/processor_job.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                ostream & operator<<(ostream & stream, const processor_job & job) {
                    return stream << "[ job: " << job.m_job_token
                            << ", session id: " << job.m_session_id
                            << ", act_num_chunks: " << job.m_act_num_chunks
                            << ", exp_num_chunks: " << job.m_exp_num_chunks
                            << ", is_canceled: " << job.m_is_canceled
                            << " ]";
                }
            }
        }
    }
}