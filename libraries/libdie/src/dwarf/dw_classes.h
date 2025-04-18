/*
 * Copyright (C) 2011-2019 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of libdie.
 *
 * libdie is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libdie is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libdie. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief A file containing definitions of classes representing DWARF entries.
 *
 * A file containing definitions of classes representing various DWARF entries.
 *
 * @file      dw_classes.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2013-03-26
 * @version   0.6
 */

#ifndef __LIBDIE__DWARF__DW_CLASSES_H__
  #define __LIBDIE__DWARF__DW_CLASSES_H__

#include <list>
#include <map>

#include "boost/shared_ptr.hpp"

#include "libdwarf/dwarf.h"
#include "libdwarf/libdwarf.h"

#ifndef __LIBDIE__DWARF__DW_VISITORS_H__
  #include "dw_visitors.h"
#else
  class DwDieVisitor;
  class DwDieTreeTraverser;
#endif

class DwDie;

#define DW_FORM_cu_ref_obj  0x41 // The value is encoded as a pointer to DwDie
#define DW_FORM_sec_ref_obj 0x42 // The value is encoded as a pointer to DwDie
#define DW_FORM_location    0x43 // The value is encoded as a DWARF location
#define DW_FORM_source_file 0x44 // The value is encoded as a string

/**
 * @brief A structure representing a list containing source files.
 *
 * Represents a list containing source files.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-08
 * @date      Last Update 2011-09-08
 * @version   0.1
 */
typedef struct Dwarf_Source_File_List_s
{
  char **srcfiles; //!< An array containing the source files.
  Dwarf_Signed srccount; //!< A size of the array with the source files.
} Dwarf_Source_File_List;

/**
 * @brief A structure representing a list containing DWARF locations.
 *
 * Represents a list containing DWARF locations.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-07
 * @date      Last Update 2011-09-07
 * @version   0.1
 */
typedef struct Dwarf_Location_List_s
{
  Dwarf_Locdesc **llbuf; //!< A buffer containing the DWARF locations.
  Dwarf_Signed listlen; //!< A size of the buffer with the DWARF locations.
} Dwarf_Location_List;

/**
 * @brief A structure representing a DWARF debugging information entry
 *   attribute.
 *
 * Represents a DWARF debugging information entry attribute.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-22
 * @date      Last Update 2011-09-07
 * @version   0.2.4
 */
typedef struct Dwarf_Attribute_Value_s
{
  Dwarf_Form_Class cls; //!< Determines the class of the value of the attribute.
  Dwarf_Half form; //!< Determines how the value of the attribute is encoded.
  union
  {
    Dwarf_Addr addr; //!< Represents an address on the target machine.
    Dwarf_Signed sdata; //!< Represents a signed constant.
    Dwarf_Unsigned udata; //!< Represents an unsigned constant.
    Dwarf_Bool flag; //!< Represents a boolean value.
    Dwarf_Off ref; //!< Represents an offset.
    char *string; //!< Represents a string.
    Dwarf_Loc *loc; //!< Represents a location.
    Dwarf_Location_List *loclist; //!< Represents a list containing locations.
    DwDie *die; //!< Represents a reference to a DIE object.
  };
} Dwarf_Attribute_Value;

std::ostream& operator<<(std::ostream& s, const Dwarf_Loc& value);
std::ostream& operator<<(std::ostream& s, const Dwarf_Locdesc& value);
std::ostream& operator<<(std::ostream& s, const Dwarf_Attribute_Value& value);

/**
 * @brief A class for retrieving values of DWARF registers.
 *
 * Retrieves values of DWARF registers.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-09-19
 * @date      Last Update 2011-09-19
 * @version   0.1
 */
class DwRegisters
{
  public: // Virtual methods for retrieving values of DWARF registers
    /**
     * Gets a value of a DWARF register.
     *
     * @param number A number identifying the DWARF register.
     * @return The value of the DWARF register or @em 0 if the value cannot be
     *   retrieved.
     */
    virtual Dwarf_Addr getValue(int number) = 0;
};

// Forward declaration of some DWARF DIE objects used by other DWARF DIE objects
class DwMember;

/**
 * @brief A class representing a DWARF debugging information entry.
 *
 * Represents a DWARF debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2012-02-17
 * @version   0.1.10
 */
class DwDie
{
  public: // Type definitions
    typedef std::map< Dwarf_Half, Dwarf_Attribute_Value >  Dwarf_Attribute_Map;
    typedef std::list< boost::shared_ptr< DwDie > > Dwarf_Die_List;
  protected: // Retrieved variables
    Dwarf_Off m_offset; //!< An offset of the DWARF DIE in a DWARF CU.
    /**
     * @brief A map containing all attributes of this DWARF DIE.
     */
    Dwarf_Attribute_Map m_attributes;
    /**
     * @brief A list containing all DWARF DIEs under this DWARF DIE.
     */
    Dwarf_Die_List m_children;
    /**
     * @brief A pointer to the parent DWARF DIE of this DWARF DIE.
     */
    DwDie* m_parent;
  protected: // Constructors
    DwDie();
    DwDie(Dwarf_Die& die);
    DwDie(const DwDie& die);
  public: // Destructors
    virtual ~DwDie();
  public: // Inline member methods for accessing the parent DWARF DIE
    /**
     * Gets a parent DWARF DIE of this DWARF DIE.
     *
     * @return The parent DWARF DIE of this DWARF DIE.
     */
    DwDie* getParent() { return m_parent; }

    /**
     * Sets a parent DWARF DIE of this DWARF DIE.
     *
     * @param parent A parent DWARF DIE of this DWARF DIE.
     */
    void setParent(DwDie* parent) { m_parent = parent; }
  public: // Inline member methods
    /**
     * Gets a read-only map containing all attributes of a DWARF DIE.
     *
     * @return A read-only map containing all attributes of the DWARF DIE.
     */
    const Dwarf_Attribute_Map& getAttributes() const { return m_attributes; }

    /**
     * Gets a map containing all attributes of a DWARF DIE.
     *
     * @return A map containing all attributes of the DWARF DIE.
     */
    Dwarf_Attribute_Map& getAttributes() { return m_attributes; }

    /**
     * Gets a read-only list containing all children of a DWARF DIE.
     *
     * @return A read-only list containing all children of the DWARF DIE.
     */
    const Dwarf_Die_List& getChildren() const { return m_children; }

    /**
     * Gets a list containing all children of a DWARF DIE.
     *
     * @return A list containing all children of the DWARF DIE.
     */
    Dwarf_Die_List& getChildren() { return m_children; }

    /**
     * Gets a CU-relative offset of a DWARF debugging information entry.
     *
     * @return The CU-relative offset of the DWARF debugging information entry.
     */
    Dwarf_Off getOffset() { return m_offset; }

    /**
     * Gets a name of a DWARF debugging information entry.
     *
     * @return The name of the DWARF debugging information entry or @em NULL if
     *   the DWARF debugging information entry has no name.
     */
    const char* getName()
    {
      // Get the attribute holding the name of the DIE
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_name);

      if (it != m_attributes.end())
      { // Attribute found, return the name of the DIE
        return it->second.string;
      }

      // Attribute not found, but might be present in another DIE object
      it = m_attributes.find(DW_AT_abstract_origin);

      if (it != m_attributes.end())
      { // Try to get the name of the DIE from another DIE object
        return it->second.die->getName();
      }

      // Attribute not found, but might be present in the DIE's declaration
      it = m_attributes.find(DW_AT_specification);

      if (it != m_attributes.end())
      { // Try to get the name of the DIE from its declaration
        return it->second.die->getName();
      }

      // Attribute not found
      return NULL;
    }
  public: // Virtual methods for dynamic creation of DWARF DIEs at runtime
    virtual DwDie* clone() = 0;
    virtual DwDie* create() = 0;
    virtual DwDie* create(Dwarf_Die& die) = 0;
  public: // Virtual methods for retrieving DWARF DIE tag information
    virtual int getTag() = 0;
    virtual bool hasTag(int tag) = 0;
  public: // Virtual methods for visiting DWARF DIEs
    virtual void accept(DwDieVisitor& visitor);
    virtual void accept(DwDieTreeTraverser& traverser);
  private:
    void loadAttributes(Dwarf_Die& die);
};

/**
 * @brief A class representing a DWARF tagged debugging information entry.
 *
 * Represents a DWARF tagged debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-08-30
 * @version   0.1.4
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
class DwTag : public DwDie
{
  protected: // Constructors
    DwTag();
    DwTag(Dwarf_Die& die);
    DwTag(const DwTag& tag);
  public: // Destructors
    virtual ~DwTag();
  public: // Virtual methods for dynamic creation of DWARF DIEs at runtime
    virtual DwDie* clone();
    virtual DwDie* create();
    virtual DwDie* create(Dwarf_Die& die);
  public: // Virtual methods for retrieving DWARF DIE tag information
    /**
     * Gets an identifying tag describing a DWARF debugging information entry.
     *
     * @return The identifying tag describing the DWARF debugging information
     *   entry.
     */
    int getTag() { return DW_TAG_ID; }

    /**
     * Checks if an identifying tag is describing a DWARF debugging information
     *   entry.
     *
     * @param tag An identifying tag.
     * @return @em True if the identifying tag describes the DWARF debugging
     *   information entry, @em false otherwise.
     */
    bool hasTag(int tag) { return tag == DW_TAG_ID; }
  public: // DwDie class virtual methods redefinition
    virtual void accept(DwDieVisitor& visitor);
    virtual void accept(DwDieTreeTraverser& traverser);
};

/**
 * @brief A class representing a DWARF data object debugging information entry.
 *
 * Represents a DWARF data object (variable, formal parameter or constant)
 *   debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-01
 * @date      Last Update 2012-02-17
 * @version   0.1.4
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
class DwDataObject : public DwTag< DW_TAG_CLASS, DW_TAG_ID >
{
  public: // Constructors
    DwDataObject();
    DwDataObject(Dwarf_Die& die);
    DwDataObject(const DwDataObject& dobj);
  public: // Destructors
    virtual ~DwDataObject();
  public: // Member methods
    DwDie* getDataType();
    std::string getDeclarationSpecifier();
    Dwarf_Unsigned getSize();
  public: // Member methods
    bool isClass();
    bool isStructure();
    bool isUnion();
  public: // Inline member methods
    /**
     * Gets a type of a data object.
     *
     * @note The type is encoded as a tree of DWARF debugging information entry
     *   objects, where each tree node contains a part of the type (e.g. base
     *   type information, pointer information, modifiers, ...).
     *
     * @return The type of the data object or @em NULL if the type was not
     *   found.
     */
    DwDie* getType()
    {
      // Get the attribute holding the type of the data object
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_type);

      if (it == this->m_attributes.end())
      { // Static data members have their type stored under specification
        it = this->m_attributes.find(DW_AT_specification);
      }

      if (it != this->m_attributes.end())
      { // Attribute found, return the type of the data object
        return it->second.die;
      }

      // Attribute not found
      return NULL;
    }

    /**
     * Gets a specification of a data object.
     *
     * @return The specification of the data object or @em NULL if the
     *   specification was not found.
     */
    DwDie* getSpecification()
    {
      // Get the attribute holding the specification of the data object
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_specification);

      if (it != this->m_attributes.end())
      { // Attribute found, return the specification of the data object
        return it->second.die;
      }

      // Attribute not found
      return NULL;
    }

    /**
     * Gets a location of a data object at run-time.
     *
     * @return The location of the data object at run-time or @em NULL if the
     *   location was not found.
     */
    Dwarf_Loc* getLocation()
    {
      // Get the attribute holding the location
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_location);

      if (it != this->m_attributes.end())
      { // Attribute found, return the location
        return it->second.loc;
      }

      // Attribute not found
      return NULL;
    }

    /**
     * Gets a source file in which the declaration of a data object appeared.
     *
     * @return The source file in which the declaration of the data object
     *   appeared or @em NULL if the source file was not found.
     */
    const char* getSourceFile()
    {
      // Get the attribute holding the name of the source file
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_decl_file);

      if (it != this->m_attributes.end())
      { // Attribute found, return the name of the source file
        return it->second.string;
      }

      // Attribute not found
      return NULL;
    }

    /**
     * Gets a line number at which the first character of the identifier of a
     *   data object appeared.
     *
     * @return The line number at which the first character of the identifier of
     *   the data object appeared or @em 0 if the line number was not found.
     */
    Dwarf_Unsigned getLineNumber()
    {
      // Get the attribute holding the line number
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_decl_line);

      if (it != this->m_attributes.end())
      { // Attribute found, return the line number
        return it->second.udata;
      }

      // Attribute not found
      return 0;
    }
};

/**
 * @brief A class representing a DWARF compound type debugging information
 *   entry.
 *
 * Represents a DWARF compound type (class, structure or union) debugging
 *   information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-07
 * @date      Last Update 2011-12-07
 * @version   0.1.1
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
class DwCompoundType : public DwTag< DW_TAG_CLASS, DW_TAG_ID >
{
  public: // Constructors
    DwCompoundType();
    DwCompoundType(Dwarf_Die& die);
    DwCompoundType(const DwCompoundType& ct);
  public: // Destructors
    virtual ~DwCompoundType();
  public: // Member methods
    DwMember* getMember(Dwarf_Off offset);
    std::string getMemberName(Dwarf_Off offset);
  public: // Inline member methods
    /**
     * Gets a size in bytes of a compound type.
     *
     * @return The size in bytes of the compound type or @em 0 if the size
     *   cannot be determined.
     */
    Dwarf_Unsigned getSize()
    {
      // Get the attribute holding the size of the compound type
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_byte_size);

      if (it != this->m_attributes.end())
      { // Attribute found, return the size of the compound type
        return it->second.udata;
      }

      // Attribute not found
      return 0;
    }
};

/**
 * @brief A class representing a DWARF array type debugging information entry.
 *
 * Represents a DWARF array type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2013-03-07
 * @version   0.2
 */
class DwArrayType
  : public DwTag< DwArrayType, DW_TAG_array_type >
{
  public: // Constructors
    DwArrayType();
    DwArrayType(Dwarf_Die& die);
    DwArrayType(const DwArrayType& at);
  public: // Destructors
    virtual ~DwArrayType();
  public:
    /**
     * Gets a type of elements in an array.
     *
     * @note The type is encoded as a tree of DWARF debugging information entry
     *   objects, where each tree node contains a part of the type (e.g. base
     *   type information, pointer information, modifiers, ...).
     *
     * @return The type of elements in the array or @em NULL if the type cannot
     *   be determined.
     */
    DwDie* getElementType()
    {
      // Get the attribute holding the type of the elements in the array
      DwDie::Dwarf_Attribute_Map::iterator
        it = this->m_attributes.find(DW_AT_type);

      if (it != this->m_attributes.end())
      { // Attribute found, return the type of the elements in the array
        return it->second.die;
      }

      // Attribute not found
      return NULL;
    }
};

/**
 * @brief A class representing a DWARF class type debugging information entry.
 *
 * Represents a DWARF class type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-12-07
 * @version   0.2
 */
class DwClassType
  : public DwCompoundType< DwClassType, DW_TAG_class_type >
{
  public: // Constructors
    DwClassType();
    DwClassType(Dwarf_Die& die);
    DwClassType(const DwClassType& ct);
  public: // Destructors
    virtual ~DwClassType();
};

/**
 * @brief A class representing a DWARF enumeration type debugging information
 *   entry.
 *
 * Represents a DWARF enumeration type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-31
 * @date      Last Update 2011-08-31
 * @version   0.1
 */
class DwEnumerationType
  : public DwTag< DwEnumerationType, DW_TAG_enumeration_type >
{
  public: // Constructors
    DwEnumerationType();
    DwEnumerationType(Dwarf_Die& die);
    DwEnumerationType(const DwEnumerationType& et);
  public: // Destructors
    virtual ~DwEnumerationType();
};

/**
 * @brief A class representing a DWARF formal parameter debugging information
 *   entry.
 *
 * Represents a DWARF formal parameter debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-12-01
 * @version   0.2
 */
class DwFormalParameter
  : public DwDataObject< DwFormalParameter, DW_TAG_formal_parameter >
{
  public: // Constructors
    DwFormalParameter();
    DwFormalParameter(Dwarf_Die& die);
    DwFormalParameter(const DwFormalParameter& fp);
  public: // Destructors
    virtual ~DwFormalParameter();
};

/**
 * @brief A class representing a DWARF imported declaration debugging
 *   information entry.
 *
 * Represents a DWARF imported declaration debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwImportedDeclaration
  : public DwTag< DwImportedDeclaration, DW_TAG_imported_declaration >
{
  public: // Constructors
    DwImportedDeclaration();
    DwImportedDeclaration(Dwarf_Die& die);
    DwImportedDeclaration(const DwImportedDeclaration& id);
  public: // Destructors
    virtual ~DwImportedDeclaration();
};

/**
 * @brief A class representing a DWARF label debugging information entry.
 *
 * Represents a DWARF label debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-31
 * @date      Last Update 2011-08-31
 * @version   0.1
 */
class DwLabel
  : public DwTag< DwLabel, DW_TAG_label >
{
  public: // Constructors
    DwLabel();
    DwLabel(Dwarf_Die& die);
    DwLabel(const DwLabel& l);
  public: // Destructors
    virtual ~DwLabel();
};

/**
 * @brief A class representing a DWARF lexical block debugging information
 *   entry.
 *
 * Represents a DWARF lexical block debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwLexicalBlock
  : public DwTag< DwLexicalBlock, DW_TAG_lexical_block >
{
  public: // Constructors
    DwLexicalBlock();
    DwLexicalBlock(Dwarf_Die& die);
    DwLexicalBlock(const DwLexicalBlock& lb);
  public: // Destructors
    virtual ~DwLexicalBlock();
};

/**
 * @brief A class representing a DWARF member debugging information entry.
 *
 * Represents a DWARF member debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-10-10
 * @version   0.1.7
 */
class DwMember
  : public DwTag< DwMember, DW_TAG_member >
{
  public: // Constructors
    DwMember();
    DwMember(Dwarf_Die& die);
    DwMember(const DwMember& m);
  public: // Destructors
    virtual ~DwMember();
  public: // Member methods
    DwDie *getDataType();
    std::string getDeclarationSpecifier();
    Dwarf_Unsigned getSize();
  public:
    bool isClass();
  public: // Inline member methods
    /**
     * Gets a type of a member of a class, structure or union.
     *
     * @note The type is encoded as a tree of DWARF debugging information entry
     *   objects, where each tree node contains a part of the type (e.g. base
     *   type information, pointer information, modifiers, ...).
     *
     * @return The type of the member or @em NULL if the type was not found.
     */
    DwDie *getType()
    {
      // Get the attribute holding the type of the member
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_type);

      if (it != m_attributes.end())
      { // Attribute found, return the type of the member
        return it->second.die;
      }

      // Attribute not found
      return NULL;
    }

    /**
     * Gets an offset in bytes of a member within a class, structure or union.
     *
     * @return The offset in bytes of the member or @em 0 if the offset cannot
     *   be determined.
     */
    Dwarf_Off getMemberOffset()
    {
      // Get the attribute holding the location of the member
      Dwarf_Attribute_Map::iterator it = m_attributes.find(
        DW_AT_data_member_location);

      if (it != m_attributes.end())
      { // Attribute found, location should always be 'DW_OP_plus_uconst offset'
        return it->second.loc->lr_number;
      }

      // Attribute not found
      return 0;
    }

    /**
     * Checks if a member is a static data member of a class or a structure.
     *
     * @return @em True if the member is a static data member, @em false
     *   otherwise.
     */
    bool isStatic()
    {
      // Static data members should have the external attribute defined
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_external);

      if (it != m_attributes.end())
      { // Attribute found, no need to check the value, should always be true
        return true;
      }

      // Attribute not found, the member is a non-static data member
      return false;
    }
};

/**
 * @brief A class representing a DWARF pointer type debugging information entry.
 *
 * Represents a DWARF pointer type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-10-05
 * @version   0.1.1
 */
class DwPointerType
  : public DwTag< DwPointerType, DW_TAG_pointer_type >
{
  public: // Constructors
    DwPointerType();
    DwPointerType(Dwarf_Die& die);
    DwPointerType(const DwPointerType& pt);
  public: // Destructors
    virtual ~DwPointerType();
  public: // Inline member methods
    /**
     * Gets a size in bytes of a pointer data type.
     *
     * @return The size in bytes of the pointer data type or @em 0 if the size
     *   cannot be determined.
     */
    Dwarf_Unsigned getSize()
    {
      // Get the attribute holding the size of the pointer data type
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_byte_size);

      if (it != m_attributes.end())
      { // Attribute found, return the size of the pointer data type
        return it->second.udata;
      }

      // Attribute not found
      return 0;
    }
};

/**
 * @brief A class representing a DWARF reference type debugging information
 *   entry.
 *
 * Represents a DWARF reference type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwReferenceType
  : public DwTag< DwReferenceType, DW_TAG_reference_type >
{
  public: // Constructors
    DwReferenceType();
    DwReferenceType(Dwarf_Die& die);
    DwReferenceType(const DwReferenceType& rt);
  public: // Destructors
    virtual ~DwReferenceType();
};

/**
 * @brief A class representing a DWARF compile unit debugging information entry.
 *
 * Represents a DWARF compile unit debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-03
 * @date      Last Update 2011-09-09
 * @version   0.1.2.2
 */
class DwCompileUnit
  : public DwTag< DwCompileUnit, DW_TAG_compile_unit >
{
  protected: // Retrieved variables
    Dwarf_Off m_globalOffset; //!< A global offset of the DWARF CU.
    Dwarf_Off m_length; //!< A length of the DWARF CU.
    /**
     * @brief A list containing all source files referenced in the DWARF CU.
     */
    Dwarf_Source_File_List m_srcFileList;
  public: // Constructors
    DwCompileUnit();
    DwCompileUnit(Dwarf_Die& die);
    DwCompileUnit(const DwCompileUnit& cu);
  public: // Destructors
    virtual ~DwCompileUnit();
  public: // Member methods
    /**
     * Gets a global offset of a DWARF CU in a DWARF debug info section.
     *
     * @return The global offset of the DWARF CU in a DWARF debug info section.
     */
    Dwarf_Off getGlobalOffset() { return m_globalOffset; }

    /**
     * Gets a length of a DWARF CU.
     *
     * @return The length of the DWARF CU.
     */
    Dwarf_Off getLength() { return m_length; }

    /**
     * Gets a list containing all source files referenced in a DWARF CU.
     *
     * @return A list containing all source files referenced in the DWARF CU.
     */
    const Dwarf_Source_File_List& getSourceFiles() { return m_srcFileList; }

    /**
     * Gets a current working directory of a compilation command which produced
     *   a DWARF CU.
     *
     * @return The current working directory of the compilation command which
     *   produced this DWARF CU or @em NULL if the current working directory of
     *   the compilation command which produced this DWARF CU was not found.
     */
    const char* getCompDir()
    {
      // Get the attribute holding the directory of the compilation command
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_comp_dir);

      if (it != m_attributes.end())
      { // Attribute found, return the directory of the compilation command
        return it->second.string;
      }

      // Attribute not found
      return NULL;
    }
};

/**
 * @brief A class representing a DWARF structure type debugging information
 *   entry.
 *
 * Represents a DWARF structure type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-12-07
 * @version   0.2
 */
class DwStructureType
  : public DwCompoundType< DwStructureType, DW_TAG_structure_type >
{
  public: // Constructors
    DwStructureType();
    DwStructureType(Dwarf_Die& die);
    DwStructureType(const DwStructureType& st);
  public: // Destructors
    virtual ~DwStructureType();
};

/**
 * @brief A class representing a DWARF subroutine type debugging information
 *   entry.
 *
 * Represents a DWARF subroutine type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwSubroutineType
  : public DwTag< DwSubroutineType, DW_TAG_subroutine_type >
{
  public: // Constructors
    DwSubroutineType();
    DwSubroutineType(Dwarf_Die& die);
    DwSubroutineType(const DwSubroutineType& st);
  public: // Destructors
    virtual ~DwSubroutineType();
};

/**
 * @brief A class representing a DWARF typedef debugging information entry.
 *
 * Represents a DWARF typedef debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwTypedef
  : public DwTag< DwTypedef, DW_TAG_typedef >
{
  public: // Constructors
    DwTypedef();
    DwTypedef(Dwarf_Die& die);
    DwTypedef(const DwTypedef& t);
  public: // Destructors
    virtual ~DwTypedef();
};

/**
 * @brief A class representing a DWARF union type debugging information entry.
 *
 * Represents a DWARF union type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-12-07
 * @version   0.2
 */
class DwUnionType
  : public DwCompoundType< DwUnionType, DW_TAG_union_type >
{
  public: // Constructors
    DwUnionType();
    DwUnionType(Dwarf_Die& die);
    DwUnionType(const DwUnionType& ut);
  public: // Destructors
    virtual ~DwUnionType();
};

/**
 * @brief A class representing a DWARF unspecified parameters debugging
 *   information entry.
 *
 * Represents a DWARF unspecified parameters debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwUnspecifiedParameters
  : public DwTag< DwUnspecifiedParameters, DW_TAG_unspecified_parameters >
{
  public: // Constructors
    DwUnspecifiedParameters();
    DwUnspecifiedParameters(Dwarf_Die& die);
    DwUnspecifiedParameters(const DwUnspecifiedParameters& up);
  public: // Destructors
    virtual ~DwUnspecifiedParameters();
};

/**
 * @brief A class representing a DWARF inheritance debugging information entry.
 *
 * Represents a DWARF inheritance debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwInheritance
  : public DwTag< DwInheritance, DW_TAG_inheritance >
{
  public: // Constructors
    DwInheritance();
    DwInheritance(Dwarf_Die& die);
    DwInheritance(const DwInheritance& i);
  public: // Destructors
    virtual ~DwInheritance();
};

/**
 * @brief A class representing a DWARF inlined subroutine debugging information
 *   entry.
 *
 * Represents a DWARF inlined subroutine debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-04
 * @date      Last Update 2012-02-04
 * @version   0.1
 */
class DwInlinedSubroutine
  : public DwTag< DwInlinedSubroutine, DW_TAG_inlined_subroutine >
{
  public: // Constructors
    DwInlinedSubroutine();
    DwInlinedSubroutine(Dwarf_Die& die);
    DwInlinedSubroutine(const DwInlinedSubroutine& is);
  public: // Destructors
    virtual ~DwInlinedSubroutine();
};

/**
 * @brief A class representing a DWARF pointer to member type debugging
 *   information entry.
 *
 * Represents a DWARF pointer to member type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwPointerToMemberType
  : public DwTag< DwPointerToMemberType, DW_TAG_ptr_to_member_type >
{
  public: // Constructors
    DwPointerToMemberType();
    DwPointerToMemberType(Dwarf_Die& die);
    DwPointerToMemberType(const DwPointerToMemberType& ptmt);
  public: // Destructors
    virtual ~DwPointerToMemberType();
};

/**
 * @brief A class representing a DWARF subrange type debugging information
 *   entry.
 *
 * Represents a DWARF subrange type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2013-03-07
 * @version   0.2
 */
class DwSubrangeType
  : public DwTag< DwSubrangeType, DW_TAG_subrange_type >
{
  public: // Constructors
    DwSubrangeType();
    DwSubrangeType(Dwarf_Die& die);
    DwSubrangeType(const DwSubrangeType& st);
  public: // Destructors
    virtual ~DwSubrangeType();
  public: // Inline member methods
    /**
     * Gets the number of elements in a subrange.
     *
     * @return The number of elements in the subrange.
     */
    Dwarf_Unsigned getCount()
    {
      // Get the attribute holding the index of the last element (more frequent)
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_upper_bound);

      if (it != m_attributes.end())
      { // Attribute found, as C/C++ starts indexing from 0, add 1 to the index
        return it->second.udata + 1;
      }

      // Attribute not found, use attribute holding the number of elements
      it = m_attributes.find(DW_AT_count);

      if (it != m_attributes.end())
      { // Attribute found, as the index, this should be unsigned number
        return it->second.udata;
      }

      // Attribute not found, assume subrange consisting of a single element
      return 1;
    }
};

/**
 * @brief A class representing a DWARF base type debugging information entry.
 *
 * Represents a DWARF base type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-10-14
 * @version   0.1.2
 */
class DwBaseType
  : public DwTag< DwBaseType, DW_TAG_base_type >
{
  public: // Constructors
    DwBaseType();
    DwBaseType(Dwarf_Die& die);
    DwBaseType(const DwBaseType& bt);
  public: // Destructors
    virtual ~DwBaseType();
  public: // Inline member methods
    /**
     * Gets a size in bytes of a base data type.
     *
     * @return The size in bytes of the base data type or @em 0 if the size
     *   cannot be determined.
     */
    Dwarf_Unsigned getSize()
    {
      // Get the attribute holding the size of the base data type
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_byte_size);

      if (it != m_attributes.end())
      { // Attribute found, return the size of the base data type
        return it->second.udata;
      }

      // Attribute not found
      return 0;
    }

    /**
     * Gets the encoding a base data type.
     *
     * @return The encoding of the base data type or @em 0 if the encoding
     *   cannot be determined.
     */
    Dwarf_Unsigned getEncoding()
    {
      // Get the attribute holding the size of the encoding
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_encoding);

      if (it != m_attributes.end())
      { // Attribute found, return the size of the encoding
        return it->second.udata;
      }

      // Attribute not found
      return 0;
    }
};

/**
 * @brief A class representing a DWARF constant type debugging information
 *   entry.
 *
 * Represents a DWARF constant type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwConstType
  : public DwTag< DwConstType, DW_TAG_const_type >
{
  public: // Constructors
    DwConstType();
    DwConstType(Dwarf_Die& die);
    DwConstType(const DwConstType& ct);
  public: // Destructors
    virtual ~DwConstType();
};

/**
 * @brief A class representing a DWARF enumerator debugging information entry.
 *
 * Represents a DWARF enumerator debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-31
 * @date      Last Update 2011-08-31
 * @version   0.1
 */
class DwEnumerator
  : public DwTag< DwEnumerator, DW_TAG_enumerator >
{
  public: // Constructors
    DwEnumerator();
    DwEnumerator(Dwarf_Die& die);
    DwEnumerator(const DwEnumerator& e);
  public: // Destructors
    virtual ~DwEnumerator();
};

/**
 * @brief A class representing a DWARF subprogram debugging information entry.
 *
 * Represents a DWARF subprogram debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-12-05
 * @version   0.1.3.2
 */
class DwSubprogram
  : public DwTag< DwSubprogram, DW_TAG_subprogram >
{
  public: // Constructors
    DwSubprogram();
    DwSubprogram(Dwarf_Die& die);
    DwSubprogram(const DwSubprogram& s);
  public: // Destructors
    virtual ~DwSubprogram();
  public: // Member methods
    DwDie* findDataObject(Dwarf_Addr accessedAddr, Dwarf_Addr insAddr,
      DwRegisters& registers, unsigned int* offset = NULL);
  public: // Inline member methods
    /**
     * Gets a relocated address of the first machine instruction generated for
     *   a subroutine (subprogram).
     *
     * @return The relocated address of the first machine instruction generated
     *   for the subroutine (subprogram) or @em 0 if the relocated address was
     *   not found.
     */
    Dwarf_Addr getLowPC()
    {
      // Get the attribute holding the relocated address
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_low_pc);

      if (it != m_attributes.end())
      { // Attribute found, return the relocate address
        return it->second.addr;
      }

      // Attribute not found
      return 0;
    }

    /**
     * Gets a relocated address of the first location past the last machine
     *   instruction generated for a subroutine (subprogram).
     *
     * @return The relocated address of the first location past the last machine
     *   instruction generated for the subroutine (subprogram) or @em 0 if the
     *   relocated address was not found.
     */
    Dwarf_Addr getHighPC()
    {
      // Get the attribute holding the relocated address
      Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_high_pc);

      if (it != m_attributes.end())
      { // Attribute found, return the relocate address
        return it->second.addr;
      }

      // Attribute not found
      return 0;
    }
  private: // Internal member methods
    Dwarf_Addr getFrameBaseAddress(Dwarf_Addr insAddr, DwRegisters& registers);
};

/**
 * @brief A class representing a DWARF template type parameter debugging
 *   information entry.
 *
 * Represents a DWARF template type parameter debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwTemplateTypeParameter
  : public DwTag< DwTemplateTypeParameter, DW_TAG_template_type_parameter >
{
  public: // Constructors
    DwTemplateTypeParameter();
    DwTemplateTypeParameter(Dwarf_Die& die);
    DwTemplateTypeParameter(const DwTemplateTypeParameter& ttp);
  public: // Destructors
    virtual ~DwTemplateTypeParameter();
};

/**
 * @brief A class representing a DWARF template value parameter debugging
 *   information entry.
 *
 * Represents a DWARF template value parameter debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwTemplateValueParameter
  : public DwTag< DwTemplateValueParameter, DW_TAG_template_value_parameter >
{
  public: // Constructors
    DwTemplateValueParameter();
    DwTemplateValueParameter(Dwarf_Die& die);
    DwTemplateValueParameter(const DwTemplateValueParameter& tvp);
  public: // Destructors
    virtual ~DwTemplateValueParameter();
};

/**
 * @brief A class representing a DWARF variable debugging information entry.
 *
 * Represents a DWARF variable debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2011-12-05
 * @version   0.2.2
 */
class DwVariable
  : public DwDataObject< DwVariable, DW_TAG_variable >
{
  public: // Constructors
    DwVariable();
    DwVariable(Dwarf_Die& die);
    DwVariable(const DwVariable& v);
  public: // Destructors
    virtual ~DwVariable();
  public: // Member methods
    bool isGlobal();
};

/**
 * @brief A class representing a DWARF volatile type debugging information
 *   entry.
 *
 * Represents a DWARF volatile type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-11-21
 * @date      Last Update 2011-11-21
 * @version   0.1
 */
class DwVolatileType
  : public DwTag< DwVolatileType, DW_TAG_volatile_type >
{
  public: // Constructors
    DwVolatileType();
    DwVolatileType(Dwarf_Die& die);
    DwVolatileType(const DwVolatileType& vt);
  public: // Destructors
    virtual ~DwVolatileType();
};

/**
 * @brief A class representing a DWARF namespace debugging information entry.
 *
 * Represents a DWARF namespace debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwNamespace
  : public DwTag< DwNamespace, DW_TAG_namespace >
{
  public: // Constructors
    DwNamespace();
    DwNamespace(Dwarf_Die& die);
    DwNamespace(const DwNamespace& n);
  public: // Destructors
    virtual ~DwNamespace();
};

/**
 * @brief A class representing a DWARF imported module debugging information
 *   entry.
 *
 * Represents a DWARF imported module debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-30
 * @date      Last Update 2011-08-30
 * @version   0.1
 */
class DwImportedModule
  : public DwTag< DwImportedModule, DW_TAG_imported_module >
{
  public: // Constructors
    DwImportedModule();
    DwImportedModule(Dwarf_Die& die);
    DwImportedModule(const DwImportedModule& im);
  public: // Destructors
    virtual ~DwImportedModule();
};

/**
 * @brief A class representing a DWARF unspecified type debugging information
 *   entry.
 *
 * Represents a DWARF unspecified type debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-02-04
 * @date      Last Update 2012-02-04
 * @version   0.1
 */
class DwUnspecifiedType
  : public DwTag< DwUnspecifiedType, DW_TAG_unspecified_type >
{
  public: // Constructors
    DwUnspecifiedType();
    DwUnspecifiedType(Dwarf_Die& die);
    DwUnspecifiedType(const DwUnspecifiedType& ut);
  public: // Destructors
    virtual ~DwUnspecifiedType();
};

/**
 * @brief A class representing a DWARF GNU call site debugging information
 *   entry.
 *
 * Represents a DWARF GNU call site debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-26
 * @date      Last Update 2013-03-26
 * @version   0.1
 */
class DwGnuCallSite : public DwTag< DwGnuCallSite, DW_TAG_GNU_call_site >
{
  public: // Constructors
    DwGnuCallSite();
    DwGnuCallSite(Dwarf_Die& die);
    DwGnuCallSite(const DwGnuCallSite& cs);
  public: // Destructors
    virtual ~DwGnuCallSite();
};

/**
 * @brief A class representing a DWARF GNU call site parameter debugging
 *   information entry.
 *
 * Represents a DWARF GNU call site parameter debugging information entry.
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2013-03-26
 * @date      Last Update 2013-03-26
 * @version   0.1
 */
class DwGnuCallSiteParameter
  : public DwTag< DwGnuCallSiteParameter, DW_TAG_GNU_call_site_parameter >
{
  public: // Constructors
    DwGnuCallSiteParameter();
    DwGnuCallSiteParameter(Dwarf_Die& die);
    DwGnuCallSiteParameter(const DwGnuCallSiteParameter& csp);
  public: // Destructors
    virtual ~DwGnuCallSiteParameter();
};

/**
 * @brief A class for creating DWARF debugging information entries at runtime.
 *
 * Creates DWARF debugging information entry objects representing specific DWARF
 *   tags at runtime by calling the DwDie::create(Dwarf_Die&,Dwarf_Debug&)
 *   method of the registered reference objects (concrete objects representing
 *   the DWARF tags).
 *
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-25
 * @date      Last Update 2011-08-30
 * @version   0.1.3
 */
class DwDieFactory
{
  public: // Static constants
    static const int OK = 0; //!< Signals that no error occurred.
    /**
     * @brief Signals that some object representing the same DWARF tag as the
     *   specified object is already registered.
     */
    static const int ALREADY_REGISTERED = 1;
  private:
    /**
     * @brief A map containing all registered reference objects.
     */
    std::map< Dwarf_Half, boost::shared_ptr< DwDie > > m_registeredTags;
  public: // Constructors
    DwDieFactory();
  public: // Member methods
    int registerTag(DwDie *tag);
    DwDie* createTag(Dwarf_Half tag, Dwarf_Die& die, DwDie *parent = NULL);
};

#endif /* __LIBDIE__DWARF__DW_CLASSES_H__ */

/** End of file dw_classes.h **/
