#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"


// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// "tree" obstacle
Entity createTree(RenderSystem* renderer, vec2 pos, vec2 scale);
