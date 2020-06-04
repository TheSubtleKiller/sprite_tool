
#pragma once

#include "spritesheet.hpp"
#include "compound_sprite.hpp"

#include "glm/glm.hpp"

//========================================
namespace gl_render_helper
{
	void DrawSprite(glm::mat4& _matModelView, 
					CSpriteSheet::SSpriteCell const &_SpriteCell,
					CCompoundSprite::SActorState const & _ActorState,
					uint32_t _uShaderProgram,
					uint32_t _uTexture);
};
//========================================
