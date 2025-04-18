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
 * @brief A file containing implementation of classes for visiting DWARF
 *   entries.
 *
 * A file containing implementation of classes for visiting various DWARF
 *   entries.
 *
 * @file      dw_visitors.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-08-23
 * @date      Last Update 2011-09-19
 * @version   0.2.4
 */

#include "dw_visitors.h"

#include <iomanip>
#include <map>

#include "boost/assign/list_of.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/format.hpp"

namespace
{ // Static global variables (usable only within this module)
  /**
   * @brief A table translating tag numbers to strings describing these tags.
   */
  std::map< unsigned short, const char* >
    g_tagToStringTable = boost::assign::map_list_of
      (0x01, "DW_TAG_array_type")
      (0x02, "DW_TAG_class_type")
      (0x03, "DW_TAG_entry_point")
      (0x04, "DW_TAG_enumeration_type")
      (0x05, "DW_TAG_formal_parameter")
      (0x08, "DW_TAG_imported_declaration")
      (0x0a, "DW_TAG_label")
      (0x0b, "DW_TAG_lexical_block")
      (0x0d, "DW_TAG_member")
      (0x0f, "DW_TAG_pointer_type")
      (0x10, "DW_TAG_reference_type")
      (0x11, "DW_TAG_compile_unit")
      (0x12, "DW_TAG_string_type")
      (0x13, "DW_TAG_structure_type")
      (0x15, "DW_TAG_subroutine_type")
      (0x16, "DW_TAG_typedef")
      (0x17, "DW_TAG_union_type")
      (0x18, "DW_TAG_unspecified_parameters")
      (0x19, "DW_TAG_variant")
      (0x1a, "DW_TAG_common_block")
      (0x1b, "DW_TAG_common_inclusion")
      (0x1c, "DW_TAG_inheritance")
      (0x1d, "DW_TAG_inlined_subroutine")
      (0x1e, "DW_TAG_module")
      (0x1f, "DW_TAG_ptr_to_member_type")
      (0x20, "DW_TAG_set_type")
      (0x21, "DW_TAG_subrange_type")
      (0x22, "DW_TAG_with_stmt")
      (0x23, "DW_TAG_access_declaration")
      (0x24, "DW_TAG_base_type")
      (0x25, "DW_TAG_catch_block")
      (0x26, "DW_TAG_const_type")
      (0x27, "DW_TAG_constant")
      (0x28, "DW_TAG_enumerator")
      (0x29, "DW_TAG_file_type")
      (0x2a, "DW_TAG_friend")
      (0x2b, "DW_TAG_namelist")
      (0x2c, "DW_TAG_namelist_item")
      (0x2c, "DW_TAG_namelist_items")
      (0x2d, "DW_TAG_packed_type")
      (0x2e, "DW_TAG_subprogram")
      (0x2f, "DW_TAG_template_type_parameter")
      (0x2f, "DW_TAG_template_type_param")
      (0x30, "DW_TAG_template_value_parameter")
      (0x30, "DW_TAG_template_value_param")
      (0x31, "DW_TAG_thrown_type")
      (0x32, "DW_TAG_try_block")
      (0x33, "DW_TAG_variant_part")
      (0x34, "DW_TAG_variable")
      (0x35, "DW_TAG_volatile_type")
      (0x36, "DW_TAG_dwarf_procedure")
      (0x37, "DW_TAG_restrict_type")
      (0x38, "DW_TAG_interface_type")
      (0x39, "DW_TAG_namespace")
      (0x3a, "DW_TAG_imported_module")
      (0x3b, "DW_TAG_unspecified_type")
      (0x3c, "DW_TAG_partial_unit")
      (0x3d, "DW_TAG_imported_unit")
      (0x3e, "DW_TAG_mutable_type")
      (0x3f, "DW_TAG_condition")
      (0x40, "DW_TAG_shared_type")
      (0x41, "DW_TAG_type_unit")
      (0x42, "DW_TAG_rvalue_reference_type")
      (0x43, "DW_TAG_template_alias")
      (0x4080, "DW_TAG_lo_user")
      (0x4081, "DW_TAG_MIPS_loop")
      (0x4090, "DW_TAG_HP_array_descriptor")
      (0x4101, "DW_TAG_format_label")
      (0x4102, "DW_TAG_function_template")
      (0x4103, "DW_TAG_class_template")
      (0x4104, "DW_TAG_GNU_BINCL")
      (0x4105, "DW_TAG_GNU_EINCL")
      (0x4106, "DW_TAG_GNU_template_template_parameter")
      (0x4106, "DW_TAG_GNU_template_template_param")
      (0x4107, "DW_TAG_GNU_template_parameter_pack")
      (0x4108, "DW_TAG_GNU_formal_parameter_pack")
      (0x5101, "DW_TAG_ALTIUM_circ_type")
      (0x5102, "DW_TAG_ALTIUM_mwa_circ_type")
      (0x5103, "DW_TAG_ALTIUM_rev_carry_type")
      (0x5111, "DW_TAG_ALTIUM_rom")
      (0x8765, "DW_TAG_upc_shared_type")
      (0x8766, "DW_TAG_upc_strict_type")
      (0x8767, "DW_TAG_upc_relaxed_type")
      (0xa000, "DW_TAG_PGI_kanji_type")
      (0xa020, "DW_TAG_PGI_interface_block")
      (0x4201, "DW_TAG_SUN_function_template")
      (0x4202, "DW_TAG_SUN_class_template")
      (0x4203, "DW_TAG_SUN_struct_template")
      (0x4204, "DW_TAG_SUN_union_template")
      (0x4205, "DW_TAG_SUN_indirect_inheritance")
      (0x4206, "DW_TAG_SUN_codeflags")
      (0x4207, "DW_TAG_SUN_memop_info")
      (0x4208, "DW_TAG_SUN_omp_child_func")
      (0x4209, "DW_TAG_SUN_rtti_descriptor")
      (0x420a, "DW_TAG_SUN_dtor_info")
      (0x420b, "DW_TAG_SUN_dtor")
      (0x420c, "DW_TAG_SUN_f90_interface")
      (0x420d, "DW_TAG_SUN_fortran_vax_structure")
      (0x42ff, "DW_TAG_SUN_hi")
      (0xffff, "DW_TAG_hi_user");

  /**
   * @brief A table translating attribute numbers to strings describing these
   *   attributes.
   */
  std::map< unsigned short, const char* >
    g_attrToStringTable = boost::assign::map_list_of
      (0x01, "DW_AT_sibling")
      (0x02, "DW_AT_location")
      (0x03, "DW_AT_name")
      (0x09, "DW_AT_ordering")
      (0x0a, "DW_AT_subscr_data")
      (0x0b, "DW_AT_byte_size")
      (0x0c, "DW_AT_bit_offset")
      (0x0d, "DW_AT_bit_size")
      (0x0f, "DW_AT_element_list")
      (0x10, "DW_AT_stmt_list")
      (0x11, "DW_AT_low_pc")
      (0x12, "DW_AT_high_pc")
      (0x13, "DW_AT_language")
      (0x14, "DW_AT_member")
      (0x15, "DW_AT_discr")
      (0x16, "DW_AT_discr_value")
      (0x17, "DW_AT_visibility")
      (0x18, "DW_AT_import")
      (0x19, "DW_AT_string_length")
      (0x1a, "DW_AT_common_reference")
      (0x1b, "DW_AT_comp_dir")
      (0x1c, "DW_AT_const_value")
      (0x1d, "DW_AT_containing_type")
      (0x1e, "DW_AT_default_value")
      (0x20, "DW_AT_inline")
      (0x21, "DW_AT_is_optional")
      (0x22, "DW_AT_lower_bound")
      (0x25, "DW_AT_producer")
      (0x27, "DW_AT_prototyped")
      (0x2a, "DW_AT_return_addr")
      (0x2c, "DW_AT_start_scope")
      (0x2e, "DW_AT_bit_stride")
      (0x2e, "DW_AT_stride_size")
      (0x2f, "DW_AT_upper_bound")
      (0x31, "DW_AT_abstract_origin")
      (0x32, "DW_AT_accessibility")
      (0x33, "DW_AT_address_class")
      (0x34, "DW_AT_artificial")
      (0x35, "DW_AT_base_types")
      (0x36, "DW_AT_calling_convention")
      (0x37, "DW_AT_count")
      (0x38, "DW_AT_data_member_location")
      (0x39, "DW_AT_decl_column")
      (0x3a, "DW_AT_decl_file")
      (0x3b, "DW_AT_decl_line")
      (0x3c, "DW_AT_declaration")
      (0x3d, "DW_AT_discr_list")
      (0x3e, "DW_AT_encoding")
      (0x3f, "DW_AT_external")
      (0x40, "DW_AT_frame_base")
      (0x41, "DW_AT_friend")
      (0x42, "DW_AT_identifier_case")
      (0x43, "DW_AT_macro_info")
      (0x44, "DW_AT_namelist_item")
      (0x45, "DW_AT_priority")
      (0x46, "DW_AT_segment")
      (0x47, "DW_AT_specification")
      (0x48, "DW_AT_static_link")
      (0x49, "DW_AT_type")
      (0x4a, "DW_AT_use_location")
      (0x4b, "DW_AT_variable_parameter")
      (0x4c, "DW_AT_virtuality")
      (0x4d, "DW_AT_vtable_elem_location")
      (0x4e, "DW_AT_allocated")
      (0x4f, "DW_AT_associated")
      (0x50, "DW_AT_data_location")
      (0x51, "DW_AT_byte_stride")
      (0x51, "DW_AT_stride")
      (0x52, "DW_AT_entry_pc")
      (0x53, "DW_AT_use_UTF8")
      (0x54, "DW_AT_extension")
      (0x55, "DW_AT_ranges")
      (0x56, "DW_AT_trampoline")
      (0x57, "DW_AT_call_column")
      (0x58, "DW_AT_call_file")
      (0x59, "DW_AT_call_line")
      (0x5a, "DW_AT_description")
      (0x5b, "DW_AT_binary_scale")
      (0x5c, "DW_AT_decimal_scale")
      (0x5d, "DW_AT_small")
      (0x5e, "DW_AT_decimal_sign")
      (0x5f, "DW_AT_digit_count")
      (0x60, "DW_AT_picture_string")
      (0x61, "DW_AT_mutable")
      (0x62, "DW_AT_threads_scaled")
      (0x63, "DW_AT_explicit")
      (0x64, "DW_AT_object_pointer")
      (0x65, "DW_AT_endianity")
      (0x66, "DW_AT_elemental")
      (0x67, "DW_AT_pure")
      (0x68, "DW_AT_recursive")
      (0x69, "DW_AT_signature")
      (0x6a, "DW_AT_main_subprogram")
      (0x6b, "DW_AT_data_bit_offset")
      (0x6c, "DW_AT_const_expr")
      (0x6d, "DW_AT_enum_class")
      (0x6e, "DW_AT_linkage_name")
      (0x2000, "DW_AT_HP_block_index")
      (0x2000, "DW_AT_lo_user")
      (0x2001, "DW_AT_MIPS_fde")
      (0x2002, "DW_AT_MIPS_loop_begin")
      (0x2003, "DW_AT_MIPS_tail_loop_begin")
      (0x2004, "DW_AT_MIPS_epilog_begin")
      (0x2005, "DW_AT_MIPS_loop_unroll_factor")
      (0x2006, "DW_AT_MIPS_software_pipeline_depth")
      (0x2007, "DW_AT_MIPS_linkage_name")
      (0x2008, "DW_AT_MIPS_stride")
      (0x2009, "DW_AT_MIPS_abstract_name")
      (0x200a, "DW_AT_MIPS_clone_origin")
      (0x200b, "DW_AT_MIPS_has_inlines")
      (0x200c, "DW_AT_MIPS_stride_byte")
      (0x200d, "DW_AT_MIPS_stride_elem")
      (0x200e, "DW_AT_MIPS_ptr_dopetype")
      (0x200f, "DW_AT_MIPS_allocatable_dopetype")
      (0x2010, "DW_AT_MIPS_assumed_shape_dopetype")
      (0x2011, "DW_AT_MIPS_assumed_size")
      (0x2001, "DW_AT_HP_unmodifiable")
      (0x2010, "DW_AT_HP_actuals_stmt_list")
      (0x2011, "DW_AT_HP_proc_per_section")
      (0x2012, "DW_AT_HP_raw_data_ptr")
      (0x2013, "DW_AT_HP_pass_by_reference")
      (0x2014, "DW_AT_HP_opt_level")
      (0x2015, "DW_AT_HP_prof_version_id")
      (0x2016, "DW_AT_HP_opt_flags")
      (0x2017, "DW_AT_HP_cold_region_low_pc")
      (0x2018, "DW_AT_HP_cold_region_high_pc")
      (0x2019, "DW_AT_HP_all_variables_modifiable")
      (0x201a, "DW_AT_HP_linkage_name")
      (0x201b, "DW_AT_HP_prof_flags")
      (0x2001, "DW_AT_CPQ_discontig_ranges")
      (0x2002, "DW_AT_CPQ_semantic_events")
      (0x2003, "DW_AT_CPQ_split_lifetimes_var")
      (0x2004, "DW_AT_CPQ_split_lifetimes_rtn")
      (0x2005, "DW_AT_CPQ_prologue_length")
      (0x2026, "DW_AT_INTEL_other_endian")
      (0x2101, "DW_AT_sf_names")
      (0x2102, "DW_AT_src_info")
      (0x2103, "DW_AT_mac_info")
      (0x2104, "DW_AT_src_coords")
      (0x2105, "DW_AT_body_begin")
      (0x2106, "DW_AT_body_end")
      (0x2107, "DW_AT_GNU_vector")
      (0x2108, "DW_AT_GNU_template_name")
      (0x2300, "DW_AT_ALTIUM_loclist")
      (0x2201, "DW_AT_SUN_template")
      (0x2201, "DW_AT_VMS_rtnbeg_pd_address")
      (0x2202, "DW_AT_SUN_alignment")
      (0x2203, "DW_AT_SUN_vtable")
      (0x2204, "DW_AT_SUN_count_guarantee")
      (0x2205, "DW_AT_SUN_command_line")
      (0x2206, "DW_AT_SUN_vbase")
      (0x2207, "DW_AT_SUN_compile_options")
      (0x2208, "DW_AT_SUN_language")
      (0x2209, "DW_AT_SUN_browser_file")
      (0x2210, "DW_AT_SUN_vtable_abi")
      (0x2211, "DW_AT_SUN_func_offsets")
      (0x2212, "DW_AT_SUN_cf_kind")
      (0x2213, "DW_AT_SUN_vtable_index")
      (0x2214, "DW_AT_SUN_omp_tpriv_addr")
      (0x2215, "DW_AT_SUN_omp_child_func")
      (0x2216, "DW_AT_SUN_func_offset")
      (0x2217, "DW_AT_SUN_memop_type_ref")
      (0x2218, "DW_AT_SUN_profile_id")
      (0x2219, "DW_AT_SUN_memop_signature")
      (0x2220, "DW_AT_SUN_obj_dir")
      (0x2221, "DW_AT_SUN_obj_file")
      (0x2222, "DW_AT_SUN_original_name")
      (0x2223, "DW_AT_SUN_hwcprof_signature")
      (0x2224, "DW_AT_SUN_amd64_parmdump")
      (0x2225, "DW_AT_SUN_part_link_name")
      (0x2226, "DW_AT_SUN_link_name")
      (0x2227, "DW_AT_SUN_pass_with_const")
      (0x2228, "DW_AT_SUN_return_with_const")
      (0x2229, "DW_AT_SUN_import_by_name")
      (0x222a, "DW_AT_SUN_f90_pointer")
      (0x222b, "DW_AT_SUN_pass_by_ref")
      (0x222c, "DW_AT_SUN_f90_allocatable")
      (0x222d, "DW_AT_SUN_f90_assumed_shape_array")
      (0x222e, "DW_AT_SUN_c_vla")
      (0x2230, "DW_AT_SUN_return_value_ptr")
      (0x2231, "DW_AT_SUN_dtor_start")
      (0x2232, "DW_AT_SUN_dtor_length")
      (0x2233, "DW_AT_SUN_dtor_state_initial")
      (0x2234, "DW_AT_SUN_dtor_state_final")
      (0x2235, "DW_AT_SUN_dtor_state_deltas")
      (0x2236, "DW_AT_SUN_import_by_lname")
      (0x2237, "DW_AT_SUN_f90_use_only")
      (0x2238, "DW_AT_SUN_namelist_spec")
      (0x2239, "DW_AT_SUN_is_omp_child_func")
      (0x223a, "DW_AT_SUN_fortran_main_alias")
      (0x223b, "DW_AT_SUN_fortran_based")
      (0x3210, "DW_AT_upc_threads_scaled")
      (0x3a00, "DW_AT_PGI_lbase")
      (0x3a01, "DW_AT_PGI_soffset")
      (0x3a02, "DW_AT_PGI_lstride")
      (0x3fe4, "DW_AT_APPLE_closure")
      (0x3fe5, "DW_AT_APPLE_major_runtime_vers")
      (0x3fe6, "DW_AT_APPLE_runtime_class")
      (0x3fff, "DW_AT_hi_user");
}

/**
 * Constructs a DwDieVisitor object.
 */
DwDieVisitor::DwDieVisitor()
{
}

/**
 * Destroys a DwDieVisitor object.
 */
DwDieVisitor::~DwDieVisitor()
{
}

/**
 * Visits a DWARF debugging information entry object.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwDieVisitor::visit(DwDie& die)
{
}

/**
 * Visits a DWARF formal parameter debugging information entry object.
 *
 * @param fp A DWARF formal parameter debugging information entry object.
 */
void DwDieVisitor::visit(DwFormalParameter& fp)
{
  this->visit(static_cast< DwDie& >(fp));
}

/**
 * Visits a DWARF compile unit debugging information entry object.
 *
 * @param cu A DWARF compile unit debugging information entry object.
 */
void DwDieVisitor::visit(DwCompileUnit& cu)
{
  this->visit(static_cast< DwDie& >(cu));
}

/**
 * Visits a DWARF subprogram debugging information entry object.
 *
 * @param s A DWARF subprogram debugging information entry object.
 */
void DwDieVisitor::visit(DwSubprogram& s)
{
  this->visit(static_cast< DwDie& >(s));
}

/**
 * Visits a DWARF variable debugging information entry object.
 *
 * @param v A DWARF variable debugging information entry object.
 */
void DwDieVisitor::visit(DwVariable& v)
{
  this->visit(static_cast< DwDie& >(v));
}

/**
 * Destroys a DwReferenceLinker object.
 */
DwReferenceLinker::~DwReferenceLinker()
{
  // Check if all references have been updated
  assert(m_attributes.empty());
}

/**
 * Visits a DWARF debugging information entry object.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwReferenceLinker::visit(DwDie& die)
{
  this->updateReferences(die);
}

/**
 * Visits a DWARF compile unit debugging information entry object.
 *
 * @param cu A DWARF compile unit debugging information entry object.
 */
void DwReferenceLinker::visit(DwCompileUnit& cu)
{
  m_currentCUGlobalOffset = cu.getGlobalOffset();

  this->updateReferences(cu);
}

/**
 * Updates all references of a DWARF debugging information entry object.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwReferenceLinker::updateReferences(DwDie& die)
{
  // Get the global offset of the DIE
  Dwarf_Off globalOffset = m_currentCUGlobalOffset + die.getOffset();

  // Global references (offsets) of all visited DIEs are stored in this table
  m_references[globalOffset] = &die;

  // Update all attributes which already referenced this DIE
  if (m_attributes.find(globalOffset) != m_attributes.end())
  { // Some attributes already referenced this DIE, update their references
    std::list< Dwarf_Attribute_Value* >::iterator it;

    for (it = m_attributes[globalOffset].begin();
      it != m_attributes[globalOffset].end(); it++)
    { // Update the references in all attributes which referenced this DIE
      (*it)->die = &die;

      switch ((*it)->form)
      { // The value of the attribute is now a pointer to a DWARF DIE object
        case DW_FORM_ref_addr:
        case DW_FORM_sec_offset:
          (*it)->form = DW_FORM_sec_ref_obj;
          break;
        default:
          (*it)->form = DW_FORM_cu_ref_obj;
          break;
      }
    }

    // Remove the attributes which referenced this DIE, they are updated now
    m_attributes.erase(globalOffset);
  }

  // Helper variables
  DwDie::Dwarf_Attribute_Map::iterator it;

  // Update references in all attributes of the currently visited DIE
  for (it = die.getAttributes().begin(); it != die.getAttributes().end(); it++)
  { // Process a single attribute
    if (it->second.cls == DW_FORM_CLASS_REFERENCE)
    { // Update only references, ignore other attribute forms
      switch (it->second.form)
      {
        case DW_FORM_ref_addr:
        case DW_FORM_sec_offset:
          // Attribute with a section-relative offset values
          if (m_references.find(it->second.ref) != m_references.end())
          { // Referenced DIE already visited before, update the attribute
            it->second.form = DW_FORM_sec_ref_obj;
            it->second.die = m_references[it->second.ref];
          }
          else
          { // Referenced DIE not visited yet, update the attribute later
            m_attributes[it->second.ref].push_back(&it->second);
          }
          break;
        default:
          // Attribute with a CU-relative offset values
          if (m_references.find(m_currentCUGlobalOffset + it->second.ref)
            != m_references.end())
          { // Referenced DIE already visited before, update the attribute
            it->second.form = DW_FORM_cu_ref_obj;
            it->second.die = m_references[m_currentCUGlobalOffset
              + it->second.ref];
          }
          else
          { // Referenced DIE not visited yet, update the attribute later
            m_attributes[m_currentCUGlobalOffset + it->second.ref].push_back(
              &it->second);
          }
          break;
      }
    }
  }
}

/**
 * Destroys a DwSourceFileIndexEvaluator object.
 */
DwSourceFileIndexEvaluator::~DwSourceFileIndexEvaluator()
{
}

/**
 * Visits a DWARF debugging information entry object.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwSourceFileIndexEvaluator::visit(DwDie& die)
{
  this->replaceIndexesWithPointers(die);
}

/**
 * Visits a DWARF compile unit debugging information entry object.
 *
 * @param cu A DWARF compile unit debugging information entry object.
 */
void DwSourceFileIndexEvaluator::visit(DwCompileUnit& cu)
{
  m_srcFileList = &cu.getSourceFiles();

  this->replaceIndexesWithPointers(cu);
}

/**
 * Replaces indexes to a table containing names of source files with pointers
 *   to these names. All indexes are stored in the DW_AT_decl_file attribute
 *   of a DWARF debugging information entry as constants.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwSourceFileIndexEvaluator::replaceIndexesWithPointers(DwDie& die)
{
  // Get the attribute holding the source file index to be evaluated
  DwDie::Dwarf_Attribute_Map::iterator it = die.getAttributes().find(
    DW_AT_decl_file);

  if (it != die.getAttributes().end())
  { // The DWARF DIE have the DW_AT_decl_file attribute holding the index
    assert(it->second.cls == DW_FORM_CLASS_CONSTANT);
    assert(it->second.form != DW_FORM_sdata);
    assert(it->second.udata <= (Dwarf_Unsigned)m_srcFileList->srccount);

    // Replace the index with a pointer to the file which the index references
    it->second.cls = DW_FORM_CLASS_STRING;
    it->second.form = DW_FORM_source_file;
    it->second.string = m_srcFileList->srcfiles[it->second.udata - 1];
  }
}

/**
 * Destroys a DwDataObjectFinder object.
 */
DwDataObjectFinder::~DwDataObjectFinder()
{
}

/**
 * Visits a DWARF formal parameter debugging information entry object.
 *
 * @param fp A DWARF formal parameter debugging information entry object.
 */
void DwDataObjectFinder::visit(DwFormalParameter& fp)
{
  m_dataObjectList.push_back(&fp);
}

/**
 * Visits a DWARF variable debugging information entry object.
 *
 * @param v A DWARF variable debugging information entry object.
 */
void DwDataObjectFinder::visit(DwVariable& v)
{
  m_dataObjectList.push_back(&v);
}

/**
 * Constructs a DwVariablePrinter object.
 *
 * @param stream A stream to which the variables should be printed.
 */
DwVariablePrinter::DwVariablePrinter(std::ostream& stream) : m_stream(stream)
{
}

/**
 * Destroys a DwVariablePrinter object.
 */
DwVariablePrinter::~DwVariablePrinter()
{
}

/**
 * Visits a DWARF variable debugging information entry object.
 *
 * @param v A DWARF variable debugging information entry object.
 */
void DwVariablePrinter::visit(DwVariable& v)
{
  // Get the name of the variable (may be NULL for unnamed variables)
  const char *name = v.getName();
  // Get the source file and line number of the declaration of the variable
  const char *srcFile = v.getSourceFile();
  Dwarf_Unsigned lineNumber = v.getLineNumber();

  // Print the information about the variable
  m_stream << v.getType() << " " << ((name) ? name : "UNKNOWN") << " at "
    <<  ((srcFile) ? srcFile : "UNKNOWN") << ", line " << lineNumber << "\n";
}

/**
 * Constructs a DwDieTreeTraverser object.
 */
DwDieTreeTraverser::DwDieTreeTraverser() : m_depth(0)
{
}

/**
 * Destroys a DwDieTreeTraverser object.
 */
DwDieTreeTraverser::~DwDieTreeTraverser()
{
}

/**
 * Constructs a DwDebugInfoPrinter object.
 *
 * @param stream A stream to which the debugging information should be printed.
 */
DwDebugInfoPrinter::DwDebugInfoPrinter(std::ostream& stream)
  : m_stream(stream), m_maxOffsetWidth(0)
{
}

/**
 * Destroys a DwDebugInfoPrinter object.
 */
DwDebugInfoPrinter::~DwDebugInfoPrinter()
{
}

/**
 * Visits a DWARF debugging information entry object.
 *
 * @param die A DWARF debugging information entry object.
 */
void DwDebugInfoPrinter::visit(DwDie& die)
{
  this->printDie(die);
}

/**
 * Visits a DWARF compile unit debugging information entry object.
 *
 * @param die A DWARF compile unit debugging information entry object.
 */
void DwDebugInfoPrinter::visit(DwCompileUnit& cu)
{
  // Update the width needed to print the offsets of the DIEs in the DWARF CU
  m_maxOffsetWidth = boost::lexical_cast< std::string >(cu.getLength()).size();

  // Print basic information about the DWARF CU
  std::cout << "\nCOMPILE_UNIT<header overall offset = " << cu.getGlobalOffset()
    << ">:\n";

  this->printDie(cu);

  // Print information about source files referenced in the DWARF CU
  std::cout << "\nSOURCE_FILES:\n";

  // Get a list containing all source files referenced in the DWARF CU
  const Dwarf_Source_File_List& srcFileList = cu.getSourceFiles();

  // Helper variables
  size_t witdh = boost::lexical_cast< std::string >(srcFileList.srccount).size();

  for (int i = 0; i < srcFileList.srccount; i++)
  { // Print all source files referenced in the DWARF CU
    std::cout << boost::format("[%1%]%|20t|%2%\n") % boost::io::group(
      std::setw(witdh), i + 1) % srcFileList.srcfiles[i];
  }

  // DWARF DIE objects visited after this CU contain info about local symbols
  std::cout << "\nLOCAL_SYMBOLS:\n";
}

/**
 * Prints information about a DWARF debugging information entry to a stream.
 *
 * @param die A DWARF compile unit debugging information entry object.
 */
void DwDebugInfoPrinter::printDie(DwDie& die)
{
  // Print basic information about the DIE (depth, offset and tag name)
  m_stream << boost::format("<%1%><%2%>%|20t|%3%\n") % this->getDepth()
    % boost::io::group(std::setw(m_maxOffsetWidth), die.getOffset())
    % g_tagToStringTable[die.getTag()];

  // Helper variables
  DwDie::Dwarf_Attribute_Map::const_iterator it;

  for (it = die.getAttributes().begin(); it != die.getAttributes().end(); it++)
  { // Print all attributes of the DIE together with their values
    m_stream << boost::format("%|20t|%1%%|50t|%2%\n")
      % g_attrToStringTable[it->first] % it->second;
  }
}

/** End of file dw_visitors.cpp **/
