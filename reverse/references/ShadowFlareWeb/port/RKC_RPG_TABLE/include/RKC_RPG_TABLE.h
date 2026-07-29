#ifndef SFDE_RKC_RPG_TABLE_H
#define SFDE_RKC_RPG_TABLE_H


typedef struct
{
    long tableNo;
    long rowCount, colCount;
    long *values;
} RKC_RPG_TABLEDATA;

typedef struct
{
    RKC_RPG_TABLEDATA *tables;
    long count;
} RKC_RPG_TABLE;

void RKC_RPG_TABLE_Init(RKC_RPG_TABLE *self);
void RKC_RPG_TABLE_Release(RKC_RPG_TABLE *self);

int RKC_RPG_TABLE_Read(RKC_RPG_TABLE *self, const char *path);

const RKC_RPG_TABLEDATA *RKC_RPG_TABLE_GetFromTableNo(const RKC_RPG_TABLE *self, long tableNo);

long RKC_RPG_TABLEDATA_GetValue(const RKC_RPG_TABLEDATA *self, long row, long col);

#endif
