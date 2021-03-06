/* 
 * File:   supp_lang_req_out.hpp
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

#ifndef SUPP_LANG_REQ_OUT_HPP
#define SUPP_LANG_REQ_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/supp_lang_req.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents an outgoing request for supported
                     * languages, created on the client side.
                     */
                    class supp_lang_req_out : public outgoing_msg, public supp_lang_req{
                    public:

                        /**
                         * The basic constructor
                         */
                        supp_lang_req_out()
                        : outgoing_msg(msg_type::MESSAGE_SUPP_LANG_REQ), supp_lang_req() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~supp_lang_req_out() {
                            //Nothing to be done here
                        }
                    private:
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_REQ_OUT_HPP */

