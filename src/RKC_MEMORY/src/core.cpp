#include <windows.h>
#include <cstdint>

class RKC_MEMORY
{
public:
    DWORD size;
    char* allocated;
};


extern "C"
{
    void __thiscall RKC_MEMORY_Release(RKC_MEMORY* _this)
    {
        if (_this->allocated)
        {
            GlobalFree(_this->allocated);
            _this->allocated = 0;
        }

        _this->size = 0;
    }

    void __thiscall RKC_Memory_constructor(RKC_MEMORY* _this)
    {
        _this->size = 0;
        _this->allocated = 0;
    }

    void __thiscall RKC_MEMORY_deconstructor(RKC_MEMORY* _this)
    {
        RKC_MEMORY_Release(_this);
    }

    RKC_MEMORY& __thiscall equalsOperator(RKC_MEMORY* _this, const RKC_MEMORY& lhs)
    {
        _this->allocated = lhs.allocated;
        _this->size = lhs.size;
        return *_this;
    }

    char* __thiscall Allocation(RKC_MEMORY* _this, long bytesToAllocate, int zeroInit)
    {
        RKC_MEMORY_Release(_this);
        _this->allocated = (char*)GlobalAlloc(zeroInit ? GMEM_ZEROINIT : GMEM_FIXED, bytesToAllocate);

        if (_this->allocated)
            _this->size = bytesToAllocate;

        return _this->allocated;
    }

    long __thiscall GetSize(RKC_MEMORY* _this)
    {
        return _this->size;
    }

    char* __thiscall Get(RKC_MEMORY* _this) {
        return _this->allocated;
    }

    // unused
    int __thiscall Copy(RKC_MEMORY* _this, char* data, long sizeToCopy, long start)
    {
        if (sizeToCopy <= 0)
            return 0;
        if (_this->size - start < sizeToCopy)
            return 0;

        memcpy((void*)(start + _this->allocated), data, sizeToCopy);
        return 1;
    }

    // unused
    int __thiscall Clear(RKC_MEMORY* _this, char* data, long sz, long start)
    {
        // auto size = sz;
        // if(size == 0)
        //     return 0;

        // if(size < 0)
        //     size = _this->size;

        // if(_this->size - start < size)
        //     return 0;

        // char* src = (char*)(start + _this->allocated);


        // implement memset operations
        return 0;
    }
}

bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}