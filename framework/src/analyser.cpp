/*
 * Copyright (C) 2011-2018 Jan Fiedor <fiedorjan@centrum.cz>
 *
 * This file is part of ANaConDA.
 *
 * ANaConDA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * ANaConDA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @brief Contains implementation of classes representing program analysers.
 *
 * A file containing implementation of classes representing program analysers.
 *
 * @file      analyser.cpp
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2011-12-08
 * @date      Last Update 2012-03-16
 * @version   0.1.2
 */

#include "analyser.h"

/**
 * Constructs an Analyser object.
 *
 * @param shlib A shared library containing a program analyser.
 */
Analyser::Analyser(SharedLibrary* shlib) : m_shlib(shlib)
{
}

/**
 * Constructs an Analyser object from an existing Analyser object.
 *
 * @param a An object representing a program analyser.
 */
Analyser::Analyser(const Analyser& a) : m_shlib(new SharedLibrary(*a.m_shlib))
{
}

/**
 * Destroys an Analyser object.
 */
Analyser::~Analyser()
{
  delete m_shlib;
}

/**
 * Initialises a program analyser.
 */
void Analyser::init()
{
  // Type definitions
  typedef void (*init_func_t)();

  // Get a generic pointer to the init function in the program analyser
  void* symbol = m_shlib->resolve("init");

  if (symbol != NULL)
  { // If the init function is present in the program analyser, call it
    ((init_func_t)symbol)();
  }
}

/**
 * Finalises a program analyser.
 */
void Analyser::finish()
{
  // Type definitions
  typedef void (*finish_func_t)();

  // Get a generic pointer to the finish function in the program analyser
  void* symbol = m_shlib->resolve("finish");

  if (symbol != NULL)
  { // If the finish function is present in the program analyser, call it
    ((finish_func_t)symbol)();
  }
}

/**
 * Loads a program analyser.
 *
 * @param path A path to the program analyser.
 * @param error A reference to a string where the description of an error will
 *   be stored if the program analyser could not be loaded.
 * @return A pointer to an object representing the loaded program analyser or
 *   @em NULL if the program analyser could not be loaded.
 */
Analyser* Analyser::Load(fs::path path, std::string& error)
{
  // Load the shared library containing the program analyser
  SharedLibrary* shlib = SharedLibrary::Load(path, error);

  // If the shared library cannot be loaded, signal error
  if (shlib == NULL) return NULL;

  // If everything loaded successfully, return a new analyser object
  return new Analyser(shlib);
}

/** End of file analyser.cpp **/
