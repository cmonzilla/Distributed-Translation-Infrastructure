/* 
 * File:   tm_limiting_builder.hpp
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
 * Created on May 4, 2016, 12:02 PM
 */

#ifndef TM_LIMITING_BUILDER_HPP
#define TM_LIMITING_BUILDER_HPP

#include <cmath>
#include <unordered_map>

#include "tm_builder.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/containers/ordered_list.hpp"

#include "server/server_consts.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/lm/proxy/lm_fast_query_proxy.hpp"
#include "server/lm/lm_configurator.hpp"

#include "server/tm/tm_parameters.hpp"
#include "server/tm/models/tm_target_entry.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace builders {
                        //The typedef for the list of targets
                        typedef ordered_list<tm_tmp_target_entry> targets_list;
                        //The typedef for the pointer to the list of targets
                        typedef ordered_list<tm_tmp_target_entry>* targets_list_ptr;
                        //Define the map storing the source phrase ids and the number of translations per phrase
                        typedef unordered_map<phrase_uid, targets_list_ptr> tm_data_map;

                        /**
                         * This class represents a builder of the translation model.
                         * It allows to read a text-formatted translation model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The translation model is also commonly known as a phrase table.
                         * This implementation is slow on loading the models when the 
                         * translation limit is too large or is not set.
                         */
                        template< typename model_type, typename reader_type>
                        class tm_basic_builder {
                        public:
                            //Stores the best-translations threshold for the builder.
                            //If the requested best trans. is less or equal to this
                            //value then the translation limit is ignored with a warning.
                            constexpr static uint32_t BEST_TRANS_THRESHOLD = targets_list::CAPACITY_THRESHOLD;

                            /**
                             * The basic constructor of the builder object
                             * @param params the model parameters
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            tm_basic_builder(const tm_parameters & params, model_type & model, reader_type & reader)
                            : m_params(params), m_data(NULL), m_model(model), m_reader(reader),
                            m_lm_query(lm_configurator::allocate_fast_query_proxy()), m_tmp_num_words(0) {
                                if (m_params.m_trans_limit <= BEST_TRANS_THRESHOLD) {
                                    LOG_WARNING << "The translation limit: " << m_params.m_trans_limit
                                            << " is too small, will be ignored!" << END_LOG;
                                    LOG_INFO << "The translation limit must be > " << BEST_TRANS_THRESHOLD
                                            << " to be taken into account." << END_LOG;
                                }
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_basic_builder() {
                                //Dispose the query proxy
                                lm_configurator::dispose_fast_query_proxy(m_lm_query);
                                //Destroy the data container if it is not destroyed yet
                                if (m_data != NULL) {
                                    delete m_data;
                                }
                            }

                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
                             * of distinct source phrases.
                             */
                            void build() {
                                //Set the number of TM features
                                tm_target_entry::set_num_features(m_params.m_num_lambdas);

                                //Load the model data into memory and filter
                                load_tm_data();
                                
                                //Convert the loaded data into the
                                //model and delete the temporary data
                                convert_tm_data();

                                //Add the unk (unknown translation) entry
                                add_unk_translation();
                            }

                        protected:

                            /**
                             * Allows to post-process a single feature, i.e. do:
                             *         log_e(feature)*lambda
                             * @param raw_feature the feature to post-process
                             * @param lambda the lambda weight to multiply the log_e feature with
                             * @param feature the out parameter into which the resulting value will be placed: log_10(raw_feature)*lambda
                             * @return the log_10 of the provided raw_feature
                             */
                            inline prob_weight post_process_feature(const prob_weight raw_feature, const prob_weight lambda, prob_weight & feature) {
                                //Convert the feature into the log scale
                                const prob_weight log_feature = std::log(raw_feature);

                                //Multiply the log-scale feature weight with the appropriate lambda
                                feature = log_feature * lambda;

                                LOG_DEBUG << "log_e(" << raw_feature << ") * " << lambda << " = " << feature << END_LOG;

                                //Return the log scale of the raw features
                                return log_feature;
                            }

                            /**
                             * Allows to add an unk entry to the model
                             */
                            inline void add_unk_translation() {
                                //Declare an array of features, zero-valued
                                feature_array unk_features = {}, pure_unk_features = {};

                                LOG_DEBUG << "The UNK initial features: "
                                        << array_to_string<prob_weight>(m_params.m_num_unk_features, m_params.m_unk_features) << END_LOG;
                                LOG_DEBUG << "The lambdas: "
                                        << array_to_string<prob_weight>(m_params.m_num_lambdas, m_params.m_lambdas) << END_LOG;

                                //Copy the values of the unk features to the writable array
                                for (size_t idx = 0; idx < m_params.m_num_unk_features; ++idx) {
                                    //Now convert to the log probability and multiply with the appropriate weight
                                    pure_unk_features[idx] = post_process_feature(m_params.m_unk_features[idx], m_params.m_lambdas[idx], unk_features[idx]);
                                }

                                //Set the unk features to the model
                                m_model.set_unk_entry(UNKNOWN_WORD_ID, unk_features, m_params.m_wp_lambda,
                                        m_lm_query.get_unk_word_prob(), pure_unk_features);
                            }

                            /**
                             * Allows to extract the features from the text piece and to
                             * check that they are valid with respect to the option bound
                             * If needed the weights will be converted to log scale and
                             * multiplied with the lambda factors
                             * @param weights [in] the text piece with weights, that starts with a space!
                             * @param features [out] the read and post-processed features features if they satisfy on the constraints
                             * @param pure_features [out] the non-processed features in case needed for tuning
                             * @return true if the features satisfy the constraints, otherwise false
                             */
                            inline bool process_features(text_piece_reader weights,
                                    prob_weight * features, prob_weight * pure_features = NULL) {
                                //Declare the token
                                text_piece_reader token;
                                //Store the read probability weight
                                size_t idx = 0;
                                //Store the read weight value
                                prob_weight raw_feature;

                                LOG_DEBUG << "LB Reading the features from: " << weights << END_LOG;

                                //Skip the first space
                                weights.get_first_space(token);

                                LOG_DEBUG3 << "TM features to parse: " << weights << END_LOG;

                                //Read the subsequent weights, check that the number of weights is as expected
                                while (weights.get_first_space(token) && (idx < m_params.m_num_lambdas)) {
                                    //Parse the token into the entry weight
                                    ASSERT_CONDITION_THROW(!fast_s_to_f(raw_feature, token.str().c_str()),
                                            string("Could not parse the token: ") + token.str());

                                    LOG_DEBUG3 << "parsed: " << token << " -> " << raw_feature << END_LOG;

                                    //Check that the probabilities before the phrase penalty match the threshold
                                    if ((idx < tm_parameters::TM_PHRASE_PENALTY_LAMBDA_IDX) &&
                                            (raw_feature < m_params.m_min_tran_prob)) {
                                        LOG_DEBUG1 << "The feature[" << idx << "] = " << raw_feature
                                                << " < " << m_params.m_min_tran_prob << END_LOG;
                                        return false;
                                    } else {
                                        //Now convert to the log probability and multiply with the appropriate weight
#if IS_SERVER_TUNING_MODE
                                        ASSERT_SANITY_THROW((pure_features == NULL), "The pure_features is NULL!");
                                        pure_features[idx] = post_process_feature(raw_feature, m_params.m_lambdas[idx], features[idx]);
#else
                                        (void) post_process_feature(raw_feature, m_params.m_lambdas[idx], features[idx]);
#endif
                                    }

                                    //Increment the index 
                                    ++idx;
                                }

                                //Check that the number of weights is good
                                ASSERT_CONDITION_THROW(!weights.get_rest_str().empty(),
                                        string("The TM model contains more features than ") +
                                        string("the specified lambda values (") +
                                        to_string(m_params.m_num_lambdas)+(") in the config file"));

                                LOG_DEBUG1 << "Got " << idx << " good features!" << END_LOG;

                                return true;
                            }

                            /**
                             * Allows to create a new target entry in case the 
                             * @param rest the text piece reader to parse the target entry from
                             * @param source_uid the if of the source phrase
                             * @param entry [out] the reference to the entry pointer
                             * @return true if the entry was created, otherwise false
                             */
                            inline bool get_target_entry(text_piece_reader &rest, phrase_uid source_uid, tm_tmp_target_entry *&entry) {
                                LOG_DEBUG2 << "Got translation line to parse: ___" << rest << "___" << END_LOG;

                                //Declare an array of weights for temporary use
                                feature_array tmp_features = {}, tmp_pure_features = {};

                                //Declare the target and weights entry reader
                                text_piece_reader target, weights;

                                //Skip the first space symbol that follows the delimiter with the source
                                rest.get_first_space(target);

                                //Read the target phrase, it is surrounded by spaces
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                //Read the weights
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(weights);

                                //Check that the weights are good and retrieve them if they are
                                if (process_features(weights, tmp_features, tmp_pure_features)) {
                                    LOG_DEBUG2 << "The target phrase is: " << target << END_LOG;
                                    //Add the translation entry to the model
                                    string target_str = target.str();
                                    trim(target_str);
                                    //Compute the target phrase and its uid
                                    const phrase_uid target_uid = get_phrase_uid<true>(target_str);

                                    //Use the language model to get the target translation word ids
                                    m_lm_query.get_word_ids(target, m_tmp_num_words, m_tmp_word_ids);

                                    LOG_DEBUG << "The phrase: ___" << target << "__ got "
                                            << m_tmp_num_words << " word ids: " <<
                                            array_to_string<word_uid>(m_tmp_num_words, m_tmp_word_ids) << END_LOG;
                                    
                                    //Initiate a new temporary target entry
                                    entry = new tm_tmp_target_entry();
                                    
                                    //Set the target entry data first, to make it distinguishable
                                    entry->set_data(source_uid, target_str, target_uid,
                                            tmp_features, m_tmp_num_words, m_tmp_word_ids,
                                            m_params.m_wp_lambda, tmp_pure_features);

                                    //Get the Language Model weight for the target translation
                                    const prob_weight lm_weight = m_lm_query.execute(m_tmp_num_words, m_tmp_word_ids);
                                    LOG_DEBUG << "The phrase: ___" << target_str << "__ lm-weight: " << lm_weight << END_LOG;
                                    
                                    //Set the LM target weight, is needed to compute the temporary total
                                    //weight for ordering the entries and is also later needed to compute
                                    //the minimum translation cost of the source entry
                                    entry->set_lm_weight(lm_weight);

                                    LOG_DEBUG1 << "The source/target (" << source_uid
                                            << "/" << target_uid << ") entry was created!" << END_LOG;

                                    return true;
                                } else {
                                    LOG_DEBUG1 << "The target entry for the source "
                                            << source_uid << " was filtered out!" << END_LOG;
                                    return false;
                                }
                            }

                            /**
                             * Parses the tm file and loads the data
                             */
                            inline void parse_tm_file() {
                                //Declare the text piece reader for storing the read line and source phrase
                                text_piece_reader line, source;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;

                                //The pointers to the current targets list
                                targets_list_ptr targets = NULL;
                                //Declare the pointer variable for the target entry
                                tm_tmp_target_entry * entry = NULL;

                                LOG_DEBUG << "Start parsing the TM file" << END_LOG;

                                //Instantiate the data map container
                                m_data = new tm_data_map();

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uid
                                    string next_source_str = source.str();
                                    trim(next_source_str);

                                    LOG_DEBUG << "Got the source phrase: ___" << next_source_str << "___" << END_LOG;

                                    //If we are now reading the new source entry
                                    if (source_str != next_source_str) {
                                        //Delete the empty targets of the previous source entry
                                        //and erase the entry as it got no translations
                                        if (targets != NULL && (targets->get_size() == 0)) {
                                            LOG_DEBUG << "Deleting the previous source entry "
                                                    << source_uid << ", as it has no targets!" << END_LOG;
                                            delete targets;
                                            m_data->erase(source_uid);
                                        }

                                        //Store the new source string
                                        source_str = next_source_str;
                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);

                                        LOG_DEBUG1 << "The NEW source ___" << source_str << "___ id is: " << source_uid << END_LOG;

                                        //Create a new list of targets, or retrieve an existing one
                                        tm_data_map::iterator iter = m_data->find(source_uid);
                                        if (iter == m_data->end()) {
                                            targets = new targets_list(m_params.m_trans_limit);
                                            m_data->operator[](source_uid) = targets;
                                        } else {
                                            targets = iter->second;
                                        }
                                    }

                                    //Get the target entry if it is passes
                                    if (get_target_entry(line, source_uid, entry)) {
                                        LOG_DEBUG1 << "Adding the new target entry to the source " << source_uid << END_LOG;
                                        //Add the translation entry to the list
                                        targets->add_elemenent(entry);
                                    }
                                    LOG_DEBUG1 << "The source " << source_uid << " targets count is " << targets->get_size() << END_LOG;

                                    //Update the progress bar status
                                    logger::update_progress_bar();
                                }
                            }

                            /**
                             * Loads the translation model data into memory and filters is
                             */
                            inline void load_tm_data() {
                                logger::start_progress_bar(string("Pre-loading phrase translations"));

                                //Count the good entries
                                parse_tm_file();

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();

                                LOG_INFO << "The number of loaded TM source entries is: " << m_data->size() << END_LOG;
                            }

                            /**
                             * Loads the translation model data into memory and filters is
                             */
                            inline void convert_tm_data() {
                                logger::start_progress_bar(string("Storing the pre-loaded phrase translations"));

                                //Set the number of entries into the model
                                m_model.set_num_entries(m_data->size());

                                //Iterate through the map elements and do conversion
                                for (tm_data_map::iterator it = m_data->begin(); it != m_data->end(); ++it) {
                                    //The current source id
                                    phrase_uid source_uid = it->first;
                                    //The pointers to the current targets list
                                    targets_list_ptr targets = it->second;

                                    //Add the source entry and the translation entries
                                    if (targets->get_size() != 0) {
                                        //Open the new source entry
                                        tm_source_entry * source_entry = m_model.begin_entry(source_uid, targets->get_size());

                                        ASSERT_SANITY_THROW((m_params.m_trans_limit > 0) &&
                                                (targets->get_size() > m_params.m_trans_limit),
                                                string("The COUNTED targets size ") + to_string(targets->get_size()) +
                                                string(" is exceeding the trans limit ") + to_string(m_params.m_trans_limit));

                                        LOG_DEBUG << "TM source: " << source_uid << ", #targets: " << targets->get_size() << END_LOG;

                                        //The pointer variable for the target entry container
                                        targets_list::elem_container * entry = targets->get_first();

                                        //Add the translation entries
                                        while (entry != NULL) {
                                            //Get the reference to the target entry
                                            tm_tmp_target_entry & target = **entry;

                                            source_entry->emplace_target(target);

                                            //Move on to the next entry
                                            entry = entry->m_next;
                                        }

                                        ASSERT_SANITY_THROW((m_params.m_trans_limit > 0) &&
                                                (source_entry->num_targets() > m_params.m_trans_limit),
                                                string("The ACTUAL targets size ") + to_string(source_entry->num_targets()) +
                                                string(" is exceeding the trans limit ") + to_string(m_params.m_trans_limit));

                                        //Finalize the source entry
                                        m_model.finalize_entry(source_uid);
                                    }

                                    //Delete the entry
                                    delete targets;

                                    //Update the progress bar status
                                    logger::update_progress_bar();
                                }

                                //Erase the map as we do not need it any more
                                delete m_data;
                                m_data = NULL;

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();

                                LOG_INFO << "The phrase-translations table is created and loaded" << END_LOG;
                            }


                        private:
                            //Stores the reference to the model parameters
                            const tm_parameters & m_params;

                            //The map storing the model sizes
                            tm_data_map * m_data;

                            //Stores the reference to the model
                            model_type & m_model;

                            //Stores the reference to the builder;
                            reader_type & m_reader;

                            //Stores the reference to the LM query proxy
                            lm_fast_query_proxy & m_lm_query;

                            //The temporary variable to store the number of words in the target translation phrase
                            phrase_length m_tmp_num_words;
                            //The temporary variable to store word ids for the target translation phrase LM word ids
                            word_uid m_tmp_word_ids[TM_MAX_TARGET_PHRASE_LEN];
                        };
                    };
                }
            }
        }
    }
}

#endif /* TM_LIMITING_BUILDER_HPP */

