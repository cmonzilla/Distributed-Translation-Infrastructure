/* 
 * File:   tm_consts.hpp
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
 * Created on February 24, 2016, 9:44 AM
 */

#ifndef TM_CONSTS_HPP
#define	TM_CONSTS_HPP

#include <string>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {
                        namespace __tm_basic_model {
                            //Influences the number of buckets that will be created for the basic model implementations
                            static constexpr double SOURCES_BUCKETS_FACTOR = 3.0;
                        }
                    }
                }
            }
        }
    }
}

#endif	/* TM_CONSTS_HPP */

