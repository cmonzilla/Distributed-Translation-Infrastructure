/* 
 * File:   ARPANGramBuilderFactory.hpp
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
 * Created on July 27, 2015, 11:17 PM
 */

#ifndef LM_GRAM_BUILDER_FACTORY_HPP
#define LM_GRAM_BUILDER_FACTORY_HPP

#include <string>       // std::stringstream
#include <ios>          //std::hex
#include <functional>   // std::function

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_configs.hpp"
#include "server/lm/lm_parameters.hpp"
#include "server/lm/builders/lm_gram_builder.hpp"

using namespace std;

using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace arpa {

                        /**
                         * This is the ARPA N-gram Builder Factory class that is supposed
                         * to be used to instantiate the proper ARPA N-Gram Builder class.
                         * Note that there can be a small difference in the data provided
                         * for the N-grams of different levels. For example the N-gram of 
                         * the maximum level does not have back-off weights, so knowing 
                         * that can allow for a more optimal reading the data and filling 
                         * in the Trie. Also the first level N-grams (N==1) are just words 
                         * and have to be added as vocabulary words into the Trie and not
                         * as regular N-grams.
                         */
                        template<typename TrieType>
                        class lm_gram_builder_factory {
                        public:
                            typedef typename TrieType::WordIndexType WordIndexType;

                            /**
                             * This is a template method for getting the proper ARPA
                             * N-gram Builder for the given N-gram level. The two
                             * template parameters of this method N and doCache have
                             * to do with the Trie template parameters. N is the maximum
                             * N-gram level and doCache indicates whether the given Trie
                             * does caching of query results.
                             * 
                             * Note: the returned pointer to the dynamically allocated
                             * builder is to be freed by the caller!
                             * 
                             * @tparam CURR_LEVEL the level of the N-gram we currently need the builder for
                             * @tparam is_mult_weight true if the loaded elements are to be multiplied with the weight, otherwise false
                             * @param params the model parameters weights are to be multiplies with the language model m-gram weight
                             * @param trie the trie to be filled in with the N-grams
                             * @param ppBuilder the pointer to the pointer to a dynamically allocated N-Gram builder
                             */
                            template<phrase_length CURR_LEVEL, bool is_mult_weight>
                            static inline void get_builder(const lm_parameters & params, TrieType & trie, lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight> **ppBuilder) {
                                //First reset the pointer to NULL
                                *ppBuilder = NULL;
                                LOG_DEBUG << "Requested a " << CURR_LEVEL << "-Gram builder, the maximum level is " << LM_M_GRAM_LEVEL_MAX << END_LOG;


                                //Then check that the level values are correct!
                                if (DO_SANITY_CHECKS && (CURR_LEVEL < M_GRAM_LEVEL_1 || CURR_LEVEL > LM_M_GRAM_LEVEL_MAX)) {
                                    stringstream msg;
                                    msg << "The requested N-gram level is '" << CURR_LEVEL
                                            << "', but it must be within [" << M_GRAM_LEVEL_1
                                            << ", " << LM_M_GRAM_LEVEL_MAX << "]!";
                                    THROW_EXCEPTION(msg.str());
                                } else {
                                    //Here we are to get the builder instance
                                    LOG_DEBUG1 << "Instantiating the " << CURR_LEVEL << "-Gram builder.." << END_LOG;
                                    //Create a builder with the proper lambda as an argument
                                    *ppBuilder = new lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight>(params, trie.get_word_index(),
                                            [&] (const model_m_gram & gram) {
                                                trie.template add_m_gram<CURR_LEVEL>(gram);
                                            });
                                    LOG_DEBUG2 << "DONE Instantiating the " << CURR_LEVEL << "-Gram builder!" << END_LOG;
                                }
                                LOG_DEBUG << "The " << CURR_LEVEL << "-Gram builder (" << hex << long(*ppBuilder) << ") is produced!" << END_LOG;
                            }

                            virtual ~lm_gram_builder_factory() {
                            }
                        private:

                            lm_gram_builder_factory() {
                            }

                            lm_gram_builder_factory(const lm_gram_builder_factory & other) {
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* ARPANGRAMBUILDERFACTORY_HPP */