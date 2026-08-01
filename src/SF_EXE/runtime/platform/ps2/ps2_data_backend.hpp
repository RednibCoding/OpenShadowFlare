// PlayStation 2 game-data backend.
//
// The BIOS fileio module strips '\' and '/' from cdrom paths and cdvdman
// searches ISO9660 names case-sensitively, so only flat, uppercase, short
// root files are reachable through fileio.  The game data therefore ships as a
// single root file (SFGAME.BIN) packed by tools/ps2/pack.c.

#ifndef OSF_PS2_DATA_BACKEND_HPP_
#define OSF_PS2_DATA_BACKEND_HPP_

namespace osf {
namespace runtime {
namespace platform {
namespace ps2 {

int initDataBackend();

}  // namespace ps2
}  // namespace platform
}  // namespace runtime
}  // namespace osf

#endif  // OSF_PS2_DATA_BACKEND_HPP_
