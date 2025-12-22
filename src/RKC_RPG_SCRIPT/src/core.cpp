/**
 * RKC_RPG_SCRIPT - Script handling for RPG scripting
 * 
 * Classes: RKC_RPG_SCRIPT, RKC_RPG_SCRIPT_COMMAND, RKC_RPG_SCRIPT_MESSAGE,
 *          RKC_RPG_SCRIPT_MESSAGEBLOCK, RKC_RPG_SCRIPT_SENTENCE,
 *          RKC_RPG_SCRIPT_SENTENCEBLOCK, RKC_RPG_SCRIPT_STATUS,
 *          RKC_RPG_SCRIPT_STATUSBLOCK
 */

#include <windows.h>

extern "C" {

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_RPG_SCRIPT_COMMAND - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_COMMAND_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_COMMAND_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_COMMAND_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_RPG_SCRIPT_COMMAND_Release(void* self) {}

// RKC_RPG_SCRIPT_MESSAGE - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_MESSAGE_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_MESSAGE_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_MESSAGE_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPG_SCRIPT_MESSAGE_GetID(void* self) { return 0; }
char* __thiscall RKC_RPG_SCRIPT_MESSAGE_GetName(void* self) { return nullptr; }
void __thiscall RKC_RPG_SCRIPT_MESSAGE_Release(void* self) {}

// RKC_RPG_SCRIPT_MESSAGEBLOCK - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Delete(void* self, long index, void** out) { return 0; }
long __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetCount(void* self) { return 0; }
long __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetMessageNoFromName(void* self, char* name) { return 0; }
long __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_GetNo(void* self, void* msg) { return 0; }
void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Get(void* self, long index) { return nullptr; }
void* __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Insert(void* self, long index, void* msg) { return nullptr; }
void __thiscall RKC_RPG_SCRIPT_MESSAGEBLOCK_Release(void* self) {}

// RKC_RPG_SCRIPT_SENTENCE - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_SENTENCE_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_SENTENCE_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_SENTENCE_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_SCRIPT_SENTENCE_Delete(void* self, long index, void** out) { return 0; }
long __thiscall RKC_RPG_SCRIPT_SENTENCE_GetNo(void* self, void* cmd) { return 0; }
void* __thiscall RKC_RPG_SCRIPT_SENTENCE_Insert(void* self, long index, void* cmd) { return nullptr; }
void __thiscall RKC_RPG_SCRIPT_SENTENCE_Release(void* self) {}

// RKC_RPG_SCRIPT_SENTENCEBLOCK - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Delete(void* self, long index, void** out) { return 0; }
long __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_GetCount(void* self) { return 0; }
long __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_GetNo(void* self, void* sentence) { return 0; }
void* __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Insert(void* self, long index, void* sentence) { return nullptr; }
void __thiscall RKC_RPG_SCRIPT_SENTENCEBLOCK_Release(void* self) {}

// RKC_RPG_SCRIPT_STATUS - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_STATUS_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_STATUS_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_STATUS_operatorAssign(void* self, const void* src) { return self; }

// RKC_RPG_SCRIPT_STATUSBLOCK - NOT USED BY EXE
void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_constructor(void* self) { return self; }
void __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_destructor(void* self) {}
void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_operatorAssign(void* self, const void* src) { return self; }
int __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Delete(void* self, long index, void** out) { return 0; }
long __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_GetNo(void* self, void* status) { return 0; }
void* __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Insert(void* self, long index, void* status) { return nullptr; }
void __thiscall RKC_RPG_SCRIPT_STATUSBLOCK_Release(void* self) {}

// RKC_RPG_SCRIPT - partial stubs
void* __thiscall RKC_RPG_SCRIPT_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPG_SCRIPT_AnalyzeCharacterNo(void* self, char* str) { return 0; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeCommand(void* self, char* str, void* cmd) { return 0; }
long __thiscall RKC_RPG_SCRIPT_AnalyzeOPERAND_NETFLAG(void* self, char* str) { return 0; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeOPERAND(void* self, char* str, void* operand) { return 0; }
int __thiscall RKC_RPG_SCRIPT_AnalyzeOperator(void* self, char* str, void* operand) { return 0; }
long __thiscall RKC_RPG_SCRIPT_GetNetFlag_index(void* self, long index) { return 0; }
long __thiscall RKC_RPG_SCRIPT_GetTempFlag_index(void* self, long index) { return 0; }
int __thiscall RKC_RPG_SCRIPT_ReadSentence(void* self, void* file, void* sentence) { return 0; }
int __thiscall RKC_RPG_SCRIPT_ReadText(void* self, char* path) { return 0; }
void __thiscall RKC_RPG_SCRIPT_Release(void* self) {}
int __thiscall RKC_RPG_SCRIPT_WriteBinary(void* self, char* path) { return 0; }

} // extern "C"
