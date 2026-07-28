<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of LAL.

LAL is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

LAL is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
LAL. If not, see <https://www.gnu.org/licenses/>.
-->

# LAL

LAL is a small C99 audio library for loading and playing PCM WAV files.

It uses a fixed 44.1 kHz, signed 16-bit stereo software mixer with up to 32
simultaneous voices. WAV data is converted to that format when loaded.
Supported input is uncompressed 8-bit or 16-bit PCM, mono or stereo.
Voices support independent volume, stereo pan, looping, and fractional
playback rates with linear interpolation.

Create `LalPlayOptions` with `lal_play_options_default()` and pass it to
`lal_play_ex()`. Volume uses the range 0 to 1, pan uses -1 (left) through
0 (center) to 1 (right), and playback rate uses 1 for the sound's original
speed. The `lal_set_voice_*()` functions can change those properties while a
voice is playing. The compact `lal_play(sound, volume, loop)` API remains
available for centered playback at the original rate.

Native output backends are:

- Windows: waveOut
- Linux: ALSA
- macOS: AudioQueue

The public API is declared in `lal.h`; platform audio headers remain private.

Sounds can be loaded from a WAV path, decoded from a WAV image already in
memory, or created from raw PCM with `lal_sound_create_pcm()`. Raw PCM accepts
unsigned 8-bit or little-endian signed 16-bit samples, with one or two
interleaved channels. `frame_stride_bytes` may be zero for tightly packed
frames.

## License

LAL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
