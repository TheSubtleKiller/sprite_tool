
#include <string>
#include <vector>
#include <memory>

//========================================
namespace FileHelper
{
	std::string GetFileContentsString(std::string const &_sFilePath);
	std::vector<uint8_t> GetFileContents(std::string const& _sFilePath);

    std::shared_ptr<std::vector<uint8_t>> LoadPng(std::string const& _sFilePath,
                                                  int32_t & width,
                                                  int32_t & height,
                                                  bool bConvertGrey = true,
                                                  bool bSetFiller = true,
                                                  bool bFlipPng = true);
};
//========================================