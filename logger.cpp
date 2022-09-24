#ifndef Elem_t
    #include "stackType.h"
#endif
#include "stackHeader.h"
#include <cstdio>

const size_t SIZE_LIMIT = 1e6;
const int dumpFree = 1;


FILE *logFile = fopen("logs.txt", "w");

int StackError(Stack *stack) {
    int error = 0;

    if (stack == nullptr) {
        error |= NullInStack;
        return error;
    }
    if (stack -> data == nullptr) {
        error |= NullInData;
    }
    if (stack -> size > SIZE_LIMIT) {
        error |= SizeOverflow;
    }
    if (stack -> capacity > SIZE_LIMIT * 2) {
        error |= CapacityOverflow;
    }
    if (stack -> capacity < stack -> size) {
        error |= SizeOutOfCapacity;
    }

    return error;
}

size_t max(size_t aParam, size_t bParam) {
    if (aParam > bParam)
        return aParam;
    return bParam;
}

void stDump(Stack *stack, const char* functionName, const char *fileName, unsigned line) {
    fprintf(logFile, "%s at %s(%u);\n", functionName, fileName, line);
    fprintf(logFile, "Stack[%p] '%s' at '%s' at %s(%u) ", stack->pointerInfo, stack->variableName, stack->declarationFunc, stack->declarationPlace, stack->line);
    fprintf(logFile, "{\n"
           "      size   =  %zu\n"
           "    capacity =  %zu\n"
           "      data   =  %p {\n", stack->size, stack->capacity, stack->data);
    size_t currentElem = 0;

    for (; currentElem < stack->size; currentElem++) {
        fprintf(logFile, "        *[%zu]  =  %d\n", currentElem, stack->data[currentElem]);
    }
    for (size_t printed = 0; printed < dumpFree && currentElem < stack->capacity; currentElem++, printed++) {
        fprintf(logFile, "         [%zu]  =  %d  (POISON)\n", currentElem, stack->data[currentElem]);
    }
    if (currentElem + dumpFree < stack->capacity)
        fprintf(logFile, "       .....\n");
    currentElem = max(currentElem, stack->capacity - dumpFree);
    for (; currentElem < stack->capacity; currentElem++) {
        fprintf(logFile, "         [%zu]  =  %d  (POISON)\n", currentElem, stack->data[currentElem]);
    }
    fprintf(logFile, "    }\n"
                     "}\n");
}

void printErrorMessage(int error) {
    if (error == 0) {
        fprintf(logFile, "No Errors occured :)\n");
        return;
    }

    size_t numberOfErrors = 0;
    for (size_t currentError = 0; currentError < 10; currentError++)
        if (error & (1 << currentError))
            numberOfErrors++;
    fprintf(logFile, "Program stopped with %d errors: \n", numberOfErrors);

    size_t currentError = 1;
    if (error & 1)
        fprintf(logFile, "%zu)  Struct Stack was nullptr!\n",                                currentError++);
    if (error & 2)
        fprintf(logFile, "%zu)  Elem_t data in struct Stack was nullptr!\n",                 currentError++);
    if (error & 4)
        fprintf(logFile, "%zu)  Capacity in struct Stack was too big!\n",                    currentError++);
    if (error & 8)
        fprintf(logFile, "%zu)  Size in struct Stack was too big!\n",                        currentError++);
    if (error & 16)
        fprintf(logFile, "%zu)  Size was bigger than Capacity in struct Stack!\n",           currentError++);
}

void logClose() {
    fclose(logFile);
}