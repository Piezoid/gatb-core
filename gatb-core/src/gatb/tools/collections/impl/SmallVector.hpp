/*****************************************************************************
 *   GATB : Genome Assembly Tool Box
 *   Copyright (C) 2014  INRIA
 *   Authors: R.Chikhi, G.Rizk, E.Drezen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

/** \file SmallVector.hpp
 *  \date 08/03/2017
 *  \author Mael Kerbiriou
 *  \brief Small bounded size vector
 *
 *  This file holds ithe SmallVector class definition (header-only)
 */

#ifndef _GATB_CORE_TOOLS_COLLECTIONS_IMPL_SMALL_VECTOR_HPP_
#define _GATB_CORE_TOOLS_COLLECTIONS_IMPL_SMALL_VECTOR_HPP_

/********************************************************************************/

#include <cstdlib>
#include <cassert>
#include <algorithm>

/********************************************************************************/
namespace gatb          {
namespace core          {
namespace tools         {
namespace collections   {
namespace impl          {

/** \brief A small bounded size vector with contiguous (no heap allocations)
 * storage.
 *
 * This allows to have a vector allocated entierly on the stack. It's usefull
 * for non-POD types because allocating NonPOD[max_size] is O(max_size).
 * This implementation only call the constructor on actually used slots.
 *
 * The interface is disigned to be minimal and non-virtual.
 *
 * When NDEBUG is not defined, bounds are checked with assertions.
 */
template<typename T, int _max_size>
class SmallVector
{
public:
    /* Types, constants */
    using Item = T;
    static constexpr size_t max_size = _max_size;

    /* Constructors, destructor, assignment */
    inline SmallVector () : _size(0) {}
    inline ~SmallVector() { clear(); } // NOTE: purposely avoiding virtual pointer.

    /* Concerning move/copy constructor we have three choices:
    *  * Let the default constructor copy the whole backend storage,
    *  * Copy only the used pod_Items,
    *  * Let the objects copy themselves by calling their constructor.
    *
    * The two first approachs never call the elements' constructor and basically
    * do a memcopy.
    * The latter approach is used here because its follow c++ semantics and it
    * could allow the elements' copy constructor to only initialize used space
    * (think LargeInt<1> in big Integer variant).
    */
    inline SmallVector(SmallVector&& other) : _size(0) { extend(std::move(other)); }
    inline SmallVector& operator=(SmallVector&& other) {
        clear();
        extend(std::move(other));
        return *(this);
    }

    // Better if we can avoid copies and assignments
    SmallVector(const SmallVector& other) = delete;
    SmallVector& operator=(const SmallVector& other) = delete;

    /* Accessors */
    inline size_t size() const { return _size; }

    inline Item& operator[] (size_t i) { return *at(i); }
    inline const Item& operator[] (size_t i) const { return *at(i); }

    template<typename Functor>
    void iterate (const Functor& f)
        { for (size_t i=0; i<_size; i++) f(*at(i)); }
    template<typename Functor>
    void iterate (const Functor& f) const
        { for (size_t i=0; i<_size; i++) f(*at(i)); }

    /* Mutators */
    /** \brief Remove all the elements
     * Call the elements' destructor and resize to zero
     */
    inline void clear() {
        for(size_t i=0; i < _size; i++) at(i)->~Item();
        _size = 0;
    }

    /** \brief Add an element
     * Call the item's constructor with the supplied arguments.
     * The item's (default) copy/move constructor shoud allow to place elements
     * already initialized.
     */
    template< class... Args >
    inline Item& emplace_back(Args&&... args) {
        Item* ptr = at(_size++);
        new(ptr) Item(std::forward<Args>(args)...);
        return *ptr;
    }

    /** \brief Steal the elements from another vector.
     */
    inline void extend(SmallVector&& other) {
        for(size_t i=0; i < other._size; i++)
            emplace_back(std::move(other[i]));
    }
protected:
    size_t _size;

    // This type mimics Item in terms of pointer arithmetic but act as a POD type
    typedef typename std::aligned_storage<sizeof(Item), alignof(Item)>::type pod_Item;
    pod_Item _items[max_size];

    // Do the pointer arithmetic and cast
    inline Item* at(size_t i) {
        assert(i < _size && i < max_size);
        return reinterpret_cast<Item*>(_items+i);
    }
    inline const Item* at(size_t i) const {
        assert(i < _size && i < max_size);
        return reinterpret_cast<const Item*>(_items+i);
    }
};

/********************************************************************************/
} } } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_COLLECTIONS_IMPL_SMALL_VECTOR_HPP_ */
