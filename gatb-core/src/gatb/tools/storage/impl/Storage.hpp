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

/** \file Storage.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Storage interface
 *
 *  This file holds interfaces related to the Collection, Group, Partition, Factory interfaces.
 *
 *  I believe this deals with both on-disk and in-memory structures.. (from discussions with Erwan)
 *  So if you're looking at this file to figure out whether a Collection/Group/Partition is stored on
 *  disk or on memory, look elsewhere!
 */

#ifndef _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_HPP_
#define _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_HPP_

/********************************************************************************/

#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <cstring>
#include <mutex>

#include <gatb/tools/storage/impl/Cell.hpp>

#include <gatb/tools/collections/api/Collection.hpp>
#include <gatb/tools/collections/impl/CollectionAbstract.hpp>
#include <gatb/tools/collections/impl/CollectionCache.hpp>

#include <gatb/tools/misc/api/IProperty.hpp>

#include <gatb/tools/math/NativeInt8.hpp>

#include <gatb/tools/designpattern/impl/IteratorHelpers.hpp>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace tools     {
/** File system storage for collections. */
namespace storage   {
namespace impl      {
/********************************************************************************/

/** Enumeration of the supported storage mechanisms.*/
enum StorageMode_e
{
    /** Simple file. */
    STORAGE_FILE,
    /** HDF5 file. */
    STORAGE_HDF5,
    /** Experimental. */
    STORAGE_GZFILE,
    /** Experimental. */
    STORAGE_COMPRESSED_FILE
};

/********************************************************************************/

/** Forward declarations. */
class Group;
template <typename Type>  class Partition;
template <typename Type>  class CollectionNode;

class StorageFactory;

/********************************************************************************
     #####   #######  #        #        #######   #####   #######
    #     #  #     #  #        #        #        #     #     #
    #        #     #  #        #        #        #           #
    #        #     #  #        #        #####    #           #
    #        #     #  #        #        #        #           #
    #     #  #     #  #        #        #        #     #     #      ##
     #####   #######  #######  #######  #######   #####      #      ##
********************************************************************************/
/** \brief Class that add some features to the Collection interface
 *
 * The CollectionNode has two aspects:
 *      - this is a Collection
 *      - this is a Node
 *
 * The idea was not to have a Collection interface extending the INode interface
 * => we introduce the CollectionNode for this.
 */
template <class Item>
class CollectionNode : public Cell, public collections::impl::CollectionAbstract<Item>
{
public:

    /** Constructor.
     *  The idea is to use a referred collection for:
     *      - its bag part
     *      - its iterable part
     *      - its remove part
     * \param[in] factory : factory to be used
     * \param[in] parent : parent node
     * \param[in] id  : identifier of the collection to be created
     * \param[in] ref : referred collection.
     */
    CollectionNode (StorageFactory& factory, ICell::sptr parent, const std::string& id, collections::ICollection<Item>* ref):
        Cell(parent,id), collections::impl::CollectionAbstract<Item> (ref->bag(), ref->iterable()), _factory(factory), _ref(ref->share())
    {}

    /** Destructor. */
    virtual ~CollectionNode() {}

    /** \copydoc tools::storage::ICell::remove */
    void remove () { _ref->remove(); }

    /** \copydoc collections::impl::CollectionAbstract::addProperty */
    void addProperty (const std::string& key, const std::string value)
    { _ref->addProperty (key, value); }

    /** \copydoc collections::impl::CollectionAbstract::getProperty */
    std::string getProperty (const std::string& key)
    { return _ref->getProperty (key); }

    /** Get a pointer to the delegate Collection instance
     * \return the delegate instance.
     */
    collections::ICollection<Item>& getRef () { return *_ref; }

private:
    StorageFactory& _factory;
    typename std::shared_ptr<collections::ICollection<Item>> _ref;
};




/**********************************************************************
             #####   ######   #######  #     #  ######
            #     #  #     #  #     #  #     #  #     #
            #        #     #  #     #  #     #  #     #
            #  ####  ######   #     #  #     #  ######
            #     #  #   #    #     #  #     #  #
            #     #  #    #   #     #  #     #  #
             #####   #     #  #######   #####   #
**********************************************************************/

/** \brief Group concept.
 *
 * This class define a container concept for other items that can be stored
 * in a storage (like groups, partitions, collections).
 *
 * In a regular implementation, a Group could be a directory.
 *
 * In a HDF5 implementation, a group could be a HDF5 group.
 */
class Group : public Cell, public system::SharedObject<Group>
{
    using SO = system::SharedObject<Group>;
public:
    using SO::sptr;
    using SO::csptr;
    using SO::uptr;
    using SO::share;

    /** Constructor. */
    Group (StorageFactory& factory, ICell::sptr parent, const std::string& name)
        : Cell(std::move(parent), name), _factory(factory)
    {}

    /** Destructor. */
    ~Group();

    /** Get a child group from its name. Created if not already exists.
     * \param[in] name : name of the child group to be retrieved.
     * \return the child group.
     */
    Group& getGroup (const std::string& name)
    {
        Group::sptr group=0;
        for (auto& grp_ptr : _groups)
            if (grp_ptr->getId() == name)
                group = grp_ptr;

        if (group == 0)
        {
            group = _factory.createGroup (this->share(), name);
            _groups.push_back(group);
        }
        return *group;
    }

    /** Get a child partition from its name. Created if not already exists.
     * \param[in] name : name of the child partition to be retrieved.
     * \param[in] nb : in case of creation, tells how many collection belong to the partition.
     * \return the child partition.
     */
    template <class Type>  Partition<Type>& getPartition (const std::string& name, size_t nb=0)
    {
        Partition<Type>* result = _factory.createPartition<Type> (this, name, nb);
        _partitions.push_back(result);
        return *result;
    }

    /** Get a child collection from its name. Created if not already exists.
     * \param[in] name : name of the child collection to be retrieved.
     * \return the child collection .
     */
    template <class Type>  CollectionNode<Type>& getCollection (const std::string& name)
    {
        CollectionNode<Type>* result = _factory->createCollection<Type> (this, name, 0);
        _collections.push_back (result);
        return *result;
    }

    /** \copydoc Cell::remove */
    void remove ()
    {
        /** We remove the collections. */
        for (size_t i=0; i<_collections.size(); i++)  {  _collections[i]->remove ();  }

        /** We remove the partitions. */
        for (size_t i=0; i<_partitions.size(); i++)   { _partitions[i]->remove(); }

        /** We remove the children groups. */
        for (size_t i=0; i<_groups.size(); i++)       { _groups[i]->remove(); }
    }

    /** Associate a [key,value] to the group. Note: according to the kind of storage,
     * this feature may be not supported (looks like it's only supported in HDF5).
     * \param[in] key : key
     * \param[in] value : value
     */
    virtual void addProperty (const std::string& key, const std::string value) { throw system::ExceptionNotImplemented (); }

    /** Get a [key,value] from the group. Note: according to the kind of storage,
     * this feature may be not supported (looks like it's only supported in HDF5).
     * \param[in] key : key
     * \return the value associated to the string.
     */
    virtual std::string getProperty (const std::string& key)  { return "?"; throw system::ExceptionNotImplemented ();  }
    
    /* same as addProperty but sets the value if it already exists */
    virtual void setProperty (const std::string& key, const std::string value) { throw system::ExceptionNotImplemented (); }

protected:

    StorageFactory& _factory;
    std::vector<ICell::sptr> _collections;
    std::vector<ICell::sptr> _partitions;
    std::vector<Group::sptr> _groups;
};




	
	////////////////////////////////////////////////////////////
	////////////////// superkmer storage ///////////////////////
	////////////////////////////////////////////////////////////
	
//this class manages the set of temporary files needed to store superkmers
//to be used in conjunction with the CacheSuperKmerBinFiles below for buffered IO

	
// Note (guillaume) :  not very GATB-friendly  since it completely ignores GATB bag, bagcache, collection, partition, iterableFile,   etc ..
// but I do not know how to use gatb classes with a variable size type (the superkmer)
// and anyway the gatb storage complex hierarchy is error-prone (personal opinion)
// so, hell, just recreate an adhoc buffered storage here for superkmers
// it does need to be templated for kmer size, superkmers are inserted as u_int8_t*

	
	
//block header = 4B = block size
//puis block = liste de couple  < superk length = 1B  , superkmer = nB  >
//the  block structure makes it easier for buffered read,
//otherwise we would not know how to read a big chunk without stopping in the middle of superkmer

class SuperKmerBinFiles
{
	
public:
	
	//construtor will open the files for writing
	//use closeFiles to close them all then openFiles to open in different mode
	SuperKmerBinFiles(const std::string& path,const std::string& name, size_t nb_files)
    {
        _nbKmerperFile.resize(_nb_files,0);
        _FileSize.resize(_nb_files,0);

        openFiles("wb"); //at construction will open file for writing
        // then use close() and openFiles() to open for reading

    }
	
	~SuperKmerBinFiles()
    {
        this->closeFiles();
        this->eraseFiles();
    }

	void closeFiles()
    {
        for(size_t ii=0;ii<_files.size();ii++)
        {
            if(_files[ii]!=0)
            {
                delete _files[ii];
                _files[ii] = 0;
                _synchros[ii]->forget();
            }
        }
    }

	void flushFiles()
    {
        for(size_t ii=0;ii<_files.size();ii++)
        {
            _synchros[ii]->lock();

            if(_files[ii]!=0)
            {
                _files[ii]->flush();
            }

            _synchros[ii]->unlock();

        }
    }
	void eraseFiles()
    {
        for(size_t ii=0;ii<_files.size();ii++)
        {
            std::stringstream ss;
            ss << _path << "/" <<_basefilename << "." << ii;
            system::impl::System::file().remove(ss.str());
        }
        system::impl::System::file().rmdir(_path);

    }

	void openFiles(const char* mode)
    {
        _files.resize(_nb_files);
        _synchros.resize(_nb_files);

        system::impl::System::file().mkdir(_path, 0755);

        for(size_t ii=0;ii<_files.size();ii++)
        {
            std::stringstream ss;
            ss << _basefilename << "." << ii;

            _files[ii] = system::impl::System::file().newFile (_path, ss.str(), mode);
            _synchros[ii] = system::impl::System::thread().newSynchronizer();
        }
    }
	void openFile( const char* mode, int fileId)
    {
        std::stringstream ss;
        ss << _basefilename << "." << fileId;

        _files[fileId] = system::impl::System::file().newFile (_path, ss.str(), mode);
        _synchros[fileId] = system::impl::System::thread().newSynchronizer();
    }

	void closeFile(  int fileId)
    {
        if(_files[fileId]!=0)
        {
            _files[fileId] = 0;
            _synchros[fileId]->forget();
        }
    }

	//read/write block of superkmers to filefile_id
	//readBlock will re-allocate the block buffer if needed (current size passed by max_block_size)
	int readBlock(unsigned char ** block, size_t* max_block_size, size_t* nb_bytes_read, int file_id)
    {
        _synchros[file_id]->lock();

        //block header
        int nbr = _files[file_id]->fread(nb_bytes_read, sizeof(*max_block_size),1);

        if(nbr == 0)
        {
            //printf("__ end of file %i __\n",file_id);
            _synchros[file_id]->unlock();
            return 0;
        }

        if(*nb_bytes_read > *max_block_size)
        {
            *block = (unsigned char *) realloc(*block, *nb_bytes_read);
            *max_block_size = *nb_bytes_read;
        }

        //block
        _files[file_id]->fread(*block, sizeof(unsigned char),*nb_bytes_read);

        _synchros[file_id]->unlock();

        return *nb_bytes_read;
    }

	void writeBlock(unsigned char * block, size_t block_size, int file_id, int nbkmers)
    {

        _synchros[file_id]->lock();

        _nbKmerperFile[file_id]+=nbkmers;
        _FileSize[file_id] += block_size+sizeof(block_size);
        //block header
        _files[file_id]->fwrite(&block_size, sizeof(block_size),1);

        //block
        _files[file_id]->fwrite(block, sizeof(unsigned char),block_size);

        _synchros[file_id]->unlock();

    }

	int nbFiles() { return _files.size(); }
	int getNbItems(int fileId) const { return _nbKmerperFile[fileId]; }
	
	void getFilesStats(u_int64_t & total, u_int64_t & biggest, u_int64_t & smallest, float & mean)
    {
        total =0;
        smallest = ~0;
        biggest  = 0;
        mean=0;
        for(size_t ii=0;ii<_FileSize.size();ii++)
        {
            smallest = std::min (smallest, _FileSize[ii]);
            biggest  = std::max (biggest,  _FileSize[ii]);
            total+=_FileSize[ii];
        }
        if(_FileSize.size()!=0)
            mean= total/_FileSize.size();

    }

	u_int64_t getFileSize(int fileId) const { return _FileSize[fileId]; }

	
	std::string getFileName(int fileId)
    {

        std::stringstream ss;
        ss << _path << "/" <<_basefilename << "." << fileId;

        return ss.str();
    }

private:

	std::string _basefilename;
	std::string _path;
	
	std::vector<int> _nbKmerperFile;
	std::vector<u_int64_t> _FileSize;

	std::vector<std::unique_ptr<system::IFile>> _files;
	std::vector <system::ISynchronizer::sptr> _synchros;
	int _nb_files;
};



//encapsulate SuperKmerBinFiles I/O with a buffer
class CacheSuperKmerBinFiles
{
	public:
	CacheSuperKmerBinFiles(SuperKmerBinFiles * ref, int buffsize)
    {
        _ref = ref;

        _nb_files = _ref->nbFiles();
        _nbKmerperFile.resize(_nb_files,0);

        _buffer_max_capacity = buffsize; // this is per file, per thread
        //printf("buffsize %i per file per thread \n",_buffer_max_capacity);

        _max_superksize= 255; // this is extra size from regular kmer; ie total max superksize is kmersize +  _max_superksize

        _buffers.resize(_nb_files);
        _buffers_idx.resize(_nb_files,0);

        for(size_t ii=0; ii<_buffers.size();ii++ )
        {
            _buffers[ii] = (u_int8_t*) MALLOC (sizeof(u_int8_t) * _buffer_max_capacity);
        }

    }
	
	CacheSuperKmerBinFiles (const CacheSuperKmerBinFiles& p)
    {
        _ref = p._ref;
        _nb_files= p._nb_files;
        _buffer_max_capacity= p._buffer_max_capacity;
        _max_superksize= p._max_superksize;
        _nbKmerperFile.resize(_nb_files,0);

        _buffers.resize(_nb_files);
        _buffers_idx.resize(_nb_files,0);

        for(size_t ii=0; ii<_buffers.size();ii++ )
        {
            _buffers[ii] = (u_int8_t*) MALLOC (sizeof(u_int8_t) * _buffer_max_capacity);
        }
    }

	void insertSuperkmer(u_int8_t* superk, int nb_bytes, u_int8_t nbk, int file_id)
    {
        if( (_buffers_idx[file_id]+nb_bytes+1) > _buffer_max_capacity)
        {
            flush(file_id);
        }

        _buffers[file_id][_buffers_idx[file_id]++] = nbk;

        memcpy(_buffers[file_id] + _buffers_idx[file_id]  , superk,nb_bytes);
        _buffers_idx[file_id] += nb_bytes;
        _nbKmerperFile[file_id]+=nbk;

    }

	void flushAll()
    {
        //printf("flush all buffers\n");
        for(size_t ii=0; ii<_buffers.size();ii++ )
        {
            flush(ii);
        }
    }
	void flush(int file_id)
    {
        if(_buffers_idx[file_id]!=0)
        {
            _ref->writeBlock(_buffers[file_id],_buffers_idx[file_id],file_id,_nbKmerperFile[file_id]);

            _buffers_idx[file_id]=0;
            _nbKmerperFile[file_id] = 0;
        }
    }

	~CacheSuperKmerBinFiles()
    {
        this->flushAll();
        for(size_t ii=0; ii<_buffers.size();ii++ )
        {
            FREE (_buffers[ii]);
        }
    }

private:
	SuperKmerBinFiles * _ref;
	int _max_superksize;
	int _buffer_max_capacity;
	int _nb_files;
	
	std::vector< u_int8_t* > _buffers;
	std::vector<int> _buffers_idx;
	std::vector<int> _nbKmerperFile;

};


/**********************************************************************
######      #     ######   #######  ###  #######  ###  #######  #     #
#     #    # #    #     #     #      #      #      #   #     #  ##    #
#     #   #   #   #     #     #      #      #      #   #     #  # #   #
######   #     #  ######      #      #      #      #   #     #  #  #  #
#        #######  #   #       #      #      #      #   #     #  #   # #
#        #     #  #    #      #      #      #      #   #     #  #    ##
#        #     #  #     #     #     ###     #     ###  #######  #     #
**********************************************************************/

/** \brief Define a set of Collection instances having the same type.
 *
 * The Partition class groups several Collections that share the same kind
 * of items.
 *
 * It is defined as a subclass of Group.
 */
template<typename Type>
class Partition :
        public Group,
        public tools::collections::Iterable<Type>,
        public system::SharedObject< Partition<Type> >
{
    using SO = system::SharedObject< Partition<Type> >;
public:
    using SO::sptr;
    using SO::csptr;
    using SO::uptr;
    using SO::cuptr;
    using SO::share;

    /** Constructor.
     * \param[in] factory : factory to be used
     * \param[in] parent : the parent node
     * \param[in] id : the identifier of the instance to be created
     * \param[in] nbCollections : number of collections for this partition
     */
    Partition (StorageFactory& factory, ICell::sptr parent, const std::string& id, size_t nbCollections)
        : Group (factory, parent, id), _factory(factory), _typedCollections(nbCollections), _synchro(0)
    {
        /** We create a synchronizer to be shared by the collections. */
        _synchro = system::impl::System::thread().newSynchronizer();

        /** We want to instantiate the wanted number of collections. */
        for (size_t i=0; i<_typedCollections.size(); i++)
        {
            /** We define the name of the current partition as a mere number. */
            std::stringstream ss;  ss << i;

            CollectionNode<Type>* result = _factory->createCollection<Type> (this, ss.str(), _synchro);

            /** We add the collection node to the dedicated vector and take a token for it. */
            (_typedCollections [i] = result)->use ();
        }
    }

    /** Destructor. */
    virtual ~Partition () {}

    /** Return the number of collections for this partition.
     * \return the number of collections. */
    size_t size()  const { return _typedCollections.size(); }

    /** Get the ith collection
     * \param[in] idx : index of the collection to be retrieved
     * \return the wanted collection.
     */
    collections::ICollection<Type>& operator[] (size_t idx)
    { return  * _typedCollections[idx].getRef(); }

    /** \copydoc tools::collections::Iterable::iterator */
    dp::Iterator<Type>* iterator ()
    {
        std::vector <std::shared_ptr<dp::Iterator<Type>>> iterators;
        for (size_t i=0; i<this->size(); i++) { iterators.emplace_back((*this)[i].iterator()); }
        return new dp::impl::CompositeIterator<Type> (std::move(iterators));
    }

    /** \copydoc tools::collections::Iterable::getNbItems */
    size_t getNbItems ()
    {
        size_t result = 0;
        for(auto& node : _typedCollections) node.getRef()->getNbItems();
        return result;
    }

    /** \copydoc tools::collections::Iterable::estimateNbItems */
    size_t estimateNbItems ()
    {
        size_t result = 0;
        for(auto& node : _typedCollections) node.getRef()->estimateNbItems();
        return result;
    }

    /** Return the sum of the items size.
     * \return the total size. */
    size_t getSizeItems ()
    {
        size_t result = 0;
        for(auto& node : _typedCollections) node.getRef()->getNbItems(); * sizeof(Type);
        return result;
    }

    /** Flush the whole partition (ie flush each collection). */
    void flush ()
    { for(auto& node : _typedCollections) node.flush(); }

    /** Remove physically the partition (ie. remove each collection). */
    void remove () {
        /** We remove each collection of this partition. */
        for(auto& node : _typedCollections) node.remove();

        /** We call the remove of the parent class. */
        Group::remove ();
    }

protected:
    StorageFactory& _factory;
    std::vector <CollectionNode<Type>> _typedCollections;
    system::ISynchronizer::sptr _synchro;
};





/********************************************************************************/

/** \brief Cache (with potential synchronization) of a Partition.
 *
 * This class implements a cache for a Partition instance:
 *  -> an 'insert' is first cached in memory; when the cache is full, all the items are inserted in the
 *  real partition
 *  -> flushing the cache in the real partition is protected with a synchronizer
 *
 *  A typical use case is to create several PartitionCache referring the same Partition, and use them
 *  independently in different threads => in each thread, we will work on the local (cached) partition
 *  like a normal partition (ie. use 'insert'), but without taking care to the concurrent accesses to
 *  the referred partition (it is checked by the PartitionCache class).
 */
template<typename Type>
class PartitionCache
{
public:

    /** Constructor */
    PartitionCache (Partition<Type>& ref, size_t nbItemsCache)
        :  _ref(ref), _nbItemsCache(nbItemsCache), _cachedCollections()
    {

        _cachedCollections.reserve(ref.size());
        /** We create the partition files. */
        for (size_t i=0; i<ref.size(); i++)
            _cachedCollections.emplace_back(ref[i], nbItemsCache);
    }

    /** Copy constructor. */
    PartitionCache (const PartitionCache<Type>& p)
        : _ref(p._ref), _nbItemsCache(p._nbItemsCache), _cachedCollections()
    {
        _cachedCollections.reserve(p.size());
        /** We create the partition files. */
        for(auto& ccache : p._cachedCollections) {
            _cachedCollections.emplace_back(p.getRef(), p._nbItemsCache);
        }
    }

    /** Destructor. */
    virtual ~PartitionCache () { flush(); }

    /** Return the number of collections for this partition.
     * \return the number of collections. */
    size_t size() const { return _cachedCollections.size(); }

    /** Get the ith collection
     * \param[in] idx : index of the collection to be retrieved
     * \return the wanted collection.
     */
    collections::impl::CollectionCache<Type>& operator[] (size_t idx)
    { return _cachedCollections[idx]; }

    /** Flush the whole partition (ie flush each collection). */
    void flush ()
    { for(auto& ccache : _cachedCollections) ccache.flush(); }

    /** Remove physically the partition (ie. remove each collection). */
    void remove ()
    { for(auto& ccache : _cachedCollections) ccache.remove(); }

protected:
    Partition<Type>& _ref;
    size_t                     _nbItemsCache;
    std::vector <collections::impl::CollectionCache<Type>> _cachedCollections;
};




/********************************************************************************/
template<typename Type>
class PartitionCacheSorted
{
public:
    
    /** Constructor */
    PartitionCacheSorted (Partition<Type>& ref, size_t nbItemsCache, u_int32_t max_memory, system::ISynchronizer::sptr synchro=0)
        :  _ref(ref), _nbItemsCache(nbItemsCache), _synchro(synchro), _cachedCollections(ref.size()) , _synchros(ref.size()) , _outsynchros(ref.size()), _sharedBuffers(ref.size()), _idxShared(ref.size()), _max_memory(max_memory),_numthread(-1),_nbliving(0)
    {

        //todo should also take into account the temp buffer used for sorting, ie max nb threads * buffer, so should be
       // _sharedBuffersSize = (_max_memory*system::MBYTE / (_cachedCollections.size()  + nbthreads) ) / sizeof(Type); //in nb elems

        _sharedBuffersSize = (_max_memory*system::MBYTE / _cachedCollections.size()  ) / sizeof(Type); //in nb elems
        _sharedBuffersSize = std::max(2*nbItemsCache,_sharedBuffersSize);
       // _sharedBuffersSize = 1*system::MBYTE  / sizeof(Type);

        printf("sort cache of %zu elems \n",_sharedBuffersSize);
        /** We create the partition files. */
        for (size_t i=0; i<_cachedCollections.size(); i++)
        {

            if(synchro==0)
            {
                _synchros[i] = system::impl::System::thread().newSynchronizer();
            }
            else
            {
                _synchros[i] = synchro;
            }

            _outsynchros[i] = system::impl::System::thread().newSynchronizer();

            _synchros[i]->use();
            _outsynchros[i]->use();

           // printf("Creating outsync %p   \n",_outsynchros[i] );

            _sharedBuffers[i] = (Type*) system::impl::System::memory().calloc (_sharedBuffersSize, sizeof(Type));
            //system::impl::System::memory().memset (_sharedBuffers[i], 0, _sharedBuffersSize*sizeof(Type));
           // printf("Creating shared buffer %p   \n",_sharedBuffers[i] );



            _cachedCollections[i] = new collections::impl::CollectionCacheSorted<Type> (ref[i], nbItemsCache,_sharedBuffersSize, _synchros[i],_outsynchros[i],_sharedBuffers[i],&_idxShared[i]);
            //
            _cachedCollections[i]->use ();
        }
    }
    
    /** Copy constructor. */
    PartitionCacheSorted (const PartitionCacheSorted<Type>& p)
        : _ref(p._ref), _nbItemsCache(p._nbItemsCache), _synchro(p._synchro), _cachedCollections(p.size()) , _synchros(p._synchros), _outsynchros(p._outsynchros), _sharedBuffers(0),_idxShared(0)
    {

       //_numthread =  __sync_fetch_and_add (&p._nbliving, 1);

        /** We create the partition files. */
        for (size_t i=0; i<_cachedCollections.size(); i++)
        {

            _synchros[i]->use();
            _outsynchros[i]->use();

            PartitionCacheSorted<Type>& pp = (PartitionCacheSorted<Type>&)p;

            _cachedCollections[i] = new collections::impl::CollectionCacheSorted<Type> (pp[i].getRef(), p._nbItemsCache,p._sharedBuffersSize, p._synchros[i],p._outsynchros[i],p._sharedBuffers[i],&(pp._idxShared[i]) ); //
            _cachedCollections[i]->use ();
        }
    }
    
    /** Destructor. */
    virtual ~PartitionCacheSorted ()
    {
        //printf("destruc parti cache sorted tid %i \n",_numthread);
        flush ();

        for (size_t i=0; i<_cachedCollections.size(); i++)  {
            //destruction of collection will also flush the output bag (which is shared between all threads) , so need to lock it also
            _outsynchros[i]->lock();
            _cachedCollections[i]->forget ();
            _outsynchros[i]->unlock();


            _synchros[i]->forget();
            _outsynchros[i]->forget();
        }

        for (typename std::vector<Type*>::iterator it = _sharedBuffers.begin() ; it != _sharedBuffers.end(); ++it)
        {
            //printf("destroying shared buffer %p \n",*it);
            system::impl::System::memory().free (*it);
        }

    }
    
    /** Return the number of collections for this partition.
     * \return the number of collections. */
    size_t size() { return _cachedCollections.size(); }
    
    /** Get the ith collection
     * \param[in] idx : index of the collection to be retrieved
     * \return the wanted collection.
     */
    collections::impl::CollectionCacheSorted<Type>& operator[] (size_t idx)
    { return * _cachedCollections[idx]; }
    
    /** Flush the whole partition (ie flush each collection). */
    void flush ()
    { for(auto& ccache : _cachedCollections) ccache.flush(); }

    /** Remove physically the partition (ie. remove each collection). */
    void remove ()
    { for(auto& ccache : _cachedCollections) ccache.remove(); }
    
protected:
    Partition<Type>& _ref;
    size_t                     _nbItemsCache;
    system::ISynchronizer::sptr     _synchro;
    size_t _sharedBuffersSize;
    u_int32_t _max_memory;
    //only the model "parent" will have them, objects created with copy construc wont
    std::vector <Type*> _sharedBuffers;
    std::vector <size_t> _idxShared;
    int  _numthread;
    int  _nbliving;
    std::vector <collections::impl::CollectionCacheSorted<Type>> _cachedCollections;
};



    
/********************************************************************************
         #####   #######  #######  ######      #      #####   #######
        #     #     #     #     #  #     #    # #    #     #  #
        #           #     #     #  #     #   #   #   #        #
         #####      #     #     #  ######   #     #  #  ####  #####
              #     #     #     #  #   #    #######  #     #  #
        #     #     #     #     #  #    #   #     #  #     #  #
         #####      #     #######  #     #  #     #   #####   #######
********************************************************************************/

/** \brief Storage class
 *
 * The Storage class is the entry point for managing collections and groups.
 *
 * It delegates all the actions to a root group (retrievable through an operator overload).
 *
 * Such a storage is supposed to gather several information sets in a single environment,
 * with possible hierarchical composition.
 *
 * It is a template class: one should provide the actual type of the collections containers.
 * Possible template types could be designed for:
 *   - classical file system
 *   - HDF5 files
 *   - memory
 */
class Storage : public Cell, public virtual system::SharedObject<Storage>
{
    using SO = SharedObject<Storage>;
public:
    using SO::sptr;
    using SO::csptr;
    using SO::uptr;
    using SO::cuptr;
    using SO::share;

    /** Constructor.
     * \param[in] mode : storage mode
     * \param[in] name : name of the storage.
     * \param[in] autoRemove : tells whether the storage has to be physically deleted when this object is deleted. */
    Storage (StorageMode_e mode, const std::string& name, bool autoRemove=false)
        : Cell(0, ""), _name(name), _factory(mode), _root(0), _autoRemove(autoRemove)
    {}

    /** Destructor */
    virtual ~Storage () {}

    /** Get the name of the storage. */
    std::string getName() const { return _name; }

    Group::sptr Storage::getRoot ()
    {
        if (!_root) {
            _root = (_factory->createGroup (as_shared_ptr(this), ""));
            _root->setCompressLevel (this->getCompressLevel());
        }
        return _root;
    }

    /** Facility for retrieving the root group.
     * \return the root group. */
    Group& getGroup (const std::string name) { return this->operator() (name); }

    /** Facility for retrieving the root group.
     * \return the root group. */
    Group& operator() (const std::string name="")
    {
        if (name.empty())  { return *getRoot(); }
        else               { return getRoot ()->getGroup (name);  }
    }


    /** Remove physically the storage. */
    virtual void remove ();

    /** */
    StorageFactory& getFactory() const { return _factory; }

    /** WRAPPER C++ OUTPUT STREAM */
    class ostream : public std::ostream
    {
    public:
        ostream (Group& group, const std::string& name);
        ~ostream ();
    };

    /** WRAPPER C++ INPUT STREAM */
    class istream : public std::istream
    {
    public:
        istream (Group& group, const std::string& name);
        ~istream();
    };

protected:

    std::string _name;

    StorageFactory _factory;

    /** Root group. */
    Group::sptr _root;

    bool _autoRemove;
};




/********************************************************************************
                #######     #      #####   #######  #######  ######   #     #
                #          # #    #     #     #     #     #  #     #   #   #
                #         #   #   #           #     #     #  #     #    # #
   storage -    #####    #     #  #           #     #     #  ######      #
                #        #######  #           #     #     #  #   #       #
                #        #     #  #     #     #     #     #  #    #      #
                #        #     #   #####      #     #######  #     #     #
********************************************************************************/

/** \brief Factory that creates instances related to the storage feature.
 *
 * This class provides some createXXX methods for instantiations.
 *
 * It also provides a few general methods like exists and load, so the name 'factory'
 * may be not well choosen...
 *
 * Example 1:
 * \snippet storage1.cpp  snippet1_storage
 *
 * Example 2:
 * \snippet storage3.cpp  snippet1_storage
 */
class StorageFactory : public system::SharedObject<StorageFactory>
{
public:

    /** Constructor
     * \param[in] mode : kind of storage to be used (HDF5 for instance)
     */
    StorageFactory (StorageMode_e mode) : _mode(mode)  {}

    /** Create a Storage instance. This function is a bit of a misnomer: it can create a new storage instance and is also used to load an existing one.
     * \param[in] name : name of the instance to be created
     * \param[in] deleteIfExist : if the storage exits in file system, delete it if true.
     * \param[in] autoRemove : auto delete the storage from file system during Storage destructor.
     * \return the created Storage instance
     */
    Storage::sptr create (const std::string& name, bool deleteIfExist, bool autoRemove, bool dont_add_extension = false, bool append = false)
    {
        switch (_mode)
        {
            case STORAGE_HDF5:  return StorageHDF5Factory::createStorage (name, deleteIfExist, autoRemove, dont_add_extension, append);
            case STORAGE_FILE:  return StorageFileFactory::createStorage (name, deleteIfExist, autoRemove);
            case STORAGE_GZFILE:  return StorageGzFileFactory::createStorage (name, deleteIfExist, autoRemove);
            case STORAGE_COMPRESSED_FILE:  return StorageSortedFactory::createStorage (name, deleteIfExist, autoRemove);
            default:            throw system::Exception ("Unknown mode in StorageFactory::createStorage");
        }
    }

    /** Tells whether or not a Storage exists in file system given a name
     * \param[in] name : name of the storage to be checked
     * \return true if the storage exists in file system, false otherwise.
     */
    bool exists (const std::string& name)
    {
        switch (_mode)
        {
            case STORAGE_HDF5:              return StorageHDF5Factory::exists (name);
            case STORAGE_FILE:              return StorageFileFactory::exists (name);
            case STORAGE_GZFILE:            return StorageGzFileFactory::exists (name);
            case STORAGE_COMPRESSED_FILE:   return StorageSortedFactory::exists (name);
            default:            throw system::Exception ("Unknown mode in StorageFactory::exists");
        }
    }

    /** Create a Storage instance from an existing storage in file system.
     * \param[in] name : name of the file in file system
     * \return the created Storage instance
     */
    Storage::sptr load (const std::string& name) { return create (name, false, false); }

    /** Create a Group instance and attach it to a cell in a storage.
     * \param[in] parent : parent of the group to be created
     * \param[in] name : name of the group to be created
     * \return the created Group instance.
     */
    Group::sptr createGroup (ICell::sptr parent, const std::string& name)
    {
        switch (_mode)
        {
            case STORAGE_HDF5:  return StorageHDF5Factory::createGroup (parent, name);
            case STORAGE_FILE:  return StorageFileFactory::createGroup (parent, name);
            case STORAGE_GZFILE:  return StorageGzFileFactory::createGroup (parent, name);
            case STORAGE_COMPRESSED_FILE:  return StorageSortedFactory::createGroup (parent, name);

            default:            throw system::Exception ("Unknown mode in StorageFactory::createGroup");
        }
    }

    /** Create a Partition instance and attach it to a cell in a storage.
     * \param[in] parent : parent of the partition to be created
     * \param[in] name : name of the partition to be created
     * \param[in] nb : number of collections of the partition
     * \return the created Partition instance.
     */
    template<typename Type>
    Partition<Type>* createPartition (ICell::sptr parent, const std::string& name, size_t nb)
    {
        switch (_mode)
        {
            case STORAGE_HDF5:  return StorageHDF5Factory::createPartition<Type> (parent, name, nb);
            case STORAGE_FILE:  return StorageFileFactory::createPartition<Type> (parent, name, nb);
            case STORAGE_GZFILE:  return StorageGzFileFactory::createPartition<Type> (parent, name, nb);
            case STORAGE_COMPRESSED_FILE:  return StorageSortedFactory::createPartition<Type> (parent, name, nb);

            default:            throw system::Exception ("Unknown mode in StorageFactory::createPartition");
        }
    }

    /** Create a Collection instance and attach it to a cell in a storage.
     * \param[in] parent : parent of the collection to be created
     * \param[in] name : name of the collection to be created
     * \param[in] synchro : synchronizer instance if needed
     * \return the created Collection instance.
     */
    template<typename Type>
    CollectionNode<Type>* createCollection (ICell::sptr parent, const std::string& name, system::ISynchronizer::sptr synchro)
    {
        switch (_mode)
        {
            case STORAGE_HDF5:  return StorageHDF5Factory::createCollection<Type> (parent, name, synchro);
            case STORAGE_FILE:  return StorageFileFactory::createCollection<Type> (parent, name, synchro);
            case STORAGE_GZFILE:  return StorageGzFileFactory::createCollection<Type> (parent, name, synchro);
            case STORAGE_COMPRESSED_FILE:  return StorageSortedFactory::createCollection<Type> (parent, name, synchro);

            default:            throw system::Exception ("Unknown mode in StorageFactory::createCollection");
        }
    }

private:

    StorageMode_e _mode;
};




/********************************************************************************/
} } } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_HPP_ */
