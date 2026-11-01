/* Copyright 2026 ALTESSA SOLUTIONS INC.
 * Use of this source code is governed by an MIT-style license that can be
 * found in the LICENSE file. */
#include "rmm_test.h"
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int main(void) {
    uint64_t state = UINT64_C(0x524d4d41524b53);
    uint8_t data[1024];
    for (size_t run = 0; run < 20000; run += 1) {
        size_t size = run % sizeof data;
        for (size_t i = 0; i < size; i += 1) {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            data[i] = (uint8_t)(state >> (run % 2 ? 56 : 57));
        }
        RM_CHECK(LLVMFuzzerTestOneInput(data, size) == 0, "fuzz smoke %zu", run);
    }
    return rm_test_report("fuzz smoke");
}
