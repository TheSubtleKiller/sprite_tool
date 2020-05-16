
#include "file_helper.hpp"

#include "stl_helper.hpp"

#include "libpng/png.h"

#include "libjpeg/jpeglib.h"
#include "libjpeg/jerror.h"

// native file dialog
#include "nfd/nfd.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <sys/stat.h>

//========================================
namespace FileHelper
{
    std::string OpenFileDialog(std::string const& _sExt, std::string const& _sDefaultPath /*= ""*/)
    {
        std::string _sPathResult;

        nfdchar_t* _pOutPath = nullptr;
        nfdresult_t _Result = NFD_OpenDialog(_sExt.c_str(), _sDefaultPath.c_str(), &_pOutPath);

        if (_Result == NFD_OKAY)
        {
            _sPathResult = std::string(_pOutPath);
            free(_pOutPath);
        }
        else if (_Result == NFD_CANCEL)
        {
            printf("User cancelled.\n");
        }
        else
        {
            printf("Error: %s\n", NFD_GetError());
        }

        return _sPathResult;
    }

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

    bool FileExists(std::string const& _sFilePath)
    {
        struct stat buffer;
        return (stat(_sFilePath.c_str(), &buffer) == 0);
    }

    namespace
    {
        void PNGErrorFunction(png_struct* pstruct, const char* perror)
        {
            fprintf(stderr, "PNG Error: %s\n", perror);
        }

        struct SPNGCustomReadInfo
        {
            SPNGCustomReadInfo(uint8_t* _pData,
                               size_t& _uReadIndex)
                : m_pData(_pData)
                , m_uReadIndex(_uReadIndex)
            { }
            uint8_t* m_pData;
            size_t& m_uReadIndex;
        };

        void PNGCustomReadData(png_structp _pPNG, png_bytep _pData, png_size_t _uLength)
        {
            png_voidp _pPngIO = png_get_io_ptr(_pPNG);

            SPNGCustomReadInfo* _pCustomReadInfo = static_cast<SPNGCustomReadInfo*>(_pPngIO);
            memcpy(_pData, &_pCustomReadInfo->m_pData[_pCustomReadInfo->m_uReadIndex], _uLength);
            _pCustomReadInfo->m_uReadIndex += _uLength;
        }
    }

    const std::vector<std::string> c_vectorExtensions =
    {
        "png",
        "jpeg",
        "jpg",
        "jpng",
    };

    SImageData LoadImage(std::string const& _sFilePath,
                         int32_t& _iWidth,
                         int32_t& _iHeight)
    {
        std::string _sExtension;

        size_t _uExtpos = _sFilePath.rfind(".");
        if (_uExtpos != std::string::npos)
        {
            _sExtension = stl_helper::ToLower( _sFilePath.substr(_uExtpos+1) );
        }

        if (_sExtension == "png")
        {
            auto _pContents = std::make_shared<std::vector<uint8_t>>(FileHelper::GetFileContents(_sFilePath));

            return LoadPNG(_pContents->data(),
                           _iWidth,
                           _iHeight);
        }
        else if (_sExtension == "jpeg" || _sExtension == "jpg")
        {
            auto _pContents = std::make_shared<std::vector<uint8_t>>(FileHelper::GetFileContents(_sFilePath));

            return LoadJPEG(_pContents,
                            _pContents->size(),
                            _iWidth,
                            _iHeight);
        }
        else if (_sExtension == "jpng")
        {
            auto _pContents = std::make_shared<std::vector<uint8_t>>(FileHelper::GetFileContents(_sFilePath));

            return LoadJPNG(_pContents,
                            _iWidth,
                            _iHeight);
        }
        else
        {
            if (_sExtension.empty())
            {
                for (auto const& _sExt : c_vectorExtensions)
                {
                    if (FileExists(_sFilePath + "." + _sExt))
                    {
                        return LoadImage(_sFilePath + "." + _sExt,
                                         _iWidth,
                                         _iHeight);
                    }
                }
            }

            return SImageData();
        }
    }

    SImageData LoadPNG(uint8_t* _pData,
                       int32_t& _iWidth,
                       int32_t& _iHeight,
                       bool const _bConvertGrey /*= true*/,
                       bool const _bSetFiller /*= true*/,
                       bool const _bFlipPng /*= true*/)
    {
        size_t _uReadIndex = 0;

        size_t const c_uPngSigBytes = 8;

        uint8_t _Header[c_uPngSigBytes];
        memcpy(_Header, _pData, c_uPngSigBytes);
        _uReadIndex += c_uPngSigBytes;

        if (png_sig_cmp(_Header, 0, c_uPngSigBytes) != 0)
        {
            fprintf(stderr, "PNG Signature mismatch.\n");
            return SImageData();
        }

        png_structp _pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNGErrorFunction, PNGErrorFunction);
        assert(_pPngStruct);

        png_infop _pPngInfo = png_create_info_struct(_pPngStruct);
        assert(_pPngInfo);

        png_infop _pPngEndInfo = png_create_info_struct(_pPngStruct);
        assert(_pPngEndInfo);

        SPNGCustomReadInfo _CustomReadInfo(_pData, _uReadIndex);
        png_set_read_fn(_pPngStruct, (png_voidp)(&_CustomReadInfo), PNGCustomReadData);
        png_set_sig_bytes(_pPngStruct, c_uPngSigBytes);
        png_read_info(_pPngStruct, _pPngInfo);

        png_uint_32 _uBitDepth, _uColourType;
        _uBitDepth = png_get_bit_depth(_pPngStruct, _pPngInfo);
        _uColourType = png_get_color_type(_pPngStruct, _pPngInfo);

        if (_uColourType == PNG_COLOR_TYPE_GRAY && _uBitDepth < 8) 
        {
            png_set_expand_gray_1_2_4_to_8(_pPngStruct);
        }

        if (_uBitDepth == 16)
        {
            png_set_strip_16(_pPngStruct);
        }

        if (_uColourType == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(_pPngStruct);
        }
        else if (_bConvertGrey && (_uColourType == PNG_COLOR_TYPE_GRAY ||
                                   _uColourType == PNG_COLOR_TYPE_GRAY_ALPHA))
        {
            png_set_gray_to_rgb(_pPngStruct);
        }

        if (png_get_valid(_pPngStruct, _pPngInfo, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(_pPngStruct);
        }
        else if (_bSetFiller)
        {
            png_set_filler(_pPngStruct, 0xff, PNG_FILLER_AFTER);
        }

        png_read_update_info(_pPngStruct, _pPngInfo);

        _iWidth = png_get_image_width(_pPngStruct, _pPngInfo);
        _iHeight = png_get_image_height(_pPngStruct, _pPngInfo);

        png_size_t _uRowBytes = png_get_rowbytes(_pPngStruct, _pPngInfo);
        png_size_t _uTotalBytes = _uRowBytes * (_iHeight);

        std::shared_ptr<std::vector<uint8_t>> _pOutData = std::make_shared<std::vector<uint8_t>>();
        _pOutData->resize(_uTotalBytes);

        png_byte** _pRowPtrs = (png_byte**)malloc((_iHeight) * sizeof(png_byte*));

        if (_bFlipPng)
        {
            for (int32_t i = 0; i < _iHeight; i++)
            {
                _pRowPtrs[i] = _pOutData->data() + i * _uRowBytes;
            }
        }
        else
        {
            for (int32_t i = 0; i < _iHeight; i++)
            {
                _pRowPtrs[i] = _pOutData->data() + (int32_t(_iHeight) - int32_t(1) - i) * _uRowBytes;
            }
        }

        png_read_image(_pPngStruct, _pRowPtrs);

        free(_pRowPtrs);
        png_destroy_read_struct(&_pPngStruct, &_pPngInfo, &_pPngEndInfo);

        return SImageData{ _pOutData, 4 };
    }

    namespace
    {
        METHODDEF(void) JPEGCustomErrorExit(j_common_ptr _pCommon)
        {
            (*_pCommon->err->output_message)(_pCommon);
            throw std::runtime_error("JPEG code error.");
        }

        static char s_JPEGError[JMSG_LENGTH_MAX] = "<NO ERROR>";
        static void JPEGCustomOutputMessage(j_common_ptr _pCommon)
        {
            (*_pCommon->err->format_message)(_pCommon, s_JPEGError);
        }
    };

    SImageData LoadJPEG(std::shared_ptr<std::vector<uint8_t>> _pFileData,
                        size_t m_uDataSize,
                        int32_t& _iWidth,
                        int32_t& _iHeight)
    {
        std::shared_ptr<std::vector<uint8_t>> _pData = std::make_shared<std::vector<uint8_t>>();

        // JPEG decompression parameters and pointers to working space
        struct jpeg_decompress_struct _JPEGInfo;

        JSAMPARRAY _pOutputRowbuffer = nullptr;    // Output row buffer

        // Override error_exit and output message.
        jpeg_error_mgr _ErrorMgr;
        _JPEGInfo.err = jpeg_std_error(&_ErrorMgr);
        _ErrorMgr.error_exit = JPEGCustomErrorExit;
        _ErrorMgr.output_message = JPEGCustomOutputMessage;

        try
        {
            // Initialize JPEG decompression object
            jpeg_create_decompress(&_JPEGInfo);

            // Setup data source
            {
                struct jpeg_source_mgr* _pSrc;

                if (_JPEGInfo.src == NULL)
                {
                    _JPEGInfo.src = (struct jpeg_source_mgr*)
                        (*_JPEGInfo.mem->alloc_small) ((j_common_ptr)&_JPEGInfo,
                                                        JPOOL_PERMANENT,
                                                        sizeof(struct jpeg_source_mgr));
                }

                _pSrc = (struct jpeg_source_mgr*) _JPEGInfo.src;
                _pSrc->init_source = [](j_decompress_ptr _pJPEGInfo) -> void {};
                _pSrc->fill_input_buffer = [](j_decompress_ptr _pJPEGInfo) -> boolean
                {
                    ERREXIT(_pJPEGInfo, JERR_INPUT_EMPTY);
                    return true;
                };
                _pSrc->skip_input_data = [](j_decompress_ptr _pJPEGInfo, long num_bytes) -> void
                {
                    struct jpeg_source_mgr* _pSrc = (struct jpeg_source_mgr*) _pJPEGInfo->src;

                    if (num_bytes > 0) {
                        _pSrc->next_input_byte += (size_t)num_bytes;
                        _pSrc->bytes_in_buffer -= (size_t)num_bytes;
                    }
                };
                _pSrc->resync_to_restart = jpeg_resync_to_restart; /* use default method */
                _pSrc->term_source = [](j_decompress_ptr _pJPEGInfo) {};
                _pSrc->bytes_in_buffer = static_cast<long>(m_uDataSize);
                _pSrc->next_input_byte = (JOCTET*)_pFileData->data();
            }

            (void)jpeg_read_header(&_JPEGInfo, TRUE);

            (void)jpeg_start_decompress(&_JPEGInfo);

            int32_t _iRowStride = _JPEGInfo.output_width * _JPEGInfo.output_components;

            // Make one-row-high sample array
            _pOutputRowbuffer = (*_JPEGInfo.mem->alloc_sarray)
                ((j_common_ptr)&_JPEGInfo, JPOOL_IMAGE, _iRowStride, 1);

            _iWidth = _JPEGInfo.output_width;
            _iHeight = _JPEGInfo.output_height;

            uint32_t _uTotalBytes = _iWidth * _iHeight * _JPEGInfo.output_components;

            // Allocate data buffer
            _pData->resize(_uTotalBytes);

            // Read JPEG scanlines
            while (_JPEGInfo.output_scanline < _JPEGInfo.output_height)
            {
                // Reading 1 at a time
                (void)jpeg_read_scanlines(&_JPEGInfo, _pOutputRowbuffer, 1);

                memcpy(&_pData->data()[(_JPEGInfo.output_scanline - 1) * _iWidth * _JPEGInfo.output_components], _pOutputRowbuffer[0], _iRowStride);
            }

            (void)jpeg_finish_decompress(&_JPEGInfo);

            // Release some memory
            jpeg_destroy_decompress(&_JPEGInfo);
        }
        catch (std::runtime_error const& e)
        {
            std::cout << e.what() << ": '" << s_JPEGError << "'." << std::endl;

            // JPEG error, clean up
            jpeg_destroy_decompress(&_JPEGInfo);
            return SImageData();
        }

        return SImageData{ _pData, 3 };
    }

    namespace
    {
        struct SJPNGInfo
        {
            uint32_t m_uDataSizeJPEG;
            uint32_t m_uDataSizePNG;
            uint16_t m_uSizeJPNGInfo;
            uint8_t m_uVersionMajor;
            uint8_t m_uVersionMinor;
            uint32_t m_uID;
        };

        struct SRGB
        {
            uint8_t m_RGB[3];
        };

        struct SRGBA
        {
            SRGBA(SRGB const & _RGB, uint8_t const _A)
            {
                memcpy(m_RGBA, _RGB.m_RGB, sizeof(SRGB));
                m_RGBA[3] = _A;
            }

            uint8_t m_RGBA[4];
        };
    };


    SImageData LoadJPNG(std::shared_ptr<std::vector<uint8_t>> _pFileData, int32_t& _iWidth, int32_t& _iHeight)
    {
        // Read the JPNG info chunk
        SJPNGInfo _JPNGInfo;
        memcpy(&_JPNGInfo, &_pFileData->data()[_pFileData->size() - sizeof(SJPNGInfo)], sizeof(SJPNGInfo));

        // Get pointer to the PNG data
        uint8_t* _pPNGData = _pFileData->data() + _JPNGInfo.m_uDataSizeJPEG;

        // Read PNG data
        int32_t _iPNGWidth = 0, _iPNGHeight = 0;
        SImageData _ImageDataPNG = LoadPNG(_pPNGData, _iPNGWidth, _iPNGHeight, false, false);

        // Resize output buffer
        int32_t _uTotalBytes = _iPNGWidth * _iPNGHeight * 4;
        std::shared_ptr<std::vector<uint8_t>> _pOutData = std::make_shared<std::vector<uint8_t>>();
        _pOutData->resize(_uTotalBytes);

        // Read JPEG Data
        int32_t _iJPGWidth = 0, _iJPGHeight = 0;
        SImageData _ImageDataJPEG = LoadJPEG(_pFileData, _JPNGInfo.m_uDataSizeJPEG, _iJPGWidth, _iJPGHeight);

        // Put RGB and A data together
        //========================================
        SRGB* _pDataRGB = (SRGB*)_ImageDataJPEG.m_pData->data();
        SRGBA* _pDataRGBA = (SRGBA*)_pOutData->data();
        uint8_t* _pDataA = _ImageDataPNG.m_pData->data();

        _iWidth = _iJPGWidth;
        _iHeight = _iJPGHeight;

        for (int x = 0; x < _iWidth; x++)
        {
            for (int y = 0; y < _iHeight; y++)
            {
                _pDataRGBA[(y * _iWidth) + x] = SRGBA(_pDataRGB[(y * _iWidth) + x],
                                                      _pDataA[(y * _iWidth) + x]);
            }
        }
        //========================================

        return SImageData{ _pOutData , 4 };
    }
};
//========================================