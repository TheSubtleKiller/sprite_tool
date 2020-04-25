
#include "file_helper.hpp"

#include <iostream>
#include <fstream>
#include <string>

//========================================
namespace FileHelper
{
	std::string GetFileContentsString(std::string const& _sFilePath)
	{
        std::string _sContents;

        std::ifstream _File(_sFilePath, std::ios::in | std::ios::binary);
        if (_File)
        {
            _File.seekg(0, std::ios::end);
            _sContents.resize(_File.tellg());
            _File.seekg(0, std::ios::beg);
            _File.read(&_sContents[0], _sContents.size());
            _File.close();
        }

        return _sContents;
	}

    std::vector<uint8_t> GetFileContents(std::string const& _sFilePath)
    {
        std::vector<uint8_t> _Contents;

        std::ifstream _File(_sFilePath, std::ios::in | std::ios::binary);
        if (_File)
        {
            _File.seekg(0, std::ios::end);
            _Contents.resize(_File.tellg());
            _File.seekg(0, std::ios::beg);
            _File.read(reinterpret_cast<char*>(&_Contents[0]), _Contents.size());
            _File.close();
        }

        return _Contents;
    }
};
//========================================