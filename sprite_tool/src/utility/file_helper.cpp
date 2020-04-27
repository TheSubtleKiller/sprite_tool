
#include "file_helper.hpp"

#include "libpng/png.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

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

    namespace
    {
        void PNGErrorFunction(png_struct* pstruct, const char* perror)
        {
            fprintf(stderr, "PNG Error: %s\n", perror);
        }

        struct SPNGCustomReadInfo
        {
            SPNGCustomReadInfo(std::vector<uint8_t>& _Contents,
                               size_t& _uReadIndex)
                : m_Contents(_Contents)
                , m_uReadIndex(_uReadIndex)
            { }
            std::vector<uint8_t>& m_Contents;
            size_t& m_uReadIndex;
        };

        void custom_png_read_data(png_structp pngPtr, png_bytep data, png_size_t length)
        {
            png_voidp _pPngIO = png_get_io_ptr(pngPtr);

            SPNGCustomReadInfo* _pCustomReadInfo = static_cast<SPNGCustomReadInfo*>(_pPngIO);
            memcpy(data, &_pCustomReadInfo->m_Contents[_pCustomReadInfo->m_uReadIndex], length);
            _pCustomReadInfo->m_uReadIndex += length;
        }
    }

    std::shared_ptr<std::vector<uint8_t>> LoadPng(std::string const& _sFilePath,
                                                  int32_t& _iWidth,
                                                  int32_t& _iHeight,
                                                  bool const _bConvertGrey /*= true*/,
                                                  bool const _bSetFiller /*= true*/,
                                                  bool const _bFlipPng /*= true*/)
    {
        std::vector<uint8_t> _Contents = FileHelper::GetFileContents(_sFilePath);
        size_t _uReadIndex = 0;

        size_t const c_uPngSigBytes = 8;

        uint8_t header[c_uPngSigBytes];
        memcpy(header, _Contents.data(), c_uPngSigBytes);
        _uReadIndex += c_uPngSigBytes;

        if (png_sig_cmp(header, 0, c_uPngSigBytes) != 0)
        {
            fprintf(stderr, "PNG Signature mismatch.\n");
            return nullptr;
        }

        png_structp _pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, PNGErrorFunction, PNGErrorFunction);
        assert(_pPngStruct);

        png_infop _pPngInfo = png_create_info_struct(_pPngStruct);
        assert(_pPngInfo);

        png_infop _pPngEndInfo = png_create_info_struct(_pPngStruct);
        assert(_pPngEndInfo);

        SPNGCustomReadInfo _CustomReadInfo(_Contents, _uReadIndex);
        png_set_read_fn(_pPngStruct, (png_voidp)(&_CustomReadInfo), custom_png_read_data);
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

        std::shared_ptr<std::vector<uint8_t>> _pData = std::make_shared<std::vector<uint8_t>>();
        _pData->resize(_uTotalBytes);

        png_byte** _pRowPtrs = (png_byte**)malloc((_iHeight) * sizeof(png_byte*));

        if (_bFlipPng)
        {
            for (int32_t i = 0; i < _iHeight; i++)
            {
                _pRowPtrs[i] = _pData->data() + i * _uRowBytes;
            }
        }
        else
        {
            for (int32_t i = 0; i < _iHeight; i++)
            {
                _pRowPtrs[i] = _pData->data() + ((_iHeight) - int32_t(1) - i) * _uRowBytes;
            }
        }

        png_read_image(_pPngStruct, _pRowPtrs);

        free(_pRowPtrs);
        png_destroy_read_struct(&_pPngStruct, &_pPngInfo, &_pPngEndInfo);

        return _pData;
    }
};
//========================================