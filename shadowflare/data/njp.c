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

#include "data/njp.h"

#include "data/rclib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define SF_NJP_PART_LIMIT 80u
#define SF_NJP_PATTERN_LIMIT 80u
#define SF_NJP_REFERENCE_LIMIT 128u
#define SF_NJP_META_PALETTE_LIMIT 80u

typedef struct SfNjpPartMeta {
  long offset;
  uint32_t encoded_size;
  uint32_t decoded_size;
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  uint8_t bits_per_pixel;
  bool compressed;
} SfNjpPartMeta;

typedef struct SfNjpReferenceMeta {
  int32_t part;
  int32_t x;
  int32_t y;
  int32_t palette_offset;
  int32_t scale_x;
  int32_t scale_y;
} SfNjpReferenceMeta;

typedef struct SfNjpPatternMeta {
  uint16_t first_reference;
  uint16_t reference_count;
  int32_t default_palette;
} SfNjpPatternMeta;

typedef struct SfNjpMeta {
  SfNjpPartMeta parts[SF_NJP_PART_LIMIT];
  SfNjpPatternMeta patterns[SF_NJP_PATTERN_LIMIT];
  SfNjpReferenceMeta references[SF_NJP_REFERENCE_LIMIT];
  uint16_t palettes[SF_NJP_META_PALETTE_LIMIT][256];
  uint8_t part_count;
  uint8_t pattern_count;
  uint8_t palette_count;
  uint16_t reference_count;
  uint8_t version;
  bool united;
  bool shadow;
} SfNjpMeta;

static bool sf_read(FILE *file, void *destination, size_t size) {
  return size == 0u || fread(destination, 1u, size, file) == size;
}

static bool sf_skip(FILE *file, long size) {
  return size >= 0 && fseek(file, size, SEEK_CUR) == 0;
}

static bool sf_i32(FILE *file, int32_t *value) {
  uint8_t bytes[4];
  uint32_t result;
  if (!sf_read(file, bytes, sizeof(bytes))) return false;
  result = (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
  *value = (int32_t) result;
  return true;
}

static uint32_t sf_u32_bytes(const uint8_t *bytes) {
  return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8u) |
    ((uint32_t) bytes[2] << 16u) | ((uint32_t) bytes[3] << 24u);
}

static bool sf_dimension(int32_t value, uint16_t *output) {
  if (value <= 0 || value > UINT16_MAX) return false;
  *output = (uint16_t) value;
  return true;
}

static bool sf_stride(uint8_t bits, uint16_t width, uint16_t *output) {
  uint32_t stride;
  if (bits == 1u) stride = ((uint32_t) width + 7u) / 8u;
  else if (bits == 4u) stride = ((uint32_t) width + 1u) / 2u;
  else if (bits == 8u) stride = width;
  else return false;
  stride = (stride + 3u) & ~UINT32_C(3);
  if (stride > UINT16_MAX) return false;
  *output = (uint16_t) stride;
  return true;
}

static bool sf_njp_parse(FILE *file, SfNjpMeta *meta) {
  char header[16];
  int32_t count;
  int32_t index;
  if (!file || !meta || !sf_read(file, header, sizeof(header))) return false;
  memset(meta, 0, sizeof(*meta));
  meta->united = memcmp(header, "UnitePatData", 12u) == 0;
  meta->shadow = memcmp(header, "ShadowLowPat", 12u) == 0;
  if (!meta->united && !meta->shadow &&
      memcmp(header, "NJudgeUniPat", 12u) != 0) return false;
  if (header[12] < '0' || header[12] > '9' ||
      header[13] < '0' || header[13] > '9' ||
      header[14] < '0' || header[14] > '9') return false;
  meta->version = (uint8_t) (
    (header[12] - '0') * 100 + (header[13] - '0') * 10 + header[14] - '0');
  if (meta->version > 3u || !sf_i32(file, &count) || count < 0 ||
      count > (int32_t) SF_NJP_PART_LIMIT) return false;
  meta->part_count = (uint8_t) count;
  if (meta->version > 2u && !sf_skip(file, 4)) return false;
  for (index = 0; index < count; ++index) {
    SfNjpPartMeta *part = &meta->parts[index];
    int32_t bits;
    int32_t width;
    int32_t height;
    int32_t compressed;
    uint8_t compression_header[16];
    if (!sf_i32(file, &bits) || !sf_i32(file, &width) ||
        !sf_i32(file, &height) || !sf_i32(file, &compressed) ||
        !sf_dimension(width, &part->width) ||
        !sf_dimension(height, &part->height)) return false;
    if (meta->shadow) bits = 1;
    if (bits < 0 || bits > UINT8_MAX ||
        !sf_stride((uint8_t) bits, part->width, &part->stride)) return false;
    part->bits_per_pixel = (uint8_t) bits;
    if ((uint32_t) part->stride > UINT32_MAX / part->height) return false;
    part->decoded_size = (uint32_t) part->stride * part->height;
    part->offset = ftell(file);
    if (part->offset < 0) return false;
    part->compressed = compressed != 0;
    if (!part->compressed) {
      part->encoded_size = part->decoded_size;
      if (part->encoded_size > UINT32_C(0x7fffffff) ||
          !sf_skip(file, (long) part->encoded_size)) return false;
    } else {
      if (!sf_read(file, compression_header, sizeof(compression_header)))
        return false;
      part->encoded_size = sf_u32_bytes(compression_header + 12u) + 16u;
      if (part->encoded_size < 16u ||
          part->encoded_size - 16u > UINT32_C(0x7fffffff) ||
          !sf_skip(file, (long) (part->encoded_size - 16u))) return false;
    }
  }
  if (!sf_i32(file, &count) || count < 0 ||
      count > (int32_t) SF_NJP_PATTERN_LIMIT) return false;
  meta->pattern_count = (uint8_t) count;
  if (meta->version > 2u && !sf_skip(file, 4)) return false;
  for (index = 0; index < count; ++index) {
    SfNjpPatternMeta *pattern = &meta->patterns[index];
    int32_t references;
    int32_t ignored;
    int32_t reference;
    if (!sf_i32(file, &references) || references < 0 ||
        references > (int32_t) (SF_NJP_REFERENCE_LIMIT - meta->reference_count) ||
        !sf_i32(file, &ignored) || !sf_i32(file, &ignored) ||
        !sf_i32(file, &ignored) || !sf_i32(file, &ignored)) return false;
    if (meta->united && !sf_skip(file, 0xa8)) return false;
    pattern->default_palette = -1;
    if (meta->version > 0u && !sf_i32(file, &pattern->default_palette))
      return false;
    pattern->first_reference = meta->reference_count;
    pattern->reference_count = (uint16_t) references;
    for (reference = 0; reference < references; ++reference) {
      SfNjpReferenceMeta *item = &meta->references[meta->reference_count++];
      if (!sf_i32(file, &ignored) || !sf_i32(file, &item->part) ||
          !sf_i32(file, &item->x) || !sf_i32(file, &item->y) ||
          !sf_i32(file, &item->palette_offset) ||
          !sf_i32(file, &item->scale_x) || !sf_i32(file, &item->scale_y))
        return false;
    }
  }
  if (!sf_i32(file, &count) || count < 0 ||
      count > (int32_t) SF_NJP_META_PALETTE_LIMIT) return false;
  meta->palette_count = (uint8_t) count;
  for (index = 0; index < count; ++index) {
    unsigned entry;
    for (entry = 0u; entry < 256u; ++entry) {
      uint8_t color[4];
      if (!sf_read(file, color, sizeof(color))) return false;
      meta->palettes[index][entry] = sf_rgb555(
        (uint8_t) (color[0] >> 3u), (uint8_t) (color[1] >> 3u),
        (uint8_t) (color[2] >> 3u));
    }
  }
  return true;
}

static bool sf_single_reference(
    const SfNjpMeta *meta, uint8_t pattern_index,
    const SfNjpPatternMeta **pattern, const SfNjpReferenceMeta **reference,
    const SfNjpPartMeta **part) {
  const SfNjpPatternMeta *selected;
  const SfNjpReferenceMeta *item;
  if (pattern_index >= meta->pattern_count) return false;
  selected = &meta->patterns[pattern_index];
  if (selected->reference_count != 1u || selected->default_palette < 0 ||
      selected->default_palette >= meta->palette_count) return false;
  item = &meta->references[selected->first_reference];
  if (item->part < 0 || item->part >= meta->part_count ||
      item->palette_offset != 0 || item->scale_x != 1000 ||
      item->scale_y != 1000 || item->x < INT16_MIN || item->x > INT16_MAX ||
      item->y < INT16_MIN || item->y > INT16_MAX) return false;
  *pattern = selected;
  *reference = item;
  *part = &meta->parts[item->part];
  return true;
}

bool sf_njp_load_selected(
    const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
    SfArena *arena, SfNjpSelected *output) {
  FILE *file;
  SfNjpMeta meta;
  uint8_t index;
  size_t mark;
  if (!path || !pattern_indices || pattern_count == 0u ||
      pattern_count > SF_NJP_SELECTED_LIMIT || !arena || !output) return false;
  mark = sf_arena_mark(arena);
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_njp_parse(file, &meta)) {
    fclose(file);
    return false;
  }
  memset(output, 0, sizeof(*output));
  for (index = 0u; index < pattern_count; ++index) {
    const SfNjpPatternMeta *pattern;
    const SfNjpReferenceMeta *reference;
    const SfNjpPartMeta *part;
    SfNjpPatternImage *image = &output->images[index];
    uint8_t *pixels;
    uint8_t palette_slot;
    if (!sf_single_reference(&meta, pattern_indices[index],
          &pattern, &reference, &part)) break;
    for (palette_slot = 0u; palette_slot < output->palette_count;
         ++palette_slot) {
      if (output->palette_sources[palette_slot] ==
          pattern->default_palette) break;
    }
    if (palette_slot == output->palette_count) {
      if (output->palette_count >= SF_NJP_SELECTED_PALETTE_LIMIT) break;
      output->palette_sources[palette_slot] =
        (uint8_t) pattern->default_palette;
      memcpy(output->palettes[palette_slot],
        meta.palettes[pattern->default_palette],
        sizeof(output->palettes[palette_slot]));
      ++output->palette_count;
    }
    pixels = (uint8_t *) sf_arena_push(arena, part->decoded_size, 4u);
    if (!pixels || fseek(file, part->offset, SEEK_SET) != 0) break;
    if (part->compressed) {
      if (!sf_rclib_decode_stream(file, pixels, part->decoded_size)) break;
    } else if (!sf_read(file, pixels, part->decoded_size)) break;
    image->image.pixels = pixels;
    image->image.palette = output->palettes[palette_slot];
    image->image.width = part->width;
    image->image.height = part->height;
    image->image.stride = part->stride;
    image->image.palette_size = part->bits_per_pixel == 8u
      ? 256u : (uint16_t) (1u << part->bits_per_pixel);
    image->image.bits_per_pixel = part->bits_per_pixel;
    image->image.bottom_up = true;
    image->x = (int16_t) reference->x;
    image->y = (int16_t) reference->y;
    ++output->image_count;
  }
  fclose(file);
  if (output->image_count == pattern_count) return true;
  (void) sf_arena_rewind(arena, mark);
  memset(output, 0, sizeof(*output));
  return false;
}

bool sf_njp_load_animation(
    const char *path, SfArena *arena, SfNjpAnimation *output) {
  FILE *file;
  SfNjpMeta meta;
  uint8_t index;
  size_t mark;
  if (!path || !arena || !output) return false;
  mark = sf_arena_mark(arena);
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_njp_parse(file, &meta) || meta.pattern_count == 0u ||
      meta.pattern_count > SF_NJP_FRAME_LIMIT) {
    fclose(file);
    return false;
  }
  memset(output, 0, sizeof(*output));
  for (index = 0u; index < meta.pattern_count; ++index) {
    const SfNjpPatternMeta *pattern;
    const SfNjpReferenceMeta *reference;
    const SfNjpPartMeta *part;
    SfNjpCompressedFrame *frame = &output->frames[index];
    uint8_t *bytes;
    if (!sf_single_reference(&meta, index, &pattern, &reference, &part)) break;
    if (index == 0u) {
      output->palette_size = (uint16_t) (1u << part->bits_per_pixel);
      memcpy(output->palette, meta.palettes[pattern->default_palette],
        (size_t) (part->bits_per_pixel == 8u ? 256u : output->palette_size) *
          sizeof(uint16_t));
    }
    bytes = (uint8_t *) sf_arena_push(arena, part->encoded_size, 4u);
    if (!bytes || fseek(file, part->offset, SEEK_SET) != 0 ||
        !sf_read(file, bytes, part->encoded_size)) break;
    frame->bytes = bytes;
    frame->encoded_size = part->encoded_size;
    frame->decoded_size = part->decoded_size;
    frame->width = part->width;
    frame->height = part->height;
    frame->stride = part->stride;
    frame->x = (int16_t) reference->x;
    frame->y = (int16_t) reference->y;
    frame->bits_per_pixel = part->bits_per_pixel;
    frame->compressed = part->compressed;
    ++output->frame_count;
  }
  fclose(file);
  if (output->frame_count == meta.pattern_count) return true;
  (void) sf_arena_rewind(arena, mark);
  memset(output, 0, sizeof(*output));
  return false;
}

static int sf_njp_decoded_part_slot(
    const SfNjpDecodedResource *output, uint8_t source_index) {
  uint8_t slot;
  for (slot = 0u; slot < output->part_count; ++slot) {
    if (output->parts[slot].source_index == source_index) return slot;
  }
  return -1;
}

bool sf_njp_load_decoded_patterns(
    const char *path, const uint8_t *pattern_indices, uint8_t pattern_count,
    SfArena *arena, SfNjpDecodedResource *output) {
  FILE *file;
  SfNjpMeta meta;
  size_t mark;
  uint8_t selected;
  int8_t palette_slots[SF_NJP_META_PALETTE_LIMIT];
  uint8_t selected_palette_count = 0u;
  bool success = false;
  if (!path || !pattern_indices || pattern_count == 0u ||
      pattern_count > SF_NJP_DECODED_PATTERN_LIMIT || !arena || !output)
    return false;
  mark = sf_arena_mark(arena);
  file = fopen(path, "rb");
  if (!file) return false;
  memset(output, 0, sizeof(*output));
  if (!sf_njp_parse(file, &meta)) goto done;
  memset(palette_slots, -1, sizeof(palette_slots));
  for (selected = 0u; selected < pattern_count; ++selected) {
    const uint8_t source_pattern = pattern_indices[selected];
    const SfNjpPatternMeta *pattern;
    if (source_pattern >= meta.pattern_count) goto done;
    pattern = &meta.patterns[source_pattern];
    if (pattern->default_palette < 0 ||
        pattern->default_palette >= meta.palette_count) goto done;
    if (palette_slots[pattern->default_palette] < 0) {
      palette_slots[pattern->default_palette] =
        (int8_t) selected_palette_count++;
    }
  }
  output->palettes = (uint16_t (*)[256]) sf_arena_push(
    arena, (size_t) selected_palette_count * sizeof(*output->palettes),
    sizeof(uint16_t));
  output->palette_sources = (uint8_t *) sf_arena_push(
    arena, selected_palette_count, sizeof(uint8_t));
  if (!output->palettes || !output->palette_sources) goto done;
  output->palette_count = selected_palette_count;
  for (selected = 0u; selected < meta.palette_count; ++selected) {
    const int8_t slot = palette_slots[selected];
    if (slot < 0) continue;
    memcpy(output->palettes[(uint8_t) slot], meta.palettes[selected],
      sizeof(*output->palettes));
    output->palette_sources[(uint8_t) slot] = selected;
  }
  for (selected = 0u; selected < pattern_count; ++selected) {
    const uint8_t source_pattern = pattern_indices[selected];
    const SfNjpPatternMeta *pattern;
    SfNjpDecodedPattern *decoded_pattern;
    uint16_t reference;
    if (source_pattern >= meta.pattern_count ||
        output->pattern_count >= SF_NJP_DECODED_PATTERN_LIMIT)
      goto done;
    pattern = &meta.patterns[source_pattern];
    if (pattern->default_palette < 0 ||
        pattern->default_palette >= meta.palette_count ||
        pattern->reference_count > SF_NJP_DECODED_REFERENCE_LIMIT -
          output->reference_count) goto done;
    decoded_pattern = &output->patterns[output->pattern_count++];
    decoded_pattern->source_index = source_pattern;
    decoded_pattern->palette =
      (uint8_t) palette_slots[pattern->default_palette];
    decoded_pattern->first_reference = output->reference_count;
    decoded_pattern->reference_count = (uint8_t) pattern->reference_count;
    for (reference = 0u; reference < pattern->reference_count; ++reference) {
      const SfNjpReferenceMeta *item =
        &meta.references[pattern->first_reference + reference];
      const SfNjpPartMeta *part;
      SfNjpDecodedReference *decoded_reference;
      int slot;
      if (item->part < 0 || item->part >= meta.part_count ||
          item->palette_offset != 0 || item->scale_x != 1000 ||
          item->scale_y != 1000 || item->x < INT16_MIN ||
          item->x > INT16_MAX || item->y < INT16_MIN ||
          item->y > INT16_MAX) goto done;
      slot = sf_njp_decoded_part_slot(output, (uint8_t) item->part);
      part = &meta.parts[item->part];
      if (slot < 0) {
        SfNjpDecodedPart *decoded_part;
        uint8_t *pixels;
        if (output->part_count >= SF_NJP_DECODED_PART_LIMIT) goto done;
        slot = output->part_count++;
        decoded_part = &output->parts[slot];
        pixels = (uint8_t *) sf_arena_push(arena, part->decoded_size, 4u);
        if (!pixels || fseek(file, part->offset, SEEK_SET) != 0) goto done;
        if (part->compressed) {
          if (!sf_rclib_decode_stream(file, pixels, part->decoded_size))
            goto done;
        } else if (!sf_read(file, pixels, part->decoded_size)) {
          goto done;
        }
        decoded_part->image.pixels = pixels;
        decoded_part->image.palette = NULL;
        decoded_part->image.width = part->width;
        decoded_part->image.height = part->height;
        decoded_part->image.stride = part->stride;
        decoded_part->image.palette_size = part->bits_per_pixel == 8u
          ? 256u : (uint16_t) (1u << part->bits_per_pixel);
        decoded_part->image.bits_per_pixel = part->bits_per_pixel;
        decoded_part->image.bottom_up = true;
        decoded_part->source_index = (uint8_t) item->part;
      }
      decoded_reference = &output->references[output->reference_count++];
      decoded_reference->x = (int16_t) item->x;
      decoded_reference->y = (int16_t) item->y;
      decoded_reference->part = (uint8_t) slot;
    }
  }
  success = true;
done:
  fclose(file);
  if (success) return true;
  (void) sf_arena_rewind(arena, mark);
  memset(output, 0, sizeof(*output));
  return false;
}

bool sf_njp_read_pattern_bounds(
    const char *path, SfNjpPatternBounds *bounds,
    uint8_t capacity, uint8_t *pattern_count) {
  FILE *file;
  SfNjpMeta meta;
  uint8_t pattern_index;
  bool success = false;
  if (!path || !bounds || capacity == 0u || !pattern_count) return false;
  memset(bounds, 0, (size_t) capacity * sizeof(*bounds));
  *pattern_count = 0u;
  file = fopen(path, "rb");
  if (!file) return false;
  if (!sf_njp_parse(file, &meta) || meta.pattern_count > capacity) goto done;
  for (pattern_index = 0u; pattern_index < meta.pattern_count;
       ++pattern_index) {
    const SfNjpPatternMeta *pattern = &meta.patterns[pattern_index];
    SfNjpPatternBounds *output = &bounds[pattern_index];
    uint16_t reference_index;
    int64_t left = INT32_MAX;
    int64_t top = INT32_MAX;
    int64_t right = INT32_MIN;
    int64_t bottom = INT32_MIN;
    for (reference_index = 0u; reference_index < pattern->reference_count;
         ++reference_index) {
      const SfNjpReferenceMeta *reference =
        &meta.references[pattern->first_reference + reference_index];
      const SfNjpPartMeta *part;
      int64_t reference_right;
      int64_t reference_bottom;
      if (reference->part < 0 || reference->part >= meta.part_count ||
          reference->scale_x != 1000 || reference->scale_y != 1000)
        goto done;
      part = &meta.parts[reference->part];
      reference_right = (int64_t) reference->x + part->width;
      reference_bottom = (int64_t) reference->y + part->height;
      if (reference->x < left) left = reference->x;
      if (reference->y < top) top = reference->y;
      if (reference_right > right) right = reference_right;
      if (reference_bottom > bottom) bottom = reference_bottom;
    }
    if (pattern->reference_count > 0u) {
      if (left < INT32_MIN || top < INT32_MIN || right > INT32_MAX ||
          bottom > INT32_MAX || right <= left || bottom <= top) goto done;
      output->x = (int32_t) left;
      output->y = (int32_t) top;
      output->width = (int32_t) (right - left);
      output->height = (int32_t) (bottom - top);
      output->valid = true;
    }
  }
  *pattern_count = meta.pattern_count;
  success = true;
done:
  fclose(file);
  if (!success) {
    memset(bounds, 0, (size_t) capacity * sizeof(*bounds));
    *pattern_count = 0u;
  }
  return success;
}

const SfNjpDecodedPattern *sf_njp_decoded_pattern(
    const SfNjpDecodedResource *resource, uint8_t source_index) {
  uint8_t pattern;
  if (!resource) return NULL;
  for (pattern = 0u; pattern < resource->pattern_count; ++pattern) {
    if (resource->patterns[pattern].source_index == source_index)
      return &resource->patterns[pattern];
  }
  return NULL;
}

const uint16_t *sf_njp_decoded_palette(
    const SfNjpDecodedResource *resource, uint16_t source_index) {
  uint8_t palette;
  if (!resource || !resource->palettes || !resource->palette_sources)
    return NULL;
  for (palette = 0u; palette < resource->palette_count; ++palette) {
    if (resource->palette_sources[palette] == source_index)
      return resource->palettes[palette];
  }
  return NULL;
}

bool sf_njp_decode_frame(
    const SfNjpAnimation *animation, uint8_t frame_index,
    void *scratch, size_t scratch_size, SfNjpPatternImage *output) {
  const SfNjpCompressedFrame *frame;
  if (!animation || frame_index >= animation->frame_count || !scratch ||
      !output) return false;
  frame = &animation->frames[frame_index];
  if (frame->decoded_size > scratch_size) return false;
  if (frame->compressed) {
    if (!sf_rclib_decode_memory(
          frame->bytes, frame->encoded_size,
          (uint8_t *) scratch, frame->decoded_size)) return false;
  } else {
    memcpy(scratch, frame->bytes, frame->decoded_size);
  }
  output->image.pixels = (const uint8_t *) scratch;
  output->image.palette = animation->palette;
  output->image.width = frame->width;
  output->image.height = frame->height;
  output->image.stride = frame->stride;
  output->image.palette_size = animation->palette_size;
  output->image.bits_per_pixel = frame->bits_per_pixel;
  output->image.bottom_up = true;
  output->x = frame->x;
  output->y = frame->y;
  return true;
}

bool sf_njp_find_blank_frames(
    SfNjpAnimation *animation, void *scratch, size_t scratch_size) {
  uint8_t frame_index;
  if (!animation || !scratch) return false;
  for (frame_index = 0u; frame_index < animation->frame_count; ++frame_index) {
    SfNjpPatternImage image;
    const SfNjpCompressedFrame *frame = &animation->frames[frame_index];
    size_t byte;
    bool blank = true;
    if (!sf_njp_decode_frame(
          animation, frame_index, scratch, scratch_size, &image)) return false;
    for (byte = 0u; byte < frame->decoded_size; ++byte) {
      if (((const uint8_t *) image.image.pixels)[byte] != 0u) {
        blank = false;
        break;
      }
    }
    animation->frames[frame_index].blank = blank;
  }
  return true;
}
