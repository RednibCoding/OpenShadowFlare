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

#ifndef SHADOWFLARE_ASSETS_RETAIL_PATHS_H
#define SHADOWFLARE_ASSETS_RETAIL_PATHS_H

typedef struct SfRetailTitlePaths {
  const char *artwork;
  const char *smoke_artwork_format;
  const char *smoke_animation_format;
  const char *music;
  const char *common_sounds;
} SfRetailTitlePaths;

extern const SfRetailTitlePaths sf_retail_title_paths;

#endif
