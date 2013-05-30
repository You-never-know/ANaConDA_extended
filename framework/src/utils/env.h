/**
 * @brief Contains definitions of functions for accessing environment variables.
 *
 * A file containing definitions of functions for accessing environment
 *   variables.
 *
 * @file      env.h
 * @author    Jan Fiedor (fiedorjan@centrum.cz)
 * @date      Created 2012-06-18
 * @date      Last Update 2013-05-30
 * @version   0.1.0.1
 */

#ifndef __PINTOOL_ANACONDA__UTILS__ENV_H__
  #define __PINTOOL_ANACONDA__UTILS__ENV_H__

#include <map>
#include <string>

// Type definitions
typedef std::map< std::string, std::string > EnvVarMap;

void getEnvVars(EnvVarMap& envVars);

#endif /* __PINTOOL_ANACONDA__UTILS__ENV_H__ */

/** End of file env.h **/
