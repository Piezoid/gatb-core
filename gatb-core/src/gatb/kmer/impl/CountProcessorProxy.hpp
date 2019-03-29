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

#ifndef _COUNT_PROCESSOR_PROXY_HPP_
#define _COUNT_PROCESSOR_PROXY_HPP_

/********************************************************************************/

#include <gatb/kmer/impl/Model.hpp>
#include <gatb/kmer/api/ICountProcessor.hpp>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace kmer      {
namespace impl      {
/********************************************************************************/

/** */
template<size_t span=KMER_DEFAULT_SPAN>
class CountProcessorProxy : public ICountProcessor<span>
{
public:

    using CountProcessor = CountProcessor;
    using CountProcessor_ptr = typename CountProcessor::sptr;
    using Type = typename Kmer<span>::Type;

    /** Constructor. */
    CountProcessorProxy (CountProcessor* ref) : _ref(ref->share()) { }

    /** Destructor. */
    virtual ~CountProcessorProxy()  {  setRef(0); }

    /********************************************************************/
    /*   METHODS CALLED ON THE PROTOTYPE INSTANCE (in the main thread). */
    /********************************************************************/

    /** \copydoc CountProcessor::begin */
    void begin (const impl::Configuration& config)  { _ref->begin (config); }

    /** \copydoc CountProcessor::end */
    void end   ()  { _ref->end (); }

    /** \copydoc CountProcessor::beginPass */
    void beginPass (size_t passId)  { _ref->beginPass (passId); }

    /** \copydoc CountProcessor::endPass */
    void endPass   (size_t passId)  {  _ref->endPass (passId); }

    /** \copydoc CountProcessor::clone */
    CountProcessor* clone ()  { return _ref->clone(); }

    /** \copydoc CountProcessor::finishClones */
    void finishClones (std::vector<CountProcessor_ptr>& clones)  { _ref->finishClones (clones); }

    /********************************************************************/
    /*   METHODS CALLED ON ONE CLONED INSTANCE (in a separate thread).  */
    /********************************************************************/

    /** \copydoc CountProcessor::beginPart */
    void beginPart (size_t passId, size_t partId, size_t cacheSize, const char* name)  { _ref->beginPart (passId, partId, cacheSize, name); }

    /** \copydoc CountProcessor::endPart */
    void endPart   (size_t passId, size_t partId)  { _ref->endPart (passId, partId); }

    /** \copydoc CountProcessor::process */
    bool process (size_t partId, const Type& kmer, const CountVector& count, CountNumber sum=0)
    {  return _ref->process (partId, kmer, count, sum);  }

    /*****************************************************************/
    /*                          MISCELLANEOUS.                       */
    /*****************************************************************/

    /** \copydoc CountProcessor::getName */
    std::string getName() const {  return _ref->getName(); }

    /** \copydoc CountProcessor::setName */
    void setName (const std::string& name)  { _ref->setName (name); }

    /** \copydoc CountProcessor::getProperties */
    tools::misc::impl::Properties getProperties() const  { return _ref->getProperties(); }

    /** \copydoc CountProcessor::getInstances */
    std::vector<CountProcessor_ptr> getInstances () const  { return _ref->getInstances(); }

protected:

    typename CountProcessor_ptr _ref;
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _COUNT_PROCESSOR_PROXY_HPP_ */
