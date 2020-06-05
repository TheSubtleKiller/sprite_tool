
#include <string>
#include <vector>
#include <memory>

//========================================
namespace FileHelper
{
    std::string GetAbsolutePath(std::string const& _sPath);

    std::string OpenFileDialog(std::string const& _sExt, std::string const& _sDefaultPath = "");
    std::string PickFolderDialog(std::string const& _sDefaultPath = "");

	std::string GetFileContentsString(std::string const &_sFilePath);
	std::vector<uint8_t> GetFileContents(std::string const& _sFilePath);

    bool FileExists(std::string const& _sFilePath);

    struct SImageData
    {
        std::shared_ptr<std::vector<uint8_t>> m_pData;
        uint32_t m_uChannels = 0;
    };

    SImageData LoadImageFromFile(std::string const& _sFilePath,
                                 int32_t& _iWidth,
                                 int32_t& _iHeight);

    SImageData LoadPNG(uint8_t* _pData,
                       int32_t & _iWidth,
                       int32_t & _iHeight,
                       bool bConvertGrey = true,
                       bool bSetFiller = true,
                       bool bFlipPng = true);

    SImageData LoadJPEG(std::shared_ptr<std::vector<uint8_t>> _pFileData,
                        size_t _uDataSize,
                        int32_t& _iWidth, 
                        int32_t& _iHeight);

    SImageData LoadJPNG(std::shared_ptr<std::vector<uint8_t>> _pFileData, 
                        int32_t& _iWidth, 
                        int32_t& _iHeight);

};
//========================================