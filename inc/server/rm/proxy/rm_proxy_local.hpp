/* 
 * File:   rm_proxy_local.hpp
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
 * Created on February 8, 2016, 10:00 AM
 */

#ifndef RM_PROXY_LOCAL_HPP
#define RM_PROXY_LOCAL_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/monitor/statistics_monitor.hpp"

#include "server/server_configs.hpp"
#include "server/rm/rm_configs.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"
#include "server/rm/proxy/rm_query_proxy_local.hpp"

using namespace uva::utils::monitor;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;

using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::rm::proxy;
using namespace uva::smt::bpbd::server::rm::builders;
using namespace uva::smt::bpbd::server::rm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This is the reordering model proxy interface class it allows to
                         * interact with any sort of local and remote models in a uniform way.
                         */
                        class rm_proxy_local : public rm_proxy {
                        public:

                            /**
                             * The basic proxy constructor, currently does nothing except for default initialization
                             */
                            rm_proxy_local() {
                            }

                            /**
                             * The basic destructor
                             */
                            virtual ~rm_proxy_local() {
                                //Disconnect, just in case it has not been done before
                                disconnect();
                            };

                            /**
                             * @see rm_proxy
                             */
                            virtual void connect(const rm_parameters & params) {
                                //The whole purpose of this method connect here is
                                //just to load the reordering model into the memory.
                                load_model_data<rm_builder_type, rm_model_reader>("Reordering Model", params);

                                //Get the pointers to the begin and end tag reordering entries
                                m_begin_tag_entry = m_model.get_begin_tag_entry();
                                m_end_tag_entry = m_model.get_end_tag_entry();
                            }

                            /**
                             * @see rm_proxy
                             */
                            virtual void disconnect() {
                                //Nothing the be done here yet, the model is allocated on the stack
                            }

                            /**
                             * \todo {In the future we should just use a number of stack
                             * allocated objects in order to reduce the new/delete overhead}
                             * @see rm_proxy
                             */
                            virtual rm_query_proxy & allocate_query_proxy() {
                                return *(new rm_query_proxy_local<rm_model_type>(m_model, *m_begin_tag_entry, *m_end_tag_entry));
                            }

                            /**
                             * Dispose the previously allocated query object
                             * \todo {In the future we should just use a number of stack
                             * allocated objects in order to reduce the new/delete overhead}
                             * @param query the query to dispose
                             */
                            virtual void dispose_query_proxy(rm_query_proxy & query) {
                                delete &query;
                            }

                        protected:

                            /**
                             * Allows to load the model into the instance of the selected container class
                             * \todo Add the possibility to choose between the file readers from the command line!
                             * @param model_name the name of the model being loaded
                             * @param params the model parameters
                             */
                            template<typename rm_builder_type, typename file_reader_type>
                            void load_model_data(char const *model_name, const rm_parameters & params) {
                                const string & model_file_name = params.m_conn_string;

                                //Declare time variables for CPU times in seconds
                                double start_time, end_time;
                                //Declare the statistics monitor and its data
                                TMemotyUsage mem_stat_start = {}, mem_stat_end = {};

                                LOG_USAGE << "--------------------------------------------------------" << END_LOG;
                                LOG_USAGE << "Start creating and loading the " << model_name << " ..." << END_LOG;
                                LOG_USAGE << model_name << " is located in: " << model_file_name << END_LOG;

                                LOG_DEBUG << "Getting the memory statistics before opening the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);
                                LOG_DEBUG1 << "The memory statistics is obtained" << END_LOG;

                                //Attempt to open the model file
                                LOG_DEBUG << "Attempting to open the RM model file: " << model_file_name << END_LOG;
                                file_reader_type model_file(model_file_name.c_str());
                                model_file.log_reader_type_info();

                                LOG_DEBUG << "Getting the memory statistics after opening the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);

                                //Log the usage information
                                m_model.log_model_type_info();

                                LOG_DEBUG << "Getting the memory statistics before loading the " << model_name << " ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);
                                LOG_DEBUG << "Getting the time statistics before creating the " << model_name << " ..." << END_LOG;
                                start_time = stat_monitor::get_cpu_time();

                                //Assert that the model file is opened
                                ASSERT_CONDITION_THROW(!model_file.is_open(), string("The ") + string(model_name) + string(" file: '")
                                        + model_file_name + string("' does not exist!"));

                                //Create the trie builder and give it the trie
                                LOG_DEBUG << "Creating the RM model builder!" << END_LOG;
                                rm_builder_type builder(params, m_model, model_file);

                                //Load the model from the file
                                LOG_DEBUG << "Start reading the RM model with the builder!" << END_LOG;
                                builder.build();

                                LOG_DEBUG << "Getting the time statistics after loading the " << model_name << " ..." << END_LOG;
                                end_time = stat_monitor::get_cpu_time();
                                LOG_USAGE << "Reading the " << model_name << " took " << (end_time - start_time) << " CPU seconds." << END_LOG;
                                LOG_DEBUG << "Getting the memory statistics after loading the " << model_name << " ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);
                                LOG_DEBUG << "Reporting on the memory consumption" << END_LOG;
                                const string action_name = string("Loading the ") + string(model_name);
                                report_memory_usage(action_name.c_str(), mem_stat_start, mem_stat_end, true);

                                LOG_DEBUG << "Getting the memory statistics before closing the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_start);
                                LOG_DEBUG << "Closing the model file ..." << END_LOG;
                                model_file.close();
                                LOG_DEBUG << "Getting the memory statistics after closing the " << model_name << " file ..." << END_LOG;
                                stat_monitor::get_mem_stat(mem_stat_end);
                            }

                        private:
                            //Stores the reordering model instance
                            rm_model_type m_model;

                            //Stores the pointer to the begin tag reordering 
                            const rm_entry * m_begin_tag_entry;

                            //Stores the pointer to the end tag reordering 
                            const rm_entry * m_end_tag_entry;
                        };
                    }
                }
            }
        }
    }
}


#endif /* RM_PROXY_LOCAL_HPP */

