#include "hub75.h"
#include "hub75_config.h"

typedef enum {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
} PanelPosition;

typedef enum {
    UPRIGHT,
    ROTATE_180
} PanelRotation;

typedef struct {
    PanelPosition position;
    PanelRotation rotation;
} Panel;

static const Panel chain[] = HUB75_CHAIN_LAYOUT;

_Static_assert(sizeof chain / sizeof chain[0] == HUB75_PANEL_COUNT,
               "Specify all four panels in HUB75_CHAIN_LAYOUT");
_Static_assert(HUB75_FIRST_CLOCK_IS_RIGHT == 0 || HUB75_FIRST_CLOCK_IS_RIGHT == 1,
               "HUB75_FIRST_CLOCK_IS_RIGHT must be 0 or 1");

static unsigned image_pixel(const Hub75Image *image, unsigned x, unsigned y)
{
    const unsigned byte = image->row[y][x / 2u];
    return ((x % 2u == 0u) ? (byte >> 4) : byte) & 7u;
}

bool hub75_encode(const Hub75Image *image, Hub75Scan *scan)
{
    unsigned used_positions = 0u;
    for (unsigned slot = 0; slot < HUB75_PANEL_COUNT; ++slot) {
        // A later clock pushes earlier data toward the far end of the chain.
        const Panel panel = chain[HUB75_PANEL_COUNT - 1u - slot];
        if ((unsigned)panel.position >= HUB75_PANEL_COUNT ||
            (panel.rotation != UPRIGHT && panel.rotation != ROTATE_180)) {
            return false;
        }
        const unsigned position = 1u << panel.position;
        if ((used_positions & position) != 0u) {
            return false;
        }
        used_positions |= position;

        const unsigned tile_x = (panel.position % 2u) * HUB75_PANEL_SIZE;
        const unsigned tile_y = (panel.position / 2u) * HUB75_PANEL_SIZE;
        const bool rotate = panel.rotation == ROTATE_180;
        const bool reverse_x = (HUB75_FIRST_CLOCK_IS_RIGHT != 0) != rotate;

        for (unsigned row = 0; row < HUB75_SCAN_ROWS; ++row) {
            const unsigned lower = row + HUB75_SCAN_ROWS;
            const unsigned y1 = tile_y + (rotate ? HUB75_PANEL_SIZE - 1u - row : row);
            const unsigned y2 = tile_y + (rotate ? HUB75_PANEL_SIZE - 1u - lower : lower);

            for (unsigned clock = 0; clock < HUB75_PANEL_SIZE; ++clock) {
                const unsigned x = tile_x + (reverse_x ? HUB75_PANEL_SIZE - 1u - clock : clock);
                const unsigned upper_rgb = image_pixel(image, x, y1);
                const unsigned lower_rgb = image_pixel(image, x, y2) << 3;
                scan->row[row][slot * HUB75_PANEL_SIZE + clock] = (uint8_t)(upper_rgb | lower_rgb);
            }
        }
    }
    return true;
}
