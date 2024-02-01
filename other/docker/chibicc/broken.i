#include <stdio.h>

typedef struct Wrapper {
    int wrapped;
} Wrapper;

static void broken_function(Wrapper wrapper, int param2, int param3, int param4, int param5, int param6)
{
    fprintf(stderr, "%x\n", wrapper.wrapped);
    fprintf(stderr, "%x\n", param2);
    fprintf(stderr, "%x\n", param3);
    fprintf(stderr, "%x\n", param4);
    fprintf(stderr, "%x\n", param5);
    fprintf(stderr, "%x\n", param6);  // should say 66, but says 0
}

void broken(void)
{
    Wrapper wrapper = {0x11};
    int param5 = 0x55;
    int param6 = 0x66;
    broken_function(wrapper, 0x22, 0x33, 0x44, param5, param6);
}

static void working_function(int unwrapped, int param2, int param3, int param4, int param5, int param6)
{
    fprintf(stderr, "%x\n", unwrapped);
    fprintf(stderr, "%x\n", param2);
    fprintf(stderr, "%x\n", param3);
    fprintf(stderr, "%x\n", param4);
    fprintf(stderr, "%x\n", param5);
    fprintf(stderr, "%x\n", param6);
}

void working(void)
{
    int unwrapped = 0x11;
    int param5 = 0x55;
    int param6 = 0x66;
    working_function(unwrapped, 0x22, 0x33, 0x44, param5, param6);
}

int main(void)
{
    working();
    broken();
}
// vim:ft=c
