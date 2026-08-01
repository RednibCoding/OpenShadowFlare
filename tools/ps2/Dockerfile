# Builds on the official ps2dev image, but replaces its stale/legacy ps2sdk
# with a hash-verified ps2sdk revision and adds the build and disc-image tools that
# the base image omits (make, cmake, genisoimage, ...).
#
# The ps2sdk rebuild is serial on purpose: GCC 15's lto1 crashes with
# "resolution sub id not in object file" / "file too short" when the SDK's
# LTO-heavy kernel objects are built in parallel (-j).
#
# Build:
#   docker build -t openshadowflare-ps2 -f tools/ps2/Dockerfile .
#
# Run:
#   docker run --rm -it -v "$PWD:/work" -w /work openshadowflare-ps2 sh
#
# Inside the container PS2DEV, PS2SDK and GSKIT are pre-set and the EE
# compiler is `mips64r5900el-ps2-elf-gcc` (R5900, GCC 15.2.0).

FROM ps2dev/ps2dev@sha256:c64ae69c9817865ed98ff054e4ae5360b9e280ed952c97946bca95d9d35be995

# The build verifies this is still the intended PS2SDK master revision before
# compiling. If upstream advances, the image fails loudly instead of silently
# changing the SDK used by the port.
ARG PS2SDK_REF=e228ff7b61a12ad1192a49338754534362a26e58

ENV PS2DEV=/usr/local/ps2dev
ENV PS2SDK=$PS2DEV/ps2sdk
ENV GSKIT=$PS2DEV/gsKit
ENV PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2DEV/dvp/bin:$PS2SDK/bin:$PS2SDK/ee/bin:$PS2SDK/iop/bin

RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    bash \
    perl \
    cdrkit

RUN rm -rf $PS2SDK/ee $PS2SDK/iop $PS2SDK/common $PS2SDK/bin $PS2SDK/samples \
    && git clone --depth 1 --branch master https://github.com/ps2dev/ps2sdk.git /tmp/ps2sdk \
    && test "$(git -C /tmp/ps2sdk rev-parse HEAD)" = "$PS2SDK_REF" \
    && cd /tmp/ps2sdk \
    && make -j1 \
    && make install \
    && cd / \
    && rm -rf /tmp/ps2sdk \
    && apk del git bash perl \
    && ln -sf ../../../ps2sdk/ee/lib/libcglue.a $PS2DEV/ee/mips64r5900el-ps2-elf/lib/libcglue.a \
    && ln -sf ../../../ps2sdk/ee/lib/libpthreadglue.a $PS2DEV/ee/mips64r5900el-ps2-elf/lib/libpthreadglue.a \
    && ln -sf ../../../ps2sdk/ee/lib/libprofglue.a $PS2DEV/ee/mips64r5900el-ps2-elf/lib/libprofglue.a \
    && ln -sf ../../../ps2sdk/ee/lib/libkernel.a $PS2DEV/ee/mips64r5900el-ps2-elf/lib/libkernel.a \
    && ln -sf ../../../ps2sdk/ee/lib/libcdvd.a $PS2DEV/ee/mips64r5900el-ps2-elf/lib/libcdvd.a

CMD ["sh"]
