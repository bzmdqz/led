#ifndef HUB75_CONFIG_H
#define HUB75_CONFIG_H

// From the front of the display, ordered from the MCU toward the end of the chain.
// Change a panel's UPRIGHT to ROTATE_180 if it is mounted upside down.
#define HUB75_CHAIN_LAYOUT {     \
    { TOP_LEFT,     UPRIGHT },   \
    { TOP_RIGHT,    UPRIGHT },   \
    { BOTTOM_RIGHT, UPRIGHT },   \
    { BOTTOM_LEFT,  UPRIGHT },   \
}

// Set to 1 if the first bit clocked into an upright panel belongs on its right edge.
#define HUB75_FIRST_CLOCK_IS_RIGHT 0

#define HUB75_ROW_US 300u
#define HUB75_OE_US  120u

#endif
