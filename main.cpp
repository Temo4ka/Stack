#ifndef Elem_t
    #include "stackType.h"
#endif
#define PROTECTION 7
#include "stack.h"
#include <cstdio>
#include <string.h>

void userPrint(FILE *stream, const Elem_t Param) {
    fprintf(stream, "%d", Param);
    fprintf(stderr, "%d", Param);
}

int main() {

    Stack stack = {};

    stackCtor(&stack, userPrint)

    int er = 0;

    for (int i = 0; i < 10; i++) {
        int k = 0;
        scanf("%d", &k);
        if (k == 0) {
            printf("%d\n", stackPop(&stack, &er));
        } else
            stackPush(&stack, k);
    }

    stackDtor(&stack);
    return 0;
}
