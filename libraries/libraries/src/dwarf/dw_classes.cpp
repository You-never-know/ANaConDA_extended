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
 * @brief A file containing implementation of classes representing DWARF
 *   entries.
 *
 * A file containing implementation of classes representing various DWARF
 *   entries.
 *
 * @file      dw_classes.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-02
 * @date      Last Update 2013-03-26
 * @version   0.6
 */

#include "dw_classes.h"

#include <iomanip>

#include "boost/assign/list_of.hpp"
#include "boost/format.hpp"

#include "libdwarf/config.h"
#include "libdwarf/dwarf_base_types.h"
#include "libdwarf/dwarf_alloc.h"
#include "libdwarf/dwarf_opaque.h"

namespace
{ // Static global variables (usable only within this module)
  std::map< unsigned char, const char* >
    g_locOpToStringTable = boost::assign::map_list_of
      (0x03, "DW_OP_addr")
      (0x06, "DW_OP_deref")
      (0x08, "DW_OP_const1u")
      (0x09, "DW_OP_const1s")
      (0x0a, "DW_OP_const2u")
      (0x0b, "DW_OP_const2s")
      (0x0c, "DW_OP_const4u")
      (0x0d, "DW_OP_const4s")
      (0x0e, "DW_OP_const8u")
      (0x0f, "DW_OP_const8s")
      (0x10, "DW_OP_constu")
      (0x11, "DW_OP_consts")
      (0x12, "DW_OP_dup")
      (0x13, "DW_OP_drop")
      (0x14, "DW_OP_over")
      (0x15, "DW_OP_pick")
      (0x16, "DW_OP_swap")
      (0x17, "DW_OP_rot")
      (0x18, "DW_OP_xderef")
      (0x19, "DW_OP_abs")
      (0x1a, "DW_OP_and")
      (0x1b, "DW_OP_div")
      (0x1c, "DW_OP_minus")
      (0x1d, "DW_OP_mod")
      (0x1e, "DW_OP_mul")
      (0x1f, "DW_OP_neg")
      (0x20, "DW_OP_not")
      (0x21, "DW_OP_or")
      (0x22, "DW_OP_plus")
      (0x23, "DW_OP_plus_uconst")
      (0x24, "DW_OP_shl")
      (0x25, "DW_OP_shr")
      (0x26, "DW_OP_shra")
      (0x27, "DW_OP_xor")
      (0x28, "DW_OP_bra")
      (0x29, "DW_OP_eq")
      (0x2a, "DW_OP_ge")
      (0x2b, "DW_OP_gt")
      (0x2c, "DW_OP_le")
      (0x2d, "DW_OP_lt")
      (0x2e, "DW_OP_ne")
      (0x2f, "DW_OP_skip")
      (0x30, "DW_OP_lit0")
      (0x31, "DW_OP_lit1")
      (0x32, "DW_OP_lit2")
      (0x33, "DW_OP_lit3")
      (0x34, "DW_OP_lit4")
      (0x35, "DW_OP_lit5")
      (0x36, "DW_OP_lit6")
      (0x37, "DW_OP_lit7")
      (0x38, "DW_OP_lit8")
      (0x39, "DW_OP_lit9")
      (0x3a, "DW_OP_lit10")
      (0x3b, "DW_OP_lit11")
      (0x3c, "DW_OP_lit12")
      (0x3d, "DW_OP_lit13")
      (0x3e, "DW_OP_lit14")
      (0x3f, "DW_OP_lit15")
      (0x40, "DW_OP_lit16")
      (0x41, "DW_OP_lit17")
      (0x42, "DW_OP_lit18")
      (0x43, "DW_OP_lit19")
      (0x44, "DW_OP_lit20")
      (0x45, "DW_OP_lit21")
      (0x46, "DW_OP_lit22")
      (0x47, "DW_OP_lit23")
      (0x48, "DW_OP_lit24")
      (0x49, "DW_OP_lit25")
      (0x4a, "DW_OP_lit26")
      (0x4b, "DW_OP_lit27")
      (0x4c, "DW_OP_lit28")
      (0x4d, "DW_OP_lit29")
      (0x4e, "DW_OP_lit30")
      (0x4f, "DW_OP_lit31")
      (0x50, "DW_OP_reg0")
      (0x51, "DW_OP_reg1")
      (0x52, "DW_OP_reg2")
      (0x53, "DW_OP_reg3")
      (0x54, "DW_OP_reg4")
      (0x55, "DW_OP_reg5")
      (0x56, "DW_OP_reg6")
      (0x57, "DW_OP_reg7")
      (0x58, "DW_OP_reg8")
      (0x59, "DW_OP_reg9")
      (0x5a, "DW_OP_reg10")
      (0x5b, "DW_OP_reg11")
      (0x5c, "DW_OP_reg12")
      (0x5d, "DW_OP_reg13")
      (0x5e, "DW_OP_reg14")
      (0x5f, "DW_OP_reg15")
      (0x60, "DW_OP_reg16")
      (0x61, "DW_OP_reg17")
      (0x62, "DW_OP_reg18")
      (0x63, "DW_OP_reg19")
      (0x64, "DW_OP_reg20")
      (0x65, "DW_OP_reg21")
      (0x66, "DW_OP_reg22")
      (0x67, "DW_OP_reg23")
      (0x68, "DW_OP_reg24")
      (0x69, "DW_OP_reg25")
      (0x6a, "DW_OP_reg26")
      (0x6b, "DW_OP_reg27")
      (0x6c, "DW_OP_reg28")
      (0x6d, "DW_OP_reg29")
      (0x6e, "DW_OP_reg30")
      (0x6f, "DW_OP_reg31")
      (0x70, "DW_OP_breg0")
      (0x71, "DW_OP_breg1")
      (0x72, "DW_OP_breg2")
      (0x73, "DW_OP_breg3")
      (0x74, "DW_OP_breg4")
      (0x75, "DW_OP_breg5")
      (0x76, "DW_OP_breg6")
      (0x77, "DW_OP_breg7")
      (0x78, "DW_OP_breg8")
      (0x79, "DW_OP_breg9")
      (0x7a, "DW_OP_breg10")
      (0x7b, "DW_OP_breg11")
      (0x7c, "DW_OP_breg12")
      (0x7d, "DW_OP_breg13")
      (0x7e, "DW_OP_breg14")
      (0x7f, "DW_OP_breg15")
      (0x80, "DW_OP_breg16")
      (0x81, "DW_OP_breg17")
      (0x82, "DW_OP_breg18")
      (0x83, "DW_OP_breg19")
      (0x84, "DW_OP_breg20")
      (0x85, "DW_OP_breg21")
      (0x86, "DW_OP_breg22")
      (0x87, "DW_OP_breg23")
      (0x88, "DW_OP_breg24")
      (0x89, "DW_OP_breg25")
      (0x8a, "DW_OP_breg26")
      (0x8b, "DW_OP_breg27")
      (0x8c, "DW_OP_breg28")
      (0x8d, "DW_OP_breg29")
      (0x8e, "DW_OP_breg30")
      (0x8f, "DW_OP_breg31")
      (0x90, "DW_OP_regx")
      (0x91, "DW_OP_fbreg")
      (0x92, "DW_OP_bregx")
      (0x93, "DW_OP_piece")
      (0x94, "DW_OP_deref_size")
      (0x95, "DW_OP_xderef_size")
      (0x96, "DW_OP_nop")
      (0x97, "DW_OP_push_object_address")
      (0x98, "DW_OP_call2")
      (0x99, "DW_OP_call4")
      (0x9a, "DW_OP_call_ref")
      (0x9b, "DW_OP_form_tls_address")
      (0x9c, "DW_OP_call_frame_cfa")
      (0x9d, "DW_OP_bit_piece")
      (0x9e, "DW_OP_implicit_value")
      (0x9f, "DW_OP_stack_value")
      (0xe0, "DW_OP_GNU_push_tls_address")
      (0xe0, "DW_OP_lo_user")
      (0xe0, "DW_OP_HP_unknown")
      (0xe1, "DW_OP_HP_is_value")
      (0xe2, "DW_OP_HP_fltconst4")
      (0xe3, "DW_OP_HP_fltconst8")
      (0xe4, "DW_OP_HP_mod_range")
      (0xe5, "DW_OP_HP_unmod_range")
      (0xe6, "DW_OP_HP_tls")
      (0xe8, "DW_OP_INTEL_bit_piece")
      (0xf0, "DW_OP_APPLE_uninit")
      (0xff, "DW_OP_hi_user");

  const int NO_OPERAND = 1;
  const int SIGNED_CONSTANT = 2;
  const int UNSIGNED_CONSTANT = 4;
  const int ADDRESS = 8; // Unsigned constant
  const int REGISTER = 16; // Unsigned constant
  const int OFFSET = 32; // Signed constant
  const int REGISTER_AND_OFFSET = 64;
  const int STACK_INDEX = 128; // Unsigned constant
  const int SIZE = 256; // Unsigned constant

  std::map< unsigned char, int >
    g_locOpToTypeTable = boost::assign::map_list_of
      (DW_OP_addr, ADDRESS)
      (DW_OP_deref, NO_OPERAND)
      (DW_OP_const1u, UNSIGNED_CONSTANT)
      (DW_OP_const1s, SIGNED_CONSTANT)
      (DW_OP_const2u, UNSIGNED_CONSTANT)
      (DW_OP_const2s, SIGNED_CONSTANT)
      (DW_OP_const4u, UNSIGNED_CONSTANT)
      (DW_OP_const4s, SIGNED_CONSTANT)
      (DW_OP_const8u, UNSIGNED_CONSTANT)
      (DW_OP_const8s, SIGNED_CONSTANT)
      (DW_OP_constu, UNSIGNED_CONSTANT)
      (DW_OP_consts, SIGNED_CONSTANT)
      (DW_OP_dup, NO_OPERAND)
      (DW_OP_drop, NO_OPERAND)
      (DW_OP_over, NO_OPERAND)
      (DW_OP_pick, STACK_INDEX)
      (DW_OP_swap, NO_OPERAND)
      (DW_OP_rot, NO_OPERAND)
      (DW_OP_xderef, NO_OPERAND)
      (DW_OP_abs, NO_OPERAND)
      (DW_OP_and, NO_OPERAND)
      (DW_OP_div, NO_OPERAND)
      (DW_OP_minus, NO_OPERAND)
      (DW_OP_mod, NO_OPERAND)
      (DW_OP_mul, NO_OPERAND)
      (DW_OP_neg, NO_OPERAND)
      (DW_OP_not, NO_OPERAND)
      (DW_OP_or, NO_OPERAND)
      (DW_OP_plus, NO_OPERAND)
      (DW_OP_plus_uconst, UNSIGNED_CONSTANT)
      (DW_OP_shl, NO_OPERAND)
      (DW_OP_shr, NO_OPERAND)
      (DW_OP_shra, NO_OPERAND)
      (DW_OP_xor, NO_OPERAND)
      (DW_OP_bra, SIGNED_CONSTANT)
      (DW_OP_eq, NO_OPERAND)
      (DW_OP_ge, NO_OPERAND)
      (DW_OP_gt, NO_OPERAND)
      (DW_OP_le, NO_OPERAND)
      (DW_OP_lt, NO_OPERAND)
      (DW_OP_ne, NO_OPERAND)
      (DW_OP_skip, SIGNED_CONSTANT)
      (DW_OP_lit0, NO_OPERAND)
      (DW_OP_lit1, NO_OPERAND)
      (DW_OP_lit2, NO_OPERAND)
      (DW_OP_lit3, NO_OPERAND)
      (DW_OP_lit4, NO_OPERAND)
      (DW_OP_lit5, NO_OPERAND)
      (DW_OP_lit6, NO_OPERAND)
      (DW_OP_lit7, NO_OPERAND)
      (DW_OP_lit8, NO_OPERAND)
      (DW_OP_lit9, NO_OPERAND)
      (DW_OP_lit10, NO_OPERAND)
      (DW_OP_lit11, NO_OPERAND)
      (DW_OP_lit12, NO_OPERAND)
      (DW_OP_lit13, NO_OPERAND)
      (DW_OP_lit14, NO_OPERAND)
      (DW_OP_lit15, NO_OPERAND)
      (DW_OP_lit16, NO_OPERAND)
      (DW_OP_lit17, NO_OPERAND)
      (DW_OP_lit18, NO_OPERAND)
      (DW_OP_lit19, NO_OPERAND)
      (DW_OP_lit20, NO_OPERAND)
      (DW_OP_lit21, NO_OPERAND)
      (DW_OP_lit22, NO_OPERAND)
      (DW_OP_lit23, NO_OPERAND)
      (DW_OP_lit24, NO_OPERAND)
      (DW_OP_lit25, NO_OPERAND)
      (DW_OP_lit26, NO_OPERAND)
      (DW_OP_lit27, NO_OPERAND)
      (DW_OP_lit28, NO_OPERAND)
      (DW_OP_lit29, NO_OPERAND)
      (DW_OP_lit30, NO_OPERAND)
      (DW_OP_lit31, NO_OPERAND)
      (DW_OP_reg0, NO_OPERAND)
      (DW_OP_reg1, NO_OPERAND)
      (DW_OP_reg2, NO_OPERAND)
      (DW_OP_reg3, NO_OPERAND)
      (DW_OP_reg4, NO_OPERAND)
      (DW_OP_reg5, NO_OPERAND)
      (DW_OP_reg6, NO_OPERAND)
      (DW_OP_reg7, NO_OPERAND)
      (DW_OP_reg8, NO_OPERAND)
      (DW_OP_reg9, NO_OPERAND)
      (DW_OP_reg10, NO_OPERAND)
      (DW_OP_reg11, NO_OPERAND)
      (DW_OP_reg12, NO_OPERAND)
      (DW_OP_reg13, NO_OPERAND)
      (DW_OP_reg14, NO_OPERAND)
      (DW_OP_reg15, NO_OPERAND)
      (DW_OP_reg16, NO_OPERAND)
      (DW_OP_reg17, NO_OPERAND)
      (DW_OP_reg18, NO_OPERAND)
      (DW_OP_reg19, NO_OPERAND)
      (DW_OP_reg20, NO_OPERAND)
      (DW_OP_reg21, NO_OPERAND)
      (DW_OP_reg22, NO_OPERAND)
      (DW_OP_reg23, NO_OPERAND)
      (DW_OP_reg24, NO_OPERAND)
      (DW_OP_reg25, NO_OPERAND)
      (DW_OP_reg26, NO_OPERAND)
      (DW_OP_reg27, NO_OPERAND)
      (DW_OP_reg28, NO_OPERAND)
      (DW_OP_reg29, NO_OPERAND)
      (DW_OP_reg30, NO_OPERAND)
      (DW_OP_reg31, NO_OPERAND)
      (DW_OP_breg0, OFFSET)
      (DW_OP_breg1, OFFSET)
      (DW_OP_breg2, OFFSET)
      (DW_OP_breg3, OFFSET)
      (DW_OP_breg4, OFFSET)
      (DW_OP_breg5, OFFSET)
      (DW_OP_breg6, OFFSET)
      (DW_OP_breg7, OFFSET)
      (DW_OP_breg8, OFFSET)
      (DW_OP_breg9, OFFSET)
      (DW_OP_breg10, OFFSET)
      (DW_OP_breg11, OFFSET)
      (DW_OP_breg12, OFFSET)
      (DW_OP_breg13, OFFSET)
      (DW_OP_breg14, OFFSET)
      (DW_OP_breg15, OFFSET)
      (DW_OP_breg16, OFFSET)
      (DW_OP_breg17, OFFSET)
      (DW_OP_breg18, OFFSET)
      (DW_OP_breg19, OFFSET)
      (DW_OP_breg20, OFFSET)
      (DW_OP_breg21, OFFSET)
      (DW_OP_breg22, OFFSET)
      (DW_OP_breg23, OFFSET)
      (DW_OP_breg24, OFFSET)
      (DW_OP_breg25, OFFSET)
      (DW_OP_breg26, OFFSET)
      (DW_OP_breg27, OFFSET)
      (DW_OP_breg28, OFFSET)
      (DW_OP_breg29, OFFSET)
      (DW_OP_breg30, OFFSET)
      (DW_OP_breg31, OFFSET)
      (DW_OP_regx, REGISTER)
      (DW_OP_fbreg, OFFSET)
      (DW_OP_bregx, REGISTER_AND_OFFSET)
      (DW_OP_piece, SIZE)
      (DW_OP_deref_size, SIZE)
      (DW_OP_xderef_size, SIZE)
      (DW_OP_nop, NO_OPERAND)
      (DW_OP_push_object_address, ADDRESS)
      (DW_OP_call2, NO_OPERAND)
      (DW_OP_call4, NO_OPERAND)
      (DW_OP_call_ref, NO_OPERAND)
      (DW_OP_form_tls_address, ADDRESS)
      (DW_OP_call_frame_cfa, NO_OPERAND)
      (DW_OP_bit_piece, SIZE)
      (DW_OP_implicit_value, NO_OPERAND)
      (DW_OP_stack_value, NO_OPERAND)
      (DW_OP_GNU_push_tls_address, ADDRESS)
      (DW_OP_lo_user, NO_OPERAND)
      (DW_OP_HP_unknown, NO_OPERAND)
      (DW_OP_HP_is_value, NO_OPERAND)
      (DW_OP_HP_fltconst4, NO_OPERAND)
      (DW_OP_HP_fltconst8, NO_OPERAND)
      (DW_OP_HP_mod_range, NO_OPERAND)
      (DW_OP_HP_unmod_range, NO_OPERAND)
      (DW_OP_HP_tls, NO_OPERAND)
      (DW_OP_INTEL_bit_piece, SIZE)
      (DW_OP_APPLE_uninit, NO_OPERAND)
      (DW_OP_hi_user, NO_OPERAND);
}

/**
 * Prints a DWARF location record to a stream.
 *
 * @param s A stream to which the location record should be printed.
 * @param value A DWARF location record.
 * @return The stream to which was the location record printed.
 */
std::ostream& operator<<(std::ostream& s, const Dwarf_Loc& value)
{
  switch (g_locOpToTypeTable[value.lr_atom])
  { // Print the location record to the stream based on its type
    case NO_OPERAND: // No operand
      s << g_locOpToStringTable[value.lr_atom];
      break;
    case UNSIGNED_CONSTANT: // One unsigned integer operand
    case REGISTER:
    case STACK_INDEX:
    case SIZE:
      s << g_locOpToStringTable[value.lr_atom] << " " << value.lr_number;
      break;
    case ADDRESS: // One unsigned integer operand representing address
      s << g_locOpToStringTable[value.lr_atom] << " 0x" << std::hex
        << value.lr_number << std::dec;
      break;
    case SIGNED_CONSTANT: // One signed integer operand
    case OFFSET:
      s << boost::format("%1% %2$+d") % g_locOpToStringTable[value.lr_atom]
        % (Dwarf_Signed)value.lr_number;
      break;
    case REGISTER_AND_OFFSET: // Two operands, unsigned and signed integers
      s << boost::format("%1% %2% %3$+d") % g_locOpToStringTable[value.lr_atom]
        % value.lr_number % (Dwarf_Signed)value.lr_number2;
      break;
    default: // Invalid location operation type (should not happen)
      assert(false);
      break;
  }

  // Return the stream to which was the location record printed
  return s;
}

/**
 * Prints a DWARF location description to a stream.
 *
 * @param s A stream to which the location description should be printed.
 * @param value A DWARF location description.
 * @return The stream to which was the location description printed.
 */
std::ostream& operator<<(std::ostream& s, const Dwarf_Locdesc& value)
{
  // Print the location description to the stream
  s << "<lowpc=0x" << std::hex << value.ld_lopc << std::dec << "><highpc=0x"
    << std::hex << value.ld_hipc << ">" << value.ld_s[0];

  // Return the stream to which was the location description printed
  return s;
}

/**
 * Prints a value of a DWARF debugging information entry attribute to a stream.
 *
 * @param s A stream to which the value of the attribute should be printed.
 * @param value A value of the attribute.
 * @return The stream to which was the value of the attribute printed.
 */
std::ostream& operator<<(std::ostream& s, const Dwarf_Attribute_Value& value)
{
  switch (value.cls)
  { // Print the value of the attribute based on its encoding (class and form)
    case DW_FORM_CLASS_ADDRESS:
      // The value of the attribute is a memory address
      s << "0x" << std::hex << value.addr << std::dec;
      break;
    case DW_FORM_CLASS_BLOCK:
      // The value of the attribute is a block of arbitrary data
      if (value.form == DW_FORM_location)
      { // The value of the attribute is a location
        s << *value.loc;
      }
      break;
    case DW_FORM_CLASS_CONSTANT:
      // The value of the attribute is an integer
      if (value.form == DW_FORM_sdata)
      { // The value of the attribute is a signed integer
        s << boost::format("%1$+d") % value.sdata;
      }
      else
      { // The value of the attribute is an unsigned integer
        s << value.udata;
      }
      break;
    case DW_FORM_CLASS_FLAG:
      // The value of the attribute is a boolean value
      s << (value.flag ? "yes" : "no") << "(" << value.flag << ")";
      break;
    case DW_FORM_CLASS_REFERENCE:
      // The value of the attribute is a reference to another DWARF DIE
      if (value.form == DW_FORM_cu_ref_obj || value.form == DW_FORM_sec_ref_obj)
      { // The value of the attribute is a reference to a DWARF DIE object
        s << "<" << value.die->getOffset() << "> [Object at <" << value.die
          << ">]";
      }
      else
      { // The value of the attribute is a section or CU-relative offset value
        s << "<" << value.ref << ">";
      }
      break;
    case DW_FORM_CLASS_STRING:
      // The value of the attribute is a string value
      s << value.string;
      break;
    case DW_FORM_CLASS_EXPRLOC:
      break;
    case DW_FORM_CLASS_LOCLISTPTR:
      // The value of the attribute is a list containing locations
      s << "<loclist with " << value.loclist->listlen << " entries follows>";

      for (int i = 0; i < value.loclist->listlen; i++)
      { // Print all the locations in the list
        s << boost::format("\n[%1%]") % boost::io::group(std::setw(2), i)
          << *value.loclist->llbuf[i];
      }
      break;
    case DW_FORM_CLASS_RANGELISTPTR:
    case DW_FORM_CLASS_LINEPTR:
    case DW_FORM_CLASS_MACPTR:
    case DW_FORM_CLASS_FRAMEPTR:
    case DW_FORM_CLASS_UNKNOWN:
      break;
    default:
      // The value of the attribute is unknown (invalid class)
      break;
  }

  // Return the stream to which was the value of the attribute printed
  return s;
}

/**
 * Evaluates a location expression, i.e., computes the address of the location.
 *
 * @param location A location expression which should be evaluated.
 * @param registers An object holding the content of the registers.
 * @param frameBase A frame base address for the instruction which is accessing
 *   the location represented by the location expression.
 * @return The address of the location represented by the location expression or
 *   @em 0 if the address cannot be evaluated.
 */
inline
Dwarf_Addr evaluateLocExpr(Dwarf_Loc& location, DwRegisters& registers,
  Dwarf_Addr frameBase = 0)
{
  if (location.lr_atom == DW_OP_fbreg)
  { // The address is 'frame base address + signed constant'
    return frameBase + (Dwarf_Signed)location.lr_number;
  }
  else if (DW_OP_reg0 <= location.lr_atom && location.lr_atom <= DW_OP_reg31)
  { // The address is in a register
    return registers.getValue(location.lr_atom - DW_OP_reg0);
  }
  else if (DW_OP_breg0 <= location.lr_atom && location.lr_atom <= DW_OP_breg31)
  { // The address is 'register + signed constant'
    return registers.getValue(location.lr_atom - DW_OP_breg0)
      + (Dwarf_Signed)location.lr_number;
  }

  // Return invalid address (NULL) if the location cannot be evaluated
  return 0;
}

/**
 * Gets a string containing information about a type represented as a tree of
 *   DWARF debugging information entry objects.
 *
 * @param die A root of the tree containing information about the type.
 * @return A string containing information about the type or an empty string if
 *   no information about the type was found.
 */
inline
std::string getTypeDeclSpec(DwDie *die)
{
  // Helper variables
  std::string base;
  std::string mods;
  std::string type;

  while (die)
  { // Process all DIE objects containing parts of the type
    switch (die->getTag())
    { // Update the type based on the part encountered (type, modifier, ...)
      case DW_TAG_const_type: // Constant modifier (store to modifiers)
        mods = "const " + mods;
        break;
      case DW_TAG_volatile_type: // Volatile modifier (store to modifiers)
        mods = "volatile " + mods;
        break;
      case DW_TAG_typedef: // Type alias (base type for anonymous bases)
        assert(die->getName() != NULL);
        base = die->getName();
        break;
      case DW_TAG_array_type: // Array type (cannot have modifiers)
        type = "[] " + type;
        break;
      case DW_TAG_pointer_type: // Pointer type (can have modifiers)
        type = "* " + mods + type;
        mods = ""; // Reset modifiers, other may be base type modifiers
        break;
      case DW_TAG_reference_type: // Reference type (have always const modifier)
        assert(mods == "const ");
        type = "& " + type; // The const modifier is not present in the source
        mods = ""; // Reset the const modifier, should apply only to reference
        break;
      case DW_TAG_union_type: // Union (may be anonymous if typedef is used)
      case DW_TAG_structure_type: // Structure (may be anonymous like union)
        if (die->getName() != NULL)
        { // Named union, use the name of it as the name of the base type
          type = mods + die->getName() + " " + type;
        }
        else
        { // Anonymous union, typedef must have been used to name the union,
          // use the name of the typedef as the name of the union (base type)
          type = mods + base + " " + type;
        }
        break;
      case DW_TAG_base_type: // Base type (can have modifiers)
      case DW_TAG_class_type: // Class (can have modifiers)
        assert(die->getName() != NULL);
        type = mods + die->getName() + " " + type;
        break;
      default: // Unknown type or modifier (should not happen)
        assert(false);
        break;
    }

    // Try to get the next part of the type
    DwDie::Dwarf_Attribute_Map::const_iterator it = die->getAttributes().find(
      DW_AT_type);

    // Move to the next part of the type
    die = (it == die->getAttributes().end()) ? NULL : it->second.die;
  }

  if (type.empty())
  { // If no type information found, return an empty string
    return type;
  }
  else if (type[0] == '*')
  { // Pointer to void type is represented as a pointer without a type in DWARF
    return mods + "void " + type.substr(0, type.size() - 1);
  }
  else
  { // If some type information found, return it
    return type.substr(0, type.size() - 1);
  }
}

/**
 * Gets the number of elements in an array represented as a tree of DWARF
 *   debugging information entry objects.
 *
 * @param die A root of the tree containing information about the array.
 * @return The number of elements in the array or @em 1 if the number could not
 *   be determined.
 */
inline
Dwarf_Unsigned getElementCount(DwDie *die)
{
  // TODO: move to DwArrayType as a method
  // Helper variables
  DwDie::Dwarf_Die_List::const_iterator it;

  for (it = die->getChildren().begin(); it != die->getChildren().end(); it++)
  { // Search all child elements for the DIE containing the number of elements
    if (it->get()->getTag() == DW_TAG_subrange_type)
    { // Subrange holds the number of elements in the array
      return static_cast< DwSubrangeType* >(it->get())->getCount();
    }
  }

  return 1;
}

/**
 * Gets a size of a type represented as a tree of DWARF debugging information
 *   entry objects.
 *
 * @param die A root of the tree containing information about the type.
 * @return The size of the type or @em 0 if the size could not be determined.
 */
inline
Dwarf_Unsigned getTypeSize(DwDie *die)
{
  while (die)
  { // Process all DIE objects containing parts of the type
    switch (die->getTag())
    { // Search for parts of the type containing information about the size
      case DW_TAG_array_type: // Array
        return getElementCount(die) * ::getTypeSize(static_cast< DwArrayType* >
          (die)->getElementType());
      case DW_TAG_class_type: // Class
        return static_cast< DwClassType* >(die)->getSize();
      case DW_TAG_pointer_type: // Pointer
        return static_cast< DwPointerType* >(die)->getSize();
      case DW_TAG_base_type: // Base type (integer, long, double, char, ...)
        return static_cast< DwBaseType* >(die)->getSize();
      default: // Modifier, typedef or other size-irrelevant part of the type
        break;
    }

    // Size of the type not found in the currently searched part of the type
    DwDie::Dwarf_Attribute_Map::const_iterator it = die->getAttributes().find(
      DW_AT_type);

    // Move to the next part of the type if some parts were not searched yet
    die = (it == die->getAttributes().end()) ? NULL : it->second.die;
  }

  return 0;
}

/**
 * Gets a type of a data from a type represented as a tree of DWARF debugging
 *   information entry objects.
 *
 * @note The type may contain various storage and access modifiers like const,
 *   volatile, static, mutable, etc. together with the type of the actual data.
 *   This function returns a DWARF debugging information entry object holding
 *   information about the type of the data.
 *
 * @param die A root of the tree containing information about the type.
 * @return The type of the data in the type or @em NULL if the type could not
 *   be determined.
 */
inline
DwDie *getDataType(DwDie *die)
{
  while (die)
  { // Process all DIE objects containing parts of the type
    switch (die->getTag())
    { // Search for the part containing information about the type of data
      case DW_TAG_array_type: // Array
      case DW_TAG_class_type: // Class
      case DW_TAG_pointer_type: // Pointer
      case DW_TAG_structure_type: // Structure
      case DW_TAG_union_type: // Union
      case DW_TAG_base_type: // Base data type (integer, double, char, ...)
        return die;
      default: // Modifier, typedef or other data-type-irrelevant part
        break;
    }

    // Data type not found in the currently searched part of the type
    DwDie::Dwarf_Attribute_Map::const_iterator it = die->getAttributes().find(
      DW_AT_type);

    // Move to the next part of the type if some parts were not searched yet
    die = (it == die->getAttributes().end()) ? NULL : it->second.die;
  }

  return NULL;
}

/**
 * Gets a tag identifying the type of the data in a type represented as a tree
 *   of DWARF debugging information entry objects.
 *
 * @note The type may contain various storage and access modifiers like const,
 *   volatile, static, mutable, etc. together with the type of the actual data.
 *   This function returns a tag identifying the type of the data.
 *
 * @param die A root of the tree containing information about the type.
 * @return The tag identifying the type of the data in the type or @em 0 if the
 *   tag could not be determined.
 */
inline
Dwarf_Half getDataTypeTag(DwDie *die)
{
  while (die)
  { // Process all DIE objects containing parts of the type
    switch (die->getTag())
    { // Search for the part containing information about the type of data
      case DW_TAG_array_type: // Array
        return DW_TAG_array_type;
      case DW_TAG_class_type: // Class
        return DW_TAG_class_type;
      case DW_TAG_pointer_type: // Pointer
        return DW_TAG_pointer_type;
      case DW_TAG_structure_type: // Structure
        return DW_TAG_structure_type;
      case DW_TAG_union_type: // Union
        return DW_TAG_union_type;
      case DW_TAG_base_type: // Base data type (integer, double, char, ...)
        return DW_TAG_base_type;
      default: // Modifier, typedef or other base-type-irrelevant part
        break;
    }

    // Data type not found in the currently searched part of the type
    DwDie::Dwarf_Attribute_Map::const_iterator it = die->getAttributes().find(
      DW_AT_type);

    // Move to the next part of the type if some parts were not searched yet
    die = (it == die->getAttributes().end()) ? NULL : it->second.die;
  }

  return 0;
}

/**
 * Gets a member of a class or a structure on a specific offset.
 *
 * @param die A DWARF debugging information entry object representing a class
 *   or a structure containing the member.
 * @param offset An offset of the member within the class or structure.
 * @return The member on the specified offset or @em NULL if no member on the
 *   specified offset was found.
 */
inline
DwMember *getMember(DwDie *die, Dwarf_Off offset)
{
  // Only classes and structures have members
  assert(die->getTag() == DW_TAG_class_type
    || die->getTag() == DW_TAG_structure_type);

  // Helper variables
  DwDie::Dwarf_Die_List::const_iterator it;

  for (it = die->getChildren().begin(); it != die->getChildren().end(); it++)
  { // Search for all member of a class or a structure
    if (it->get()->getTag() == DW_TAG_member)
    { // Found some member of a class or a structure
      DwMember *member = static_cast< DwMember* >(it->get());

      // Ignore static members, they cannot be stored within objects
      if (member->isStatic()) continue;

      if (member->isClass())
      { // Member is a class, offset must be within the contained object
        if (member->getMemberOffset() <= offset
          && offset < member->getMemberOffset() + member->getSize())
        { // Offset is within the contained object, search for its member
          return ::getMember(member->getDataType(),
            offset - member->getMemberOffset());
        }
      }
      else
      { // Member is a basic data type, offset must match
        if (member->getMemberOffset() == offset) return member;
      }
    }
  }

  // No member on the specified offset found
  return NULL;
}

/**
 * Gets a name of a member of a class or a structure on a specific offset.
 *
 * @param die A DWARF debugging information entry object representing a class
 *   or a structure containing the member.
 * @param offset An offset of the member within the class or structure.
 * @return The name of the member on the specified offset or empty string if no
 *   member on the specified offset was found.
 */
inline
std::string getMemberName(DwDie *die, Dwarf_Off offset)
{
  // Only classes and structures have members
  assert(die->getTag() == DW_TAG_class_type
    || die->getTag() == DW_TAG_structure_type);

  // Helper variables
  DwDie::Dwarf_Die_List::const_iterator it;

  for (it = die->getChildren().begin(); it != die->getChildren().end(); it++)
  { // Search for all member of a class or a structure
    if (it->get()->getTag() == DW_TAG_member)
    { // Found some member of a class or a structure
      DwMember *member = static_cast< DwMember* >(it->get());

      // Ignore static members, they cannot be stored within objects
      if (member->isStatic()) continue;

      if (member->isClass())
      { // Member is a class, offset must be within the contained object
        if (member->getMemberOffset() <= offset
          && offset < member->getMemberOffset() + member->getSize())
        { // Offset is within the contained object, search for its member
          return std::string(member->getName()) + "." + ::getMemberName(
            member->getDataType(), offset - member->getMemberOffset());
        }
      }
      else
      { // Member is a basic data type, offset must match
        if (member->getMemberOffset() == offset) return member->getName();
      }
    }
  }

  // No member on the specified offset found
  return std::string();
}

/**
 * Constructs a DwDie object.
 */
DwDie::DwDie()
{
}

/**
 * Constructs a DwDie object.
 *
 * @param die A DWARF debugging information entry.
 */
DwDie::DwDie(Dwarf_Die& die)
{
  // Get the offset of the DWARF debugging information entry in the current CU
  dwarf_die_CU_offset(die, &m_offset, NULL);

  // Load the attributes of the DWARF debugging information entry
  this->loadAttributes(die);
}

/**
 * Constructs a DwDie object from an existing DwDie object.
 *
 * @param die A DWARF debugging information entry object.
 */
DwDie::DwDie(const DwDie& die) : m_offset(die.m_offset),
  m_attributes(die.m_attributes), m_children(die.m_children)
{
}

/**
 * Destroys a DwDie object.
 */
DwDie::~DwDie()
{
}

/**
 * Accepts a DWARF debugging information entry visitor.
 *
 * @param visitor A DWARF debugging information entry visitor.
 */
void DwDie::accept(DwDieVisitor& visitor)
{
  // Visit the base DWARF debugging information entry class
  visitor.visit(*this);

  // Helper variables
  Dwarf_Die_List::iterator it;

  for (it = m_children.begin(); it != m_children.end(); it++)
  { // Visit all contained DWARF debugging information entries
    it->get()->accept(visitor);
  }
}

/**
 * Accepts a DWARF debugging information entry tree traverser.
 *
 * @param traverser A DWARF debugging information entry tree traverser.
 */
void DwDie::accept(DwDieTreeTraverser& traverser)
{
  // Visit the base DWARF debugging information entry class
  traverser.visit(*this);

  // Increase depth before visiting the children (going deeper into the tree)
  traverser.incDepth();

  // Helper variables
  Dwarf_Die_List::iterator it;

  for (it = m_children.begin(); it != m_children.end(); it++)
  { // Visit all contained DWARF debugging information entries
    it->get()->accept(traverser);
  }

  // Decrease depth after visiting the children (emerging from the tree)
  traverser.decDepth();
}

/**
 * Loads values of all attributes of a DWARF debugging information entry.
 *
 * @param die A DWARF debugging information entry.
 */
void DwDie::loadAttributes(Dwarf_Die& die)
{
  // Helper attributes
  Dwarf_Signed attrCount;
  Dwarf_Attribute *attrList;
  Dwarf_Half attrCode;
  Dwarf_Half attrForm;

  if (dwarf_attrlist(die, &attrList, &attrCount, NULL) == DW_DLV_OK)
  { // Get all attributes of the DWARF debugging information entry
    for (int i = 0; i < attrCount; i++)
    { // Extract one attribute and its value from the DWARF DIE
      if (dwarf_whatattr(attrList[i], &attrCode, NULL) == DW_DLV_OK)
      { // Determine the type of the attribute (get the attribute code)
        if (dwarf_whatform(attrList[i], &attrForm, NULL) == DW_DLV_OK)
        { // Insert the attribute to the attribute table, work with a reference
          Dwarf_Attribute_Value& attribute = m_attributes[attrCode];

          // Determine how the value of the attribute is encoded
          attribute.cls = dwarf_get_form_class(
            die->di_cu_context->cc_version_stamp, attrCode,
            die->di_cu_context->cc_length_size, attrForm);
          attribute.form = attrForm;

          // Helper variables
          Dwarf_Location_List loclist;

          switch (attribute.cls)
          { // Extract the value of the attribute based on its encoding (form)
            case DW_FORM_CLASS_ADDRESS:
              // Attributes with integer values representing a memory address
              dwarf_formaddr(attrList[i], &attribute.addr, NULL);
              break;
            case DW_FORM_CLASS_BLOCK:
              // Attributes with blocks of arbitrary data
              if (dwarf_loclist_n(attrList[i], &loclist.llbuf, &loclist.listlen,
                NULL) == DW_DLV_OK)
              { // Attributes with blocks of data representing a single location
                assert(loclist.listlen == 1);
                assert(loclist.llbuf[0]->ld_cents == 1);

                // Returned a list containing only a single location record
                attribute.loc = loclist.llbuf[0]->ld_s;

                // The value is not encoded as a block anymore, but as location
                attribute.form = DW_FORM_location;

                // Free the location description and the list
                dwarf_dealloc(die->di_cu_context->cc_dbg, loclist.llbuf[0],
                  DW_DLA_LOCDESC);
                dwarf_dealloc(die->di_cu_context->cc_dbg, loclist.llbuf,
                  DW_DLA_LIST);
              }
              break;
            case DW_FORM_CLASS_CONSTANT:
              // Attributes with integer values
              if (attrForm == DW_FORM_sdata)
              { // Attributes with signed integer values
                dwarf_formsdata(attrList[i], &attribute.sdata, NULL);
              }
              else
              { // Attributes with unsigned integer values
                dwarf_formudata(attrList[i], &attribute.udata, NULL);
              }
              break;
            case DW_FORM_CLASS_FLAG:
              // Attributes with boolean values
              dwarf_formflag(attrList[i], &attribute.flag, NULL);
              break;
            case DW_FORM_CLASS_REFERENCE:
              switch (attrForm)
              { // Attributes with a section or CU-relative offset values
                case DW_FORM_data4:
                case DW_FORM_data8:
                  if (attrCode == DW_AT_upper_bound)
                  { // Upper bound seems to have only constants as data4/data8
                    dwarf_formudata(attrList[i], &attribute.udata, NULL);
                    // Fix the wrong class returned by the libdwarf library
                    attribute.cls = DW_FORM_CLASS_CONSTANT;
                  }
                  else
                  { // Only section-relative offsets are stored as data4/data8
                    dwarf_global_formref(attrList[i], &attribute.ref, NULL);
                  }
                  break;
                case DW_FORM_ref_addr:
                case DW_FORM_sec_offset:
                  // Attributes with a section-relative offset values
                  dwarf_global_formref(attrList[i], &attribute.ref, NULL);
                  break;
                default:
                  // Attributes with a CU-relative offset values
                  dwarf_formref(attrList[i], &attribute.ref, NULL);
                  break;
              }
              break;
            case DW_FORM_CLASS_STRING:
              // Attributes with string values
              dwarf_formstring(attrList[i], &attribute.string, NULL);
              break;
            case DW_FORM_CLASS_EXPRLOC:
              break;
            case DW_FORM_CLASS_LOCLISTPTR:
              // Attributes with a list containing locations description
              attribute.loclist = new Dwarf_Location_List;

              dwarf_loclist_n(attrList[i], &attribute.loclist->llbuf,
                &attribute.loclist->listlen, NULL);
              break;
            case DW_FORM_CLASS_RANGELISTPTR:
            case DW_FORM_CLASS_LINEPTR:
            case DW_FORM_CLASS_MACPTR:
            case DW_FORM_CLASS_FRAMEPTR:
            case DW_FORM_CLASS_UNKNOWN:
              break;
            default:
              // Attributes with unknown values
              break;
          }
        }
      }

      // Free the attribute (all info is now stored in the DWARF DIE object)
      dwarf_dealloc(die->di_cu_context->cc_dbg, attrList[i], DW_DLA_ATTR);
    }

    // Free the list of attributes (all attributes are now loaded)
    dwarf_dealloc(die->di_cu_context->cc_dbg, attrList, DW_DLA_LIST);
  }
}

/**
 * Constructs a DwTag object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwTag< DW_TAG_CLASS, DW_TAG_ID >::DwTag() : DwDie()
{
}

/**
 * Constructs a DwTag object.
 *
 * @param die A DWARF debugging information entry.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwTag< DW_TAG_CLASS, DW_TAG_ID >::DwTag(Dwarf_Die& die) : DwDie(die)
{
}

/**
 * Constructs a DwTag object from an existing DwTag object.
 *
 * @param tag A DWARF tagged debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwTag< DW_TAG_CLASS, DW_TAG_ID >::DwTag(
  const DwTag< DW_TAG_CLASS, DW_TAG_ID >& tag) : DwDie(tag)
{
}

/**
 * Destroys a DwTag object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwTag< DW_TAG_CLASS, DW_TAG_ID >::~DwTag()
{
}

/**
 * Creates a DWARF debugging information entry object at runtime.
 *
 * @return A DWARF debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDie* DwTag< DW_TAG_CLASS, DW_TAG_ID >::create()
{
  return new DW_TAG_CLASS();
}

/**
 * Creates a DWARF debugging information entry object at runtime.
 *
 * @param die A DWARF debugging information entry.
 * @return A DWARF debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDie* DwTag< DW_TAG_CLASS, DW_TAG_ID >::create(Dwarf_Die& die)
{
  return new DW_TAG_CLASS(die);
}

/**
 * Creates a copy of a DWARF debugging information entry object.
 *
 * @return The copy of the DWARF debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDie* DwTag< DW_TAG_CLASS, DW_TAG_ID >::clone()
{
  return new DW_TAG_CLASS(*dynamic_cast< DW_TAG_CLASS* >(this));
}

/**
 * Accepts a DWARF debugging information entry visitor.
 *
 * @param visitor A DWARF debugging information entry visitor.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
void DwTag< DW_TAG_CLASS, DW_TAG_ID >::accept(DwDieVisitor& visitor)
{
  // Visit the concrete derived class, not the underlying template class
  visitor.visit(*dynamic_cast< DW_TAG_CLASS* >(this));

  // Helper variables
  Dwarf_Die_List::iterator it;

  for (it = m_children.begin(); it != m_children.end(); it++)
  { // Visit all contained DWARF debugging information entries
    it->get()->accept(visitor);
  }
}

/**
 * Accepts a DWARF debugging information entry tree traverser.
 *
 * @param traverser A DWARF debugging information entry tree traverser.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
void DwTag< DW_TAG_CLASS, DW_TAG_ID >::accept(DwDieTreeTraverser& traverser)
{
  // Visit the concrete derived class, not the underlying template class
  traverser.visit(*dynamic_cast< DW_TAG_CLASS* >(this));

  // Increase depth before visiting the children (going deeper into the tree)
  traverser.incDepth();

  // Helper variables
  Dwarf_Die_List::iterator it;

  for (it = m_children.begin(); it != m_children.end(); it++)
  { // Visit all contained DWARF debugging information entries
    it->get()->accept(traverser);
  }

  // Decrease depth after visiting the children (emerging from the tree)
  traverser.decDepth();
}

/**
 * Constructs a DwDataObject object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::DwDataObject()
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >()
{
}

/**
 * Constructs a DwDataObject object.
 *
 * @param die A DWARF debugging information entry.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::DwDataObject(Dwarf_Die& die)
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >(die)
{
}

/**
 * Constructs a DwDataObject object from an existing DwDataObject object.
 *
 * @param dobj A DWARF data object debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::DwDataObject(
  const DwDataObject< DW_TAG_CLASS, DW_TAG_ID >& dobj)
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >(dobj)
{
}

/**
 * Destroys a DwDataObject object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::~DwDataObject()
{
}

/**
 * Gets a DWARF debugging information entry object holding information about the
 *   type of data stored in a data object.
 *
 * @note This method can be used to extract information about the type of data
 *   (class, structure, array, pointer, etc.) stored in a data object without
 *   the need to handle storage and access modifiers which might be present in
 *   the (complete) type.
 *
 * @return A DWARF debugging information entry object holding information about
 *   the type of data stored in a data object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwDie* DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::getDataType()
{
  return ::getDataType(this->getType());
}

/**
 * Gets the first part of the declaration of a data object containing the type
 *   of the data object.
 *
 * @return The first part of the declaration of the data object containing its
 *   type or an @em empty string if the type was not found.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
std::string DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::getDeclarationSpecifier()
{
  return ::getTypeDeclSpec(this->getType());
}

/**
 * Gets a size in bytes of a data object.
 *
 * @return The size in bytes of the data object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
Dwarf_Unsigned DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::getSize()
{
  return ::getTypeSize(this->getType());
}

/**
 * Checks if a data object is an object of some class.
 *
 * @return @em True if the data object is an object of some class, @em false
 *   otherwise.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
bool DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::isClass()
{
  return ::getDataTypeTag(this->getType()) == DW_TAG_class_type;
}

/**
 * Checks if a data object is an instance of some structure.
 *
 * @return @em True if the data object is an instance of some structure,
 *   @em false otherwise.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
bool DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::isStructure()
{
  return ::getDataTypeTag(this->getType()) == DW_TAG_structure_type;
}

/**
 * Checks if a data object is an instance of some union.
 *
 * @return @em True if the data object is an instance of some union, @em false
 *   otherwise.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
bool DwDataObject< DW_TAG_CLASS, DW_TAG_ID >::isUnion()
{
  return ::getDataTypeTag(this->getType()) == DW_TAG_union_type;
}

/**
 * Constructs a DwCompoundType object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::DwCompoundType()
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >()
{
}

/**
 * Constructs a DwCompoundType object.
 *
 * @param die A DWARF debugging information entry.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::DwCompoundType(Dwarf_Die& die)
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >(die)
{
}

/**
 * Constructs a DwCompoundType object from an existing DwCompoundType object.
 *
 * @param ct A DWARF compound type debugging information entry object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::DwCompoundType(
  const DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >& ct)
  : DwTag< DW_TAG_CLASS, DW_TAG_ID >(ct)
{
}

/**
 * Destroys a DwCompoundType object.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::~DwCompoundType()
{
}

/**
 * Gets a member of a compound type on a specific offset.
 *
 * @param offset An offset of the member within the compound type.
 * @return The member on the specified offset or @em NULL if no member on the
 *   specified offset was found.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
DwMember* DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::getMember(Dwarf_Off offset)
{
  return ::getMember(this, offset);
}

/**
 * Gets a name of a member of a compound type on a specific offset.
 *
 * @param offset An offset of the member within the compound type.
 * @return The name of the member on the specified offset.
 */
template< class DW_TAG_CLASS, int DW_TAG_ID >
std::string DwCompoundType< DW_TAG_CLASS, DW_TAG_ID >::getMemberName(
  Dwarf_Off offset)
{
  return ::getMemberName(this, offset);
}

/**
 * Constructs a DwArrayType object.
 */
DwArrayType::DwArrayType()
  : DwTag< DwArrayType, DW_TAG_array_type >()
{
}

/**
 * Constructs a DwArrayType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwArrayType::DwArrayType(Dwarf_Die& die)
  : DwTag< DwArrayType, DW_TAG_array_type >(die)
{
}

/**
 * Constructs a DwArrayType object from an existing DwArrayType object.
 *
 * @param at A DWARF array type debugging information entry object.
 */
DwArrayType::DwArrayType(const DwArrayType& at)
  : DwTag< DwArrayType, DW_TAG_array_type >(at)
{
}

/**
 * Destroys a DwArrayType object.
 */
DwArrayType::~DwArrayType()
{
}

/**
 * Constructs a DwClassType object.
 */
DwClassType::DwClassType()
  : DwCompoundType< DwClassType, DW_TAG_class_type >()
{
}

/**
 * Constructs a DwClassType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwClassType::DwClassType(Dwarf_Die& die)
  : DwCompoundType< DwClassType, DW_TAG_class_type >(die)
{
}

/**
 * Constructs a DwClassType object from an existing DwClassType object.
 *
 * @param ct A DWARF class type debugging information entry object.
 */
DwClassType::DwClassType(const DwClassType& ct)
  : DwCompoundType< DwClassType, DW_TAG_class_type >(ct)
{
}

/**
 * Destroys a DwClassType object.
 */
DwClassType::~DwClassType()
{
}

/**
 * Constructs a DwEnumerationType object.
 */
DwEnumerationType::DwEnumerationType()
  : DwTag< DwEnumerationType, DW_TAG_enumeration_type >()
{
}

/**
 * Constructs a DwEnumerationType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwEnumerationType::DwEnumerationType(Dwarf_Die& die)
  : DwTag< DwEnumerationType, DW_TAG_enumeration_type >(die)
{
}

/**
 * Constructs a DwEnumerationType object from an existing DwEnumerationType
 *   object.
 *
 * @param et A DWARF enumeration type debugging information entry object.
 */
DwEnumerationType::DwEnumerationType(const DwEnumerationType& et)
  : DwTag< DwEnumerationType, DW_TAG_enumeration_type >(et)
{
}

/**
 * Destroys a DwEnumerationType object.
 */
DwEnumerationType::~DwEnumerationType()
{
}

/**
 * Constructs a DwFormalParameter object.
 */
DwFormalParameter::DwFormalParameter()
  : DwDataObject< DwFormalParameter, DW_TAG_formal_parameter >()
{
}

/**
 * Constructs a DwFormalParameter object.
 *
 * @param die A DWARF debugging information entry.
 */
DwFormalParameter::DwFormalParameter(Dwarf_Die& die)
  : DwDataObject< DwFormalParameter, DW_TAG_formal_parameter >(die)
{
}

/**
 * Constructs a DwFormalParameter object from an existing DwFormalParameter
 *   object.
 *
 * @param fp A DWARF formal parameter debugging information entry object.
 */
DwFormalParameter::DwFormalParameter(const DwFormalParameter& fp)
  : DwDataObject< DwFormalParameter, DW_TAG_formal_parameter >(fp)
{
}

/**
 * Destroys a DwFormalParameter object.
 */
DwFormalParameter::~DwFormalParameter()
{
}

/**
 * Constructs a DwImportedDeclaration object.
 */
DwImportedDeclaration::DwImportedDeclaration()
  : DwTag< DwImportedDeclaration, DW_TAG_imported_declaration >()
{
}

/**
 * Constructs a DwImportedDeclaration object.
 *
 * @param die A DWARF debugging information entry.
 */
DwImportedDeclaration::DwImportedDeclaration(Dwarf_Die& die)
  : DwTag< DwImportedDeclaration, DW_TAG_imported_declaration >(die)
{
}

/**
 * Constructs a DwImportedDeclaration object from an existing
 *   DwImportedDeclaration object.
 *
 * @param id A DWARF imported declaration debugging information entry object.
 */
DwImportedDeclaration::DwImportedDeclaration(const DwImportedDeclaration& id)
  : DwTag< DwImportedDeclaration, DW_TAG_imported_declaration >(id)
{
}

/**
 * Destroys a DwImportedDeclaration object.
 */
DwImportedDeclaration::~DwImportedDeclaration()
{
}

/**
 * Constructs a DwLabel object.
 */
DwLabel::DwLabel()
  : DwTag< DwLabel, DW_TAG_label >()
{
}

/**
 * Constructs a DwLabel object.
 *
 * @param die A DWARF debugging information entry.
 */
DwLabel::DwLabel(Dwarf_Die& die)
  : DwTag< DwLabel, DW_TAG_label >(die)
{
}

/**
 * Constructs a DwLabel object from an existing DwLabel object.
 *
 * @param l A DWARF label debugging information entry object.
 */
DwLabel::DwLabel(const DwLabel& l)
  : DwTag< DwLabel, DW_TAG_label >(l)
{
}

/**
 * Destroys a DwLabel object.
 */
DwLabel::~DwLabel()
{
}

/**
 * Constructs a DwLexicalBlock object.
 */
DwLexicalBlock::DwLexicalBlock()
  : DwTag< DwLexicalBlock, DW_TAG_lexical_block >()
{
}

/**
 * Constructs a DwLexicalBlock object.
 *
 * @param die A DWARF debugging information entry.
 */
DwLexicalBlock::DwLexicalBlock(Dwarf_Die& die)
  : DwTag< DwLexicalBlock, DW_TAG_lexical_block >(die)
{
}

/**
 * Constructs a DwLexicalBlock object from an existing DwLexicalBlock object.
 *
 * @param lb A DWARF lexical block debugging information entry object.
 */
DwLexicalBlock::DwLexicalBlock(const DwLexicalBlock& lb)
  : DwTag< DwLexicalBlock, DW_TAG_lexical_block >(lb)
{
}

/**
 * Destroys a DwLexicalBlock object.
 */
DwLexicalBlock::~DwLexicalBlock()
{
}

/**
 * Constructs a DwMember object.
 */
DwMember::DwMember()
  : DwTag< DwMember, DW_TAG_member >()
{
}

/**
 * Constructs a DwMember object.
 *
 * @param die A DWARF debugging information entry.
 */
DwMember::DwMember(Dwarf_Die& die)
  : DwTag< DwMember, DW_TAG_member >(die)
{
}

/**
 * Constructs a DwMember object from an existing DwMember object.
 *
 * @param m A DWARF member debugging information entry object.
 */
DwMember::DwMember(const DwMember& m)
  : DwTag< DwMember, DW_TAG_member >(m)
{
}

/**
 * Destroys a DwMember object.
 */
DwMember::~DwMember()
{
}

/**
 * Gets a DWARF debugging information entry object holding information about the
 *   type of data stored in a member of a class, structure or union.
 *
 * @note This method can be used to extract information about the type of data
 *   stored in a member of a class, structure or union (e.g. class, structure,
 *   array, pointer etc.) without the need to handle storage and access
 *   modifiers which might be present in the (complete) type.
 *
 * @return A DWARF debugging information entry object holding information about
 *   the type of data stored in a member.
 */
DwDie *DwMember::getDataType()
{
  return ::getDataType(this->getType());
}

/**
 * Gets the first part of the declaration of a member of a class, structure or
 *   union containing the type of the member.
 *
 * @return The first part of the declaration of the member containing its type
 *   or an empty string if the type was not found.
 */
std::string DwMember::getDeclarationSpecifier()
{
  return ::getTypeDeclSpec(this->getType());
}

/**
 * Gets a size in bytes of a member of a class, structure or union.
 *
 * @return The size in bytes of the member or @em 0 if the size cannot be
 *   determined.
 */
Dwarf_Unsigned DwMember::getSize()
{
  return ::getTypeSize(this->getType());
}

/**
 * Checks if a member of a class, structure or union is an object of some class.
 *
 * @return @em True if the member is an object of some class, @em false
 *   otherwise.
 */
bool DwMember::isClass()
{
  return ::getDataTypeTag(this->getType()) == DW_TAG_class_type;
}

/**
 * Constructs a DwPointerType object.
 */
DwPointerType::DwPointerType()
  : DwTag< DwPointerType, DW_TAG_pointer_type >()
{
}

/**
 * Constructs a DwPointerType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwPointerType::DwPointerType(Dwarf_Die& die)
  : DwTag< DwPointerType, DW_TAG_pointer_type >(die)
{
}

/**
 * Constructs a DwPointerType object from an existing DwPointerType object.
 *
 * @param pt A DWARF pointer type debugging information entry object.
 */
DwPointerType::DwPointerType(const DwPointerType& pt)
  : DwTag< DwPointerType, DW_TAG_pointer_type >(pt)
{
}

/**
 * Destroys a DwPointerType object.
 */
DwPointerType::~DwPointerType()
{
}

/**
 * Constructs a DwReferenceType object.
 */
DwReferenceType::DwReferenceType()
  : DwTag< DwReferenceType, DW_TAG_reference_type >()
{
}

/**
 * Constructs a DwReferenceType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwReferenceType::DwReferenceType(Dwarf_Die& die)
  : DwTag< DwReferenceType, DW_TAG_reference_type >(die)
{
}

/**
 * Constructs a DwReferenceType object from an existing DwReferenceType object.
 *
 * @param rt A DWARF reference type debugging information entry object.
 */
DwReferenceType::DwReferenceType(const DwReferenceType& rt)
  : DwTag< DwReferenceType, DW_TAG_reference_type >(rt)
{
}

/**
 * Destroys a DwReferenceType object.
 */
DwReferenceType::~DwReferenceType()
{
}

/**
 * Constructs a DwCompileUnit object.
 */
DwCompileUnit::DwCompileUnit()
  : DwTag< DwCompileUnit, DW_TAG_compile_unit >()
{
}

/**
 * Constructs a DwCompileUnit object.
 *
 * @param die A DWARF debugging information entry.
 */
DwCompileUnit::DwCompileUnit(Dwarf_Die& die)
  : DwTag< DwCompileUnit, DW_TAG_compile_unit >(die)
{
  // Get the global offset and length of the CU
  dwarf_die_CU_offset_range(die, &m_globalOffset, &m_length, NULL);

  // Get a list containing source files referenced in the CU
  dwarf_srcfiles(die, &m_srcFileList.srcfiles, &m_srcFileList.srccount, NULL);
}

/**
 * Constructs a DwCompileUnit object from an existing DwCompileUnit object.
 *
 * @param cu A DWARF compile unit debugging information entry object.
 */
DwCompileUnit::DwCompileUnit(const DwCompileUnit& cu)
  : DwTag< DwCompileUnit, DW_TAG_compile_unit >(cu),
  m_globalOffset(cu.m_globalOffset), m_length(cu.m_length)
{
}

/**
 * Destroys a DwCompileUnit object.
 */
DwCompileUnit::~DwCompileUnit()
{
}

/**
 * Constructs a DwStructureType object.
 */
DwStructureType::DwStructureType()
  : DwCompoundType< DwStructureType, DW_TAG_structure_type >()
{
}

/**
 * Constructs a DwStructureType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwStructureType::DwStructureType(Dwarf_Die& die)
  : DwCompoundType< DwStructureType, DW_TAG_structure_type >(die)
{
}

/**
 * Constructs a DwStructureType object from an existing DwStructureType object.
 *
 * @param st A DWARF structure type debugging information entry object.
 */
DwStructureType::DwStructureType(const DwStructureType& st)
  : DwCompoundType< DwStructureType, DW_TAG_structure_type >(st)
{
}

/**
 * Destroys a DwStructureType object.
 */
DwStructureType::~DwStructureType()
{
}

/**
 * Constructs a DwSubroutineType object.
 */
DwSubroutineType::DwSubroutineType()
  : DwTag< DwSubroutineType, DW_TAG_subroutine_type >()
{
}

/**
 * Constructs a DwSubroutineType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwSubroutineType::DwSubroutineType(Dwarf_Die& die)
  : DwTag< DwSubroutineType, DW_TAG_subroutine_type >(die)
{
}

/**
 * Constructs a DwSubroutineType object from an existing DwSubroutineType
 *   object.
 *
 * @param st A DWARF subroutine type debugging information entry object.
 */
DwSubroutineType::DwSubroutineType(const DwSubroutineType& st)
  : DwTag< DwSubroutineType, DW_TAG_subroutine_type >(st)
{
}

/**
 * Destroys a DwSubroutineType object.
 */
DwSubroutineType::~DwSubroutineType()
{
}

/**
 * Constructs a DwTypedef object.
 */
DwTypedef::DwTypedef()
  : DwTag< DwTypedef, DW_TAG_typedef >()
{
}

/**
 * Constructs a DwTypedef object.
 *
 * @param die A DWARF debugging information entry.
 */
DwTypedef::DwTypedef(Dwarf_Die& die)
  : DwTag< DwTypedef, DW_TAG_typedef >(die)
{
}

/**
 * Constructs a DwTypedef object from an existing DwTypedef object.
 *
 * @param t A DWARF typedef debugging information entry object.
 */
DwTypedef::DwTypedef(const DwTypedef& t)
  : DwTag< DwTypedef, DW_TAG_typedef >(t)
{
}

/**
 * Destroys a DwTypedef object.
 */
DwTypedef::~DwTypedef()
{
}

/**
 * Constructs a DwUnionType object.
 */
DwUnionType::DwUnionType()
  : DwCompoundType< DwUnionType, DW_TAG_union_type >()
{
}

/**
 * Constructs a DwUnionType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwUnionType::DwUnionType(Dwarf_Die& die)
  : DwCompoundType< DwUnionType, DW_TAG_union_type >(die)
{
}

/**
 * Constructs a DwUnionType object from an existing DwUnionType object.
 *
 * @param ut A DWARF union type debugging information entry object.
 */
DwUnionType::DwUnionType(const DwUnionType& ut)
  : DwCompoundType< DwUnionType, DW_TAG_union_type >(ut)
{
}

/**
 * Destroys a DwUnionType object.
 */
DwUnionType::~DwUnionType()
{
}

/**
 * Constructs a DwUnspecifiedParameters object.
 */
DwUnspecifiedParameters::DwUnspecifiedParameters()
  : DwTag< DwUnspecifiedParameters, DW_TAG_unspecified_parameters >()
{
}

/**
 * Constructs a DwUnspecifiedParameters object.
 *
 * @param die A DWARF debugging information entry.
 */
DwUnspecifiedParameters::DwUnspecifiedParameters(Dwarf_Die& die)
  : DwTag< DwUnspecifiedParameters, DW_TAG_unspecified_parameters >(die)
{
}

/**
 * Constructs a DwUnspecifiedParameters object from an existing
 *   DwUnspecifiedParameters object.
 *
 * @param up A DWARF unspecified parameters debugging information entry object.
 */
DwUnspecifiedParameters::DwUnspecifiedParameters(
  const DwUnspecifiedParameters& up)
  : DwTag< DwUnspecifiedParameters, DW_TAG_unspecified_parameters >(up)
{
}

/**
 * Destroys a DwUnspecifiedParameters object.
 */
DwUnspecifiedParameters::~DwUnspecifiedParameters()
{
}

/**
 * Constructs a DwInheritance object.
 */
DwInheritance::DwInheritance()
  : DwTag< DwInheritance, DW_TAG_inheritance >()
{
}

/**
 * Constructs a DwInheritance object.
 *
 * @param die A DWARF debugging information entry.
 */
DwInheritance::DwInheritance(Dwarf_Die& die)
  : DwTag< DwInheritance, DW_TAG_inheritance >(die)
{
}

/**
 * Constructs a DwInheritance object from an existing DwInheritance object.
 *
 * @param i A DWARF inheritance debugging information entry object.
 */
DwInheritance::DwInheritance(const DwInheritance& i)
  : DwTag< DwInheritance, DW_TAG_inheritance >(i)
{
}

/**
 * Destroys a DwInheritance object.
 */
DwInheritance::~DwInheritance()
{
}

/**
 * Constructs a DwInlinedSubroutine object.
 */
DwInlinedSubroutine::DwInlinedSubroutine()
  : DwTag< DwInlinedSubroutine, DW_TAG_inlined_subroutine >()
{
}

/**
 * Constructs a DwInlinedSubroutine object.
 *
 * @param die A DWARF debugging information entry.
 */
DwInlinedSubroutine::DwInlinedSubroutine(Dwarf_Die& die)
  : DwTag< DwInlinedSubroutine, DW_TAG_inlined_subroutine >(die)
{
}

/**
 * Constructs a DwInlinedSubroutine object from an existing DwInlinedSubroutine
 *   object.
 *
 * @param is A DWARF inlined subroutine debugging information entry object.
 */
DwInlinedSubroutine::DwInlinedSubroutine(const DwInlinedSubroutine& is)
  : DwTag< DwInlinedSubroutine, DW_TAG_inlined_subroutine >(is)
{
}

/**
 * Destroys a DwInlinedSubroutine object.
 */
DwInlinedSubroutine::~DwInlinedSubroutine()
{
}

/**
 * Constructs a DwPointerToMemberType object.
 */
DwPointerToMemberType::DwPointerToMemberType()
  : DwTag< DwPointerToMemberType, DW_TAG_ptr_to_member_type >()
{
}

/**
 * Constructs a DwPointerToMemberType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwPointerToMemberType::DwPointerToMemberType(Dwarf_Die& die)
  : DwTag< DwPointerToMemberType, DW_TAG_ptr_to_member_type >(die)
{
}

/**
 * Constructs a DwPointerToMemberType object from an existing
 *   DwPointerToMemberType object.
 *
 * @param ptmt A DWARF pointer to member type debugging information entry
 *   object.
 */
DwPointerToMemberType::DwPointerToMemberType(const DwPointerToMemberType& ptmt)
  : DwTag< DwPointerToMemberType, DW_TAG_ptr_to_member_type >(ptmt)
{
}

/**
 * Destroys a DwPointerToMemberType object.
 */
DwPointerToMemberType::~DwPointerToMemberType()
{
}

/**
 * Constructs a DwSubrangeType object.
 */
DwSubrangeType::DwSubrangeType()
  : DwTag< DwSubrangeType, DW_TAG_subrange_type >()
{
}

/**
 * Constructs a DwSubrangeType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwSubrangeType::DwSubrangeType(Dwarf_Die& die)
  : DwTag< DwSubrangeType, DW_TAG_subrange_type >(die)
{
}

/**
 * Constructs a DwSubrangeType object from an existing DwSubrangeType object.
 *
 * @param st A DWARF subrange type debugging information entry object.
 */
DwSubrangeType::DwSubrangeType(const DwSubrangeType& st)
  : DwTag< DwSubrangeType, DW_TAG_subrange_type >(st)
{
}

/**
 * Destroys a DwSubrangeType object.
 */
DwSubrangeType::~DwSubrangeType()
{
}

/**
 * Constructs a DwBaseType object.
 */
DwBaseType::DwBaseType()
  : DwTag< DwBaseType, DW_TAG_base_type >()
{
}

/**
 * Constructs a DwBaseType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwBaseType::DwBaseType(Dwarf_Die& die)
  : DwTag< DwBaseType, DW_TAG_base_type >(die)
{
}

/**
 * Constructs a DwBaseType object from an existing DwBaseType object.
 *
 * @param bt A DWARF base type debugging information entry object.
 */
DwBaseType::DwBaseType(const DwBaseType& bt)
  : DwTag< DwBaseType, DW_TAG_base_type >(bt)
{
}

/**
 * Destroys a DwBaseType object.
 */
DwBaseType::~DwBaseType()
{
}

/**
 * Constructs a DwConstType object.
 */
DwConstType::DwConstType()
  : DwTag< DwConstType, DW_TAG_const_type >()
{
}

/**
 * Constructs a DwConstType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwConstType::DwConstType(Dwarf_Die& die)
  : DwTag< DwConstType, DW_TAG_const_type >(die)
{
}

/**
 * Constructs a DwConstType object from an existing DwConstType object.
 *
 * @param ct A DWARF constant type debugging information entry object.
 */
DwConstType::DwConstType(const DwConstType& ct)
  : DwTag< DwConstType, DW_TAG_const_type >(ct)
{
}

/**
 * Destroys a DwConstType object.
 */
DwConstType::~DwConstType()
{
}

/**
 * Constructs a DwEnumerator object.
 */
DwEnumerator::DwEnumerator()
  : DwTag< DwEnumerator, DW_TAG_enumerator >()
{
}

/**
 * Constructs a DwEnumerator object.
 *
 * @param die A DWARF debugging information entry.
 */
DwEnumerator::DwEnumerator(Dwarf_Die& die)
  : DwTag< DwEnumerator, DW_TAG_enumerator >(die)
{
}

/**
 * Constructs a DwEnumerator object from an existing DwEnumerator object.
 *
 * @param e A DWARF enumerator debugging information entry object.
 */
DwEnumerator::DwEnumerator(const DwEnumerator& e)
  : DwTag< DwEnumerator, DW_TAG_enumerator >(e)
{
}

/**
 * Destroys a DwEnumerator object.
 */
DwEnumerator::~DwEnumerator()
{
}

/**
 * Constructs a DwSubprogram object.
 */
DwSubprogram::DwSubprogram()
  : DwTag< DwSubprogram, DW_TAG_subprogram >()
{
}

/**
 * Constructs a DwSubprogram object.
 *
 * @param die A DWARF debugging information entry.
 */
DwSubprogram::DwSubprogram(Dwarf_Die& die)
  : DwTag< DwSubprogram, DW_TAG_subprogram >(die)
{
}

/**
 * Constructs a DwSubprogram object from an existing DwSubprogram object.
 *
 * @param s A DWARF subprogram debugging information entry object.
 */
DwSubprogram::DwSubprogram(const DwSubprogram& s)
  : DwTag< DwSubprogram, DW_TAG_subprogram >(s)
{
}

/**
 * Destroys a DwSubprogram object.
 */
DwSubprogram::~DwSubprogram()
{
}

/**
 * Finds a data object (variable, formal parameter or constant) which is stored
 *   at a specific address.
 *
 * @param accessedAddr An address at which is the data object stored.
 * @param insAddr An address of the instruction which accessed the data object.
 *   Needed to compute the frame base address if needed.
 * @param registers An object holding the content of the registers.
 * @param offset An offset between the accessed address and the base address at
 *   which is the found data object stored. If @em NULL, the offset will not be
 *   set by the method.
 * @return The data object which is stored at the specified address or @em NULL
 *   if no data object was found.
 */
DwDie* DwSubprogram::findDataObject(Dwarf_Addr accessedAddr, Dwarf_Addr insAddr,
  DwRegisters& registers, unsigned int* offset)
{
  // A visitor for finding variables, formal parameters and constants
  DwDataObjectFinder finder;

  // Find all variables, formal parameters and constants
  this->accept(finder);

  // Get the frame base for the instruction which has accessed the data object
  Dwarf_Addr frameBaseAddr = this->getFrameBaseAddress(insAddr, registers);

  // Helper variables
  std::list< DwDie* >::const_iterator it;

  for (it = finder.getDataObjects().begin();
    it != finder.getDataObjects().end(); it++)
  { // Search through all found data objects
    Dwarf_Loc *location = NULL;
    Dwarf_Unsigned size = 0;

    switch ((*it)->getTag())
    { // Get the location and size of the data object
      case DW_TAG_variable:// The data object is a variable
        location = static_cast< DwVariable* >(*it)->getLocation();
        size = static_cast< DwVariable* >(*it)->getSize();
        break;
      case DW_TAG_formal_parameter: // The data object is a formal parameter
        location = static_cast< DwFormalParameter* >(*it)->getLocation();
        size = static_cast< DwFormalParameter* >(*it)->getSize();
        break;
      default: // The data object finder may collect only the above DIE objects
        assert(false);
        break;
    }

    // Data objects without location are present only in the source code
    if (location == NULL) continue;

    // Evaluate the address at which is the data object stored
    Dwarf_Addr baseAddr = evaluateLocExpr(*location, registers, frameBaseAddr);

    if (baseAddr == accessedAddr)
    { // The accessed address matches the base address of some data object
      if (offset != NULL)
      { // Accessed the beginning of some data object => no offset
        *offset = 0;
      } // Return the found data object
      return *it;
    }

    if (baseAddr <= accessedAddr && accessedAddr < baseAddr + size)
    { // The accessed address is in a range of memory of a larger data object
      if (offset != NULL)
      { // Compute the offset between the accessed address and the base address
        *offset = accessedAddr - baseAddr;
      } // Return the found data object
      return *it;
    }
  }

  // No data object present at the accessed address found
  return NULL;
}

/**
 * Gets a frame base address for an instruction situated on a specific address.
 *
 * @param insAddr An address of the instruction.
 * @param registers An object holding the content of the registers.
 * @return The frame base address for the instruction situated at the specified
 *   address.
 */
Dwarf_Addr DwSubprogram::getFrameBaseAddress(Dwarf_Addr insAddr,
  DwRegisters& registers)
{
  // Get the attribute holding the frame base location or location list
  Dwarf_Attribute_Map::iterator it = m_attributes.find(DW_AT_frame_base);

  // Should be called only when frame base is present
  assert(it != m_attributes.end());

  if (it->second.cls == DW_FORM_CLASS_LOCLISTPTR)
  { // The frame base is a location list (may be different for each instruction)
    assert(insAddr >= this->getLowPC());

    // Helper variables
    Dwarf_Location_List *loclist = it->second.loclist;

    // Compute the offset of the instruction from the beginning of the CU
    Dwarf_Addr offset = insAddr - this->getLowPC() + loclist->llbuf[0]->ld_lopc;

    for (int i = 0; i < loclist->listlen; i++)
    { // Find the frame base location for the instruction
      if (offset < loclist->llbuf[i]->ld_hipc)
      { // The frame base location for the instruction is the i-th location
        return evaluateLocExpr(*loclist->llbuf[i]->ld_s, registers);
      }
    }
  }
  else if (it->second.form == DW_FORM_location)
  { // The frame base is a location (same for all instructions)
    return evaluateLocExpr(*it->second.loc, registers);
  }

  // Attribute should be a location list or a single location
  assert(false);

  // Return invalid address if the frame base location cannot be determined
  return 0;
}

/**
 * Constructs a DwTemplateTypeParameter object.
 */
DwTemplateTypeParameter::DwTemplateTypeParameter()
  : DwTag< DwTemplateTypeParameter, DW_TAG_template_type_parameter >()
{
}

/**
 * Constructs a DwTemplateTypeParameter object.
 *
 * @param die A DWARF debugging information entry.
 */
DwTemplateTypeParameter::DwTemplateTypeParameter(Dwarf_Die& die)
  : DwTag< DwTemplateTypeParameter, DW_TAG_template_type_parameter >(die)
{
}

/**
 * Constructs a DwTemplateTypeParameter object from an existing
 *   DwTemplateTypeParameter object.
 *
 * @param ttp A DWARF template type parameter debugging information entry
 *   object.
 */
DwTemplateTypeParameter::DwTemplateTypeParameter(
  const DwTemplateTypeParameter& ttp)
  : DwTag< DwTemplateTypeParameter, DW_TAG_template_type_parameter >(ttp)
{
}

/**
 * Destroys a DwTemplateTypeParameter object.
 */
DwTemplateTypeParameter::~DwTemplateTypeParameter()
{
}

/**
 * Constructs a DwTemplateValueParameter object.
 */
DwTemplateValueParameter::DwTemplateValueParameter()
  : DwTag< DwTemplateValueParameter, DW_TAG_template_value_parameter >()
{
}

/**
 * Constructs a DwTemplateValueParameter object.
 *
 * @param die A DWARF debugging information entry.
 */
DwTemplateValueParameter::DwTemplateValueParameter(Dwarf_Die& die)
  : DwTag< DwTemplateValueParameter, DW_TAG_template_value_parameter >(die)
{
}

/**
 * Constructs a DwTemplateValueParameter object from an existing
 *   DwTemplateValueParameter object.
 *
 * @param tvp A DWARF template value parameter debugging information entry
 *   object.
 */
DwTemplateValueParameter::DwTemplateValueParameter(
  const DwTemplateValueParameter& tvp)
  : DwTag< DwTemplateValueParameter, DW_TAG_template_value_parameter >(tvp)
{
}

/**
 * Destroys a DwTemplateValueParameter object.
 */
DwTemplateValueParameter::~DwTemplateValueParameter()
{
}

/**
 * Constructs a DwVariable object.
 */
DwVariable::DwVariable()
  : DwDataObject< DwVariable, DW_TAG_variable >()
{
}

/**
 * Constructs a DwVariable object.
 *
 * @param die A DWARF debugging information entry.
 */
DwVariable::DwVariable(Dwarf_Die& die)
  : DwDataObject< DwVariable, DW_TAG_variable >(die)
{
}

/**
 * Constructs a DwVariable object from an existing DwVariable object.
 *
 * @param v A DWARF variable debugging information entry object.
 */
DwVariable::DwVariable(const DwVariable& v)
  : DwDataObject< DwVariable, DW_TAG_variable >(v)
{
}

/**
 * Destroys a DwVariable object.
 */
DwVariable::~DwVariable()
{
}

/**
 * Checks if a variable is a global variable.
 *
 * @return @em True if the variable is a global variable, @em false otherwise.
 */
bool DwVariable::isGlobal()
{
  // Get the location of the variable (might be missing)
  Dwarf_Loc* location = this->getLocation();

  if (location != NULL)
  { // If the location is a concrete address, the variable is a global variable
    return location->lr_atom == DW_OP_addr;
  }

  // The variable has no location, probably a declaration only
  return false;
}

/**
 * Constructs a DwVolatileType object.
 */
DwVolatileType::DwVolatileType()
  : DwTag< DwVolatileType, DW_TAG_volatile_type >()
{
}

/**
 * Constructs a DwVolatileType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwVolatileType::DwVolatileType(Dwarf_Die& die)
  : DwTag< DwVolatileType, DW_TAG_volatile_type >(die)
{
}

/**
 * Constructs a DwVolatileType object from an existing DwVolatileType object.
 *
 * @param vt A DWARF volatile type debugging information entry object.
 */
DwVolatileType::DwVolatileType(const DwVolatileType& vt)
  : DwTag< DwVolatileType, DW_TAG_volatile_type >(vt)
{
}

/**
 * Destroys a DwVolatileType object.
 */
DwVolatileType::~DwVolatileType()
{
}

/**
 * Constructs a DwNamespace object.
 */
DwNamespace::DwNamespace()
  : DwTag< DwNamespace, DW_TAG_namespace >()
{
}

/**
 * Constructs a DwNamespace object.
 *
 * @param die A DWARF debugging information entry.
 */
DwNamespace::DwNamespace(Dwarf_Die& die)
  : DwTag< DwNamespace, DW_TAG_namespace >(die)
{
}

/**
 * Constructs a DwNamespace object from an existing DwNamespace object.
 *
 * @param n A DWARF namespace debugging information entry object.
 */
DwNamespace::DwNamespace(const DwNamespace& n)
  : DwTag< DwNamespace, DW_TAG_namespace >(n)
{
}

/**
 * Destroys a DwNamespace object.
 */
DwNamespace::~DwNamespace()
{
}

/**
 * Constructs a DwImportedModule object.
 */
DwImportedModule::DwImportedModule()
  : DwTag< DwImportedModule, DW_TAG_imported_module >()
{
}

/**
 * Constructs a DwImportedModule object.
 *
 * @param die A DWARF debugging information entry.
 */
DwImportedModule::DwImportedModule(Dwarf_Die& die)
  : DwTag< DwImportedModule, DW_TAG_imported_module >(die)
{
}

/**
 * Constructs a DwImportedModule object from an existing DwImportedModule
 *   object.
 *
 * @param im A DWARF imported module debugging information entry object.
 */
DwImportedModule::DwImportedModule(const DwImportedModule& im)
  : DwTag< DwImportedModule, DW_TAG_imported_module >(im)
{
}

/**
 * Destroys a DwImportedModule object.
 */
DwImportedModule::~DwImportedModule()
{
}

/**
 * Constructs a DwUnspecifiedType object.
 */
DwUnspecifiedType::DwUnspecifiedType()
  : DwTag< DwUnspecifiedType, DW_TAG_unspecified_type >()
{
}

/**
 * Constructs a DwUnspecifiedType object.
 *
 * @param die A DWARF debugging information entry.
 */
DwUnspecifiedType::DwUnspecifiedType(Dwarf_Die& die)
  : DwTag< DwUnspecifiedType, DW_TAG_unspecified_type >(die)
{
}

/**
 * Constructs a DwUnspecifiedType object from an existing DwUnspecifiedType
 *   object.
 *
 * @param ut A DWARF unspecified type debugging information entry object.
 */
DwUnspecifiedType::DwUnspecifiedType(const DwUnspecifiedType& ut)
  : DwTag< DwUnspecifiedType, DW_TAG_unspecified_type >(ut)
{
}

/**
 * Destroys a DwUnspecifiedType object.
 */
DwUnspecifiedType::~DwUnspecifiedType()
{
}

/**
 * Constructs a DwGnuCallSite object.
 */
DwGnuCallSite::DwGnuCallSite()
  : DwTag< DwGnuCallSite, DW_TAG_GNU_call_site >()
{
}

/**
 * Constructs a DwGnuCallSite object.
 *
 * @param die A DWARF debugging information entry.
 */
DwGnuCallSite::DwGnuCallSite(Dwarf_Die& die)
  : DwTag< DwGnuCallSite, DW_TAG_GNU_call_site >(die)
{
}

/**
 * Constructs a DwGnuCallSite object from an existing DwGnuCallSite object.
 *
 * @param cs A DWARF GNU call site debugging information entry object.
 */
DwGnuCallSite::DwGnuCallSite(const DwGnuCallSite& cs)
  : DwTag< DwGnuCallSite, DW_TAG_GNU_call_site >(cs)
{
}

/**
 * Destroys a DwGnuCallSite object.
 */
DwGnuCallSite::~DwGnuCallSite()
{
}

/**
 * Constructs a DwGnuCallSiteParameter object.
 */
DwGnuCallSiteParameter::DwGnuCallSiteParameter()
  : DwTag< DwGnuCallSiteParameter, DW_TAG_GNU_call_site_parameter >()
{
}

/**
 * Constructs a DwGnuCallSiteParameter object.
 *
 * @param die A DWARF debugging information entry.
 */
DwGnuCallSiteParameter::DwGnuCallSiteParameter(Dwarf_Die& die)
  : DwTag< DwGnuCallSiteParameter, DW_TAG_GNU_call_site_parameter >(die)
{
}

/**
 * Constructs a DwGnuCallSiteParameter object from an existing
 *   DwGnuCallSiteParameter object.
 *
 * @param csp A DWARF GNU call site parameter debugging information entry object.
 */
DwGnuCallSiteParameter::DwGnuCallSiteParameter(const DwGnuCallSiteParameter& csp)
  : DwTag< DwGnuCallSiteParameter, DW_TAG_GNU_call_site_parameter >(csp)
{
}

/**
 * Destroys a DwGnuCallSiteParameter object.
 */
DwGnuCallSiteParameter::~DwGnuCallSiteParameter()
{
}

/**
 * Constructs a DwDieFactory object.
 */
DwDieFactory::DwDieFactory()
{
  this->registerTag(new DwArrayType());
  this->registerTag(new DwClassType());
  this->registerTag(new DwEnumerationType());
  this->registerTag(new DwFormalParameter());
  this->registerTag(new DwImportedDeclaration());
  this->registerTag(new DwLabel());
  this->registerTag(new DwLexicalBlock());
  this->registerTag(new DwMember());
  this->registerTag(new DwPointerType());
  this->registerTag(new DwReferenceType());
  this->registerTag(new DwCompileUnit());
  this->registerTag(new DwStructureType());
  this->registerTag(new DwSubroutineType());
  this->registerTag(new DwTypedef());
  this->registerTag(new DwUnionType());
  this->registerTag(new DwUnspecifiedParameters());
  this->registerTag(new DwInheritance());
  this->registerTag(new DwInlinedSubroutine());
  this->registerTag(new DwPointerToMemberType());
  this->registerTag(new DwSubrangeType());
  this->registerTag(new DwBaseType());
  this->registerTag(new DwConstType());
  this->registerTag(new DwEnumerator());
  this->registerTag(new DwSubprogram());
  this->registerTag(new DwTemplateTypeParameter());
  this->registerTag(new DwTemplateValueParameter());
  this->registerTag(new DwVariable());
  this->registerTag(new DwVolatileType());
  this->registerTag(new DwNamespace());
  this->registerTag(new DwImportedModule());
  this->registerTag(new DwUnspecifiedType());
  this->registerTag(new DwGnuCallSite());
  this->registerTag(new DwGnuCallSiteParameter());
}

/**
 * Registers a reference object. This object will be used to create instances
 *   (objects) representing the same DWARF tags as this object. The tag which
 *   the object represents is obtained by calling the DwDie::getTag() method.
 *   New objects representing the same tag as the reference object will be
 *   created by calling the DwDie::create(Dwarf_Die&,Dwarf_Debug&) method.
 *
 * @param tag A reference object.
 * @return DwDieFactory::OK if the reference object registered successfully,
 *   DwDieFactory::ALREADY_REGISTERED if some object representing the same
 *   DWARF tag as the reference object is already registered.
 */
int DwDieFactory::registerTag(DwDie *tag)
{
  if (m_registeredTags.find(tag->getTag()) != m_registeredTags.end())
  { // Some object representing the same tag as the specified object is present
    return ALREADY_REGISTERED;
  }

  // Register the reference object
  m_registeredTags[tag->getTag()] = boost::shared_ptr< DwDie >(tag);

  // The reference object registered successfully
  return OK;
}

/**
 * Creates an object representing a specific DWARF tag.
 *
 * @param tag A DWARF tag which the created object must represent.
 * @param die A DWARF debugging information entry.
 * @param parent A DWARF debugging information entry object which is the parent
 *   of the created object (the created object will be added to the parent DIE
 *   object as its child).
 * @return The created object representing the specified DWARF tag or @em NULL
 *   if no such object could be created.
 */
DwDie* DwDieFactory::createTag(Dwarf_Half tag, Dwarf_Die& die, DwDie *parent)
{
  if (m_registeredTags.find(tag) != m_registeredTags.end())
  { // Reference object for the tag found, use it to create a new tag object
    DwDie *dwDie = m_registeredTags[tag].get()->create(die);

    // Set the parent DWARF DIE of the newly created DWARF DIE
    dwDie->setParent(parent);

    if (parent != NULL)
    { // Parent object specified, add the created object to it as a child
      parent->getChildren().push_back(boost::shared_ptr< DwDie >(dwDie));
    }

    // Return the created object representing the specified tag
    return dwDie;
  }

  // No reference object representing the specified tag found
  return NULL;
}

// Instantiate templates which must be present in the library
template class DwCompoundType< DwClassType, DW_TAG_class_type >;
template class DwCompoundType< DwStructureType, DW_TAG_structure_type >;
template class DwCompoundType< DwUnionType, DW_TAG_union_type >;
template class DwDataObject< DwFormalParameter, DW_TAG_formal_parameter >;
template class DwDataObject< DwVariable, DW_TAG_variable >;

/** End of file dw_classes.cpp **/
