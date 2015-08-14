/* 
 * File:   FixedMemoryAllocator.hpp
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
 * Created on August 14, 2015, 4:43 PM
 */

#ifndef FIXEDMEMORYALLOCATOR_HPP
#define	FIXEDMEMORYALLOCATOR_HPP

#include <typeinfo>  //std::typeid

#include "FixedMemoryStorage.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;
using uva::smt::exceptions::Exception;

namespace uva {
    namespace smt {
        namespace tries {
            namespace alloc {

                /**
                 * This is the fixed memory allocator class for using in the tries.
                 * Here we pre-allocate some fixed size memory and then just give
                 * it out when needed. Since the Trie is build once and then is not
                 * changed, we do no do any memory deallocation here!
                 */
                template <typename T>
                class FixedMemoryAllocator {
                public:
                    typedef T value_type;
                    typedef FixedMemoryStorage::size_type size_type;
                    typedef std::ptrdiff_t difference_type;
                    typedef T* pointer;
                    typedef const T* const_pointer;
                    typedef T& reference;
                    typedef const T& const_reference;

                    template <typename U>
                    struct rebind {
                        typedef FixedMemoryAllocator<U> other;
                    };

                    /**
                     * The basic constructor.
                     * @param buffer the buffer: pre-allocated memory to store the data in
                     * @param buffer_size the size of this memory in bytes.
                     */
                    FixedMemoryAllocator(void* buffer, size_type buffer_size) throw () :
                    _manager(_storage),
                    _storage(buffer, buffer_size) {
                        LOG_DEBUG3 << this << ": Creating FixedMemoryAllocator with : " << SSTR(buffer_size) << " bytes for " << SSTR(buffer_size/sizeof(T)) << " " << typeid(T).name() << "elements" << END_LOG;
                    }

                    /**
                     * The basic copy constructor. 
                     */
                    FixedMemoryAllocator(const FixedMemoryAllocator& other) throw () :
                    _manager(other.getStorageRef()),
                    _storage(NULL, 0) {
                        LOG_DEBUG3 << this << ": Calling the FixedMemoryAllocator copy constructor." << END_LOG;
                    }

                    /**
                     * The basic re-bind constructor. It is used internally by the
                     * container in case it needs to allocate other sort data than
                     * the stored container elements. 
                     */
                    template <typename U>
                    FixedMemoryAllocator(const FixedMemoryAllocator<U>& other) throw () :
                    _manager(other.getStorageRef()),
                    _storage(NULL, 0) {
                        LOG_DEBUG3 << this << ": Calling the FixedMemoryAllocator re-bind constructor for " << typeid(T).name() << "." << END_LOG;
                    }

                    /**
                     * The standard destructor
                     */
                    virtual ~FixedMemoryAllocator() throw () {
                        //Nothing to be done!
                        LOG_DEBUG3 << this << ": " << __FUNCTION__ << END_LOG;
                    }

                    /**
                     * Computes the address of the given object
                     * @param obj the object to compute the pointer of
                     * @return the computed pointer
                     */
                    pointer address(reference obj) const {
                        LOG_DEBUG3 << this << ": computing the object address: " << &obj << END_LOG;
                        return &obj;
                    }

                    /**
                     * Computes the address of the given object
                     * @param obj the object to compute the pointer of
                     * @return the computed pointer
                     */
                    const_pointer address(const_reference obj) const {
                        LOG_DEBUG3 << this << ": computing the const object address: " << &obj << END_LOG;
                        return &obj;
                    }

                    /**
                     * Allocates memory for the given number of objects
                     * @param num the number of objects to allocate
                     * @param cp NOT USED
                     * @return the pointer to the first allocated object
                     */
                    pointer allocate(size_type num, const_pointer cp = 0) {
                        const size_type bytes = num * sizeof (T);

                        LOG_DEBUG3 << this << ": Allocating: " << SSTR(num)
                                << " of " << typeid(T).name() << " elements, of " << SSTR(bytes)
                                << " bytes. Available: " << SSTR(available()) << "/" << SSTR(max_size()) << END_LOG;
                        
                        pointer const ptr = static_cast<pointer> (_manager.allocate(bytes));
                        
                        LOG_DEBUG3 << this << ": The pointer to the first allocated object is: " << ptr << END_LOG;
                        return ptr;
                    }

                    /**
                     * This function is supposed to deallocate the memory.
                     * We do not do that as this is fixed memory allocator
                     * @param ptr the pointer to free memory from
                     * @param num the number of objects to deallocate
                     */
                    void deallocate(pointer ptr, size_type num) {
                        LOG_DEBUG3 << this << ": Requested to deallocate: " << SSTR(num)
                                << " objects starting from: " << ptr << ". Ignoring!" << END_LOG;
                    }

                    /**
                     * Returns the available number of free elements we can store
                     * @return the available number of free elements we can store
                     */
                    size_type available() const throw () {
                        return static_cast<size_type> (_manager.available() / sizeof(T) );
                    }

                    /**
                     * Returns the maximum number of elements we can store
                     * @return the maximum number of elements we can store
                     */
                    size_type max_size() const throw () {
                        return static_cast<size_type> (_manager.buffer_size() / sizeof (T));
                    }

                    /**
                     * Calling the constructor on the given pointer
                     * @param ptr the pointer to work with
                     * @param value the type value to work with
                     */
                    void construct(pointer ptr, const value_type& value) {
                        LOG_DEBUG3 << this << ": Calling constructor on: " << ptr << END_LOG;
                        ::new ((void*) ptr) value_type(value);
                    }

                    /**
                     * Calling the destructor on the given pointer
                     * @param ptr the pointer to work with
                     */
                    void destroy(pointer ptr) {
                        LOG_DEBUG3 << this << ": Calling destructor on: " << ptr << END_LOG;
                        ptr->~T();
                    }

                    // \brief gets the buffer_manager

                    FixedMemoryStorage& getStorageRef() const {
                        return _manager;
                    }

                protected:
                    // \brief reference to the buffer manager that is actively being used
                    FixedMemoryStorage& _manager;

                private:
                    // \brief the space for a buffer_manager object if this object isn't copy-constructed
                    FixedMemoryStorage _storage;

                    // \brief default ctor that doesn't do anything meaningful.  Don't use it.

                    FixedMemoryAllocator() throw () :
                    _manager(_storage), _storage(NULL, 0) {
                        throw Exception("The default constructor is not to be used!");
                    }

                };

                // these are a necessary evil cos some STL classes (e.g. std::basic_string uses them)

                template<typename T, typename U>
                bool operator==(const FixedMemoryAllocator<T>&, const FixedMemoryAllocator<U>&) {
                    return true;
                }

                template<typename T>
                bool operator==(const FixedMemoryAllocator<T>&, const FixedMemoryAllocator<T>&) {
                    return true;
                }

                template<typename T, typename U>
                bool operator!=(const FixedMemoryAllocator<T>&, const FixedMemoryAllocator<U>&) {
                    return false;
                }

                template<typename T>
                bool operator!=(const FixedMemoryAllocator<T>&, const FixedMemoryAllocator<T>&) {
                    return false;
                }
            }
        }
    }
}


#endif	/* FIXEDMEMORYALLOCATOR_HPP */

