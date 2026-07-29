void __thiscall RKC_WINDOW::HScroll(RKC_WINDOW *this, uint param_1, long param_2)

{
    int iVar1;
    tagSCROLLINFO *ptVar2;
    HWND hwnd;
    tagRECT local_2c;
    tagSCROLLINFO local_1c;

    GetClientRect(*(HWND *)this, &local_2c);
    switch (param_1 & 0xffff)
    {
    case 0:
        *(int *)(this + 0x540) = *(int *)(this + 0x540) - *(int *)(this + 0x544);
        break;
    case 1:
        iVar1 = *(int *)(this + 0x544);
        goto LAB_10001491;
    case 2:
        *(int *)(this + 0x540) = *(int *)(this + 0x540) - local_2c.right;
        break;
    case 3:
        iVar1 = local_2c.right;
    LAB_10001491:
        *(int *)(this + 0x540) = *(int *)(this + 0x540) + iVar1;
        break;
    case 4:
        if (param_2 == -0x70000001)
        {
            hwnd = *(HWND *)this;
            ptVar2 = &local_1c;
            for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1)
            {
                ptVar2->cbSize = 0;
                ptVar2 = (tagSCROLLINFO *)&ptVar2->fMask;
            }
        LAB_10001467:
            local_1c.fMask = 0x14;
            local_1c.cbSize = 0x1c;
            GetScrollInfo(hwnd, 0, &local_1c);
            param_2 = local_1c.nTrackPos;
        }
        goto LAB_10001471;
    case 5:
        if (param_2 == -0x70000001)
        {
            ptVar2 = &local_1c;
            for (iVar1 = 7; iVar1 != 0; iVar1 = iVar1 + -1)
            {
                ptVar2->cbSize = 0;
                ptVar2 = (tagSCROLLINFO *)&ptVar2->fMask;
            }
            hwnd = *(HWND *)this;
            goto LAB_10001467;
        }
    LAB_10001471:
        *(long *)(this + 0x540) = param_2;
    }
    if (*(int *)(this + 0x540) < 0)
    {
        *(undefined4 *)(this + 0x540) = 0;
    }
    if (*(int *)(this + 0x548) - local_2c.right < *(int *)(this + 0x540))
    {
        *(int *)(this + 0x540) = *(int *)(this + 0x548) - local_2c.right;
    }
    local_1c.nPos = *(int *)(this + 0x540);
    local_1c.cbSize = 0x1c;
    local_1c.fMask = 4;
    SetScrollInfo(*(HWND *)this, 0, &local_1c, 1);
    return;
}
