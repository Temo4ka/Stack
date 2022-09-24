#ifndef Elem_t
    #include "stackType.h"
#endif
#include "stackHeader.h"
#include <cstdio>
#include <string.h>

int main() {

    Stack stack = {};

    stackCtor(&stack);

    for (int i = 0; i < 10; i++) {
        int k = 0;
        scanf("%d", &k);
        if (k == 0) {
            printf("%d", stackPop(&stack));
        } else
            stackPush(&stack, k);
        stackDump(&stack);
    }
    //printf("%s\n", __FILENAME__);

    stackDtor(&stack);
    logClose();
    return 0;
}
