
#include "compound_sprite.hpp"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "utility/file_helper.hpp"
#include "utility/stl_helper.hpp"

#include <iostream>
#include <string>
#include <stdlib.h>


//========================================
CCompoundSprite::SActor * CCompoundSprite::GetActorById(uint32_t _uId)
{
    for (auto & _Actor : m_vectorActors)
    {
        if (_Actor.m_uID == _uId)
        {
            return &_Actor;
        }
    }

    return nullptr;
}
//========================================

//========================================
CCompoundSprite::SActorState ParseActorState(rapidjson::Value const &_root)
{
    CCompoundSprite::SActorState _ActorState;

    using namespace rapidjson;

    if (_root.HasMember("Alignment"))
    {
        Value const& _valAlignment = _root["Alignment"];
        if (_valAlignment.IsArray() && _valAlignment.Size() == 2)
        {
            _ActorState.m_uAlignmentX = _valAlignment[0].GetUint();
            _ActorState.m_uAlignmentY = _valAlignment[1].GetUint();
        }
    }

    if (_root.HasMember("Alpha"))
    {
        _ActorState.m_fAlpha = _root["Alpha"].GetFloat();
    }
    _ActorState.m_fAngle = _root["Angle"].GetFloat();

    if (_root.HasMember("Colour"))
    {
        _ActorState.m_uColour = _root["Colour"].GetUint();
    }

    _ActorState.m_uFlip = _root["Flip"].GetUint();

    Value const& _valPos = _root["Position"];
    if (_valPos.IsArray() && _valPos.Size() == 2)
    {
        _ActorState.m_fPosX = _valPos[0].GetFloat();
        _ActorState.m_fPosY = _valPos[1].GetFloat();
    }

    Value const& _valScale = _root["Scale"];
    if (_valScale.IsArray() && _valScale.Size() == 2)
    {
        _ActorState.m_fScaleX = _valScale[0].GetFloat();
        _ActorState.m_fScaleY = _valScale[1].GetFloat();
    }

    _ActorState.m_bShown = _root["Shown"].GetBool();

    return _ActorState;
}

void CCompoundSprite::ParseJSONFileRecursive(std::string const& _sFile,
                                             std::map<std::string, tSharedCompoundSprite>& _mapCompounds)
{
    std::string _sAbsPath = FileHelper::GetAbsolutePath(_sFile);

    // If file not already loaded in our map
    if (_mapCompounds.find(_sAbsPath) == _mapCompounds.end())
    {
        // create and parse
        tSharedCompoundSprite _pCompound(new CCompoundSprite());
        _pCompound->ParseJSONFile(_sAbsPath);

        // add to map
        _mapCompounds[_sAbsPath] = _pCompound;

        // find any sub-compounds in this one
        auto& _vectorActors = _pCompound->GetActors();
        for (auto &_Actor : _vectorActors)
        {
            if (_Actor.m_uType == static_cast<uint32_t>(SActor::Type::Compound))
            {
                // path should be relative, so modify current path to find new compound
                std::string _sSubPath = _sAbsPath;

                size_t _uPos = _sSubPath.find_last_of("/");
                if (_uPos == std::string::npos)
                {
                    _uPos = _sSubPath.find_last_of("\\");
                }
                _sSubPath.replace(_uPos+1, std::string::npos, _Actor.m_sSprite);

                _Actor.m_sSubCompoundPath = _sSubPath;

                ParseJSONFileRecursive(_sSubPath, _mapCompounds);
            }
        }
    }
}

void CCompoundSprite::ParseJSONFile(std::string const& _sFile)
{
    std::string _sAbsPath = FileHelper::GetAbsolutePath(_sFile);

    std::string _sJson = FileHelper::GetFileContentsString(_sAbsPath);

    ParseJSONData(_sJson);
}

void CCompoundSprite::ParseJSONData(std::string const& _sJSON)
{
    assert(_sJSON.empty() == false);

    using namespace rapidjson;

    Document _doc;
    _doc.Parse(_sJSON.c_str());

    // Read alignment
    if (_doc.HasMember("Alignment"))
    {
        Value& _valAlignment = _doc["Alignment"];
        if (_valAlignment.IsArray() && _valAlignment.Size() == 2)
        {
            m_uAlignmentX = _valAlignment[0].GetUint();
            m_uAlignmentY = _valAlignment[1].GetUint();
        }
    }

    // Read Point
    if (_doc.HasMember("Point"))
    {
        Value& _valPoint = _doc["Point"];
        if (_valPoint.IsArray() && _valPoint.Size() == 2)
        {
            m_fPointX = _valPoint[0].GetFloat();
            m_fPointY = _valPoint[1].GetFloat();
        }
    }

    //---------- Read stage options
    Value& _valStageOptions = _doc["stageOptions"];
    if (_valStageOptions.IsObject())
    {
        m_fStageLength = _valStageOptions["StageLength"].GetFloat();
        if (_valStageOptions.HasMember("Version"))
        {
            m_iVersion = _valStageOptions["Version"].GetInt();
        }

        Value& _valSpriteInfo = _valStageOptions["SpriteInfo"];

        if (_valSpriteInfo.IsArray())
        {
            for (Value::ConstValueIterator _it = _valSpriteInfo.Begin(); _it != _valSpriteInfo.End(); ++_it)
            {
                std::string _sSprite = (*_it)["SpriteInfo"].GetString();
                std::string _sTexture = (*_it)["Texture"].GetString();

                m_mapTextureSprites[_sTexture].insert(_sSprite);

                //std::cout << "Sprite: " << _sSprite << ", Texture: " << _sTexture << std::endl;
            }
        }
    }

    //---------- Read actors
    Value& _valActors = _doc["actors"];
    if (_valActors.IsArray())
    {
        for (Value::ConstValueIterator _it = _valActors.Begin(); _it != _valActors.End(); ++_it)
        {
            Value const& _valActor = (*_it);

            SActor _Actor;

            _Actor.m_State = ParseActorState(_valActor);

            _Actor.m_sSprite = _valActor["sprite"].GetString();
            _Actor.m_uType = _valActor["type"].GetUint();
            _Actor.m_uID = _valActor["uid"].GetUint();

            m_vectorActors.push_back(_Actor);
        }
    }

    //---------- Read timelines
    Value& _valTimelines = _doc["timelines"];
    if (_valTimelines.IsArray())
    {
        for (Value::ConstValueIterator _itTimeline = _valTimelines.Begin(); _itTimeline != _valTimelines.End(); ++_itTimeline)
        {
            Value const& _valTimeline = (*_itTimeline);

            uint32_t _uActorId = _valTimeline["spriteuid"].GetUint();

            std::vector<STimelineFrame> _vectorFrames;

            Value const& _valStage = _valTimeline["stage"];
            if (_valStage.IsArray())
            {
                for (Value::ConstValueIterator _itFrame = _valStage.Begin(); _itFrame != _valStage.End(); ++_itFrame)
                {
                    Value const& _valFrame = (*_itFrame);

                    STimelineFrame _Frame;

                    _Frame.m_State = ParseActorState(_valFrame);

                    _Frame.m_fTime = _valFrame["Time"].GetFloat();

                    _vectorFrames.emplace_back(_Frame);
                }
            }

            m_mapTimelineStates[_uActorId] = _vectorFrames;
        }
    }

    return;
}
//========================================

//========================================
CCompoundSprite::SActorState CCompoundSprite::GetStateForActorAtTime(uint32_t const _uActorId, float const _fTime)
{
    SActorState _CurrentState;

    // Find actor for id
    auto _pActor = GetActorById(_uActorId);
    if (_pActor == nullptr)
    {
        // No actor found, return default state
        return _CurrentState;
    }
    else
    {
        // Set current state to actor initial values
        _CurrentState = _pActor->m_State;
    }

    // get timeline for sprite
    auto _itTimeline = m_mapTimelineStates.find(_uActorId);
    if (_itTimeline != m_mapTimelineStates.end())
    {
        auto const& _vectorTimeline = _itTimeline->second;

        // search for prev/next keyframe on timeline for given time
        STimelineFrame const* _pPrev = nullptr;
        STimelineFrame const* _pNext = nullptr;

        for (auto const &_Frame : _vectorTimeline)
        {
            if (_Frame.m_fTime <= _fTime)
            {
                _pPrev = &_Frame;
            }

            if (_Frame.m_fTime >= _fTime)
            {
                _pNext = &_Frame;
                break;
            }
        }

        // If we found a previous and next keyframe for given time
        if (_pNext != nullptr && 
            _pPrev != nullptr)
        {
            if (_pNext == _pPrev)
            {
                _CurrentState = _pNext->m_State;
            }
            else
            {
                _CurrentState = InterpolateActorState(_pPrev->m_State, _pNext->m_State, (_fTime - _pPrev->m_fTime) / (_pNext->m_fTime - _pPrev->m_fTime));
            }
        }
        // If we only found a previous keyframe
        else if (_pPrev != nullptr) 
        {
            _CurrentState = _pPrev->m_State;
        }
        // If we only found a next keyframe
        else if (_pNext != nullptr)
        {
            _CurrentState = _pNext->m_State;
        }
    }

    return _CurrentState;
}
//========================================