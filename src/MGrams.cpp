/* 
 * File:   MGram.hpp
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
 * Created on September 7, 2015, 9:42 PM
 */

#include "MGrams.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

using namespace std;

namespace uva {
    namespace smt {
        namespace tries {
            namespace mgrams {

                //The memory in bits needed to store different M-gram id types in
                //the M-gram id byte arrays
                //ToDo: These are the minimum values, if used then we use fewest memory
                //but the data is not byte aligned. Therefore some bit copying operations
                //are not done efficiently. Perhaps it is worth trying to round these
                //values up to full bytes, this can improve performance @ some memory costs.
                template<TModelLevel M_GRAM_LEVEL>
                const uint8_t T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::M_GRAM_2_ID_TYPE_LEN_BITS = 10;
                template<TModelLevel M_GRAM_LEVEL>
                const uint8_t T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::M_GRAM_3_ID_TYPE_LEN_BITS = 15;
                template<TModelLevel M_GRAM_LEVEL>
                const uint8_t T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::M_GRAM_4_ID_TYPE_LEN_BITS = 20;
                template<TModelLevel M_GRAM_LEVEL>
                const uint8_t T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::M_GRAM_5_ID_TYPE_LEN_BITS = 25;

                //The length of the M-gram id types in bits depending on the M-Gram level starting from 2.
                static constexpr uint8_t M_GRAM_ID_TYPE_LEN_BITS[] = {
                    T_Compressed_M_Gram_Id<M_GRAM_LEVEL_5>::M_GRAM_2_ID_TYPE_LEN_BITS,
                    T_Compressed_M_Gram_Id<M_GRAM_LEVEL_5>::M_GRAM_3_ID_TYPE_LEN_BITS,
                    T_Compressed_M_Gram_Id<M_GRAM_LEVEL_5>::M_GRAM_4_ID_TYPE_LEN_BITS,
                    T_Compressed_M_Gram_Id<M_GRAM_LEVEL_5>::M_GRAM_5_ID_TYPE_LEN_BITS
                };

                /**
                 * This method allows to get the number of bits needed to store this word id
                 * @param wordId the word id to analyze
                 * @return the number of bits needed to store this word id
                 */
                static inline uint8_t get_number_of_bits(const TShortId wordId) {
                    return log2_32(wordId);
                };

                /**
                 * Stores the multipliers up to and including level 6
                 */
                const static uint32_t gram_id_type_mult[] = {1, 32, 32 * 32, 32 * 32 * 32, 32 * 32 * 32 * 32, 32 * 32 * 32 * 32 * 32};

                /**
                 * This method is needed to compute the id type identifier.
                 * Can compute the id type for M-grams until (including) M = 6
                 * The type is computed as in a 32-based numeric system, e.g. for M==5:
                 *          (len_bits[0]-1)*32^0 + (len_bits[1]-1)*32^1 +
                 *          (len_bits[2]-1)*32^2 + (len_bits[3]-1)*32^3 +
                 *          (len_bits[4]-1)*32^4
                 * @param the number of word ids
                 * @param len_bits the bits needed per word id
                 * @param id_type [out] the resulting id type the initial value is expected to be 0
                 */
                template<TModelLevel M_GRAM_LEVEL>
                void get_gram_id_type(uint8_t len_bits[M_GRAM_LEVEL], TShortId & id_type) {
                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_gram_id_type: Unsupported m-gram level: "
                                << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], insufficient multipliers!";
                        throw Exception(msg.str());
                    }

                    //Compute the M-gram id type. Here we use the pre-computed multipliers
                    for (size_t idx = 0; idx < M_GRAM_LEVEL; ++idx) {
                        id_type += (len_bits[idx] - 1) * gram_id_type_mult[idx];
                    }
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * This implementation should work up to 6-grams! If we use it for
                 * 7-grams then the internal computations will overflow!
                 * @param ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                 * @param M_GRAM_LEVEL the number of tokens in the M-gram
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                template<uint8_t ID_TYPE_LEN_BITS, TModelLevel M_GRAM_LEVEL >
                uint8_t get_gram_id_len(const uint8_t * & m_gram_id) {
                    //Declare and initialize the id length, the initial values is
                    //what we need to store the type. Note that, the maximum number
                    //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                    //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                    //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                    //This is when we get an overflow here if we use uint8_t to
                    //store the length in bits will overflow. Therefore the sanity check.
                    uint8_t id_len_bits = ID_TYPE_LEN_BITS;

                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_gram_id_len: Unsupported m-gram level: "
                                << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    //1. ToDo: Extract the id_type from the M-gram
                    TShortId id_type;
                    copy_begin_bits_to_end<ID_TYPE_LEN_BITS>(m_gram_id, id_type);

                    //2. Compute the M-gram id length from the M-gram id type.
                    //   Here we use the pre-computed multipliers we add the
                    //   final bits at the end of the function.
                    uint8_t coeff;
                    for (size_t idx = (M_GRAM_LEVEL - 1); idx >= 0; --idx) {
                        //"coeff = len_bits[idx] - 1"
                        coeff = id_type / gram_id_type_mult[idx];
                        id_type -= coeff * gram_id_type_mult[idx];
                        id_len_bits += coeff;
                    }

                    //Note that in the loop above we have "coeff = len_bits[idx] - 1"
                    //Therefore, here we add the number of tokens to account for this -1's
                    return NUM_BITS_TO_STORE_BYTES(id_len_bits + M_GRAM_LEVEL);
                };

                /**
                 * Allows to extract the M-gram id length in bytes
                 * Note that this method is applicable only for M-grams with M <= 6;
                 * @param m_gram_id the M-gram id to extract the length for
                 * @param level the M-gram level
                 * @return the M-gram id length in bytes
                 */
                template<TModelLevel M_GRAM_LEVEL>
                uint8_t T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::get_m_gram_id_len(const uint8_t*& m_gram_id) {
                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_m_gram_id_len: Unsupported m-gram level: "
                                << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    if (m_gram_id != NULL) {
                        //Call the appropriate function, use array instead of switch should be faster.
                        return get_gram_id_len < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL - M_GRAM_LEVEL_2], M_GRAM_LEVEL > (m_gram_id);
                    } else {
                        throw Exception("get_m_gram_id_length: A NULL pointer M-gram id!");
                    }
                };

                /**
                 * This method is needed to compute the M-gram id.
                 * This implementation should work up to 6-grams! If we use it for
                 * 7-grams then the internal computations will overflow!
                 * @param ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                 * @param M_GRAM_LEVEL the number of tokens in the M-gram
                 * @param tokens the M-gram tokens
                 * @param p_word_idx the word index
                 * @param m_gram_id the resulting m-gram id, if NULL new memory
                 * will be allocated for the id, otherwise the pointer will be
                 * used to store the id. It is then assumed that there is enough
                 * memory allocated to store the id. For an M-gram it is (4*M)
                 * bytes that is needed to store the longed M-gram id.
                 * @return true if the m-gram id could be computed, otherwise false
                 */
                template<uint8_t ID_TYPE_LEN_BITS, TModelLevel M_GRAM_LEVEL>
                static inline bool create_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, uint8_t * & m_gram_id) {
                    //Declare and initialize the id length, the initial values is
                    //what we need to store the type. Note that, the maximum number
                    //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                    //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                    //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                    //This is when we get an overflow here if we use uint8_t to
                    //store the length in bits will overflow. Therefore the sanity check.
                    uint8_t id_len_bits = ID_TYPE_LEN_BITS;

                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "create_gram_id: Unsupported m-gram level: "
                                << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    //Obtain the word ids and their lengths in bits and
                    //the total length in bits needed to store the key
                    TShortId wordIds[M_GRAM_LEVEL];
                    uint8_t len_bits[M_GRAM_LEVEL];
                    for (size_t idx = 0; idx < M_GRAM_LEVEL; ++idx) {
                        //Get the word id if possible
                        if (p_word_idx->get_word_id(tokens[idx].str(), wordIds[idx])) {
                            //Get the number of bits needed to store this id
                            len_bits[idx] = get_number_of_bits(wordIds[idx]);
                            //Compute the total gram id length in bits
                            id_len_bits += len_bits[idx];
                        } else {
                            //The word id could not be found so there is no need to proceed
                            return false;
                        }
                    }
                    //Determine the size of id in bytes, divide with rounding up
                    const uint8_t id_len_bytes = NUM_BITS_TO_STORE_BYTES(id_len_bits);

                    //Allocate the id memory if there was nothing pre-allocated yet
                    if (m_gram_id == NULL) {
                        //Allocate memory
                        m_gram_id = new uint8_t[id_len_bytes];
                    } else {
                        //Clean the memory
                        memset(m_gram_id, 0, id_len_bytes);
                    }

                    //Determine the type id value from the bit lengths of the words
                    TShortId id_type_value = 0;
                    get_gram_id_type<M_GRAM_LEVEL>(len_bits, id_type_value);

                    //Append the id type to the M-gram id
                    uint8_t to_bit_pos = 0;
                    copy_end_bits_to_pos(id_type_value, ID_TYPE_LEN_BITS, m_gram_id, to_bit_pos);
                    to_bit_pos += ID_TYPE_LEN_BITS;

                    //Append the word id meaningful bits to the id in reverse order
                    for (size_t idx = (M_GRAM_LEVEL - 1); idx >= 0; --idx) {
                        copy_end_bits_to_pos(wordIds[idx], len_bits[idx], m_gram_id, to_bit_pos);
                        to_bit_pos += len_bits[idx];
                    }

                    return true;
                };

                /**
                 * This function allows to create an M-gram id for a given M-gram
                 * 
                 * Let us give an example of a 2-gram id for a given 2-gram:
                 * 
                 * 1) The 2 wordIds are to be converted to the 2-gram id:
                 * There are 32 bytes in one word id and 32 bytes in
                 * another word id, In total we have 32^2 possible 2-gram id
                 * lengths, if we only use meaningful bits of the word id for
                 * instance:
                 *       01-01 both really need just one bite
                 *       01-02 the first needs one and another two
                 *       02-01 the first needs two and another one
                 *       ...
                 *       32-32 both need 32 bits
                 * 
                 * 2) These 32^2 = 1,024 combinations uniquely identify the
                 * type of stored id. So this can be an uid of the gram id type.
                 * To store such a uid we need log2(1,024)= 10 bits.
                 * 
                 * 3) We create the 2-gram id as a byte array of 10 bits - type
                 * + the meaningful bits from wordId2 and wordId1. We start from
                 * the end (reverse the word order) as this can potentially
                 * increase speed of the comparison operation.
                 * 
                 * 4) The total length of the 2-gram id in bytes is the number
                 * of bits needed to store the key type and meaningful word id
                 * bits, rounded up to the full bytes.
                 * 
                 * @param gram the M-gram to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                template<TModelLevel M_GRAM_LEVEL>
                static inline bool create_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, uint8_t * & m_gram_id) {
                    if (DO_SANITY_CHECKS && ((gram.level < M_GRAM_LEVEL_2) || (gram.level > M_GRAM_LEVEL_5))) {
                        stringstream msg;
                        msg << "create_m_gram_id: Unsupported m-gram level: "
                                << SSTR(gram.level) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_5) << "]";
                        throw Exception(msg.str());
                    }

                    //Call the appropriate function, use array instead of switch, should be faster.
                    return create_gram_id < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL - M_GRAM_LEVEL_2], M_GRAM_LEVEL > (gram.tokens, p_word_idx, m_gram_id);

                };

                template<TModelLevel M_GRAM_LEVEL>
                T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::T_Compressed_M_Gram_Id(const T_M_Gram & gram, const AWordIndex * p_word_idx) {
                    if (!create_m_gram_id<M_GRAM_LEVEL>(gram, p_word_idx)) {
                        stringstream msg;
                        msg << "Could not create an " << SSTR(gram.level) << "-gram id for: "
                                << tokensToString<M_GRAM_LEVEL>(gram.tokens, gram.level);
                        throw Exception(msg.str());
                    }
                }
            }
        }
    }
}