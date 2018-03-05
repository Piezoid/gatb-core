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

/** \file Property.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Interface for properties, ie. list of tag values with hierarchical feature
 */

#ifndef _GATB_CORE_TOOLS_MISC_IPROPERTY_HPP_
#define _GATB_CORE_TOOLS_MISC_IPROPERTY_HPP_

#include <gatb/system/api/ISmartPointer.hpp>
#include <stdlib.h>
#include <stdarg.h>
#include <list>
#include <set>
#include <iostream>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace tools     {
namespace misc      {
/********************************************************************************/



/********************************************************************************/

#define PROP_END  ((Property*)0)

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_MISC_IPROPERTY_HPP_ */
