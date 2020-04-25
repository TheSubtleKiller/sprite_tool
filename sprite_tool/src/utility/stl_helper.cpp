
#include "stl_helper.hpp"

#include <string>

//========================================
namespace stl_helper
{
	std::string Format(char const* _psFmt, ...)
	{
		static std::string const s_sError = "<STRING FORMAT ERROR>";

		if (strlen(_psFmt) == 0)
		{
			return s_sError;
		}

		va_list ap;
		va_start(ap, _psFmt);
		int _iBytesToWrite = vsnprintf(nullptr, 0, _psFmt, ap);
		va_end(ap);

		if (_iBytesToWrite < 0)
		{
			return s_sError;
		}

		std::string _sOutput;
		_sOutput.resize(_iBytesToWrite);

		va_start(ap, _psFmt);
		int _iRetVal = vsnprintf(&_sOutput[0], _sOutput.size()+1, _psFmt, ap);
		va_end(ap);

		if (_iRetVal < 0)
		{
			return s_sError;
		}

		return _sOutput;
	}
};
//========================================