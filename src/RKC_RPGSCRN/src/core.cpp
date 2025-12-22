/**
 * RKC_RPGSCRN - RPG Screen rendering
 * 
 * Classes: RKC_RPGSCRN, RKC_RPGSCRN_OBJECT, RKC_RPGSCRN_OBJECTDISP,
 *          RKC_RPGSCRN_OBJECTDISPCELL, RKC_RPGSCRN_OBJECTBLOCK,
 *          RKC_RPGSCRN_GROUNDBLOCK, RKC_RPGSCRN_CHARANIM*, etc.
 */

#include <windows.h>

extern "C" {

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_RPGSCRN - partial stubs
void* __thiscall RKC_RPGSCRN_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_RPGSCRN_DeleteGroundBlock(void* self, long index) {}
void __thiscall RKC_RPGSCRN_DeleteObjectBlock(void* self, long index) {}
void __thiscall RKC_RPGSCRN_GetBaseParam(void* self, long* a, long* b, long* c) {}
long __thiscall RKC_RPGSCRN_GetGroundBlockCount(void* self) { return 0; }
void* __thiscall RKC_RPGSCRN_GetGroundBlock(void* self, long index) { return nullptr; }
int __thiscall RKC_RPGSCRN_GetShadowTransFlag(void* self) { return 0; }
long __thiscall RKC_RPGSCRN_GetObjectBlockCount(void* self) { return 0; }
int __thiscall RKC_RPGSCRN_ReadUpdList(void* self, char* a, char* b, long c, int d) { return 0; }
void __thiscall RKC_RPGSCRN_SetBaseParam(void* self, long a, long b, long c) {}

// RKC_RPGSCRN_OBJECT - stubs
void* __thiscall RKC_RPGSCRN_OBJECT_operatorAssign(void* self, const void* src) { return self; }
short __thiscall RKC_RPGSCRN_OBJECT_GetPaletteNo(void* self) { return 0; }
long __thiscall RKC_RPGSCRN_OBJECT_GetPatternNo(void* self) { return 0; }
short __thiscall RKC_RPGSCRN_OBJECT_GetTrans(void* self) { return 0; }
short __thiscall RKC_RPGSCRN_OBJECT_GetUpdNo(void* self) { return 0; }
long __thiscall RKC_RPGSCRN_OBJECT_GetX(void* self) { return 0; }
long __thiscall RKC_RPGSCRN_OBJECT_GetY(void* self) { return 0; }
void __thiscall RKC_RPGSCRN_OBJECT_Release(void* self) {}
int __thiscall RKC_RPGSCRN_OBJECT_SetPacket(void* self, long a, long b, long c, long d, int e, short f, void* pos) { return 0; }
void __thiscall RKC_RPGSCRN_OBJECT_SetPaletteNo(void* self, short no) {}
void __thiscall RKC_RPGSCRN_OBJECT_SetPatternNo(void* self, long no) {}
void __thiscall RKC_RPGSCRN_OBJECT_SetTrans(void* self, short trans) {}
void __thiscall RKC_RPGSCRN_OBJECT_SetUpdNo(void* self, short no) {}

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

// RKC_RPGSCRN_OBJECTDISPCELL - stubs
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_operatorAssign(void* self, const void* src) { return self; }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetStatus(void* self) { return 0; }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetTrans(void* self) { return 0; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetStatus(void* self, short status) {}
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetTrans(void* self, short trans) {}

// RKC_RPGSCRN_OBJECTBLOCK - stubs
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_Copy(void* self, void* src) { return 0; }
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_GetFromCharacterNo(void* self, long charNo) { return nullptr; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_ReadFile(void* self, char* path, void* disp, int flag, long extra) { return 0; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_WriteFile(void* self, char* path) { return 0; }

// RKC_RPGSCRN_GROUNDBLOCK - stubs
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcAreaJudgeFromMap(void* self, long* a, long* b, long* c, long* d) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(void* self, long x, long y, long* outX, long* outY) {}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Copy(void* self, void* src) { return 0; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaJudgeSize(void* self, long* a, long* b) {}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetArea(void* self) { return nullptr; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetBaseParam(void* self, long* a, long* b) {}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Get(void* self, long a, long b, short* c, short* d, short* e) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetJudge(void* self, long x, long y, short* out) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReadFile(void* self, char* path, long flags) { return 0; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseJudge(void* self) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseMap(void* self) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(void* self, long a, long b, long c, long d) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(void* self, long a, long b) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetBaseMag(void* self, long a, long b) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipHeight(void* self, long h) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipWidth(void* self, long w) {}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudgeOffset(void* self, long a, long b) {}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudge(void* self, long x, long y, short val) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Set(void* self, long a, long b, short c, short d, short e) { return 0; }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_WriteFile(void* self, char* path) { return 0; }

// RKC_RPGSCRN_CHARANIMBLOCK - stubs
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMBLOCK_GetCount(void* self) { return 0; }

// RKC_RPGSCRN_CHARANIM - stubs
void* __thiscall RKC_RPGSCRN_CHARANIM_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIM_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIM_operatorAssign(void* self, const void* src) { return self; }

// RKC_RPGSCRN_CHARANIMCELLBLOCK - stubs
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_GetCount(void* self) { return 0; }
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_Release(void* self) {}

// RKC_RPGSCRN_CHARANIMCELL - stubs
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPatternNo(void* self) { return 0; }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPriority(void* self) { return 0; }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetTrans(void* self) { return 0; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPatternNo(void* self, long no) {}
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPriority(void* self, short priority) {}
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetStatus(void* self, short status) {}
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetTrans(void* self, short trans) {}

// RKC_RPGSCRN_CHARANIMCHART - stubs
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_constructor(void* self) { return self; }
void __thiscall RKC_RPGSCRN_CHARANIMCHART_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCHART_GetCount(void* self, long index) { return 0; }
void __thiscall RKC_RPGSCRN_CHARANIMCHART_Release(void* self) {}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetMaxFrameCount(void* self, long index, short count) {}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetStatus(void* self, short status) {}

} // extern "C"
