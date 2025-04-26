#include <malloc.h>
#include <stdint.h>

void output(uint16_t data) {
    // TODO: *GPOA
}

void delay(uint16_t i) {
    while (i > 0) {
        i--;
    }
}

int main() {
    int16_t y = 0x65EB;
    int16_t *p = &y;
    int16_t x = ((*p++ - 1) * 2 ^ 4);
    // 1. take old value of p               0x65EB
    // 2. increment p
    // 3. subtract 1 from previous value    0x65EA
    // 4. multiply by two                   0xCBD4
    // 5. xor with 4                        0xCBD0
    //                                       52176
    //                                      -13360
    delay(x);
    printf("%hi\r\n", x);

    return 0;
}
