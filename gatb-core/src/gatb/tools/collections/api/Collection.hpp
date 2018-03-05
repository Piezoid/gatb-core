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

/** \file Collection.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Collection interface
 *
 *  This file holds interfaces related to the Collection interface
 */

#ifndef _GATB_CORE_TOOLS_COLLECTIONS_API_COLLECTION_HPP_
#define _GATB_CORE_TOOLS_COLLECTIONS_API_COLLECTION_HPP_

/********************************************************************************/

#include <gatb/tools/collections/api/Iterable.hpp>
#include <gatb/tools/collections/api/Bag.hpp>
#include <gatb/tools/misc/impl/Stringify.hpp>
#include <gatb/system/impl/System.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

/********************************************************************************/
namespace gatb          {
namespace core          {
namespace tools         {
namespace collections   {
/********************************************************************************/

/** \brief Abstract implementation of the Collection interface.
 *
 * This class implements the Collection interface by delegating the job to an instance
 * of Bag and an instance of Iterable.
 *
 * All the methods are delegated to one of these two instances.
 */
template <class Item>
class Collection
{
public:

    /** Constructor.
     * \param bag      : reference on the bag delegate.
     * \param iterable : reference on the iterable delegate
     */
    Collection (const std::shared_ptr<Bag<Item>>& bag,
                        const std::shared_ptr<Iterable<Item>>& iterable)
        : _bag(bag), _iterable(iterable)
    {}

    /** Destructor. */
    virtual ~Collection() {}

    /** \copydoc Collection::bag */
    const std::shared_ptr<Bag<Item>>& bag() { return _bag; }

    /** \copydoc Collection::iterable */
    const std::shared_ptr<Iterable<Item>>& iterable()  { return _iterable; }

    /** \copydoc Iterable::iterator */
    dp::Iterator<Item>* iterator ()  { return _iterable->iterator(); }

    /** \copydoc Iterable::getNbItems */
    int64_t getNbItems ()  { return _iterable->getNbItems(); }

    /** \copydoc Iterable::estimateNbItems */
    int64_t estimateNbItems () { return _iterable->estimateNbItems(); }

    /** \copydoc Iterable::getItems */
    Item* getItems (Item*& buffer)  { 
        //std::cout << "CollectionAsbtract getItems called" << std::endl;
        return _iterable->getItems(buffer); }

    /** \copydoc Iterable::getItems(Item*& buffer, size_t start, size_t nb) */
    size_t getItems (Item*& buffer, size_t start, size_t nb)  { 
        //std::cout << "CollectionAsbtract getItems called" << std::endl;
        return _iterable->getItems (buffer, start, nb); }

    /** \copydoc Bag::insert */
    void insert (const Item& item)  { _bag->insert (item); }

    /** \copydoc Bag::insert(const std::vector<Item>& items, size_t length) */
    void insert (const std::vector<Item>& items, size_t length)  { _bag->insert (items, length); }

    /** \copydoc Bag::insert(const Item* items, size_t length) */
    void insert (const Item* items, size_t length)  { _bag->insert (items, length); }

    /** \copydoc Bag::flush */
    void flush ()  { _bag->flush(); }

    /** Add a property to the collection.
     * \param[in] key : key of the property
     * \param[in] format  : printf like arguments
     * \param[in] ... : variable arguments. */
    virtual void addProperty (const std::string& key, const std::string value) = 0;

    /** \copydoc Collection::addProperty */
    void addProperty (const std::string& key, const char* format ...)
    {
        va_list args;
        va_start (args, format);
        this->addProperty (key, misc::impl::Stringify::format(format, args));
        va_end (args);
    }

    /** Retrieve a property for a given key
     * \param[in] key : key of the property to be retrieved
     * \return the value of the property. */
    virtual std::string getProperty (const std::string& key) = 0;

     /** Remove physically the collection. */
    virtual void remove() {}

protected:
    void setBag(const std::shared_ptr<Bag<Item>>& bag) { _bag = bag; }
    void setIterable(const std::shared_ptr<Iterable<Item>>& iterable) { _iterable = iterable; }
private:
    std::shared_ptr<Bag<Item>> _bag;
    std::shared_ptr<Iterable<Item>> _iterable;
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_COLLECTIONS_API_COLLECTION_HPP_ */
