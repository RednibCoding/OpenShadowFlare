/*
 * Copyright (C) 2026 Michael Binder and contributors
 *
 * This file is part of OpenShadowFlare.
 *
 * OpenShadowFlare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * OpenShadowFlare is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SHADOWFLARE_CORE_MEMORY_BUDGET_H
#define SHADOWFLARE_CORE_MEMORY_BUDGET_H

#define SF_KIBIBYTE 1024u
#define SF_MEBIBYTE (1024u * SF_KIBIBYTE)

#define SF_MAIN_MEMORY_LIMIT_BYTES (8u * SF_MEBIBYTE)
#define SF_MAIN_SYSTEM_RESERVE_BYTES (1u * SF_MEBIBYTE)
#define SF_MAIN_ARENA_BYTES \
  (SF_MAIN_MEMORY_LIMIT_BYTES - SF_MAIN_SYSTEM_RESERVE_BYTES)

#define SF_VIDEO_MEMORY_LIMIT_BYTES (4u * SF_MEBIBYTE)
#define SF_FRAME_WIDTH 640u
#define SF_FRAME_HEIGHT 480u
#define SF_FRAME_BYTES_PER_PIXEL 2u
#define SF_FRAMEBUFFER_BYTES \
  (SF_FRAME_WIDTH * SF_FRAME_HEIGHT * SF_FRAME_BYTES_PER_PIXEL)
#define SF_VIDEO_ASSET_BUDGET_BYTES \
  (SF_VIDEO_MEMORY_LIMIT_BYTES - SF_FRAMEBUFFER_BYTES)

typedef char SfMainBudgetIsValid[
  SF_MAIN_ARENA_BYTES <= SF_MAIN_MEMORY_LIMIT_BYTES ? 1 : -1];
typedef char SfFramebufferFitsVideoMemory[
  SF_FRAMEBUFFER_BYTES <= SF_VIDEO_MEMORY_LIMIT_BYTES ? 1 : -1];

#endif
