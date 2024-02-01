#include "../toxcore/tox.h"

#include <stdio.h>

#include "../toxcore/ccompat.h"

int main(void)
{
int main(void) {
    fprintf(stderr, "hello\n");
    tox_kill(tox_new(nullptr, nullptr));
    return 0;
}
