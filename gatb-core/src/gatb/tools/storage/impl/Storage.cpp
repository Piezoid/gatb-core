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

/********************************************************************************/

#include <gatb/tools/storage/impl/Storage.hpp>

/********************************************************************************/
namespace gatb { namespace core {  namespace tools {  namespace storage {  namespace impl {
/********************************************************************************/

/********************************************************************************
             #####   #######  ######   #######     #     #     #
            #     #     #     #     #  #          # #    ##   ##
            #           #     #     #  #         #   #   # # # #
             #####      #     ######   #####    #     #  #  #  #
                  #     #     #   #    #        #######  #     #
            #     #     #     #    #   #        #     #  #     #
             #####      #     #     #  #######  #     #  #     #
********************************************************************************/

/* WRAPPERS BETWEEN STORAGE AND C++ STREAMS
 *
 * Got inspiration from :
 *      http://savingyoutime.wordpress.com/2009/04/21/using-c-stl-streambufostream-to-create-time-stamped-logging-class/
 *      http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
 */

/* Output stream buffer implementation. */
class Storage_ostreambuf : public std::streambuf
{
protected:

    static const int bufferSize = 4*1024;   // size of data buffer
    char buffer[bufferSize];                // data buffer

public:
    Storage_ostreambuf (Group& group, const std::string& name) : _nbWritten(0)
    {
        setp (buffer, buffer+(bufferSize-1));
        _collection = & group.getCollection<math::NativeInt8> (name);
    }

    virtual ~Storage_ostreambuf() { 
        sync(); 
        //std::cout << "ostream destructor" << std::endl; 
        }

protected:

    collections::ICollection<math::NativeInt8>* _collection;

    // flush the characters in the buffer
    int flushBuffer ()
    {
        int num = pptr()-pbase();
        _collection->insert ((math::NativeInt8*)buffer, num);
        _collection->flush(); // important!
        _nbWritten += num;
        pbump(-num); // reset put pointer accordingly
        return num;
    }

    virtual int overflow ( int c = EOF )
    {
        if (c != EOF) {
            *pptr() = c;    // insert character into the buffer
            pbump(1);
        }
        if (flushBuffer() == EOF)
            return EOF;
        return c;
    }

    virtual int sync()
    {
        if (flushBuffer() == EOF) {  return -1; }  // ERROR
        return 0;
    }

    virtual pos_type  seekoff (off_type off, std::ios_base::seekdir dir,  std::ios_base::openmode mode)
    {
        sync ();  // We may have to flush the current buffer first
        return _nbWritten;
    }

    pos_type _nbWritten;
};

/*********************************************************************
*********************************************************************/

/* Input stream buffer implementation. */
class Storage_istreambuf : public std::streambuf
{
    public:
    Storage_istreambuf (Group& group, const std::string& name, std::size_t buff_sz = 1024, std::size_t put_back = 64) :
            put_back_(std::max(put_back, size_t(1))),
            buffer_(std::max(buff_sz, put_back_) + put_back_), currentIdx(0)
        {
            char *end = &buffer_.front() + buffer_.size();
            setg(end, end, end);
            _collection = & group.getCollection<math::NativeInt8> (name);
        }

    private:
        // overrides base class underflow()
        int_type underflow()
        {
            if (gptr() < egptr()) // buffer not exhausted
                return traits_type::to_int_type(*gptr());

            char *base = &buffer_.front();
            char *start = base;

            if (eback() == base) // true when this isn't the first fill
            {
                // Make arrangements for putback characters
                std::memmove(base, egptr() - put_back_, put_back_);
                start += put_back_;
            }
            // start is now the start of the buffer, proper.
            // Read from fptr_ in to the provided buffer
            math::NativeInt8* start2 = (math::NativeInt8*) start;
            //std::cout << "(storage) istream calling getItems with params: start2=" << start2 << " currentIdx=" << currentIdx << " buffer.size()-start+base=" << (buffer_.size() - (start - base)) << ", total buffer size " << buffer_.size()  << std::endl;
            if ( ((int64_t)base > (int64_t)start) || (((int64_t)start - (int64_t)base) > (int64_t)buffer_.size()))
            {
                std::cout << "Error: trying to read more elements " << (start - base) << " = (" << start << " - " << base << ") than the buffer size" << std::endl; exit(1);
            }
            size_t offset = currentIdx ; // in hdf5: needs to be currentIdx; in file: needs to be 0 (but it will be fixed in IteratorFile, ok, not here)
            size_t n = _collection->getItems (start2, offset, buffer_.size() - (start - base));
            currentIdx += n;

            if (n == 0)  {   return traits_type::eof();  }

            // Set buffer pointers
            setg(base, start, start + n);

            return traits_type::to_int_type(*gptr());
        }

        // copy ctor and assignment not implemented;
        // copying not allowed
        Storage_istreambuf(const Storage_istreambuf &);
        Storage_istreambuf &operator= (const Storage_istreambuf &);

    private:
        collections::ICollection<math::NativeInt8>* _collection;

        const std::size_t put_back_;
        std::vector<char> buffer_;
        size_t currentIdx;
};

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Storage::ostream::ostream (Group& group, const std::string& name)
    : std::ios(0), std::ostream(new Storage_ostreambuf(group,name))
{
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Storage::ostream::~ostream()
{
    delete rdbuf();
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Storage::istream::istream (Group& group, const std::string& name)
    : std::ios(0), std::istream(new Storage_istreambuf(group,name))
{
}

/*********************************************************************
** METHOD  :
** PURPOSE :
** INPUT   :
** OUTPUT  :
** RETURN  :
** REMARKS :
*********************************************************************/
Storage::istream::~istream ()
{
    delete rdbuf();
}
	
	
///////////////////////////////////////
////////// SuperKmerBinFiles //////////
///////////////////////////////////////

////////////////////////////////////////////
//////////  CacheSuperKmerBinFiles /////////
////////////////////////////////////////////



/********************************************************************************/
} } } } } /* end of namespaces. */
/********************************************************************************/

