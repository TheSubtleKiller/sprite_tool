
#include <string>
#include "utility/stl_helper.hpp"

uint32_t const c_uVersionMajor = 0;
uint32_t const c_uVersionMinor = 1;
uint32_t const c_uVersionFix = 0;

std::string GetVersionString()
{
	return stl_helper::Format("v%u.%u.%u", c_uVersionMajor, c_uVersionMinor, c_uVersionFix);
}
