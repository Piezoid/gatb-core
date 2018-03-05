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
 *  \brief Progress feature
 */

#ifndef _GATB_CORE_TOOLS_MISC_IMPL_PROPERTY_HPP_
#define _GATB_CORE_TOOLS_MISC_IMPL_PROPERTY_HPP_

/********************************************************************************/

#include <gatb/tools/misc/impl/Property.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <stack>
#include <set>
#include <list>
#include <cstdio>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace tools     {
namespace misc      {
/********************************************************************************/



/** \brief  Definition of a property as a [key,value] entry with a given depth.
 *
 * Define what a property can be. This is an extension of the [key,value] concept
 * since we add a notion a depth to this couple, which makes possible to have a tree
 * vision of a simple list of [depth,key,value] entries.
 *
 * Such instances are managed by the Properties class, that acts as a container of Property
 * instances.
 *
 *  \see Properties
 */
class Property
{
public:
    /** Constructor.
     * \param[in] aDepth : depth of the [key,value]
     * \param[in] aKey   : the key
     * \param[in] aValue : the value
     */
    Property (size_t aDepth, const std::string& aKey, const std::string& aValue)
        : depth(aDepth), key(aKey), value(aValue)  {}

    /** Constructor.
     * \param[in] aKey   : the key
     * \param[in] aValue : the value
     */
    Property (const std::string& aKey="", const std::string& aValue="")
        : depth(0), key(aKey), value(aValue)  {}

    /** Depth of the property. 0 should mean root property. */
    size_t      depth;

    /** Key of the property as a string. */
    std::string key;

    /** Value of the property as a string. */
    std::string value;

    /** Returns the value of the property as a C++ string.
     * \return the value
     */
    const std::string&  getValue  ()  { return value;                }

    /** Returns the value of the property as an integer (it supposes that the string represents an integer).
     * \return the value
     */
    long                getInt    ()  { return std::stol(value); }

    /** Returns the value of the property as a float (it supposes that the string represents an float).
     * \return the value
     */
    double              getDouble    ()  { return atof (value.c_str()); }

    /** Returns the value of the property as a C string.
     * \return the value
     */
    const char*         getString ()  { return value.c_str();        }
};

/********************************************************************************/

/** Visitor for a Property instance.
 *
 *  Define a Design Pattern Visitor for the Property instance.
 *
 *  In this case, we have only the Property class to visit (not a true classes hierarchy like one can
 *  find more classically for the Visitor DP), but we add two methods, one called before the Property
 *  visit, and one called after.
 *
 *  This can be seen as a improved way to iterate the Property items of a Properties instance.
 *
 *  It is defined as a SmartPointer for easing instance life cycle management.
 */
class IPropertiesVisitor //: public system::SmartPointer
{
public:

    /** Called before the true visit of the Property instance. */
    void visitBegin    ();

    /** Visit of the Property instance.
     * \param[in] prop : the instance to be visited.
     */
    void visitProperty (Property* prop);

    /** Called after the true visit of the Property instance. */
    void visitEnd      ();
};



/** \brief Container of Property instances with DP Visitor capability.
 *
 *   This class provides a constructor that can read a file holding properties as [key,value] entries
 *   (one per line). This is useful for instance for managing a configuration file.
 *
 *   It merely defines a container of Property instances; it contains
 *   several 'add' methods for adding Property instances into the container.
 *
 *   It is possible to retrieve a specific Property instance given a key.
 *
 *   The main method is 'accept'; its purpose is to visit each contained Property instance.
 *   Note that the only way to iterate the whole Property set is to define its own IPropertiesVisitor
 *   class and make it accepted by the Properties instance; the 'visitProperty' method should be then
 *   called for each Property instance.
 *
 *   It is defined as a SmartPointer for easing instance life cycle management.
 *
 * Sample of use:
 * \code
 * void sample ()
 * {
 *      // we create a Properties instance.
 *      Properties& props = new Properties ();
 *
 *      // we add some entries. Note the different depth used: we have a root property having 3 children properties.
 *      props.add (0, "root", "");
 *      props.add (1, "loud",   "len=%d", 3);
 *      props.add (1, "louder", "great");
 *      props.add (1, "stop",   "[x,y]=[%f,%f]", 3.14, 2.71);
 *
 *      // we create some visitor for dumping the props into a XML file
 *      IPropertiesVisitor* v = new XmlDumpPropertiesVisitor ("/tmp/test.xml");
 *
 *      // we accept the visitor; after that, the output file should have been created.
 *      props.accept (v);
 * }
 * \endcode
 */
class Properties
{
public:

    /** Constructor. If a file path is provided, it tries to read [key,value] entries from this file.
     * \param[in] rootname : the file (if any) to be read
     */
    Properties (const std::string& rootname = "");

    static Properties fromXml(const std::string&);

    /** Destructor. */
    virtual ~Properties () {}

    bool empty() const { return _properties.empty(); }

    /** Accept a visitor (should loop over all Property instances).
     * \param[in] visitor : visitor to be accepted
     */
    void accept (IPropertiesVisitor* visitor);

    /** Add a Property instance given a depth, a key and a value provided in a printf way.
     * \param[in] depth  : depth of the property to be added
     * \param[in] aKey   : key of the property to be added
     * \param[in] format : define the format of the value of the property, the actual value being defined by the ellipsis
     * \return a Property instance is created and returned as result of the method.
     *
     */
    Property* add (size_t depth, const std::string& aKey, const char* format=0, ...);

    /** Add a Property instance given a depth, a key and a value.
     * \param[in] depth  : depth of the property to be added
     * \param[in] aKey   : key of the property to be added
     * \param[in] aValue : value (as a string) of the property to be added
     * \return a Property instance is created and returned as result of the method.
     */
    Property* add (size_t depth, const std::string& aKey, const std::string& aValue);

    /** Add all the Property instances contained in the provided Properties instance. Note that a depth is provided
     *  and is added to the depth of each added Property instance.
     * \param[in] depth : depth to be added to each depth of added instances.
     * \param[in] prop  : instance holding Property instances to be added
     */
    void       add (size_t depth, Properties& prop);

    /** Add all the Property instances contained in the provided Properties instance. Note that a depth is provided
     *  and is added to the depth of each added Property instance.
     * \param[in] depth : depth to be added to each depth of added instances.
     * \param[in] prop  : instance holding Property instances to be added
     */
    void       add (size_t depth, const Properties& prop);


    /** Add Properties from an xml string
     * \param[in] the string to be parsed as xml
     * \returns The properties added (and only them)
     */
    Properties addXml(const std::string& xml) {
        Properties props = fromXml(xml);
        add(1, props);
        return props;
    }

    /** */
    void add (Property* prop, va_list args);

    /** Merge the Property instances contained in the provided Properties instance.
     * \param[in] prop  : instance holding Property instances to be added
     */
    void  merge (Properties& prop);

    /** Returns the Property instance given a key.
     * \param[in] key : the key
     * \return the Property instance if found, 0 otherwise.
     */
    Property* operator[] (const std::string& key);

    /** Returns the Property instance given a key.
     * \param[in] key : the key
     * \return the Property instance if found, 0 otherwise.
     */
    Property* get (const std::string& key) const;

    /** Get the value of a property given its key.
     * \param[in] key : the key of the property
     * \return the value of the key as a string.
     */
    std::optional<std::string> getStr(const std::string& key) const ;

    /** Get the value of a property given its key.
     * \param[in] key : the key of the property
     * \return the value of the key as an integer
     */
    std::optional<int64_t> getInt(const std::string& key) const;

    /** Get the value of a property given its key.
     * \param[in] key : the key of the property
     * \return the value of the key as a double.
     */
    std::optional<double> getDouble (const std::string& key) const;

    /** Set the value of a property given its key.
     * \param[in] key : the key of the property
     * \param[in] value : value to be set.
     */
    void setStr    (const std::string& key, const std::string& value);

    /** Set the value of a property given its key.
     * \param[in] key : the key of the property
     * \param[in] value : value to be set.
     */
    void setInt    (const std::string& key, const int64_t& value);

    /** Set the value of a property given its key.
     * \param[in] key : the key of the property
     * \param[in] value : value to be set.
     */
    void setDouble (const std::string& key, const double& value);

    /** Clone the instance
     * \return the cloned instance.
     */
    Properties& clone ();

    /** Distribute arguments that are comma separated list.
     * \return the list of distributed Properties instances.
     */
    std::list<Properties&> map (const char* separator);

    /** Get the known keys.
     * \return the set of keys
     */
    std::set<std::string> getKeys ();

    /** Move the item (given its key) to the front of the container.
     * \param[in] key : the key of the item to be moved.
     */
    void setToFront (const std::string& key);

    /** Output the properties object through an output stream
     * \param[in] s : the output stream
     * \param[in] p : the properties object to output
     * \return the modified output stream
     */
    friend std::ostream & operator<<(std::ostream & s, const Properties& p)  {  p.dump(s);  return s;  }

    /** Get the properties as an XML string
     * \return the XML string.
     */
    std::string getXML ();

    /** Fill a Properties instance from an XML stream.
     * \param[in] stream: the stream to be read (file, string...) */
    void readXML (std::istream& stream) const;

protected:

    /** */
    void dump (std::ostream& s) const ;

private:

    /** List of Property instances. */
    std::list<Property> _properties;

    /* */
    Property& getRecursive (const std::string& key, std::list<Property>::const_iterator& it) const ;
};

/********************************************************************************/

/* Factorization of common stuf for IPropertiesVisitor implementations. */
class AbstractOutputPropertiesVisitor : public IPropertiesVisitor
{
public:

     AbstractOutputPropertiesVisitor (std::ostream& aStream);
     AbstractOutputPropertiesVisitor (const std::string& filename);
    ~AbstractOutputPropertiesVisitor ();

protected:

    std::ostream* _stream;
    std::string   _filename;
};

/********************************************************************************/

/** \brief XML serialization of a Properties instance.
 *
 *  This kind of visitor serializes into a file the content of a Properties instance.
 *
 *  The output format is XML; the 'depth' attribute of each Property instance is used
 *  as a basis for building the XML tree.
 */
class XmlDumpPropertiesVisitor : public AbstractOutputPropertiesVisitor
{
public:

    /** Constructor.
     * \param[in] filename : uri of the file where to serialize the instance.
     * \param[in] propertiesAsRoot
     * \param[in] shouldIndent : tells whether we should use indentation
     */
    XmlDumpPropertiesVisitor (const std::string& filename, bool propertiesAsRoot=true, bool shouldIndent = true);

    /** Constructor.
     * \param[in] aStream : output stream
     * \param[in] propertiesAsRoot
     * \param[in] shouldIndent : tells whether we should use indentation
     */
    XmlDumpPropertiesVisitor (std::ostream& aStream, bool propertiesAsRoot=true, bool shouldIndent = true);

    /** Desctructor. */
    ~XmlDumpPropertiesVisitor ();

    /** \copydoc IPropertiesVisitor::visitBegin */
    void visitBegin ();

    /** \copydoc IPropertiesVisitor::visitEnd */
    void visitEnd   ();

    /** \copydoc IPropertiesVisitor::visitProperty */
    void visitProperty (Property* prop);

private:

    /** The name of the serialization file. */
    std::string             _name;

    /** Some stack for XML production. */
    std::stack<std::string> _stack;

    /** */
    int _deltaDepth;

    /** Internals. */
    void pop    (size_t depth);

    /** Indentation of the XML file. */
    void indent (size_t n);

    bool _firstIndent;
    bool _shouldIndent;

    /** Method for writing into the file. */
    void safeprintf (const char* format, ...);
};

/********************************************************************************/

/** \brief Raw dump of a Properties instance.
 *
 * This file format is simply a list of lines, with each line holding the key and the value
 * (separated by a space character). Note that the depth information is lost.
 *
 * This kind of output is perfect for keeping properties in a configuration file for instance.
 * This is used by a tool for its configuration file '.toolrc'
 */
class RawDumpPropertiesVisitor : public IPropertiesVisitor
{
public:

    /** Constructor.
     * \param os : output stream where the visitor can dump information. */
    RawDumpPropertiesVisitor (std::ostream& os = std::cout, int width=40, char sep=':');

    /** Desctructor. */
    ~RawDumpPropertiesVisitor ();

    /** \copydoc IPropertiesVisitor::visitBegin */
    void visitBegin () {}

    /** \copydoc IPropertiesVisitor::visitEnd */
    void visitEnd   () {}

    /** \copydoc IPropertiesVisitor::visitProperty */
    void visitProperty (Property* prop);

private:
    std::ostream& _os;
    int _width;
    char _sep;
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_MISC_IMPL_PROPERTY_HPP_ */
