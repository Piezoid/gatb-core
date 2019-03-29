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

/** \file IThread.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Interface for threads
 */

#ifndef _GATB_CORE_SYSTEM_ITHREAD_HPP_
#define _GATB_CORE_SYSTEM_ITHREAD_HPP_

#include <gatb/system/api/types.hpp>
#include <gatb/system/api/ISmartPointer.hpp>
#include <gatb/system/api/Exception.hpp>
#include <string>
#include <list>
#include <mutex>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace system    {
/********************************************************************************/

/** Forward declarations. */
class ISynchronizer;

/********************************************************************************/

/** \brief Define what a thread is.
 *
 * Definition of a thread in an OS independent fashion. See below how to create a thread through a factory.
 */
class IThread : public SharedObject<IThread>
{
public:

    /** We define a Id type. */
    typedef long long Id;

    /** Wait the end of the thread. */
    virtual Id getId () const = 0;

    /** Wait the end of the thread. */
    virtual void join () = 0;

    /** Destructor. */
    virtual ~IThread () {}
};

/********************************************************************************/

/** \brief Interface that defines a group of threads
 *
 * This interface allows to manage a group of threads; it may be
 * used to access a shared object by different threads.
 *
 * This class is not intended to be used by end users; instead, the
 * ThreadObject class can be used.
 */
class IThreadGroup : public SharedObject<IThreadGroup>
{
public:

    struct Info : public SharedObject<Info>
    {
        Info (IThreadGroup::sptr group, void* data, size_t idx) : group(group), data(data), idx(idx) {}
        IThreadGroup::sptr group;
        void*         data;
        size_t        idx;
    };

    /** Destructor */
    virtual ~IThreadGroup () {}

    /** Add a thread to the group. A thread is created and the provided
     * mainloop is launched as the main function of this thread.
     * \param[in] mainloop : mainloop of the added thread.
     * \param[in] data : data to be given to the main loop
     */
    virtual void add (void* (*mainloop) (void*), void* data) = 0;

    /** Start all the threads of the group at the same time. Implementations
     * should ensure this by some synchronization mechanism. */
    virtual void start () = 0;

    /** Get the synchronizer associated to this threads group.
     * \return the ISynchronizer instance. */
    virtual std::shared_ptr<ISynchronizer> getSynchro() = 0;

    /** Get the number of threads in the group.
     * \return the number of threads */
    virtual size_t size() const = 0;

    /** Get the ith thread of the group.
     * \param[in] idx : index of the thread in the group
     * \return the thread */
    virtual IThread::sptr operator[] (size_t idx) = 0;

    /** This method is used to gather exceptions occurring during the
     * execution of the threads of the group. By doing this, we can
     * launch an ExceptionComposite when all the threads of the group
     * are finished.
     * \param[in] e : exception thrown by one of the thread. */
    virtual void addException (system::Exception e) = 0;

    /** Tells whether or not exceptions have been added to the thread group.
     * \return true if some exception has been added, false otherwise */
    virtual bool hasExceptions() const = 0;

    /** Get an exception that holds all the information of exceptions that have
     * occurred during the execution of the threads of the group.
     * \return the gathering exception. */
    virtual Exception getException () const = 0;
};

/********************************************************************************/

/** \brief Define a synchronization abstraction
 *
 *  This is an abstraction layer of what we need for handling synchronization.
 *  Actual implementations may use mutex for instance.
 */
class ISynchronizer : public std::mutex, public SharedObject<ISynchronizer>
{
public:
    /** Destructor. */
    virtual ~ISynchronizer () {}
};

/********************************************************************************/

/** \brief Factory that creates IThread instances.
 *
 *  Thread creation needs merely the main loop function that will be called.
 *
 *  Note the method that can return the number of cores in case a multi-cores
 *  architecture is used. This is useful for automatically configure tools for
 *  using the maximum number of available cores for speeding up the algorithm.
 */
class IThreadFactory : public SharedObject<IThreadFactory>
{
public:

    /** Creates a new thread.
     * \param[in] mainloop : the function the thread shall execute
     * \param[in] data :  data provided to the mainloop when launched
     * \return the created thread.
     */
    virtual IThread::sptr newThread (void* (*mainloop) (void*), void* data) = 0;

    /** Creates a new synchronization object.
     * \return the created ISynchronizer instance
     */
    virtual ISynchronizer::sptr newSynchronizer (void) = 0;

    /** Return the id of the calling thread. */
    virtual IThread::Id getThreadSelf() = 0;

    /** Return the id of the current process. */
    virtual u_int64_t getProcess () = 0;

    /** Destructor. */
    virtual ~IThreadFactory ()  {}
};

/********************************************************************************/

/** \brief Tool for locally managing synchronization.
 *
 *  Instances of this class reference a ISynchronizer instance. When created, they lock
 *  their referred ISynchronizer and when destroyed, they unlock it.
 *
 *  For instance, it is a convenient way to lock/unlock a ISynchronizer instance in the scope
 *  of a statements block (where the LocalSynchronizer instance lives), a particular case being
 *  the statements block of a method.
 *
 *  Code sample:
 *  \code
 *  void sample (ISynchronizer::sptr synchronizer)
 *  {
 *      // we create a local synchronizer from the provided argument
 *      LocalSynchronizer localsynchro (synchronizer);
 *
 *      // now, all the statements block is locked for the provided synchronizer
 *      // in other word, this sample function is protected against concurrent accesses.
 *  }
 *  \endcode
 */
class LocalSynchronizer
{
public:

    /** Constructor.
     * \param[in] ref : the ISynchronizer instance to be controlled.
     */
    LocalSynchronizer (ISynchronizer::sptr ref) : _ref(std::move(ref))
    {  if (_ref) _ref->lock ();  }

    /** Destructor. */
    ~LocalSynchronizer ()  { if (_ref)  _ref->unlock (); }

private:

    /** The referred ISynchronizer instance. */
    ISynchronizer::sptr _ref;
};

/********************************************************************************/
} } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_SYSTEM_ITHREAD_HPP_ */
