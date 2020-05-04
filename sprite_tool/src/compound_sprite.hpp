
#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>

//========================================
class CCompoundSprite
{
public:
	enum class Alignment
	{
		Centre	= 0,
		Left	= 1,
		Right	= 2,
		Top		= 3,
		Bottom	= 4,
		Point	= 5,
	};

	enum class Flip : uint8_t
	{
		Default = 0,
		FlipX	= 1<<0,
		FlipY	= 1<<1,
	};

	struct SActorState
	{
		uint32_t m_uAlignmentX = 0;
		uint32_t m_uAlignmentY = 0;

		float m_fAlpha = 1.0f;
		float m_fAngle = 0.0f;

		union {
			uint32_t m_uColour = 0xFFFFFFFF;
			uint8_t m_RGBA[4];
		};

		uint32_t m_uFlip = 0;

		float m_fPosX = 0.0f;
		float m_fPosY = 0.0f;

		float m_fScaleX = 0.0f;
		float m_fScaleY = 0.0f;

		bool m_bShown = true;
	};

	static SActorState InterpolateActorState(SActorState const & _First, SActorState const & _Second, float const & _fInterp)
	{
		SActorState _Output;

		_Output.m_uAlignmentX = _First.m_uAlignmentX;
		_Output.m_uAlignmentY = _First.m_uAlignmentY;
		_Output.m_fAlpha = _First.m_fAlpha + (_Second.m_fAlpha - _First.m_fAlpha) * _fInterp;
		_Output.m_fAngle = _First.m_fAngle + (_Second.m_fAngle - _First.m_fAngle) * _fInterp;
		for (uint32_t i = 0u; i < 4u; ++i)
		{
			_Output.m_RGBA[i] = static_cast<uint8_t>(_First.m_RGBA[i] + ((_Second.m_RGBA[i] - _First.m_RGBA[i]) * _fInterp));
		}
		_Output.m_uFlip = _First.m_uFlip;
		_Output.m_fPosX = _First.m_fPosX + (_Second.m_fPosX - _First.m_fPosX) * _fInterp;
		_Output.m_fPosY = _First.m_fPosY + (_Second.m_fPosY - _First.m_fPosY) * _fInterp;
		_Output.m_fScaleX = _First.m_fScaleX + (_Second.m_fScaleX - _First.m_fScaleX) * _fInterp;
		_Output.m_fScaleY = _First.m_fScaleY + (_Second.m_fScaleY - _First.m_fScaleY) * _fInterp;
		_Output.m_bShown = _First.m_bShown;

		return _Output;
	}

	struct SActor
	{
		enum class Type
		{
			Sprite = 1,
			Compound = 2,
		};

		SActorState m_State;

		std::string m_sSprite;

		uint32_t m_uType = 1;	// 1 : sprite, 2 : compound-sprite

		uint32_t m_uID = 0;
	};

	struct STimelineFrame
	{
		SActorState m_State;

		float m_fTime = 0.0f;
	};

	void ParseJSON(std::string const& _sJSON);

	std::string const & GetTextureForSprite(std::string const& _sSprite)
	{
		static std::string s_Empty;
		for (auto &itTexture : m_mapTextureSprites)
		{
			for (auto sprite : itTexture.second)
			{
				if (sprite == _sSprite)
				{
					return itTexture.first;
				}
			}
		}
		return s_Empty;
	}

	SActorState GetStateForActorAtTime(uint32_t const _uActorId, float const _fTime);

	std::map<uint32_t, SActor> const& GetActors() const { return m_mapActors; }
	std::map<std::string, std::set<std::string>> const& GetTextureSprites() const { return m_mapTextureSprites; }
	std::map<uint32_t, std::vector<STimelineFrame>> const& GetTimelines() const { return m_mapTimelineStates; }

	float const GetStageLength() const { return m_fStageLength; }

protected:

	uint32_t m_uAlignmentX = 0;
	uint32_t m_uAlignmentY = 0;

	float m_fPointX = 0.0f;
	float m_fPointY = 0.0f;

	std::map<uint32_t, SActor> m_mapActors;
	std::map<std::string, std::set<std::string>> m_mapTextureSprites;
	std::map<uint32_t, std::vector<STimelineFrame>> m_mapTimelineStates;

	float m_fStageLength = 0.0f;
	int32_t m_iVersion = 0;
};
//========================================