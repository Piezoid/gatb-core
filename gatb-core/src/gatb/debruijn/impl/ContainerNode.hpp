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

/** \file ContainerNode.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Container implementation
 */

#ifndef _GATB_CORE_DEBRUIJN_IMPL_CONTAINER_NODE_HPP_
#define _GATB_CORE_DEBRUIJN_IMPL_CONTAINER_NODE_HPP_

/********************************************************************************/

#include <gatb/debruijn/api/IContainerNode.hpp>
#include <cstdarg>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace debruijn  {
namespace impl      {
/********************************************************************************/

/** \brief IContainerNode implementation with a Bloom filter and a cFP set
 *
 *  In the GATB terminology, this object contains the information relative to the nodes of the dBG.
 *  It is not a set of nodes, as we don't store nodes explicitly. 
 *  Only one operation is supported:
 *     contains()
 *
 *  In the ContainerNode implementation, this object is actually just the Bloom filter + the set of False positives.
 */
template <typename Item> class ContainerNode : public IContainerNode<Item>
{
public:

    /** Constructor
     * \param[in] bloom : the Bloom filter.
     * \param[in] falsePositives : the cFP container. */
    ContainerNode (
        tools::collections::ISet<Item>* bloom,
        tools::collections::ISet<Item>* falsePositives
	) : _bloom(bloom->share()), _falsePositives(falsePositives->share())
    {
		setBloom(bloom);
		setFalsePositives(falsePositives);
    }

	/** Destructor. */
    virtual ~ContainerNode () {}

    /** \copydoc IContainerNode::contains */
    bool contains (const Item& item)  {  return (_bloom->contains(item) && !_falsePositives->contains(item));  }

protected:
    typename tools::collections::ISet<Item>::sptr _bloom, _falsePositives;
};

/********************************************************************************/

/** \brief IContainerNode implementation with a Bloom filter
 *
 * This implementation has no critical False Positive set, so it implies that
 * Graph instances using it will have false positive nodes (old 'Titus' mode).
 */
template <typename Item> class ContainerNodeNoCFP : public ContainerNode<Item>
{
public:

    /** Constructor
     * \param[in] bloom : the Bloom filter. */
    ContainerNodeNoCFP (tools::collections::ISet<Item>* bloom) : ContainerNode<Item>(bloom, NULL) {}

    /** \copydoc IContainerNode::contains */
    bool contains (const Item& item)  {  return (this->_bloom)->contains(item);  }
};

/********************************************************************************/
/** \brief IContainerNode implementation with cascading Bloom filters
 *
 * This implementation uses cascading Bloom filters for coding the cFP set.
 */
template <typename Item> class ContainerNodeCascading : public IContainerNode<Item>
{
public:

    /** Constructor.
     * \param[in] bloom : the Bloom filter.
     * \param[in] bloom2 : first Bloom filter of the cascading Bloom filters
     * \param[in] bloom3 : second Bloom filter of the cascading Bloom filters
     * \param[in] bloom4 : third Bloom filter of the cascading Bloom filters
     * \param[in] falsePositives : false positives container
     */
    ContainerNodeCascading (
        tools::collections::ISet<Item>* bloom,
        tools::collections::ISet<Item>* bloom2,
        tools::collections::ISet<Item>* bloom3,
        tools::collections::ISet<Item>* bloom4,
        tools::collections::ISet<Item>* falsePositives
	) : _bloom(bloom->share()),
        _bloom2(bloom2->share()),
        _bloom3(bloom3->share()),
        _bloom4(bloom4->share()),
        _falsePositives(falsePositives->share()), _cfpArray(4)
    {
		setBloom          (bloom);
        setBloom2         (bloom2);
		setBloom3         (bloom3);
		setBloom4         (bloom4);
		setFalsePositives (falsePositives);

		_cfpArray[0] = bloom2;
        _cfpArray[1] = bloom3;
        _cfpArray[2] = bloom4;
        _cfpArray[3] = falsePositives;
    }

	/** Destructor */
	virtual ~ContainerNodeCascading () {}

    /** \copydoc IContainerNode::contains */
    bool contains (const Item& item)  {  return (_bloom->contains(item) && ! containsCFP(item));  }

private:

    typename tools::collections::ISet<Item>::sptr
        _bloom, _bloom1, _bloom2, _bloom3, _bloom4, _falsePositives;
    std::vector<tools::collections::ISet<Item>*> _cfpArray;

    /** */
    bool containsCFP (const Item& item)
    {
        if (_bloom2->contains(item))
        {
            if (!_bloom3->contains(item))
                return true;

            else if (_bloom4->contains(item) && !_falsePositives->contains(item))
                return true;
        }
        return false;
    }

};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_DEBRUIJN_IMPL_CONTAINER_NODE_HPP_ */
