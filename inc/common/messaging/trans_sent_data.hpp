/* 
 * File:   trans_sent_data.hpp
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
 * Created on June 23, 2016, 3:35 PM
 */

#ifndef TRANS_SENT_DATA_HPP
#define TRANS_SENT_DATA_HPP

#include <string>

#include <common/messaging/response_msg.hpp>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class stores the translation data for a translated sentence.
                     * It wraps around a JSON object, but it does not own it.
                     */
                    class trans_sent_data : public response_msg {
                    public:
                        //The target data field name
                        static const char * TRANS_TEXT_FIELD_NAME;
                        //The target data field name
                        static const char * STACK_LOAD_FIELD_NAME;

                        //Typedef the loads array data structure for storing the stack load percent values
                        typedef vector<int64_t> stack_loads;

                        /**
                         * The basic constructor.
                         */
                        trans_sent_data() : response_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_sent_data() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* TRANS_SENT_DATA_HPP */

