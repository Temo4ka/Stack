#ifndef Elem_t
    #include "stackType.h"
#endif
#include <string.h>
#include <cassert>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <cstdarg>

#ifndef PROTECTION
    #define PROTECTION 7
#endif

#define STACK_LOG_INFO 1
#define STACK_CANARY_PROTECTION 2
#define STACK_HASH_PROTECTION 4

enum StackStatus {
    Destructed  = 0,
    Constructed = 1,
};

enum StackErrors {
    StackOk                      =       0,
    StackIsNull                  = 1 <<  0,
    StackNullInData              = 1 <<  1,
    StackCapacityOverflow        = 1 <<  2,
    StackSizeOverflow            = 1 <<  3,
    StackSizeOutOfCapacity       = 1 <<  4,
    StackSizeBelowZero           = 1 <<  5,
    StackDoubleConstruction      = 1 <<  6,
    StackDoubleDestruction       = 1 <<  7,
    StackZeroSizePop             = 1 <<  8,
    StackIsInActive              = 1 <<  9,
    StackLftCanBirdIsDamaged     = 1 << 10,
    StackRgtCanBirdIsDamaged     = 1 << 11,
    StackDataIsDamaged           = 1 << 12,
    StackStructIsDamaged         = 1 << 13

};

struct StackInfo {
    const void*    pointerInfo      =   nullptr  ;
    const char*    declarationPlace =   nullptr  ;
    const char*    declarationFunc  =   nullptr  ;
    const char*    variableName     =   nullptr  ;
    unsigned       line             =      0     ;
};

struct Stack {
    Elem_t *data     =  nullptr  ;
    size_t  size     =     0     ;
    size_t  capacity =     1     ;
    int     koef     =     2     ;
    bool    status   = Destructed;

    #if (PROTECTION & STACK_CANARY_PROTECTION)
        uint64_t LftVictimToGods = 0xDED33DEAD;
        uint64_t RgtVictimToGods = 0xBAD99F00D;
    #endif

    void (*printElem_t) (FILE *stream, const Elem_t stackElement) = nullptr;

    #if (PROTECTION & STACK_LOG_INFO)
        StackInfo info = {}; // can be optional, new debug lvl
    #endif

    #if (PROTECTION & STACK_HASH_PROTECTION)
        uint64_t dataGnuHash   = 0;
        uint64_t structGnuHash = 0;
    #endif
};

const size_t SIZE_LIMIT = 1e6;

const int dumpFree = 1;

#ifndef POISON
    const Elem_t POISON = 1e9 + 17;
#endif

FILE *LogFile = fopen("logs.txt", "w");

#if (PROTECTION & STACK_LOG_INFO)
    #define stackDump(stack) { stackDump_((stack), __PRETTY_FUNCTION__, FILENAME_, __LINE__);}
#endif

#define FILENAME_ (strrchr(__FILE__, '/') + 1)
#define stackCtor(stack, printElem_t) { stackConstructor((stack),  #stack, __PRETTY_FUNCTION__, FILENAME_, __LINE__, (printElem_t)); }

int stackPush(Stack *stack, Elem_t value);

Elem_t stackPop(Stack *stack, int *er = nullptr);

int stackResize(Stack *stack, int *er = nullptr);

void *recalloc(Elem_t *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement);

void fillPoison(Elem_t *memPointer, size_t size);

void logClose();

#if (PROTECTION & STACK_CANARY_PROTECTION)
    void stackPrepareVictims(Stack *stack);

    bool LftCanBirdError(Stack *stack);

    bool RgtCanBirdError(Stack *stack);
#endif

#if (PROTECTION & STACK_HASH_PROTECTION)
    bool structGnuHashError(Stack *stack);

    bool dataGnuHashError(Stack *stack);

    void stackMakeHash(Stack *stack);

    uint64_t getGnuHash(const void *memPointer, size_t totalBytes);
#endif

#if (PROTECTION & STACK_LOG_INFO)
    int stackError(Stack *stack);

    void stackDump_(Stack *stack, const char* functionName, const char *fileName, unsigned line);

    void printErrorMessage(int error);
#endif

int stackConstructor(Stack *stack, const char* variableName, const char* declarationFunc, const char* declarationPlace,
                     const unsigned line, void (*printElem_t)(FILE *stream, const Elem_t stackElement) = nullptr) {
    if (stack == nullptr)
        return StackIsNull;
    if (stack -> status == Constructed)
        return StackDoubleConstruction;

    stack -> printElem_t = printElem_t;
    stack -> status      = Constructed;

    stack -> data = (Elem_t *) recalloc(stack -> data, stack -> size, stack -> capacity, sizeof(Elem_t));
    #if ((PROTECTION & STACK_CANARY_PROTECTION))
        *((uint64_t *) ((char *) stack -> data - sizeof(uint64_t))) = stack -> LftVictimToGods;
        *((uint64_t *) ((char *) stack -> data + sizeof( Elem_t ))) = stack -> RgtVictimToGods;
    #endif

    #if (PROTECTION & STACK_LOG_INFO)
        stack->info = {
                      stack     ,
                declarationPlace,
                declarationFunc ,
                  variableName  ,
                      line      ,
        };
    #endif

    #if (STACK_HASH_PROTECTION & PROTECTION)
        stack -> dataGnuHash   = getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity);
        stack -> structGnuHash = getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t));
    #endif

    return StackOk;
}

int stackDtor(Stack *stack) {
    if (!stack->status)
        return StackDoubleDestruction;

    #if (STACK_CANARY_PROTECTION & PROTECTION)
        free((char *) stack -> data - sizeof(uint64_t));
    #else
        free((char *) stack -> data);
    #endif
    stack -> data = nullptr;

    stack -> capacity =         -1;
    stack -> size     = 0xDED32DED;
    stack -> status   = Destructed;

    return StackOk;
}

int stackPush(Stack *stack, Elem_t value)  {
    int er = 0;
    er = stackError(stack);
    if (er) {
        #if (PROTECTION & STACK_LOG_INFO)
            stackDump(stack);
            printErrorMessage(er);
        #endif
        return er;
    }

    if (stackResize(stack, &er)) {
        #if (PROTECTION & STACK_LOG_INFO)
            stackDump(stack);
            printErrorMessage(er);
        #endif
        return er;
    }

    stack -> data[stack -> size++] = value;

    stackDump(stack);

    #if (PROTECTION & STACK_HASH_PROTECTION)
        stackMakeHash(stack);
    #endif


    return StackOk;
}

Elem_t stackPop(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            #if (PROTECTION & STACK_LOG_INFO)
                stackDump(stack);
                printErrorMessage(*er);
            #endif
            return POISON;
        }
    }

    if (stack -> size == 0) {
        if (er != nullptr) {
            *er |= StackZeroSizePop;
            #if (PROTECTION & STACK_LOG_INFO)
                stackDump(stack);
                printErrorMessage(*er);
            #endif
        }
        return POISON;
    }

    Elem_t value = stack -> data[--(stack -> size)];
    stack -> data[stack -> size] = POISON;
    #if (PROTECTION & STACK_HASH_PROTECTION)
        stackMakeHash(stack);
    #endif

    if (stackResize(stack, er)) {
        if (er != nullptr) {
            #if (PROTECTION & STACK_LOG_INFO)
                stackDump(stack);
                printErrorMessage(*er);
            #endif
        }
        return POISON;
    }

    return value;
}

int stackResize(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            #if (PROTECTION & STACK_LOG_INFO)
                stackDump(stack);
                printErrorMessage(*er);
            #endif
            return *er;
        }
    }

    if (stack->size == stack->capacity) {
        #if (PROTECTION & STACK_CANARY_PROTECTION)
            stack->data = (Elem_t *) recalloc((Elem_t *)((char *) stack->data - sizeof(uint64_t)), stack->size, stack->capacity * stack->koef, sizeof(Elem_t));
        #else
            stack->data = (Elem_t *) recalloc(stack->data, stack->size, stack->capacity * stack->koef, sizeof(Elem_t));
        #endif

        stack->capacity *= stack->koef;

        #if (PROTECTION & STACK_CANARY_PROTECTION)
            *(uint64_t *) ((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) = stack -> RgtVictimToGods;
        #endif

        #if (PROTECTION & STACK_HASH_PROTECTION)
            stackMakeHash(stack);
        #endif
    }
    if (stack->size < (stack->capacity) / (2 * stack->koef)) {
        #if (PROTECTION & STACK_CANARY_PROTECTION)
            stack->data = (Elem_t *) recalloc((Elem_t *)((char *) stack->data - sizeof(uint64_t)), stack->size, stack->capacity / (2 * stack->koef), sizeof(Elem_t));
        #else
            stack->data = (Elem_t *) recalloc(stack->data, stack->size, stack->capacity * stack->koef, sizeof(Elem_t));
        #endif

        stack->capacity /= (2 * stack->koef);

        #if (PROTECTION & STACK_CANARY_PROTECTION)
            *(uint64_t *) ((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) = stack -> RgtVictimToGods;
        #endif

        #if (PROTECTION & STACK_HASH_PROTECTION)
            stackMakeHash(stack);
        #endif
    }

    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            #if (PROTECTION & STACK_LOG_INFO)
                stackDump(stack);
                printErrorMessage(*er);
            #endif
            return *er;
        }
    }

    return StackOk;
}

void *recalloc(Elem_t *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement) {
    #if (PROTECTION & STACK_CANARY_PROTECTION)
        memPointer = (Elem_t *) realloc(memPointer, needNumOfElements * sizeOfElement + 2 * sizeof(uint64_t));
    #else
        memPointer = (Elem_t *) realloc(memPointer, needNumOfElements * sizeOfElement);
    #endif

    if (memPointer == nullptr)
        return nullptr;

    #if (PROTECTION & STACK_CANARY_PROTECTION)
        memPointer = (Elem_t *) ((char *) memPointer + sizeof(uint64_t));
    #endif

    fillPoison(memPointer + numberOfElements, needNumOfElements - numberOfElements);

    return memPointer;
}

void fillPoison(Elem_t *memPointer, size_t size) {
    assert(memPointer != nullptr);

    size_t filled =  0;

    while (filled < size) {
        *memPointer = POISON;
        ++filled;
        ++memPointer;
    }
}

int stackError(Stack *stack) {
    int error = 0;

    if (stack == nullptr) {
        error |= StackIsNull;
        return error;
    }
    if (stack -> data == nullptr) {
        error |= StackNullInData;
    }
    if (stack -> size > SIZE_LIMIT) {
        error |= StackSizeOverflow;
    }
    if (stack -> capacity > SIZE_LIMIT * 2) {
        error |= StackCapacityOverflow;
    }
    if (stack -> capacity < stack -> size) {
        error |= StackSizeOutOfCapacity;
    }
    if (stack -> size < 0) {
        error |= StackSizeBelowZero;
    }
    if (stack -> status == Destructed) {
        error |= StackIsInActive;
    }

    #if (PROTECTION & STACK_CANARY_PROTECTION)
        if (LftCanBirdError(stack)) {
            error |= StackLftCanBirdIsDamaged;
        }
        if (RgtCanBirdError(stack)) {
            error |= StackRgtCanBirdIsDamaged;
        }
    #endif

    #if (PROTECTION & STACK_HASH_PROTECTION)
        if (dataGnuHashError(stack)) {
            error |= StackDataIsDamaged;
        }
        if (structGnuHashError(stack)) {
            error |= StackStructIsDamaged;
        }
    #endif

    return error;
}

#if (PROTECTION & STACK_CANARY_PROTECTION)
    void stackPrepareVictims(Stack *stack) {
        if (stack == nullptr)
            return;

        *(uint64_t *) ((char *) stack -> data - sizeof(uint64_t))                   = stack -> LftVictimToGods;
        *(uint64_t *) ((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) = stack -> RgtVictimToGods;
    }


    bool LftCanBirdError(Stack *stack) {
        if (stack == nullptr)
            return false;

        return (*(uint64_t *)((char *) stack -> data - sizeof(uint64_t))                   != stack -> LftVictimToGods);
    }

    bool RgtCanBirdError(Stack *stack) {
        if (stack == nullptr)
            return false;

        return (*(uint64_t *)((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) != stack -> RgtVictimToGods);
    }
#endif

#if (PROTECTION & STACK_HASH_PROTECTION)
    bool structGnuHashError(Stack *stack) {
        return (stack -> structGnuHash != getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t)));
    }

    bool dataGnuHashError(Stack *stack) {
        return (stack -> dataGnuHash   != getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity));
    }
#endif

#if (PROTECTION & STACK_HASH_PROTECTION)
    void stackMakeHash(Stack *stack) {
        if (stack == nullptr)
            return;

        stack -> dataGnuHash   = getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity);
        stack -> structGnuHash = getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t));
    }

    uint64_t getGnuHash(const void *memPointer, size_t totalBytes) {
        uint64_t hash = 5381;

        char *pointer = (char *) memPointer;
        for (size_t currentByte = 0; currentByte < totalBytes; currentByte++) {
            hash = hash * 33 + pointer[currentByte];
        }

        return hash;
    }
#endif

size_t max(size_t aParam, size_t bParam);

#if (PROTECTION & STACK_LOG_INFO)
    void myfPrintf(FILE *stream = nullptr, const char *format = nullptr, ...) {
        if (format == nullptr)
            return;

        va_list arguments;
        va_start (arguments, format);
        if (stream != nullptr)
            vfprintf(stream, format, arguments);
        vfprintf(stderr, format, arguments);
        va_end(arguments);
    }

    void stackDump_(Stack *stack, const char* functionName, const char *fileName, unsigned line) {
        myfPrintf(LogFile, "%s at %s(%u);\n", functionName, fileName, line);
        myfPrintf(LogFile, "Stack[%08X] '%s' at '%s' at %s(%u); ", stack->info.pointerInfo, stack->info.variableName, stack->info.declarationFunc, stack->info.declarationPlace, stack->info.line);
        myfPrintf(LogFile, "\n{\n"
                         "      size   =  %zu\n"
                         "    capacity =  %zu\n"
                         "      data   =  %08X \n    {\n", stack->size, stack->capacity, stack->data);
        size_t currentElem = 0;

        for (; currentElem < stack->size; currentElem++) {
            myfPrintf(LogFile, "        *[%zu]  =  { ", currentElem);
            if (stack -> printElem_t != nullptr)
                stack -> printElem_t(LogFile, stack -> data[currentElem]);
            else
                myfPrintf(LogFile, "%08X", stack -> data[currentElem]);
            myfPrintf(LogFile, " }\n", currentElem);
        }
        for (size_t currentIndex = 0; currentIndex < dumpFree && currentElem < stack->capacity; currentElem++, currentIndex++) {
            myfPrintf(LogFile, "         [%zu]  =  { ", currentElem);
            if (stack -> printElem_t != nullptr)
                stack -> printElem_t(LogFile, stack -> data[currentElem]);
            else
                myfPrintf(LogFile, "%08X", stack -> data[currentElem]);
            myfPrintf(LogFile, " }  (POISON)\n");
        }
        if (currentElem + dumpFree < stack->capacity)
            myfPrintf(LogFile, "       ...\n");
        currentElem = max(currentElem, stack->capacity - dumpFree);
        for (; currentElem < stack->capacity; currentElem++) {
            myfPrintf(LogFile, "         [%zu]  =  { ", currentElem, stack->data[currentElem]);
            if (stack -> printElem_t != nullptr)
                stack -> printElem_t(LogFile, stack -> data[currentElem]);
            else {
                myfPrintf(LogFile, "%08X", stack -> data[currentElem]);
            }
            myfPrintf(LogFile, " }  (POISON)\n");
        }
        myfPrintf(LogFile, "    }\n"
                           "}\n");
    }

    void printErrorMessage(int error) {
        if (error == 0) {
            myfPrintf(LogFile, "No Errors occured :)\n");
            return;
        }

        size_t numberOfErrors = 0;
        for (size_t currentError = 0; currentError < 20; currentError++)
            if (error & (1 << currentError))
                numberOfErrors++;
        myfPrintf(LogFile, "Program stopped with %d errors: \n", numberOfErrors);

        size_t currentError = 1;
        if (error & (1 <<  0))
            myfPrintf(LogFile, "%zu)  Struct Stack was nullptr!\n", currentError++);
        if (error & (1 <<  1))
            myfPrintf(LogFile, "%zu)  Elem_t data in struct Stack was nullptr!\n", currentError++);
        if (error & (1 <<  2))
            myfPrintf(LogFile, "%zu)  Capacity in struct Stack was too big!\n"             , currentError++);
        if (error & (1 <<  3))
            myfPrintf(LogFile, "%zu)  Size in struct Stack was too big!\n", currentError++);
        if (error & (1 <<  4))
            myfPrintf(LogFile, "%zu)  Size was bigger than Capacity in struct Stack!\n", currentError++);
        if (error & (1 <<  5))
            myfPrintf(LogFile, "%zu)  Size was less than zero in struct Stack!\n", currentError++);
        if (error & (1 <<  6))
            myfPrintf(LogFile, "%zu)  Stack was constructed twice!\n", currentError++);
        if (error & (1 <<  7))
            myfPrintf(LogFile, "%zu)  Stack was destructed twice!\n", currentError++);
        if (error & (1 <<  8))
            myfPrintf(LogFile, "%zu)  User popped empty stack :_(!\n", currentError++);
        if (error & (1 <<  9))
            myfPrintf(LogFile, "%zu)  A try to use inactive stack!\n", currentError++);
        if (error & (1 << 10))
            myfPrintf(LogFile, "%zu)  Left victim to God in stack data is damaged, the data can be damaged!\n", currentError++);
        if (error & (1 << 11))
            myfPrintf(LogFile, "%zu)  Right victim to God in stack data is damaged, the data can be damaged!\n", currentError++);
        if (error & (1 << 12))
            myfPrintf(LogFile, "%zu)  Stack data is damaged!\n", currentError++);
        if (error & (1 << 13))
            myfPrintf(LogFile, "%zu)  The struct Stack is damaged!\n", currentError++);

    }
#endif

void logClose()  {
    if (LogFile != nullptr)
        fclose(LogFile);
}

size_t max(size_t aParam, size_t bParam) {
    if (aParam > bParam)
        return aParam;
    return bParam;
}
