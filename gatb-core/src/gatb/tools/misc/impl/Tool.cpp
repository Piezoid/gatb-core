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

#include <gatb/tools/misc/impl/Tool.hpp>
#include <gatb/system/impl/System.hpp>
#include <gatb/tools/misc/impl/Property.hpp>
#include <gatb/tools/misc/impl/Progress.hpp>
#include <gatb/tools/misc/impl/LibraryInfo.hpp>
#include <gatb/tools/designpattern/impl/Command.hpp>

#define DEBUG(a)  //printf a

using namespace std;
using namespace gatb::core::system;
using namespace gatb::core::system::impl;

using namespace gatb::core::tools::dp;
using namespace gatb::core::tools::dp::impl;

/********************************************************************************/
namespace gatb {  namespace core { namespace tools {  namespace misc {  namespace impl {
/********************************************************************************/

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Tool::Tool (const std::string& name) : userDisplayHelp(0), _helpTarget(0),userDisplayVersion(0), _versionTarget(0), _name(name),
    _parser(name),
    _dispatcher(0)
{ configureParser(_parser); }

void Tool::configureParser(IOptionsParser & parser) {
    parser.push_back (new OptionOneParam (STR_NB_CORES,    "number of cores",      false, "0"  ));
    parser.push_back (new OptionOneParam (STR_VERBOSE,     "verbosity level",      false, "1"  ));

    parser.push_back (new OptionNoParam (STR_VERSION, "version", false));
    parser.push_back (new OptionNoParam (STR_HELP, "help", false));
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Tool::~Tool ()
{
    setInput      (0);
    setOutput     (0);
    setInfo       (0);
    setParser     (0);
    setDispatcher (0);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Tool::displayVersion(std::ostream& os){
	LibraryInfo::displayVersion (os);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Properties& Tool::run (int argc, char* argv[])
{
    DEBUG (("Tool::run(argc,argv) => tool='%s'  \n", getName().c_str() ));
    try
    {
        /** We parse the user parameters. */
        Properties& props = _parser.parse (argc, argv);

        /** We run the tool. */
        return run (props);
    }
    catch (OptionFailure& e)
    {
        e.displayErrors (std::cout);
        return NULL;
    }
	catch (ExceptionHelp& h)
	{
		if(userDisplayHelp!=NULL)
		{
			this->userDisplayHelp(_helpTarget);
		}
		else
		{
			h.displayDefaultHelp (std::cout);
		}
		return NULL;
	}
	catch (ExceptionVersion& v)
	{
		if(userDisplayVersion!=NULL)
		{
			this->userDisplayVersion(_versionTarget);
		}
		return NULL;
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
Properties& Tool::run (Properties& input)
{
    /** We keep the input parameters. */
    setInput (input);

    if (getInput().get(STR_VERSION) != 0)
    {
    	displayVersion(cout);
        return _output;
    }

    /** We define one dispatcher. */
    if (_input->getInt(STR_NB_CORES) == 1)
    {
        _dispatcher = (std::make_unique<SerialDispatcher>());
    }
    else
    {
        _dispatcher = std::make_unique<Dispatcher>(_input->getInt(STR_NB_CORES).value());
    }

    /** We may have some pre processing. */
    preExecute ();

    /** We execute the actual job. */
    {
        //TIME_INFO (_timeInfo, _name);
        execute ();
    }

    /** We may have some post processing. */
    postExecute ();

    /** We return the output properties. */
    return _output;
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Tool::preExecute ()
{
    /** We add a potential config file to the input properties. */
    _input->add (1, new Properties (/*System::info().getHomeDirectory() + "/." + getName() */));

    /** We may have to add a default prefix for temporary files. */
//    if (_input->get(STR_PREFIX)==0)  { _input->add (1, STR_PREFIX, "tmp.");  }

    /** set nb cores to be actual number of free cores, if was 0. */
    if (_input->getInt(STR_NB_CORES)<=0)  { _input->setInt (STR_NB_CORES, System::info().getNbCores());  }

//    /** We add the input properties to the statistics result. */
//    _info->add (1, _input);
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
void Tool::postExecute ()
{
//    /** We add the time properties to the output result. */
//    _info->add (1, _timeInfo.getProperties ("time"));
//
//    /** We add the output properties to the output result. */
//    _info->add (1, "output");
//    _info->add (2, _output);

    /** We may have to dump execution information into a stats file. */
//    if (_input->get(STR_STATS_XML) != 0)
//    {
//        XmlDumpPropertiesVisitor visit (_info->getStr (STR_STATS_XML));
//        _info->accept (&visit);
//    }

    /** We may have to dump execution information to stdout. */
    if (_input->get(STR_VERBOSE) && _input->getInt(STR_VERBOSE) > 0)
    {
        RawDumpPropertiesVisitor visit;
        _info->accept (&visit);
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
dp::IteratorListener* Tool::createIteratorListener (size_t nbIterations, const char* message)
{
    switch (getInput().getInt(STR_VERBOSE))
    {
        case 0: default:    return new IteratorListener ();
        case 1:             return new ProgressTimer (nbIterations, message);
        case 2:             return new Progress      (nbIterations, message);
    }
}


/********************************************************************************/
} } } } } /* end of namespaces. */
/********************************************************************************/
