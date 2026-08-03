<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of TAL.

TAL is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

TAL is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
TAL. If not, see <https://www.gnu.org/licenses/>.
-->

# TAL

TAL is a compact C11 audio mixer for programs with strict memory limits. It is
independent of LAL and never loads files, decodes WAV data, starts a helper
thread, or calls a memory allocator. Sounds are immutable views over PCM that
the caller already owns.

The caller chooses the output format, voice capacity, and mix-block size,
queries `tal_memory_required()`, and supplies the complete storage block to
`tal_init()`. Voice state and the device mix block are partitioned from that
memory once. No capacity changes at runtime.

## Mixer

The current common mixer supports signed 16-bit and unsigned 8-bit mono or
stereo PCM. The 8-bit path lets memory-constrained callers keep compact source
audio instead of expanding it before playback. The mixer uses fixed-point
playback positions, volume, pan, playback rate, and interpolation. There is no
floating-point work in the C mixer. Samples can remain at their own rate and
are resampled as they play.

Manual output mode does not open a device. `tal_render()` writes directly into
a caller-provided buffer, making the same mixer usable by tests, tools, and
future pull-style console backends.

Windows and macOS synchronize voice changes once around each device block so
their system audio callbacks cannot race the caller thread. There is no locking,
allocation, or format setup inside the per-sample mixing loop.

## Backends

Implemented backends are:

- Linux: nonblocking ALSA output pumped by `tal_update()`, without a thread;
- Windows: waveOut with three caller-funded output blocks;
- macOS: Core Audio rendering directly into the device buffer;
- WebAssembly: Web Audio pulling from the caller-funded C mix block;
- null: used on targets whose real backend has not been written yet.

The Web Audio API and desktop audio drivers may allocate opaque resources
inside the browser or operating system. TAL itself never invokes an allocator
and never owns those resources as general-purpose memory.

Encoded and streaming sources will be added through the same backend boundary
before a constrained console backend is implemented. Runtime WAV decoding is
intentionally outside TAL; host-side asset tools should prepare the format a
target actually needs, such as SPU ADPCM.

## Memory guarantees

- TAL never calls `malloc`, `calloc`, `realloc`, or `free`.
- It does not use files, standard I/O, libc string functions, or threads.
- The caller owns all voice, mixer, and sample memory.
- Voice and block capacities are fixed during initialization.
- `tal_update()` performs a bounded number of nonblocking device writes.

## License

TAL is licensed under the GNU General Public License, version 3 or later. The
full license text is available in
[`../../LICENSES/GPL-3.0-or-later.txt`](../../LICENSES/GPL-3.0-or-later.txt).
