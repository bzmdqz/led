#include "hub75.h"
#include "hub75_config.h"

#define REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

// STM32F103 register addresses and encodings from ST RM0008.
#define RCC_CR       REG32(0x40021000u)
#define RCC_CFGR     REG32(0x40021004u)
#define RCC_APB2ENR  REG32(0x40021018u)
#define RCC_APB1ENR  REG32(0x4002101cu)
#define FLASH_ACR    REG32(0x40022000u)

#define GPIOA_CRL    REG32(0x40010800u)
#define GPIOA_CRH    REG32(0x40010804u)
#define GPIOA_BSRR   REG32(0x40010810u)
#define GPIOB_CRL    REG32(0x40010c00u)
#define GPIOB_CRH    REG32(0x40010c04u)
#define GPIOB_BSRR   REG32(0x40010c10u)

#define TIM1_CR1     REG32(0x40012c00u)
#define TIM1_EGR     REG32(0x40012c14u)
#define TIM1_CCMR1   REG32(0x40012c18u)
#define TIM1_CCER    REG32(0x40012c20u)
#define TIM1_PSC     REG32(0x40012c28u)
#define TIM1_ARR     REG32(0x40012c2cu)
#define TIM1_CCR1    REG32(0x40012c34u)
#define TIM1_BDTR    REG32(0x40012c44u)
#define TIM2_CR1     REG32(0x40000000u)
#define TIM2_EGR     REG32(0x40000014u)
#define TIM2_CNT     REG32(0x40000024u)
#define TIM2_PSC     REG32(0x40000028u)
#define TIM2_ARR     REG32(0x4000002cu)

enum {
    RCC_GPIOA_ENABLE = 1u << 2,
    RCC_GPIOB_ENABLE = 1u << 3,
    RCC_TIM1_ENABLE = 1u << 11,
    RCC_TIM2_ENABLE = 1u << 0,
    RCC_PLL_ENABLE = 1u << 24,
    RCC_PLL_READY = 1u << 25,
    RCC_SYSCLK_PLL = 2u,
    RCC_SYSCLK_STATUS_MASK = 3u << 2,
    RCC_SYSCLK_STATUS_PLL = 2u << 2,
    RCC_PLL_X16 = 14u << 18,
    RCC_APB1_DIV2 = 4u << 8,
    RCC_ADC_DIV6 = 2u << 14,
    FLASH_PREFETCH = 1u << 4,
    FLASH_LATENCY_2 = 2u,
    TIM_ENABLE = 1u << 0,
    TIM_ONE_PULSE = 1u << 3,
    TIM_LOAD_PRESCALER = 1u << 0,
    TIM_PWM_MODE_2 = 7u << 4,
    TIM_CH1_ENABLE = 1u << 0,
    TIM_CH1_INVERT = 1u << 1,
    TIM_MAIN_OUTPUT_ENABLE = 1u << 15,
    TIMER_MHZ = 64u,
    OE_START_US = 2u,
    RGB_PINS = 0x3fu,
    CLK_PIN = 1u << 6,
    LAT_PIN = 1u << 7,
    OE_PIN = 1u << 8,
    ADDRESS_PINS = (1u << 0) | (1u << 1) | (1u << 10) | (1u << 11) | (1u << 12),
    BUFFER_ENABLE_PIN = 1u << 13
};

_Static_assert(HUB75_ROW_US < UINT16_MAX, "Row interval exceeds TIM2 range");
_Static_assert(HUB75_OE_US > 0u && OE_START_US + HUB75_OE_US < HUB75_ROW_US,
               "Leave room in the row for shifting and blanking");

static inline void signal_delay(void)
{
    // At 64 MHz these NOPs add at least 62.5 ns to the GPIO edges.
    __asm__ volatile ("nop\n\tnop\n\tnop\n\tnop" ::: "memory");
}

void hub75_init(void)
{
    RCC_APB2ENR |= RCC_GPIOA_ENABLE | RCC_GPIOB_ENABLE | RCC_TIM1_ENABLE;
    RCC_APB1ENR |= RCC_TIM2_ENABLE;
    (void)RCC_APB2ENR;
    (void)RCC_APB1ENR;

    // Preload safe output levels while the pins are still inputs.
    GPIOA_BSRR = OE_PIN | ((RGB_PINS | CLK_PIN | LAT_PIN) << 16);
    GPIOB_BSRR = (ADDRESS_PINS | BUFFER_ENABLE_PIN) << 16;
    GPIOA_CRL = 0x33333333u;  // PA0-PA7 output push-pull, 50 MHz slew setting.
    GPIOA_CRH = (GPIOA_CRH & ~0xfu) | 0x3u;
    GPIOB_CRL = (GPIOB_CRL & ~0xffu) | 0x33u;
    GPIOB_CRH = (GPIOB_CRH & ~0x00ffff00u) | 0x00333300u;

    // Reset clock is HSI at 8 MHz. PLL input is HSI/2, so x16 gives 64 MHz.
    FLASH_ACR = FLASH_PREFETCH | FLASH_LATENCY_2;
    const uint32_t clock_config = RCC_PLL_X16 | RCC_APB1_DIV2 | RCC_ADC_DIV6;
    RCC_CFGR = clock_config;
    RCC_CR |= RCC_PLL_ENABLE;
    while ((RCC_CR & RCC_PLL_READY) == 0u) {
    }
    RCC_CFGR = clock_config | RCC_SYSCLK_PLL;
    while ((RCC_CFGR & RCC_SYSCLK_STATUS_MASK) != RCC_SYSCLK_STATUS_PLL) {
    }

    // TIM1 is on APB2 at 64 MHz. PWM2 inverted keeps OE high at CNT=0.
    // One-pulse mode returns the counter to 0 and blanks the LEDs automatically.
    TIM1_CR1 = TIM_ONE_PULSE;
    TIM1_PSC = TIMER_MHZ - 1u;
    TIM1_ARR = OE_START_US + HUB75_OE_US - 1u;
    TIM1_CCR1 = OE_START_US;
    TIM1_CCMR1 = TIM_PWM_MODE_2;
    TIM1_EGR = TIM_LOAD_PRESCALER;
    TIM1_CCER = TIM_CH1_ENABLE | TIM_CH1_INVERT;
    TIM1_BDTR = TIM_MAIN_OUTPUT_ENABLE;
    // Let the timer's inactive-high output reach the pin mux before handing over PA8.
    (void)TIM1_BDTR;
    GPIOA_CRH = (GPIOA_CRH & ~0xfu) | 0xbu;  // PA8 TIM1_CH1 push-pull.

    // TIM2 gets 2 x PCLK1 (64 MHz). Use its 1 MHz tick for the row period.
    TIM2_PSC = TIMER_MHZ - 1u;
    TIM2_ARR = UINT16_MAX;
    TIM2_EGR = TIM_LOAD_PRESCALER;
    TIM2_CR1 = TIM_ENABLE;

    GPIOB_BSRR = BUFFER_ENABLE_PIN;
}

static void shift_row(const uint8_t *pixels)
{
    for (unsigned clock = 0; clock < HUB75_SHIFT_CLOCKS; ++clock) {
        const uint32_t data = pixels[clock];
        const uint32_t clear = (RGB_PINS ^ data) | CLK_PIN;
        GPIOA_BSRR = (clear << 16) | data;
        signal_delay();
        GPIOA_BSRR = CLK_PIN;
        signal_delay();
    }
    GPIOA_BSRR = CLK_PIN << 16;
}

_Noreturn void hub75_scan_forever(const Hub75Scan *scan)
{
    for (;;) {
        for (unsigned row = 0; row < HUB75_SCAN_ROWS; ++row) {
            const uint16_t start = (uint16_t)TIM2_CNT;
            shift_row(scan->row[row]);

            // A/B are PB0/PB1; C/D/E are PB10/PB11/PB12.
            const uint32_t address = (row & 3u) | ((row & 0x1cu) << 8);
            GPIOB_BSRR = ((ADDRESS_PINS ^ address) << 16) | address;
            signal_delay();
            GPIOA_BSRR = LAT_PIN;
            signal_delay();
            GPIOA_BSRR = LAT_PIN << 16;
            TIM1_CR1 = TIM_ONE_PULSE | TIM_ENABLE;

            // Finish the row slot and always wait for OE to go high.
            while ((uint16_t)((uint16_t)TIM2_CNT - start) < HUB75_ROW_US) {
            }
            while ((TIM1_CR1 & TIM_ENABLE) != 0u) {
            }
        }
    }
}
