#ifndef SFDE_RKC_RPG_SCRIPT_H
#define SFDE_RKC_RPG_SCRIPT_H


#define RKC_RPG_SCRIPT_STATUS_CHECK 0
#define RKC_RPG_SCRIPT_STATUS_TALKEND 1
#define RKC_RPG_SCRIPT_STATUS_RECVNETFLAG 2
#define RKC_RPG_SCRIPT_STATUS_ONJUDGE 3
#define RKC_RPG_SCRIPT_STATUS_ENEMYDEAD 4
#define RKC_RPG_SCRIPT_STATUS_EXECFUNCTION 5
#define RKC_RPG_SCRIPT_STATUS_ROUTIN 6
#define RKC_RPG_SCRIPT_STATUS_ENTRYSCENARIO 7
#define RKC_RPG_SCRIPT_STATUS_ENDCARDGAME 8

typedef struct
{
    long field0, field1;
} RKC_RPG_SCRIPT_Flag;

typedef struct
{
    long id;
    char *text;
    long textLen;
} RKC_RPG_SCRIPT_Message;

typedef struct
{
    long networkFlag; /* 0 or 1 -- CHECK vs CHECKNET */
    long status;
    long characterNo;
    long sentence;
} RKC_RPG_SCRIPT_Status;

typedef struct
{
    long type;
    long value;
} RKC_RPG_SCRIPT_Operand;

typedef struct
{
    long opcode;
    RKC_RPG_SCRIPT_Operand *operands;
    long operandCount;
} RKC_RPG_SCRIPT_Command;

typedef struct
{
    RKC_RPG_SCRIPT_Command *commands;
    long commandCount;
} RKC_RPG_SCRIPT_Sentence;

typedef struct
{
    RKC_RPG_SCRIPT_Flag *netFlags;
    long netFlagCount;
    RKC_RPG_SCRIPT_Flag *tempFlags;
    long tempFlagCount;

    RKC_RPG_SCRIPT_Message *messages;
    long messageCount;

    RKC_RPG_SCRIPT_Status *statuses;
    long statusCount;

    RKC_RPG_SCRIPT_Sentence *sentences;
    long sentenceCount;
} RKC_RPG_SCRIPT;

void RKC_RPG_SCRIPT_Init(RKC_RPG_SCRIPT *self);
void RKC_RPG_SCRIPT_Release(RKC_RPG_SCRIPT *self);

int RKC_RPG_SCRIPT_Read(RKC_RPG_SCRIPT *self, const char *path);

#endif
