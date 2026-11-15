#include "hub75.h"
#include "logo.h"

static Hub75Scan scan;

int main(void)
{
    hub75_init();
    if (!hub75_encode(&logo, &scan)) {
        return 1;  // An invalid panel map leaves OE high.
    }
    hub75_scan_forever(&scan);
}
