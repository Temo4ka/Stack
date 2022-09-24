#ifndef Elem_t
    #include "stackType.h"
#endif

#include <stdlib.h>

enum Errors {
    Ok                = 0,
    NullInStack       = 1,
    NullInData        = 2,
    CapacityOverflow  = 4,
    SizeOverflow      = 8,
    SizeOutOfCapacity = 16,
};

struct Stack {
    Elem_t *data     = (Elem_t *) calloc(1, sizeof(Elem_t));
    size_t  size     =             0            ;
    size_t  capacity =             1            ;
    int     koef     =             2            ;

    const void*    pointerInfo      = nullptr;
    const char*    declarationPlace = nullptr;
    const char*    declarationFunc  = nullptr;
    const char*    variableName     = nullptr;
    unsigned       line             =    0   ;
};

#define __FILENAME__ (strrchr(__FILE__, '/') + 1)
#define stackCtor(stack) { stackConstructor((stack),  #stack, __PRETTY_FUNCTION__, __FILENAME__, __LINE__); }
#define stackDump(stack) { stDump((stack), __PRETTY_FUNCTION__, __FILENAME__, __LINE__);}

int stackPush(Stack *stack, Elem_t value);

Elem_t stackPop(Stack *stack, int *er = nullptr);

int stackResize(Stack *stack, int *er = nullptr);

int stackConstructor(Stack *stack, const char *variableName, const char *declarationFunc, const char* declarationPlace, const unsigned line);

int stackDtor(Stack *stack);

int StackError(Stack *stack);

void stDump(Stack *stack, const char* functionName, const char *fileName, unsigned line);

void logClose();

void printErrorMessage(int error);