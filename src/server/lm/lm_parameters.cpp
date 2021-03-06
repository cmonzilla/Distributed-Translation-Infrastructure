/* 
 * File:   lm_parameters.cpp
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
 * Created on May 17, 2016, 12:23 AM
 */

#include "server/lm/lm_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    const string lm_parameters_struct::LM_CONFIG_SECTION_NAME = "Language Models";
                    const string lm_parameters_struct::LM_CONN_STRING_PARAM_NAME = "lm_conn_string";
                    const string lm_parameters_struct::LM_WEIGHTS_PARAM_NAME = "lm_feature_weights";
                    const string lm_parameters_struct::LM_WEIGHT_NAMES[MAX_NUM_LM_FEATURES] = {
                        LM_WEIGHTS_PARAM_NAME + string("[0]")
                    };
                    size_t lm_parameters_struct::LM_WEIGHT_GLOBAL_IDS[MAX_NUM_LM_FEATURES] = {};
                    const string lm_parameters_struct::LM_UNK_WORD_LOG_E_PROB_PARAM_NAME = "unk_word_log_e_prob";
                }
            }
        }
    }
}