#include "RKC_RPG_SCRIPT.h"

#include "RKC_FILE.h"

#include <stdlib.h>
#include <string.h>

#define SCS_MAX_COUNT 1000000
#define SCS_MAX_TEXT_LEN 10000000

typedef struct
{
    const unsigned char *p, *end;
    int ok;
} Reader;

static const unsigned char *Take(Reader *r, long n)
{
    if (!r->ok || n < 0 || (unsigned long)n > (unsigned long)(r->end - r->p))
    {
        r->ok = 0;
        return NULL;
    }
    const unsigned char *cur = r->p;
    r->p += n;
    return cur;
}

static long ReadI32(Reader *r)
{
    const unsigned char *d = Take(r, 4);
    return d ? *(const int *)d : 0;
}

static long ReadCount(Reader *r)
{
    long c = ReadI32(r);
    if (c < 0 || c > SCS_MAX_COUNT)
        r->ok = 0;
    return c;
}

static void *TakeCopy(Reader *r, long n)
{
    if (n == 0)
        return NULL;
    const unsigned char *d = Take(r, n);
    if (!d)
        return NULL;
    void *copy = malloc((size_t)n);
    if (!copy)
    {
        r->ok = 0;
        return NULL;
    }
    memcpy(copy, d, (size_t)n);
    return copy;
}

static void ReleaseCommand(RKC_RPG_SCRIPT_Command *cmd)
{
    free(cmd->operands);
}

static void ReleaseSentence(RKC_RPG_SCRIPT_Sentence *sentence)
{
    for (long i = 0; i < sentence->commandCount; i++)
        ReleaseCommand(&sentence->commands[i]);
    free(sentence->commands);
}

static void ReleaseMessage(RKC_RPG_SCRIPT_Message *msg)
{
    free(msg->text);
}

void RKC_RPG_SCRIPT_Init(RKC_RPG_SCRIPT *self)
{
    memset(self, 0, sizeof(*self));
}

void RKC_RPG_SCRIPT_Release(RKC_RPG_SCRIPT *self)
{
    free(self->netFlags);
    free(self->tempFlags);

    for (long i = 0; i < self->messageCount; i++)
        ReleaseMessage(&self->messages[i]);
    free(self->messages);

    free(self->statuses);

    for (long i = 0; i < self->sentenceCount; i++)
        ReleaseSentence(&self->sentences[i]);
    free(self->sentences);

    memset(self, 0, sizeof(*self));
}

static int ReadFlagArray(Reader *r, RKC_RPG_SCRIPT_Flag **outFlags, long *outCount)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_SCRIPT_Flag *flags = (RKC_RPG_SCRIPT_Flag *)TakeCopy(r, count * 8);
    if (!r->ok)
        return 0;

    *outFlags = flags;
    *outCount = count;
    return 1;
}

static int ReadMessage(Reader *r, RKC_RPG_SCRIPT_Message *out)
{
    memset(out, 0, sizeof(*out));

    out->id = ReadI32(r);

    long textLen = ReadI32(r);
    if (textLen < 0 || textLen > SCS_MAX_TEXT_LEN)
    {
        r->ok = 0;
        return 0;
    }

    const unsigned char *rawText = Take(r, textLen);
    if (!rawText)
        return 0;

    out->text = (char *)malloc((size_t)textLen + 1);
    if (!out->text)
    {
        r->ok = 0;
        return 0;
    }
    for (long i = 0; i < textLen; i++)
        out->text[i] = (char)~rawText[i];
    out->text[textLen] = '\0';
    out->textLen = textLen;

    return 1;
}

static int ReadMessageArray(Reader *r, RKC_RPG_SCRIPT_Message **outMessages, long *outCount)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_SCRIPT_Message *messages = count > 0 ? (RKC_RPG_SCRIPT_Message *)calloc((size_t)count, sizeof(RKC_RPG_SCRIPT_Message)) : NULL;
    if (count > 0 && !messages)
        return 0;

    for (long i = 0; i < count; i++)
    {
        if (!ReadMessage(r, &messages[i]))
        {
            for (long j = 0; j <= i; j++)
                ReleaseMessage(&messages[j]);
            free(messages);
            return 0;
        }
    }

    *outMessages = messages;
    *outCount = count;
    return 1;
}

static int ReadStatusArray(Reader *r, RKC_RPG_SCRIPT_Status **outStatuses, long *outCount)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_SCRIPT_Status *statuses = (RKC_RPG_SCRIPT_Status *)TakeCopy(r, count * 16);
    if (!r->ok)
        return 0;

    *outStatuses = statuses;
    *outCount = count;
    return 1;
}

static int ReadCommand(Reader *r, RKC_RPG_SCRIPT_Command *out)
{
    memset(out, 0, sizeof(*out));

    out->opcode = ReadI32(r);

    long operandCount = ReadCount(r);
    if (!r->ok)
        return 0;

    out->operands = (RKC_RPG_SCRIPT_Operand *)TakeCopy(r, operandCount * 8);
    if (!r->ok)
        return 0;
    out->operandCount = operandCount;

    return 1;
}

static int ReadSentence(Reader *r, RKC_RPG_SCRIPT_Sentence *out)
{
    memset(out, 0, sizeof(*out));

    long commandCount = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_SCRIPT_Command *commands = commandCount > 0 ? (RKC_RPG_SCRIPT_Command *)calloc((size_t)commandCount, sizeof(RKC_RPG_SCRIPT_Command)) : NULL;
    if (commandCount > 0 && !commands)
    {
        r->ok = 0;
        return 0;
    }

    for (long i = 0; i < commandCount; i++)
    {
        if (!ReadCommand(r, &commands[i]))
        {
            for (long j = 0; j <= i; j++)
                ReleaseCommand(&commands[j]);
            free(commands);
            return 0;
        }
    }

    out->commands = commands;
    out->commandCount = commandCount;
    return 1;
}

static int ReadSentenceArray(Reader *r, RKC_RPG_SCRIPT_Sentence **outSentences, long *outCount)
{
    long count = ReadCount(r);
    if (!r->ok)
        return 0;

    RKC_RPG_SCRIPT_Sentence *sentences = count > 0 ? (RKC_RPG_SCRIPT_Sentence *)calloc((size_t)count, sizeof(RKC_RPG_SCRIPT_Sentence)) : NULL;
    if (count > 0 && !sentences)
        return 0;

    for (long i = 0; i < count; i++)
    {
        if (!ReadSentence(r, &sentences[i]))
        {
            for (long j = 0; j <= i; j++)
                ReleaseSentence(&sentences[j]);
            free(sentences);
            return 0;
        }
    }

    *outSentences = sentences;
    *outCount = count;
    return 1;
}

static int ReadFromMemory(RKC_RPG_SCRIPT *self, const unsigned char *src, unsigned long srcSize)
{
    if (srcSize < 0x10 || memcmp(src, "ScenaScriptV000", 12) != 0)
        return 0;

    Reader r = {src, src + srcSize, 1};
    Take(&r, 16); /* magic */

    if (!ReadFlagArray(&r, &self->netFlags, &self->netFlagCount))
        return 0;
    if (!ReadFlagArray(&r, &self->tempFlags, &self->tempFlagCount))
        return 0;
    if (!ReadMessageArray(&r, &self->messages, &self->messageCount))
        return 0;
    if (!ReadStatusArray(&r, &self->statuses, &self->statusCount))
        return 0;
    if (!ReadSentenceArray(&r, &self->sentences, &self->sentenceCount))
        return 0;

    return r.ok;
}

int RKC_RPG_SCRIPT_Read(RKC_RPG_SCRIPT *self, const char *path)
{
    RKC_RPG_SCRIPT_Release(self);

    RKC_FILE file;
    RKC_FILE_Init(&file);
    if (!RKC_FILE_Create(&file, path, 0))
        return 0;

    long size = RKC_FILE_GetSize(&file);
    unsigned char *buf = size > 0 ? (unsigned char *)malloc((size_t)size) : NULL;
    if (!buf || !RKC_FILE_Read(&file, buf, size))
    {
        RKC_FILE_Close(&file);
        free(buf);
        return 0;
    }
    RKC_FILE_Close(&file);

    int ok = ReadFromMemory(self, buf, (unsigned long)size);
    free(buf);
    if (!ok)
        RKC_RPG_SCRIPT_Release(self);
    return ok;
}
