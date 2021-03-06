/* 
 * File:   de_sent_info.hpp
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
 * Created on February 15, 2016, 1:33 PM
 */

#ifndef DE_SENTENCE_DATA_MAP_HPP
#define DE_SENTENCE_DATA_MAP_HPP

#include <string>
#include <cstdint>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/containers/upp_diag_matrix.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace sentence {
                        //Declare the minimum word index in the sentence
                        static constexpr int32_t MIN_SENT_WORD_INDEX = 0;

                        /**
                         * This structure stores the source phrase information
                         * data. This data is the begin and end character position
                         * of the phrase in the original sentence, also the first
                         * and the last word indexes, the phrase id and the available
                         * translation, i.e. source entry.
                         */
                        struct phrase_data_entry {

                            /**
                             * The basic constructor, does default initialization of the structure fields
                             */
                            phrase_data_entry()
                            : m_begin_ch_idx(0), m_end_ch_idx(0), m_phrase_uid(UNDEFINED_PHRASE_ID),
                            m_source_entry(NULL), future_cost(UNKNOWN_LOG_PROB_WEIGHT) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~phrase_data_entry() {
                            }

                            //Stores the phrase first word begin character index
                            uint32_t m_begin_ch_idx;
                            //Stores the phrase last word end character index plus one
                            uint32_t m_end_ch_idx;

                            //Stores the entire phrase uid
                            phrase_uid m_phrase_uid;

                            //Stores the pointer to the translation model source entry
                            tm_const_source_entry_ptr m_source_entry;
                            
                            //Stores the future cost, log scale, for the given phrase.
                            prob_weight future_cost;
                        };

                        //Define the sentence data map that stores some of the sentence related data
                        typedef upp_diag_matrix<phrase_data_entry> sentence_data_map;
                    }
                }
            }
        }
    }
}

#endif /* DE_SENT_INFO_HPP */

