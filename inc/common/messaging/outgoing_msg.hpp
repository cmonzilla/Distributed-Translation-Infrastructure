/* 
 * File:   outgoing_msg.hpp
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
 * Created on June 23, 2016, 3:33 PM
 */

#ifndef OUTGOING_MSG_HPP
#define OUTGOING_MSG_HPP

#include "rapidjson/prettywriter.h" // for stringify JSON

#include "common/messaging/msg_base.hpp"

using namespace rapidjson;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    
                    //The JSON writer type, can be configures to use two variants
                    //typedef PrettyWriter<StringBuffer> JSONWriter; //This one is good for testing, to make JSON more readable
                    typedef Writer<StringBuffer> JSONWriter; //This one is good for production, works faster, no extra characters
                    
                    /**
                     * This class represents a JSON message begin sent between the client and the server.
                     */
                    class outgoing_msg : public msg_base {
                    public:

                        /**
                         * The basic constructor
                         * @param type the message type
                         */
                        outgoing_msg(msg_type type) : msg_base(), m_string_buf(), m_writer(m_string_buf) {
                            //Initialize the outgoing message with the protocol version and type data
                            m_writer.StartObject();
                            m_writer.String(PROT_VER_FIELD_NAME);
                            m_writer.Uint(PROTOCOL_VERSION);
                            m_writer.String(MSG_TYPE_FIELD_NAME);
                            m_writer.Int(type);
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~outgoing_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to serialize the outgoing message into a string
                         * @return the string representation of the message
                         */
                        inline string serialize() {
                            //Finish the object
                            m_writer.EndObject();

                            //Return the serialized string
                            return m_string_buf.GetString();
                        }

                    private:
                        //Stores the string buffer to where the JSON string will be written
                        StringBuffer m_string_buf;

                    protected:
                        //Stores the JSON writer to be used for streaming response.
                        JSONWriter m_writer;
                    };

                }
            }
        }
    }
}


#endif /* OUTGOING_MSG_HPP */

