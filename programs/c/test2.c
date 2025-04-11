#include <stdint.h>
#include <malloc.h>
void output(uint16_t data){/*TODO: *GPOA*/}void delay(uint16_t i){while(i>0){i--;}}void test(){output(0);delay(1000);output(1);delay(1000);}int main() {while(1){test();}return 0;}
