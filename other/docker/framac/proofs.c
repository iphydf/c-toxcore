#include "../../../toxcore/tox.h"

int main(void) {
    tox_kill(tox_new(NULL, NULL));
}
