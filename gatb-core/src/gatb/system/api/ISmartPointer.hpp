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

/** \file ISmartPointer.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Smart Pointer Design Pattern interface
 *
 *  Define tools for easing life cycle of objects. Also known as smart pointer.
 */

#ifndef _GATB_CORE_SYSTEM_SMART_POINTER_HPP_
#define _GATB_CORE_SYSTEM_SMART_POINTER_HPP_

#include <memory>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace system    {
/********************************************************************************/

/** \brief Tool for managing instances life cycle
 *
 *  The goal of this class is to share easily objects between clients.
 *
 *  This class has an integer attribute that acts as a reference counter, i.e. a
 *  token counting how many clients are interested by the instance.
 *
 *  This is useful for sharing instances; if a client is interested by using an
 *  instance 'obj', she/he may call 'obj->use()' which will increase the internal
 *  token number. When the client is no more interested by using the instance 'obj',
 *  she/he may call 'obj->forget()', which will decrease the internal token.
 *
 *  When the token becomes 0, the instance is automatically destroyed.
 *
 *  Note that use() and forget() are virtual; it may happen (for singleton management
 *  for instance) that some subclass needs to refine them.
 *
 *  This pattern is often known as Smart Pointer.
 *
 *  Note that the STL provides its own smart pointer mechanism, known as auto_ptr.
 *  Here, our approach relies on subclassing instead of template use. The interest of our
 *  approach is to ease methods prototypes writing; with STL approach, one needs to uses
 *  every time auto_ptr<T> instead of only T, which can lower the readability.
 *
 *  On the other hand, our approach may be a little more dangerous than the STL approach
 *  since one has to be sure to forget an instance when needed. In our case, we use Smart
 *  Pointers mainly for attributes in class, so only have to be careful in constructors
 *  and destructors. Moreover, one can use the SP_SETATTR macro which eases this process.
 *  Note also the LOCAL macro that eases the local usage of an instance.
 *
 *  \see SP_SETATTR
 *  \see LOCAL
 */

struct  SharedObjectVBase: public std::enable_shared_from_this<SharedObjectVBase>
{
    virtual ~SharedObjectVBase() {}
};

template<typename Derived>
struct SharedObject : public virtual SharedObjectVBase {
    using sptr = std::shared_ptr<Derived>;
    using csptr = std::shared_ptr<const Derived>;
    using uptr = std::unique_ptr<Derived>;
    using cuptr = std::unique_ptr<const Derived>;

    sptr share() {
        return std::reinterpret_pointer_cast<Derived>(this->shared_from_this());
    }

    csptr share() const {
        return std::reinterpret_pointer_cast<const Derived>(this->shared_from_this());
    }

    virtual ~SharedObject() {} //FIXME: not sure if it is needed as we don't have ressources to free locally
};

/********************************************************************************/

/** \brief Implementation of the ISmartPointer interface
 *
 * This class implements also a security against concurrent access by several clients acting
 * from different threads. This is achieved by using intrinsics __sync_fetch_and_add and
 * __sync_fetch_and_sub in use and forget respectively.
 *
 * This class can't be instantiated since its default constructor is protected.
 *
 *  \see SP_SETATTR
 *  \see LOCAL
 */

/// Alows to easily get a shared_pointer of the derived type
template<typename T>
std::shared_ptr<T> as_shared_ptr(T* ptr)
{
#ifdef NDEBUG
    return std::reinterpret_pointer_cast<T>(ptr->shared_from_this());
#else
    return std::dynamic_pointer_cast<T>(ptr->shared_from_this());
#endif
}

template<typename T>
std::shared_ptr<T> as_shared_ptr(std::shared_ptr<T> ptr)
{ return ptr; }


/********************************************************************************/
} } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_SYSTEM_SMART_POINTER_HPP_ */
