#include "stdint.h"
#include "bitmap_helpers.h"
void *__dso_handle = 0;
uint8_t * a, *b;
namespace tflite{
void run()
 {resize<uint8_t>(a,b,1, 1, 1, 1,1, 1);
}
}
