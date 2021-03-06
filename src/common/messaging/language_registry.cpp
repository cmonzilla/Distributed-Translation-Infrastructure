/* 
 * File:   language_registry.cpp
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
 * Created on July 14, 2016, 1:30 PM
 */

#include "common/messaging/language_registry.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    constexpr language_uid language_registry::UNKNONW_LANGUAGE_ID;
                    const string language_registry::UNKNONW_LANGUAGE_NAME = "<unknown-language>";

                    constexpr language_uid language_registry::MIN_LANGUAGE_ID;
                    id_manager<language_uid> language_registry::m_id_mgr(MIN_LANGUAGE_ID);
                    shared_mutex language_registry::m_lang_mutex;
                    unordered_map<string, language_uid> language_registry::m_lang_to_id;
                    unordered_map<language_uid, string> language_registry::m_id_to_lang;
                }
            }
        }
    }
}