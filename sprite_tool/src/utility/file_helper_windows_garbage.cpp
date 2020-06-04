
#include "file_helper.hpp"

#include <windows.h>
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <string>
#include <cassert>


//========================================
namespace FileHelper
{
    std::string GetAbsolutePath(std::string const& _sPath)
    {
        char _cBuffer[MAX_PATH + 1] = {0};
        LPSTR * lppPart = { NULL };
        GetFullPathNameA(_sPath.c_str(), MAX_PATH, _cBuffer, lppPart);

        std::string _sRetVal(_cBuffer);

        return _sRetVal;
    }
};
//========================================