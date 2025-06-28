#ifndef LENET_5_TEST_H
#define LENET_5_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

int init_tflite();
int infer(const char *data, size_t len, int8_t **out, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif
