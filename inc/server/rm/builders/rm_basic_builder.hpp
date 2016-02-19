/* 
 * File:   rm_basic_builder.hpp
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
 * Created on February 8, 2016, 9:56 AM
 */

#ifndef RM_BASIC_BUILDER_HPP
#define RM_BASIC_BUILDER_HPP

#include <cmath>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/string_utils.hpp"

#include "server/tm/tm_configurator.hpp"
#include "server/tm/proxy/tm_query_proxy.hpp"

#include "server/common/models/phrase_uid.hpp"
#include "server/rm/rm_parameters.hpp"
#include "server/rm/models/rm_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::rm::models;
using namespace uva::smt::bpbd::server::rm::proxy;
using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace builders {

                        //Stores the translation model delimiter character for parsing one line
                        static const char RM_DELIMITER = '|';
                        //Stores the translation model delimiter character cardinality
                        static const size_t RM_DELIMITER_CDTY = 3;

                        /**
                         * This class represents a basic reader of the reordering model.
                         * It allows to read a text-formatted reordering model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The reordering model is also commonly known as a phrase table.
                         */
                        template< typename model_type, typename reader_type>
                        class rm_basic_builder {
                        public:

                            /**
                             * The basic constructor of the builder object
                             * @params params the model parameters
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            rm_basic_builder(const rm_parameters & params, model_type & model, reader_type & reader)
                            : m_params(params), m_model(model), m_reader(reader), m_num_entries(0) {
                            }

                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
                             * of distinct source phrases.
                             */
                            void build() {
                                //Obtains the query proxy
                                tm_query_proxy & query = tm_configurator::allocate_query_proxy();

                                //Count and set the number of source phrases if needed
                                if (m_model.is_num_entries_needed()) {
                                    count_source_target_phrases(query);
                                }

                                //Process the translations
                                process_source_entries(query);

                                //Dispose the query proxy
                                tm_configurator::dispose_query_proxy(query);
                            }

                        protected:

                            /**
                             * Allows to parse the reordering weights and set them into the reordering entry
                             * @param rest the line to be parsed, starts with a space
                             * @param entry the entry to put the values into
                             */
                            void process_entry_weights(TextPieceReader & rest, rm_entry & entry) {
                                //Declare the token to store weights
                                TextPieceReader token;

                                //Skip the first space
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not skip the first space!");

                                //Read the subsequent weights, check that the number of weights is as expected
                                size_t idx = 0;
                                while (rest.get_first_space(token) && (idx < rm_entry::NUM_FEATURES)) {
                                    //Parse the token into the entry weight
                                    ASSERT_CONDITION_THROW(!fast_s_to_f(entry[idx], token.str().c_str()),
                                            string("Could not parse the token: ") + token.str());
                                    //Now convert to the log probability and multiply with the appropriate weight
                                    entry[idx] = log10(entry[idx]) * m_params.m_lambdas[idx];
                                    //Increment the index 
                                    ++idx;
                                }
                            }

                            /**
                             * Allows to parse the RM model file and do two things depending on the value of the template parameter:
                             * 1. Count the number of valid entries
                             * 2. Build the RM model
                             * NOTE: This two pass parsing is not optimal but we have to do it as we need to know
                             *       the number of valid entries beforehand, an optimization might be needed!
                             * @param count_or_build if true then count if false then build
                             * @param query the TM query to check if the source/taret are known
                             */
                            template<bool count_or_build>
                            inline void parse_rm_file(tm_query_proxy & query) {
                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source, target;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;
                                phrase_uid target_uid = UNDEFINED_PHRASE_ID;
                                tm_const_source_entry_ptr source_entry = NULL;
                                bool is_good_source = false;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<RM_DELIMITER, RM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase
                                    string next_source_str = source.str();
                                    trim(next_source_str);
                                    
                                    //Check if this is a new source phrase
                                    if (source_str != next_source_str) {
                                        //Store the new source string
                                        source_str = next_source_str;
                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);
                                        //Check if the have an UNK
                                        if (source_uid == m_model.SOURCE_UNK_UID) {
                                            //The UNK source is always good
                                            is_good_source = true;
                                        } else {
                                            //Obtain the new source entry
                                            source_entry = query.get_source_entry(source_uid);
                                            //Check if we shall ignore this source
                                            is_good_source = (source_entry != NULL);
                                            
                                            LOG_USAGE << "Got a TM source entry for " << source_uid << END_LOG;
                                        }
                                    }

                                    //If the source is present then count the entry
                                    if (is_good_source) {
                                        //Read the target phrase
                                        line.get_first<RM_DELIMITER, RM_DELIMITER_CDTY>(target);
                                        string target_str = target.str();
                                        trim(target_str);

                                        LOG_DEBUG << "Got rm entry: " << source_str << " / " << target_str << END_LOG;

                                        //Parse the rest of the target entry
                                        target_uid = get_phrase_uid<true>(target_str);

                                        //Check if the given translation phrase is known
                                        if ((target_uid == m_model.TARGET_UNK_UID) ||
                                                source_entry->has_translation(target_uid)) {
                                            if (count_or_build) {
                                                //Increment the counter
                                                ++m_num_entries;
                                            } else {
                                                //If the target translation is present then process the features
                                                process_entry_weights(line, m_model.add_entry(source_uid, target_uid));
                                            }
                                        }
                                    }

                                    //Update the progress bar status
                                    Logger::update_progress_bar();
                                }
                            }

                            /**
                             * Allows to count and set the number of source phrases
                             * @param count_or_build if true then we do count entries
                             *                       if false then we do build be model
                             * @param query the translation model query object to
                             * query the translation model for present entries
                             */
                            inline void count_source_target_phrases(tm_query_proxy & query) {
                                Logger::start_progress_bar(string("Counting reordering entries"));

                                //Count the number of valid entries
                                parse_rm_file<true>(query);

                                //Set the number of entries into the model
                                m_model.set_num_entries(m_num_entries);

                                //Re-set the reader to start all over again
                                m_reader.reset();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();

                                LOG_INFO << "The number of RM entries matching TM is: " << m_num_entries << END_LOG;
                            }

                            /**
                             * Allows to process translations.
                             * @param query the translation model query object to
                             * query the translation model for present entries
                             */
                            void process_source_entries(tm_query_proxy & query) {
                                Logger::start_progress_bar(string("Building reordering model"));

                                //Build the RM model
                                parse_rm_file<false>(query);

                                //Find the UNK entry, this is needed for performance optimization.
                                m_model.find_unk_entry();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();
                            }

                        private:
                            //Stores the reference to the model parameters
                            const rm_parameters & m_params;
                            //Stores the reference to the model
                            model_type & m_model;
                            //Stores the reference to the builder;
                            reader_type & m_reader;
                            //Stores the number of valid RM model entries
                            size_t m_num_entries;
                        };
                    }
                }
            }
        }
    }
}

#endif /* RM_BASIC_BUILDER_HPP */

