#ifndef Elem_t
    #include "stackType.h"
#endif
#include <string.h>
#include <cassert>
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <cstdarg>

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
    bool           active           =  Destructed;
};

struct Stack {
    Elem_t *data     = (Elem_t *) calloc(1, sizeof(Elem_t) + sizeof(uint64_t) + sizeof(uint64_t));
    size_t  size     =                                       0                                   ;
    size_t  capacity =                                       1                                   ;
    int     koef     =                                       2                                   ;

    uint64_t LftVictimToGods = 0xDED33DEAD;
    uint64_t RgtVictimToGods = 0xDEAD33DED;

    void (*printElem_t) (FILE *stream, const Elem_t stackElement) = nullptr;

    StackInfo info = {};

    uint64_t dataGnuHash   = 0;
    uint64_t structGnuHash = 0;
};

const size_t SIZE_LIMIT = 1e6;
const int dumpFree = 1;
const Elem_t POISON = 1e9 + 17;

FILE *logFile = fopen("logs.txt", "w");

#define FILENAME_ (strrchr(__FILE__, '/') + 1)
#define stackCtor(stack, printElem_t) { stackConstructor((stack),  #stack, __PRETTY_FUNCTION__, FILENAME_, __LINE__, (printElem_t)); }
#define stackDump(stack) { stackDump_((stack), __PRETTY_FUNCTION__, FILENAME_, __LINE__);}

int stackPush(Stack *stack, Elem_t value);

Elem_t stackPop(Stack *stack, int *er = nullptr);

int stackResize(Stack *stack, int *er = nullptr);

void *recalloc(void *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement);

void null(void *memPointer, size_t size);

void stackMakeHash(Stack *stack);

void stackPrepareVictims(Stack *stack);

uint64_t getGnuHash(const void *memPointer, size_t totalBytes);

bool LftCanBirdError(Stack *stack);

bool RgtCanBirdError(Stack *stack);

bool structGnuHashError(Stack *stack);

bool dataGnuHashError(Stack *stack);

int stackError(Stack *stack);

void stackDump_(Stack *stack, const char* functionName, const char *fileName, unsigned line);

void printErrorMessage(int error);

int stackConstructor(Stack *stack, const char* variableName, const char* declarationFunc, const char* declarationPlace, const unsigned line, void (*printElem_t)(FILE *stream, const Elem_t stackElement) = nullptr) {
    if (stack == nullptr)
        return StackIsNull;
    if (stack -> info.active == Constructed)
        return StackDoubleConstruction;

    stack -> printElem_t = printElem_t;

    *(uint64_t *) (stack -> data) = stack -> LftVictimToGods;
      stack -> data  = (Elem_t *) ((char *) stack -> data + sizeof(uint64_t));
    *((uint64_t *)((char *) stack -> data + sizeof(Elem_t))) = stack -> RgtVictimToGods;

    stack->info = {
                  stack     ,
            declarationPlace,
            declarationFunc ,
              variableName  ,
                  line      ,
               Constructed  ,
    };

    stack -> dataGnuHash   = getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity);
    stack -> structGnuHash = getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t));

    return StackOk;
}

int stackDtor(Stack *stack) {
    if (!stack->info.active)
        return StackDoubleDestruction;

    free((char *) stack -> data - sizeof(uint64_t));
    stack -> data = nullptr;

    stack -> capacity    =         -1;
    stack -> size        = 0xDED32DED;
    stack -> info.active = Destructed;

    return StackOk;
}

int stackPush(Stack *stack, Elem_t value)  {
    int er = 0;

    if (stackResize(stack, &er)) {
        stackDump(stack);
        printErrorMessage(er);
        return er;
    }

    stack -> data[stack -> size++] = value;
    stackMakeHash(stack);

    er = stackError(stack);
    if (er) {
        stackDump(stack);
        printErrorMessage(er);
        return er;
    }

    return StackOk;
}

Elem_t stackPop(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            stackDump(stack);
            printErrorMessage(*er);
            return POISON;
        }
    }

    if (stack -> size == 0) {
        if (er != nullptr) {
            *er |= StackZeroSizePop;
            stackDump(stack);
            printErrorMessage(*er);
        }
        return POISON;
    }

    Elem_t value = stack -> data[--(stack -> size)];
    stackMakeHash(stack);

    if (stackResize(stack, er)) {
        if (er != nullptr) {
            stackDump(stack);
            printErrorMessage(*er);
        }
        return POISON;
    }

    return value;
}

int stackResize(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            stackDump(stack);
            printErrorMessage(*er);
            return *er;
        }
    }

    if (stack->size == stack->capacity) {
        stack->data = (Elem_t *) recalloc((char *) stack->data - sizeof(uint64_t), stack->size, stack->capacity * stack->koef, sizeof(Elem_t));
        stack->capacity *= stack->koef;

        stackPrepareVictims(stack);

        stackMakeHash(stack);
    }
    if (stack->size < (stack->capacity) / (2 * stack->koef)) {
        stack->data = (Elem_t *) recalloc((char *) stack->data - sizeof(uint64_t), stack->size, stack->capacity / (2 * stack->koef), sizeof(Elem_t));
        stack->capacity /= (2 * stack->koef);

        stackPrepareVictims(stack);

        stackMakeHash(stack);
    }

    if (er != nullptr) {
        *er = stackError(stack);
        if (*er) {
            stackDump(stack);
            printErrorMessage(*er);
            return *er;
        }
    }

    return StackOk;
}

void *recalloc(void *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement) {
    if (memPointer == nullptr)
        return nullptr;

    memPointer = realloc(memPointer, needNumOfElements * sizeOfElement + 2 * sizeof(uint64_t));

    if (memPointer == nullptr)
        return nullptr;

    memPointer = (char *) memPointer + sizeof(uint64_t);

    null((char*) memPointer + numberOfElements * sizeOfElement, sizeOfElement * (needNumOfElements - numberOfElements));

    return memPointer;
}

void null(void *memPointer, size_t size) {
    assert(memPointer != nullptr);

    size_t filled =  0;

    while (filled + sizeof(uint64_t) <= size) {
        *(uint64_t *) memPointer = 0;
        memPointer = (char *) memPointer + sizeof(uint64_t);
        filled += sizeof(uint64_t);
    }

    while (filled + sizeof(uint32_t) <= size) {
        *(uint32_t *) memPointer = 0;
        memPointer = (char *) memPointer + sizeof(uint32_t);
        filled += sizeof(uint32_t);
    }

    while (filled + sizeof(uint16_t) <= size) {
        *(uint16_t *) memPointer = 0;
        memPointer = (char *) memPointer + sizeof(uint16_t);
        filled += sizeof(uint16_t);
    }

    while (filled + sizeof(uint8_t) <= size) {
        *(uint8_t *) memPointer = 0;
        memPointer = (char *) memPointer + sizeof(uint8_t);
        filled += sizeof(uint8_t);
    }
}

void stackPrepareVictims(Stack *stack) {
    *(uint64_t *) ((char *) stack -> data - sizeof(uint64_t))                   = stack -> LftVictimToGods;
    *(uint64_t *) ((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) = stack -> RgtVictimToGods;
}

void stackMakeHash(Stack *stack) {
    if (stack == nullptr)
        return;

    stack -> dataGnuHash   = getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity);
    stack -> structGnuHash = getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t));
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
    if (stack -> info.active == Destructed) {
        error |= StackIsInActive;
    }
    if (LftCanBirdError(stack)) {
        error |= StackLftCanBirdIsDamaged;
    }
    if (RgtCanBirdError(stack)) {
        error |= StackRgtCanBirdIsDamaged;
    }
    if (dataGnuHashError(stack)) {
        error |= StackDataIsDamaged;
    }
    if (structGnuHashError(stack)) {
        error |= StackStructIsDamaged;
    }

    return error;
}

bool LftCanBirdError(Stack *stack) {
    return (*(uint64_t *)((char *) stack -> data - sizeof(uint64_t))                   != stack -> LftVictimToGods);
}

bool RgtCanBirdError(Stack *stack) {
    return (*(uint64_t *)((char *) stack -> data + sizeof(Elem_t) * stack -> capacity) != stack -> RgtVictimToGods);
}

bool structGnuHashError(Stack *stack) {
    return (stack -> structGnuHash != getGnuHash(stack, sizeof(Stack) - sizeof(uint64_t)));
}

bool dataGnuHashError(Stack *stack) {
    return (stack -> dataGnuHash   != getGnuHash(stack -> data, sizeof(Elem_t) * stack -> capacity));
}

uint64_t getGnuHash(const void *memPointer, size_t totalBytes) {
    uint64_t hash = 5381;

    char *pointer = (char *) memPointer;
    for (size_t currentByte = 0; currentByte < totalBytes; currentByte++) {
        hash = hash * 33 + pointer[currentByte];
    }

    return hash;
}

size_t max(size_t aParam, size_t bParam);

void myfPrintf(FILE *stream = nullptr, const char *format = nullptr, ...) {
    assert(format != nullptr);

    va_list arguments;
    va_start (arguments, format);
    if (stream != nullptr)
        vfprintf(stream, format, arguments);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

void stackDump_(Stack *stack, const char* functionName, const char *fileName, unsigned line) {
    myfPrintf(logFile, "%s at %s(%u);\n", functionName, fileName, line);
    myfPrintf(logFile, "Stack[%x] '%s' at '%s' at %s(%u); ", stack->info.pointerInfo, stack->info.variableName, stack->info.declarationFunc, stack->info.declarationPlace, stack->info.line);
    myfPrintf(logFile, "\n{\n"
                     "      size   =  %zu\n"
                     "    capacity =  %zu\n"
                     "      data   =  %x \n    {\n", stack->size, stack->capacity, stack->data);
    size_t currentElem = 0;

    for (; currentElem < stack->size; currentElem++) {
        myfPrintf(logFile, "        *[%zu]  =  { ", currentElem);
        if (stack -> printElem_t != nullptr)
            stack -> printElem_t(logFile, stack -> data[currentElem]);
        else
            myfPrintf(logFile, "%x", stack -> data[currentElem]);
        myfPrintf(logFile, " }\n", currentElem);
    }
    for (size_t currentIndex = 0; currentIndex < dumpFree && currentElem < stack->capacity; currentElem++, currentIndex++) {
        myfPrintf(logFile, "         [%zu]  =  { ", currentElem);
        if (stack -> printElem_t != nullptr)
            stack -> printElem_t(logFile, stack -> data[currentElem]);
        else
            myfPrintf(logFile, "%x", stack -> data[currentElem]);
        myfPrintf(logFile, " }  (POISON)\n");
    }
    if (currentElem + dumpFree < stack->capacity)
        myfPrintf(logFile, "       ...\n");
    currentElem = max(currentElem, stack->capacity - dumpFree);
    for (; currentElem < stack->capacity; currentElem++) {
        myfPrintf(logFile, "         [%zu]  =  { ", currentElem, stack->data[currentElem]);
        if (stack -> printElem_t != nullptr)
            stack -> printElem_t(logFile, stack -> data[currentElem]);
        else
            myfPrintf(logFile, "%x", stack -> data[currentElem]);
        myfPrintf(logFile, " }  (POISON)\n");
    }
    myfPrintf(logFile, "    }\n"
                     "}\n");
}

void printErrorMessage(int error) {
    if (error == 0) {
        myfPrintf(logFile, "No Errors occured :)\n");
        return;
    }

    size_t numberOfErrors = 0;
    for (size_t currentError = 0; currentError < 20; currentError++)
        if (error & (1 << currentError))
            numberOfErrors++;
    myfPrintf(logFile, "Program stopped with %d errors: \n", numberOfErrors);

    size_t currentError = 1;
    if (error & (1 <<  0))
        myfPrintf(logFile, "%zu)  Struct Stack was nullptr!\n",                                              currentError++);
    if (error & (1 <<  1))
        myfPrintf(logFile, "%zu)  Elem_t data in struct Stack was nullptr!\n",                               currentError++);
    if (error & (1 <<  2))
        myfPrintf(logFile, "%zu)  Capacity in struct Stack was too big!\n"             ,                     currentError++);
    if (error & (1 <<  3))
        myfPrintf(logFile, "%zu)  Size in struct Stack was too big!\n",                                      currentError++);
    if (error & (1 <<  4))
        myfPrintf(logFile, "%zu)  Size was bigger than Capacity in struct Stack!\n",                         currentError++);
    if (error & (1 <<  5))
        myfPrintf(logFile, "%zu)  Size was less than zero in struct Stack!\n",                               currentError++);
    if (error & (1 <<  6))
        myfPrintf(logFile, "%zu)  Stack was constructed twice!\n",                                           currentError++);
    if (error & (1 <<  7))
        myfPrintf(logFile, "%zu)  Stack was destructed twice!\n",                                            currentError++);
    if (error & (1 <<  8))
        myfPrintf(logFile, "%zu)  User popped empty stack :_(!\n",                                           currentError++);
    if (error & (1 <<  9))
        myfPrintf(logFile, "%zu)  A try to use inactive stack!\n",                                           currentError++);
    if (error & (1 << 10))
        myfPrintf(logFile, "%zu)  Left victim to God in stack data is damaged, the data can be damaged!\n",  currentError++);
    if (error & (1 << 11))
        myfPrintf(logFile, "%zu)  Right victim to God in stack data is damaged, the data can be damaged!\n", currentError++);
    if (error & (1 << 12))
        myfPrintf(logFile, "%zu)  Stack data is damaged!\n",                                                 currentError++);
    if (error & (1 << 13))
        myfPrintf(logFile, "%zu)  The struct Stack is damaged!\n",                                           currentError++);

}

void logClose()  {
    fclose(logFile);
}

size_t max(size_t aParam, size_t bParam) {
    if (aParam > bParam)
        return aParam;
    return bParam;
}