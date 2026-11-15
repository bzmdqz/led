#ifndef HUB75_H
#define HUB75_H

#include <stdbool.h>
#include <stdint.h>

enum {
    HUB75_PANEL_SIZE = 64,
    HUB75_PANEL_COUNT = 4,
    HUB75_WIDTH = 2 * HUB75_PANEL_SIZE,
    HUB75_HEIGHT = 2 * HUB75_PANEL_SIZE,
    HUB75_IMAGE_STRIDE = HUB75_WIDTH / 2,
    HUB75_SCAN_ROWS = HUB75_PANEL_SIZE / 2,
    HUB75_SHIFT_CLOCKS = HUB75_PANEL_COUNT * HUB75_PANEL_SIZE
};

// Two pixels per byte, left pixel in the high nibble, R/G/B in bits 0/1/2.
typedef struct {
    uint8_t row[HUB75_HEIGHT][HUB75_IMAGE_STRIDE];
} Hub75Image;

typedef struct {
    uint8_t row[HUB75_SCAN_ROWS][HUB75_SHIFT_CLOCKS];
} Hub75Scan;

void hub75_init(void);
bool hub75_encode(const Hub75Image *image, Hub75Scan *scan);
_Noreturn void hub75_scan_forever(const Hub75Scan *scan);

#endif
