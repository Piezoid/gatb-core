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

#include <gatb/bank/impl/Bank.hpp>

#include <gatb/bank/impl/BankFasta.hpp>
#include <gatb/tools/compression/Leon.hpp>

#include <gatb/bank/impl/BankBinary.hpp>
#include <gatb/bank/impl/BankAlbum.hpp>

#include <gatb/system/api/Exception.hpp>

using namespace std;

#define DEBUG(a)  //printf a

/********************************************************************************/
namespace gatb {  namespace core {  namespace bank {  namespace impl {
/********************************************************************************/

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::Bank ()
{
    /** We register most known factories. */
    _registerFactory_ ("album",  std::make_unique<BankAlbumFactory >(),  false);
    _registerFactory_ ("fasta",  std::make_unique<BankFastaFactory >(),  false);
    _registerFactory_ ("leon",   std::make_unique<BankLeonFactory  >(), false);
    _registerFactory_ ("binary", std::make_unique<BankBinaryFactory>(), false);

    DEBUG (("Bank::Bank,  found %ld factories\n", _factories.size()));
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Bank::~Bank ()
{
    for (list<Entry>::iterator it = _factories.begin(); it != _factories.end(); it++)
    {
        (it->second)->forget ();
    }
    _factories.clear();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Bank::_registerFactory_ (const std::string& name, std::unique_ptr<IBankFactory>&& instance, bool beginning)
{
    /** We look whether the factory is already registered. */
    const IBankFactory* factory = _getFactory_ (name);

    DEBUG (("Bank::registerFactory : name='%s'  instance=%p  => factory=%p \n", name.c_str(), instance, factory));

    if (factory == 0)
    {
        if (beginning)  { _factories.emplace_front (name, std::move(instance));  }
        else            { _factories.emplace_back  (name, std::move(instance));  }
    }
    else
    {
        throw system::Exception ("Bank factory '%s already registered", name.c_str());
    }
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
bool Bank::_unregisterFactory_ (const std::string& name)
{
    for (list<Entry>::iterator it = _factories.begin(); it != _factories.end(); it++)
    {
        if (it->first == name)  {
            _factories.erase(it);
            return true;
        }
    }
    return false;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
const IBankFactory* Bank::_getFactory_ (const std::string& name)
{
    for (const auto& entry : _factories)
        if (entry.first == name)
            return entry.second.get();

    return 0;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
std::unique_ptr<IBank> Bank::_open_ (const std::string& uri)
{
    DEBUG (("Bank::open : %s  nbFactories=%ld \n", uri.c_str(), _factories.size()));

    std::unique_ptr<IBank> result;
    for (const auto& entry : _factories)
    {
        result = entry.second->createBank(uri);
        DEBUG (("   factory '%s' => result=%p \n", entry.first.c_str(), result ));

        if (result) // if one of the factories produce a result, we can just stop, no need to try other factories.
            break;
    }

    if (result == 0) { throw system::Exception ("Unable to open bank '%s' (if it is a list of files, perhaps some of the files inside don't exist)", uri.c_str()); }

    return result;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
std::string Bank::_getType_ (const std::string& uri)
{
    string result = "unknown";

    /** We try to create the bank; if a bank is valid, then we have the factory name. */
    for (list<Entry>::iterator it = _factories.begin(); it != _factories.end(); it++)
    {
        std::unique_ptr<IBank> bank = it->second->createBank(uri);
        if (bank != 0)
        {
            result = it->first;
			if(!result.compare("fasta"))
			{
				//distinguish fasta and fastq
				tools::dp::Iterator<Sequence>* its = bank->iterator(); LOCAL(its);
				its->first();
				if(!its->isDone())
				{
					std::string qual = its->item().getQuality();
					if(!qual.empty())
					{
						result= "fastq";
					}
				}
			}
            break;
        }
    }

    return result;
}

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

