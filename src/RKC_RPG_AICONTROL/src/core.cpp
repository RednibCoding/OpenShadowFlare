/**
 * RKC_RPG_AICONTROL - AI Control system
 * 
 * Manages AI behavior for NPCs and enemies.
 * Classes: RKC_RPG_AICONTROL, RKC_RPG_AIDATA, RKC_RPG_AIEVENT, RKC_RPG_AILIST
 */

#include <windows.h>

extern "C" {

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_RPG_AILIST - NOT REFERENCED
void* __thiscall RKC_RPG_AILIST_constructor(void* self) { return self; }
void __thiscall RKC_RPG_AILIST_destructor(void* self) {}
void* __thiscall RKC_RPG_AILIST_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_AILIST_Copy(void* self, void* src) { return 0; }
long __thiscall RKC_RPG_AILIST_GetAIDataCount(void* self) { return 0; }
void* __thiscall RKC_RPG_AILIST_GetAIDataFromAIDataNo(void* self, long no) { return nullptr; }
void* __thiscall RKC_RPG_AILIST_GetAIData(void* self, long a, long b) { return nullptr; }
long __thiscall RKC_RPG_AILIST_GetCount(void* self) { return 0; }
long __thiscall RKC_RPG_AILIST_GetEventNoFromAIDataNo(void* self, long no) { return 0; }
char* __thiscall RKC_RPG_AILIST_GetName(void* self) { return nullptr; }
void __thiscall RKC_RPG_AILIST_Release(void* self) {}
void __thiscall RKC_RPG_AILIST_SetName(void* self, char* name) {}
void __thiscall RKC_RPG_AILIST_SetWalkPointSpeed(void* self, long speed) {}

// RKC_RPG_AICONTROL - partial (some used, some not)
void* __thiscall RKC_RPG_AICONTROL_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_AICONTROL_Copy(void* self, void* src) { return 0; }
int __thiscall RKC_RPG_AICONTROL_Delete(void* self, long index, void** outList) { return 0; }
long __thiscall RKC_RPG_AICONTROL_GetCount(void* self) { return 0; }
void* __thiscall RKC_RPG_AICONTROL_Insert(void* self, long index, void* item) { return nullptr; }
void __thiscall RKC_RPG_AICONTROL_Release(void* self) {}
int __thiscall RKC_RPG_AICONTROL_WriteFile(void* self, char* path) { return 0; }

// RKC_RPG_AIEVENT - partial
void* __thiscall RKC_RPG_AIEVENT_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_AIEVENT_Copy(void* self, void* src) { return 0; }
int __thiscall RKC_RPG_AIEVENT_Delete(void* self, long index, void** outData) { return 0; }
long __thiscall RKC_RPG_AIEVENT_GetNo(void* self, void* data) { return 0; }

// RKC_RPG_AIDATA - partial
void* __thiscall RKC_RPG_AIDATA_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPG_AIDATA_GetEventNo(void* self) { return 0; }
void __thiscall RKC_RPG_AIDATA_Release(void* self) {}
void __thiscall RKC_RPG_AIDATA_SetActionNo(void* self, long no) {}
void __thiscall RKC_RPG_AIDATA_SetCondition(void* self, void* cond) {}
void __thiscall RKC_RPG_AIDATA_SetEventNo(void* self, long no) {}
void __thiscall RKC_RPG_AIDATA_SetParameter(void* self, void* param) {}

} // extern "C"
