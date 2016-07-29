/* 
 * File:   translation_client.hpp
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
 * Created on January 26, 2016, 12:13 PM
 */

#ifndef CLIENT_CONFIG_HPP
#define CLIENT_CONFIG_HPP

#include <regex>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * This structure stores the translation client execution parameters
                 */
                struct client_config_struct {
                    //The source file name with the text to translate
                    string m_source_file;
                    //The language to translate from
                    string m_source_lang;
                    //The target file name to put the translation into
                    string m_target_file;
                    //The language to translate into
                    string m_target_lang;
                    //The server URI to connect to
                    string m_uri;
                    //The maximum number of source sentences to send per translation request
                    uint64_t m_max_sent;
                    //The minimum number of source sentences to send per translation request
                    uint64_t m_min_sent;
                    //Stores the flag that indicates whether the source sentence is to be pre-processed
                    bool m_is_pre_process;
                    //The flag indicating whether the client requests the translation details from the translation server or not.
                    bool m_is_trans_info;

                    /**
                     * The basic constructor
                     */
                    client_config_struct()
                    : m_uri_reg_exp("ws://.*:\\d+") {
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    void finalize() {
                        //Check the uri format
                        ASSERT_CONDITION_THROW(!regex_match(m_uri, m_uri_reg_exp),
                                string("The server uri: '") + m_uri +
                                string("' does not match the format: ws://<server>:<port>"));
                    }

                private:
                    //The regular expression for matching the server uri
                    const regex m_uri_reg_exp;
                };

                typedef client_config_struct client_config;
            }
        }
    }
}

#endif