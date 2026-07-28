/**
 * RKC_RPGSCRN - RPG Screen rendering
 * 
 * Classes: RKC_RPGSCRN, RKC_RPGSCRN_OBJECT, RKC_RPGSCRN_OBJECTDISP,
 *          RKC_RPGSCRN_OBJECTDISPCELL, RKC_RPGSCRN_OBJECTBLOCK,
 *          RKC_RPGSCRN_GROUNDBLOCK, RKC_RPGSCRN_CHARANIM*, etc.
 */

#include <windows.h>
#include <cstring>

extern "C" {

// ============================================================================
// INCREMENTAL RECONSTRUCTION
// Foundational layouts and accessors are exact; complex map, animation-file,
// sorting, and display operations below remain incremental.
// ============================================================================

// RKC_RPGSCRN - partial stubs
void* __thiscall RKC_RPGSCRN_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x2c);
    return self;
}
void __thiscall RKC_RPGSCRN_DeleteGroundBlock(void* self, long index) {}
void __thiscall RKC_RPGSCRN_DeleteObjectBlock(void* self, long index) {}
void __thiscall RKC_RPGSCRN_GetBaseParam(void* self, long* a, long* b, long* c) {
    *a = *(long*)((char*)self + 0x18);
    *b = *(long*)((char*)self + 0x1c);
    *c = *(long*)((char*)self + 0x20);
}
long __thiscall RKC_RPGSCRN_GetGroundBlockCount(void* self) {
    long count = 0;
    for (char* item = *(char**)((char*)self + 8); item;
         item = *(char**)(item + 0x38))
        ++count;
    return count;
}
void* __thiscall RKC_RPGSCRN_GetGroundBlock(void* self, long index) {
    char* item = *(char**)((char*)self + 8);
    for (long current = 0; item && current != index; ++current)
        item = *(char**)(item + 0x38);
    return item;
}
int __thiscall RKC_RPGSCRN_GetShadowTransFlag(void* self) {
    return *(int*)((char*)self + 0x28);
}
long __thiscall RKC_RPGSCRN_GetObjectBlockCount(void* self) {
    long count = 0;
    for (char* item = *(char**)((char*)self + 4); item;
         item = *(char**)(item + 0xc))
        ++count;
    return count;
}
int __thiscall RKC_RPGSCRN_ReadUpdList(void* self, char* a, char* b, long c, int d) { return 0; }
void __thiscall RKC_RPGSCRN_SetBaseParam(void* self, long a, long b, long c) {
    *(long*)((char*)self + 0x18) = a;
    *(long*)((char*)self + 0x1c) = b;
    *(long*)((char*)self + 0x20) = c;
}
void __thiscall RKC_RPGSCRN_CalcRealPos(
    void* self, long x, long y, long* outX, long* outY) {
    *outX = ((x - y) * *(long*)((char*)self + 0x18)) / 100;
    *outY = ((x + y) * *(long*)((char*)self + 0x1c)) / 100;
}
void __thiscall RKC_RPGSCRN_CalcWorldPos(
    void* self, long x, long y, long* outX, long* outY) {
    const long baseX = *(long*)((char*)self + 0x18);
    const long baseY = *(long*)((char*)self + 0x1c);
    *outX = ((baseX * y + baseY * x) * 100) / (baseX * baseY * 2);
    *outY = ((baseX * y - baseY * x) * 100) / (baseX * baseY * 2);
    if (x * baseY + y * baseX < 0)
        --*outX;
    if (y * baseX - x * baseY < 0)
        --*outY;
}
void __thiscall RKC_RPGSCRN_SetShadowTransFlag(void* self, int value) {
    *(int*)((char*)self + 0x28) = value;
}

// RKC_RPGSCRN_OBJECT
void* __thiscall RKC_RPGSCRN_OBJECT_constructor(void* self) {
    auto* bytes = static_cast<unsigned char*>(self);
    *(short*)(bytes + 0x10) = 0;
    *(long*)(bytes + 0x14) = 0;
    *(short*)(bytes + 0x18) = -1;
    *(long*)(bytes + 4) = 0;
    *(long*)(bytes + 8) = 0;
    *(short*)(bytes + 0x1a) = 1000;
    std::memset(bytes + 0x28, 0, 0x10);
    *(long*)(bytes + 0x38) = 0;
    *(long*)(bytes + 0x3c) = 0;
    *(short*)(bytes + 0x1c) = 0;
    *(short*)(bytes + 0x1e) = 0;
    *(short*)(bytes + 0x20) = 1000;
    *(short*)(bytes + 0x22) = 1000;
    *(short*)(bytes + 0x24) = 1000;
    *(long*)(bytes + 0xc) = -1;
    return self;
}
void* __thiscall RKC_RPGSCRN_OBJECT_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x40);
    return self;
}
int __thiscall RKC_RPGSCRN_OBJECT_Copy(void* self, void* src) {
    std::memcpy(self, src, 0x40);
    return 1;
}
short __thiscall RKC_RPGSCRN_OBJECT_GetPaletteNo(void* self) { return *(short*)((char*)self + 0x18); }
long __thiscall RKC_RPGSCRN_OBJECT_GetPatternNo(void* self) { return *(long*)((char*)self + 0x14); }
short __thiscall RKC_RPGSCRN_OBJECT_GetTrans(void* self) { return *(short*)((char*)self + 0x1a); }
short __thiscall RKC_RPGSCRN_OBJECT_GetUpdNo(void* self) { return *(short*)((char*)self + 0x10); }
long __thiscall RKC_RPGSCRN_OBJECT_GetX(void* self) { return *(long*)((char*)self + 4); }
long __thiscall RKC_RPGSCRN_OBJECT_GetY(void* self) { return *(long*)((char*)self + 8); }
long __thiscall RKC_RPGSCRN_OBJECT_GetCharacterNo(void* self) { return *(long*)((char*)self + 0xc); }
short __thiscall RKC_RPGSCRN_OBJECT_GetStatus(void* self) { return *(short*)((char*)self + 0x1c); }
short __thiscall RKC_RPGSCRN_OBJECT_GetHeight(void* self) { return *(short*)((char*)self + 0x1e); }
short __thiscall RKC_RPGSCRN_OBJECT_GetRStrong(void* self) { return *(short*)((char*)self + 0x20); }
short __thiscall RKC_RPGSCRN_OBJECT_GetGStrong(void* self) { return *(short*)((char*)self + 0x22); }
short __thiscall RKC_RPGSCRN_OBJECT_GetBStrong(void* self) { return *(short*)((char*)self + 0x24); }
void* __thiscall RKC_RPGSCRN_OBJECT_GetPositionAddr(void* self) { return (char*)self + 4; }
void* __thiscall RKC_RPGSCRN_OBJECT_GetJudgementAddr(void* self) { return (char*)self + 0x28; }
int __thiscall RKC_RPGSCRN_OBJECT_GetPosition(void* self, void* out) {
    std::memcpy(out, (char*)self + 4, 8);
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECT_GetJudgement(void* self, void* out) {
    std::memcpy(out, (char*)self + 0x28, 0x10);
    return 1;
}
void __thiscall RKC_RPGSCRN_OBJECT_Release(void* self) {}
int __thiscall RKC_RPGSCRN_OBJECT_SetPacket(void* self, long a, long b, long c, long d, int e, short f, void* pos) { return 0; }
void __thiscall RKC_RPGSCRN_OBJECT_SetPaletteNo(void* self, short no) { *(short*)((char*)self + 0x18) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetPatternNo(void* self, long no) { *(long*)((char*)self + 0x14) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetTrans(void* self, short trans) { *(short*)((char*)self + 0x1a) = trans; }
void __thiscall RKC_RPGSCRN_OBJECT_SetUpdNo(void* self, short no) { *(short*)((char*)self + 0x10) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetCharacterNo(void* self, long value) { *(long*)((char*)self + 0xc) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetStatus(void* self, short value) { *(short*)((char*)self + 0x1c) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetHeight(void* self, short value) { *(short*)((char*)self + 0x1e) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetRStrong(void* self, short value) { *(short*)((char*)self + 0x20) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetGStrong(void* self, short value) { *(short*)((char*)self + 0x22) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetBStrong(void* self, short value) { *(short*)((char*)self + 0x24) = value; }
int __thiscall RKC_RPGSCRN_OBJECT_SetPosition(void* self, void* value) {
    std::memcpy((char*)self + 4, value, 8);
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECT_SetJudgement(void* self, void* value) {
    std::memcpy((char*)self + 0x28, value, 0x10);
    return 1;
}

// RKC_RPGSCRN_OBJECTDISP - stubs
void* __thiscall RKC_RPGSCRN_OBJECTDISP_constructor(void* self, void* screen) { return self; }
void* __thiscall RKC_RPGSCRN_OBJECTDISP_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPGSCRN_OBJECTDISP_CheckExistStatus(void* self, short status) { return 0; }
int __thiscall RKC_RPGSCRN_OBJECTDISP_Copy(void* self, void* src) { return 0; }
void* __thiscall RKC_RPGSCRN_OBJECTDISP_GetHostScreen(void* self) { return nullptr; }
long __thiscall RKC_RPGSCRN_OBJECTDISP_GetNo_Cell(void* self, void* cell) { return 0; }
long __thiscall RKC_RPGSCRN_OBJECTDISP_GetNo_Object(void* self, void* obj) { return 0; }
void* __thiscall RKC_RPGSCRN_OBJECTDISP_InsertSortObjectNo(void* self, void* obj, void* block) { return nullptr; }
void* __thiscall RKC_RPGSCRN_OBJECTDISP_InsertSort(void* self, void* pos, void* rect, short a, short b) { return nullptr; }

// RKC_RPGSCRN_OBJECTDISPCELL
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_constructor(void* self) {
    *(long*)((char*)self + 8) = 0;
    *(long*)((char*)self + 0xc) = 0;
    *(long*)((char*)self + 0x10) = 0;
    *(short*)self = 0;
    *(long*)((char*)self + 4) = 1000;
    return self;
}
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_operatorAssign(void* self, const void* src) { return self; }
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_Get(void* self) { return *(void**)((char*)self + 8); }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetStatus(void* self) { return *(short*)self; }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetTrans(void* self) { return *(short*)((char*)self + 4); }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_Set(void* self, void* object) { *(void**)((char*)self + 8) = object; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetStatus(void* self, short status) { *(short*)self = status; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetTrans(void* self, short trans) { *(long*)((char*)self + 4) = trans; }

// RKC_RPGSCRN_OBJECTBLOCK - stubs
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_constructor(void* self) {
    *(long*)((char*)self + 4) = 0;
    *(long*)((char*)self + 8) = 0;
    *(long*)((char*)self + 0xc) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_Copy(void* self, void* src) { return 0; }
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_GetFromCharacterNo(void* self, long charNo) { return nullptr; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_ReadFile(void* self, char* path, void* disp, int flag, long extra) { return 0; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_WriteFile(void* self, char* path) { return 0; }

// RKC_RPGSCRN_GROUNDBLOCK
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_constructor(void* self) {
    std::memset(self, 0, 0x3c);
    *(long*)((char*)self + 0xc) = 0x40;
    *(long*)((char*)self + 0x10) = 0x40;
    *(long*)((char*)self + 0x18) = 0x10;
    *(long*)((char*)self + 0x1c) = 0x10;
    return self;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcAreaJudgeFromMap(void* self, long* a, long* b, long* c, long* d) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(void* self, long x, long y, long* outX, long* outY) {}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Copy(void* self, void* src) { return 0; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaJudgeSize(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x20);
    *b = *(long*)((char*)self + 0x24);
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaSize(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 4);
    *b = *(long*)((char*)self + 8);
}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetArea(void* self) { return *(void**)((char*)self + 0x14); }
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaJudge(void* self) { return *(void**)((char*)self + 0x30); }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetBaseParam(void* self, long* a, long* b) {}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Get(void* self, long a, long b, short* c, short* d, short* e) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetJudge(void* self, long x, long y, short* out) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReadFile(void* self, char* path, long flags) { return 0; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseJudge(void* self) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseMap(void* self) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(void* self, long a, long b, long c, long d) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(void* self, long a, long b) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetBaseMag(void* self, long a, long b) {
    *(long*)((char*)self + 0x18) = a;
    *(long*)((char*)self + 0x1c) = b;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetBaseMag(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x18);
    *b = *(long*)((char*)self + 0x1c);
}
long __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetChipHeight(void* self) { return *(long*)((char*)self + 0x10); }
long __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetChipWidth(void* self) { return *(long*)((char*)self + 0xc); }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipHeight(void* self, long h) { *(long*)((char*)self + 0x10) = h; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipWidth(void* self, long w) { *(long*)((char*)self + 0xc) = w; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudgeOffset(void* self, long a, long b) {
    *(long*)((char*)self + 0x28) = a;
    *(long*)((char*)self + 0x2c) = b;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetJudgeOffset(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x28);
    *b = *(long*)((char*)self + 0x2c);
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudge(void* self, long x, long y, short val) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Set(void* self, long a, long b, short c, short d, short e) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_WriteFile(void* self, char* path) { return 0; }

// RKC_RPGSCRN_CHARANIMBLOCK - stubs
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMBLOCK_GetCount(void* self) { return 0; }

// RKC_RPGSCRN_CHARANIM
void* __thiscall RKC_RPGSCRN_CHARANIM_constructor(void* self) {
    *(long*)self = 0;
    *(long*)((char*)self + 8) = -1;
    *(long*)((char*)self + 0x10) = 0;
    *(long*)((char*)self + 0x14) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIM_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIM_operatorAssign(void* self, const void* src) { return self; }

// RKC_RPGSCRN_CHARANIMCELLBLOCK
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_constructor(void* self) {
    *(long*)self = 0;
    *(long*)((char*)self + 4) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_GetCount(void* self) { return 0; }
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_Release(void* self) {}

// RKC_RPGSCRN_CHARANIMCELL
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_constructor(void* self) {
    *(short*)self = 0;
    *(short*)((char*)self + 2) = 0;
    *(short*)((char*)self + 4) = 0;
    *(long*)((char*)self + 8) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPatternNo(void* self) { return *(long*)((char*)self + 8); }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPriority(void* self) { return *(short*)((char*)self + 2); }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetStatus(void* self) { return *(short*)self; }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetTrans(void* self) { return *(short*)((char*)self + 4); }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPatternNo(void* self, long no) { *(long*)((char*)self + 8) = no; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPriority(void* self, short priority) { *(short*)((char*)self + 2) = priority; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetStatus(void* self, short status) { *(short*)self = status; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetTrans(void* self, short trans) { *(short*)((char*)self + 4) = trans; }

// RKC_RPGSCRN_CHARANIMCHART
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_constructor(void* self) {
    std::memset((char*)self + 4, 0, 0x48);
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCHART_GetCount(void* self, long index) {
    return *(long*)((char*)self + 4 + index * 4);
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_Release(void* self) {}
short __thiscall RKC_RPGSCRN_CHARANIMCHART_GetMaxFrameCount(void* self, long index) {
    return *(short*)((char*)self + 0x4c + index * 2);
}
short __thiscall RKC_RPGSCRN_CHARANIMCHART_GetStatus(void* self) { return *(short*)self; }
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetMaxFrameCount(void* self, long index, short count) {
    *(short*)((char*)self + 0x4c + index * 2) = count;
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetStatus(void* self, short status) { *(short*)self = status; }

} // extern "C"
