/**
 * RKC_RPG_TABLE - Game data table management
 * 
 * Manages linked list of table data structures for game parameters,
 * item stats, enemy data, etc.
 * USED BY: ShadowFlare.exe
 */

#include <iostream>
#include <windows.h>
#include <cstdint>
#include "../../utils.h"

class RKC_RPG_TABLEDATA {
public:
    long tableNo;                 // Located at offset 0x00
    int32_t rowCount;             // Located at offset +4
    int32_t columnCount;          // Located at offset +8
    long** table;                 // Located at offset +0x0C
    char*** stringsTable;         // Located at offset +0x10
    RKC_RPG_TABLEDATA* next;      // Located at offset +0x14, Pointer to the next data in the linked list

};


class RKC_RPG_TABLE {
public:
    RKC_RPG_TABLEDATA* headData;
};


extern "C" {
    // Forward declarations
    void __thiscall RKC_RPG_TABLEDATA_deconstructor(RKC_RPG_TABLEDATA* self);
    void __thiscall RKC_RPG_TABLEDATA_Release(RKC_RPG_TABLEDATA* self);

    /*************************************************************/
    /*                        RKC_TABLE                          */
    /*************************************************************/

    /**
     * Constructor - initialize empty table list
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_RPG_TABLE_constructor(RKC_RPG_TABLE* self)
    {
        self->headData = nullptr;
    }

    /**
     * Destructor - free all table data
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_TABLE_deconstructor(RKC_RPG_TABLE* self)
    {
        RKC_RPG_TABLEDATA* currentNode = self->headData;
        while (currentNode != nullptr) {
            RKC_RPG_TABLEDATA* nextNode = currentNode->next;
            delete currentNode;     // Deallocate memory for current node
            currentNode = nextNode; // Move to the next node
        }
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLE& __thiscall RKC_TABLE_equalsOperator(RKC_RPG_TABLE* self, RKC_RPG_TABLE rhs)
    {
        // Creates a shallow copy (only pointers are copied but not the values)
        if (self != &rhs) { // Protect against self-assignment
            self->headData = rhs.headData;
        }
        return *self;
    }

    /**
     * Insert table data at position
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    RKC_RPG_TABLEDATA* __thiscall Insert(RKC_RPG_TABLE* self, long tableNo, RKC_RPG_TABLEDATA* data)
    {
        RKC_RPG_TABLEDATA* result = CallFunctionInDLL<RKC_RPG_TABLEDATA*>(
            "o_RKC_RPG_TABLE.dll",
            "?Insert@RKC_RPG_TABLE@@QAEPAVRKC_RPG_TABLEDATA@@JPAV2@@Z",
            self, tableNo, data
        );
        return result;
    }

    /**
     * Read table data from binary file
     * USED BY: ShadowFlare.exe
     * Note: Forwards to original DLL
     */
    int32_t __thiscall ReadBinaryFile(RKC_RPG_TABLE* self, char* fileName)
    {
        int result = CallFunctionInDLL<int32_t>(
            "o_RKC_RPG_TABLE.dll",
            "?ReadBinaryFile@RKC_RPG_TABLE@@QAEHPAD@Z",
            self, fileName
        );
        return result;
    }

    /**
     * Write table data to binary file
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t __thiscall WriteBinaryFile(RKC_RPG_TABLE* self, char* fileName)
    {
        int result = CallFunctionInDLL<int32_t>(
            "o_RKC_RPG_TABLE.dll",
            "?WriteBinaryFile@RKC_RPG_TABLE@@QAEHPAD@Z",
            self, fileName
        );
        return result;
    }

    /**
     * Read single text table
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t ReadTextTable(RKC_RPG_TABLE* self, char* fileName)
    {
        int result = CallFunctionInDLL<int32_t>(
            "o_RKC_RPG_TABLE.dll",
            "?ReadTextTable@RKC_RPG_TABLEDATA@@QAEHPAD@Z",
            self, fileName
        );
        return result;
    }

    /**
     * Read all text tables from file
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    int32_t ReadAllTextTable(RKC_RPG_TABLE* self, char* fileName)
    {
        int result = CallFunctionInDLL<int32_t>(
            "o_RKC_RPG_TABLE.dll",
            "?ReadAllTextTable@RKC_RPG_TABLE@@QAEHPAD@Z",
            self, fileName
        );
        return result;
    }

    /**
     * Delete table data at index
     * NOT REFERENCED - not imported by any module
     */
    bool __thiscall Delete(RKC_RPG_TABLE* self, long index, RKC_RPG_TABLEDATA** dataToDelete)
    {
        if (!self->headData || !dataToDelete) {
            return false;
        }

        if (index == 0) {
            *dataToDelete = self->headData;
            self->headData = self->headData->next;
            return true;
        }

        RKC_RPG_TABLEDATA* prevNode = nullptr;
        RKC_RPG_TABLEDATA* currentNode = self->headData;
        long currentIndex = 0;

        while (currentNode && currentIndex < index) {
            prevNode = currentNode;
            currentNode = currentNode->next;
            currentIndex++;
        }

        if (currentNode) {
            if (prevNode) {
                prevNode->next = currentNode->next;
            }
            *dataToDelete = currentNode;
            return true;
        }

        return false;
    }

    /**
     * Release all table data
     * USED BY: ShadowFlare.exe
     * Note: Currently empty - crashes when implemented
     */
    void __thiscall RKC_RPG_TABLE_Release(RKC_RPG_TABLE* self)
    {
        // TODO: find out why this crashes the game

        // RKC_RPG_TABLEDATA* currentNode = self->headData;
        // while (currentNode != nullptr) {
        //     RKC_RPG_TABLEDATA* nextNode = currentNode->next;
        //     RKC_RPG_TABLEDATA_deconstructor(currentNode);  // Call destructor for current node
        //     delete currentNode;                        // Deallocate memory for current node
        //     currentNode = nextNode;                    // Move to the next node
        // }
        // self->headData = nullptr;  // Set headData to nullptr after releasing all
    }

    /**
     * Get table data at index
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLEDATA* __thiscall Get(RKC_RPG_TABLE* self, long index)
    {
        if (index < 0) {
            return nullptr;
        }

        RKC_RPG_TABLEDATA* currentNode = self->headData;
        for (int i = 0; currentNode != nullptr && i < index; ++i) {
            currentNode = currentNode->next;
        }

        return currentNode;
    }

    /**
     * Get total number of tables
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetCount(RKC_RPG_TABLE* self)
    {
        int count = 0;
        RKC_RPG_TABLEDATA* currentData = self->headData;

        while (currentData != nullptr) {
            count++;
            currentData = currentData->next;
        }

        return count;
    }

    /**
     * Get table data by table number
     * USED BY: ShadowFlare.exe
     */
    RKC_RPG_TABLEDATA* __thiscall GetFromTableNo(RKC_RPG_TABLE* self, long tableNo)
    {
        RKC_RPG_TABLEDATA* currentData = self->headData;

        while (currentData != nullptr) {
            if (currentData->tableNo == tableNo) {
                return currentData;
            }
            currentData = currentData->next;
        }

        return nullptr;  // If not found, return nullptr.
    }

    /**
     * Get index of table data in list
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetNo(RKC_RPG_TABLE* self, RKC_RPG_TABLEDATA* targetData)
    {
        RKC_RPG_TABLEDATA* currentData = self->headData;
        int index = 0;

        while (currentData != nullptr) {
            if (currentData == targetData) {
                return index;
            }
            currentData = currentData->next; // Traversing to the next node
            index++;
        }

        return -1;  // If the node isn't found in the list
    }


    /*************************************************************/
    /*                      RKC_TABLEDATA                        */
    /*************************************************************/

    /**
     * Constructor - initialize table data
     * NOT REFERENCED - not imported by any module
     */
    void __thiscall RKC_RPG_TABLEDATA_constructor(RKC_RPG_TABLEDATA* self)
    {
        self->tableNo = 0xffffffff;
        self->rowCount = 0;
        self->columnCount = 0;
        self->table = nullptr;
        self->stringsTable = nullptr;
    }

    /**
     * Destructor - release table data
     * NOT REFERENCED - not imported by any module
     */
    void __thiscall RKC_RPG_TABLEDATA_deconstructor(RKC_RPG_TABLEDATA* self)
    {
        RKC_RPG_TABLEDATA_Release(self);
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_RPG_TABLEDATA& __thiscall RKC_RPG_TABLEDATA_equalsOperator(RKC_RPG_TABLEDATA* self, RKC_RPG_TABLEDATA rhs)
    {
        // Creates a shallow copy (only pointers are copied but not the values)
        if (self != &rhs) { // Protect against self-assignment
            self->tableNo = rhs.tableNo;
            self->rowCount = rhs.rowCount;
            self->columnCount = rhs.columnCount;
            self->table = rhs.table;
            self->stringsTable = rhs.stringsTable;
            self->next = rhs.next;
        }
        return *self;
    }

    /**
     * Release table data memory
     * NOT REFERENCED - not imported by any module
     */
    void __thiscall RKC_RPG_TABLEDATA_Release(RKC_RPG_TABLEDATA* self)
    {
        // Deallocate memory for unknownPointerFieldC
        if (self->table != nullptr) {
            for (int rowIndex = 0; rowIndex < self->rowCount; ++rowIndex) {
                GlobalFree(reinterpret_cast<HGLOBAL>(&self->table[rowIndex]));
            }
            GlobalFree(reinterpret_cast<HGLOBAL>(self->table));
            self->table = nullptr;
        }

        // Deallocate memory for stringsTable
        if (self->stringsTable != nullptr) {
            for (int rowIndex = 0; rowIndex < self->rowCount; ++rowIndex) {
                if (self->stringsTable[rowIndex] != nullptr) {
                    for (int columnIndex = 0; columnIndex < self->columnCount; ++columnIndex) {
                        if (self->stringsTable[rowIndex][columnIndex] != nullptr) {
                            GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable[rowIndex][columnIndex]));
                        }
                    }
                    GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable[rowIndex]));
                }
            }
            GlobalFree(reinterpret_cast<HGLOBAL>(self->stringsTable));
            self->stringsTable = nullptr;
        }

        // Reset other members
        self->tableNo = 0xffffffff;
        self->rowCount = 0;
        self->columnCount = 0;
    }

    /**
     * Get string value at row/column
     * USED BY: ShadowFlare.exe
     */
    char* __thiscall GetStrings(RKC_RPG_TABLEDATA* self, long row, long column)
    {
        if (row >= 0 && row < self->rowCount && column >= 0 && column < self->columnCount) {
            return self->stringsTable[row][column];
        }
        return nullptr;
    }

    /**
     * Get entire strings table
     * NOT REFERENCED - not imported by any module
     */
    char*** __thiscall GetStringsTable(RKC_RPG_TABLEDATA* self)
    {
        return self->stringsTable;
    }

    /**
     * Get row count
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetRowCount(RKC_RPG_TABLEDATA* self)
    {
        return self->rowCount;
    }

    /**
     * Get column count
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetColCount(RKC_RPG_TABLEDATA* self)
    {
        return self->columnCount;
    }

    /**
     * Get entire table
     * NOT REFERENCED - not imported by any module
     */
    long** GetTable(RKC_RPG_TABLEDATA* self)
    {
        return self->table;
    }

    /**
     * Get table number
     * NOT REFERENCED - not imported by any module
     */
    long __thiscall GetTableNo(RKC_RPG_TABLEDATA* self)
    {
        return self->tableNo;
    }

    /**
     * Get numeric value at row/column
     * USED BY: ShadowFlare.exe
     */
    long __thiscall GetValue(RKC_RPG_TABLEDATA* self, long row, long column)
    {
        if (row >= 0 && row < self->columnCount && column >= 0 && column < self->columnCount)
        {
            return self->table[row][column];
        }
        return -1;
    }
}

bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}