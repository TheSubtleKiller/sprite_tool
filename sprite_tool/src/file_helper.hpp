
#include <string>
#include <vector>

//========================================
namespace FileHelper
{
	std::string GetFileContentsString(std::string const &_sFilePath);
	std::vector<uint8_t> GetFileContents(std::string const& _sFilePath);
};
//========================================