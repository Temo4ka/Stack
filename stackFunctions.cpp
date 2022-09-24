#ifndef Elem_t
    #include "stackType.h"
#endif
#include "stackHeader.h"
#include <string.h>
#include <cassert>

int stackPush(Stack *stack, Elem_t value) {
    int er = StackError(stack);
    if (er){
        stackDump(stack);
        return er;
    }

    if (stackResize(stack, &er)) {
        stackDump(stack);
        return er;
    }
    stack -> data[stack -> size++] = value;

    er = StackError(stack);
    if (er){
        stackDump(stack);
        return er;
    }

    return Ok;
}

Elem_t stackPop(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = StackError(stack);
        if (*er) {
            stackDump(stack);
            return 0;
        }
    }

    Elem_t value = stack -> data[--(stack -> size)];
    if (stackResize(stack, er) && er != nullptr)
        return *er;

    if (er != nullptr) {
        *er = StackError(stack);
        if (*er) {
            stackDump(stack);
            return 0;
        }
    }

    return value;
}

void *recalloc(void *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement) ;

int stackResize(Stack *stack, int *er) {
    if (er != nullptr) {
        *er = StackError(stack);
        if (*er) {
            stackDump(stack);
            return 1;
        }
    }

    if (stack->size == stack->capacity) {
        stack->data = (Elem_t *) recalloc(stack->data, stack->size * sizeof(Elem_t), (stack->capacity) * (stack->koef), sizeof(Elem_t));
        stack->capacity *= stack->koef;
    }
    if (stack->size < (stack->capacity) / (2 * stack->koef)) {
        stack->data = (Elem_t *) recalloc(stack->data, stack->size * sizeof(Elem_t), (stack->capacity) / (2 * stack->koef), sizeof(Elem_t));
        stack->capacity /= (2 * stack->koef);
    }

    if (er != nullptr) {
        *er = StackError(stack);
        if (*er) {
            stackDump(stack);
            return 1;
        }
    }

    return Ok;
}

void *recalloc(void *memPointer, size_t numberOfElements, size_t needNumOfElements, size_t sizeOfElement) {
    if (memPointer == nullptr)
        return nullptr;

    void *newPointer = calloc(needNumOfElements, sizeOfElement);

    if (newPointer == nullptr)
        return nullptr;

    memcpy(newPointer, memPointer, numberOfElements);
    free(memPointer);

    return newPointer;
}

int stackConstructor(Stack *stack, const char* variableName, const char* declarationFunc, const char* declarationPlace, const unsigned line) {
    int er = StackError(stack);
    if (er) {
        stackDump(stack);
        return er;
    }

    stack ->   pointerInfo    =       stack     ;
    stack -> declarationPlace = declarationPlace;
    stack ->      line        =       line      ;
    stack -> declarationFunc  = declarationFunc ;
    stack ->   variableName   =   variableName  ;

    return Ok;
}

int stackDtor(Stack *stack) {
    //?assert

    free(stack -> data);
    stack -> capacity = -1     ;
    stack -> size     = -1     ;
    stack -> data     = nullptr;

    return Ok;
}