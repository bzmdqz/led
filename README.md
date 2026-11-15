# STM32F103 四块 HUB75E 点阵屏

一颗 STM32F103C8T6 驱动四块 64×64、1/32 扫描的 HUB75E 灯板。四块屏排成 2×2，显示 128×128 的红色标志、黑色圆徽和白底。图案已去掉原图的直径标注、箭头和尺寸线。

![128×128 红黑圆徽](assets/logo_128.png)

## 拼屏和接线

从发光面正视，默认四块都正装。MCU 接 **① 左上**的 `IN`，之后从上一块 `OUT` 接下一块 `IN`。最后一块的 `OUT` 留空。

```text
STM32 ──► ① 左上 ───────► ② 右上
                            │
                            ▼
          ④ 左下 ◄─────── ③ 右下
```

| HUB75E 插头脚位 | 信号 | STM32F103 |
| ---: | --- | --- |
| 1 / 2 / 3 | R1 / G1 / B1 | PA0 / PA1 / PA2 |
| 4 / 16 | GND | GND |
| 5 / 6 / 7 | R2 / G2 / B2 | PA3 / PA4 / PA5 |
| 8 | E | PB12 |
| 9 / 10 / 11 / 12 | A / B / C / D | PB0 / PB1 / PB10 / PB11 |
| 13 | CLK | PA6 |
| 14 | LAT（也标作 STB） | PA7 |
| 15 | OE（低有效） | PA8 |

- 表中插头编号沿用参考灯板，接线前按实际 `IN` 插座丝印核对，尤其是 E 和 GND。默认时序以参考仓库的 ICN2012/ICN2027 灯板为基准。驱动芯片若要求专用上电配置，需按其手册补充初始化。
- 四块屏的电源端子分别并联到 5 V 电源，STM32、电平转换器和灯板共地。按灯板标注的最大电流选电源和电源线。
- STM32 的 GPIO 是 3.3 V。需要 5 V 逻辑输入时，用两片 5 V 供电的 SN74AHCT245 等缓冲器转换这 14 路信号。用 SN74AHCT245 时，MCU 接 A 侧、灯板接 B 侧，DIR 置高。PB13 在初始化完成后置高，供参考板的反相使能电路使用，直接接屏时可留空。SN74AHCT245 的 `/OE` 为低有效。用三态缓冲时，可在灯板侧的显示 OE 线上加上拉，使缓冲关闭期间保持消隐。 [TI SN74AHCT245](https://www.ti.com/product/SN74AHCT245)

## 编译和烧录

本机用 STM32CubeCLT 的 ARM GCC、CMake 和 Ninja。以此目录为工作目录运行以下命令。图像数据已经包含在源文件中，编译无需 Python。

```powershell
cmake --preset f103
cmake --build --preset f103
```

`build/hub75e_logo.elf` 包含 DWARF 4 调试信息，可供调试器定位源码和变量。`build/hub75e_logo.map` 列出各段和符号的链接地址，内存布局由 `linker/stm32f103c8.ld` 定义。烧录文件为 `build/hub75e_logo.hex` 和 `build/hub75e_logo.bin`。

ST-LINK 接 PA13（SWDIO）、PA14（SWCLK）、GND，按调试器要求接 3.3 V 电平参考，可接 NRST。BOOT0 保持低。用 STM32CubeProgrammer 的 SWD 模式打开并烧录生成的 HEX，也可执行以下命令。

```powershell
STM32_Programmer_CLI -c port=SWD -w build/hub75e_logo.hex -v -rst
```

## 屏序、方向和亮度

在 [include/hub75_config.h](include/hub75_config.h) 中，`HUB75_CHAIN_LAYOUT` 按从 MCU 到末屏的实际接线填写。某块屏倒装时把对应项改为 `ROTATE_180`。若所有正装屏各自左右镜像，把 `HUB75_FIRST_CLOCK_IS_RIGHT` 改为 `1`。每次修改后重新编译和烧录。

默认每行 300 µs，OE 点亮 120 µs，整屏约 104 Hz。可用 `HUB75_OE_US` 调整亮度。调大时给 256 次移位和锁存留出时间，否则刷新率会降低。扫描时消隐、移位、换地址和锁存，再由 TIM1 输出低有效点亮脉冲。

## 换图

使用 128×128 PNG。当前每个 RGB 通道有开、关两级，支持黑、红、绿、蓝、青、品红、黄、白。首次换图需安装 Pillow。运行后重新编译。

```powershell
python -m pip install Pillow
python tools/image_to_logo.py assets/logo_128.png
cmake --build --preset f103
```

图片放在 Flash，启动时按屏序生成 8 KiB 扫描缓冲。每个行地址发 256 个 CLK，先发送最远一块的像素。每个 CLK 同时写入屏内第 `r` 行的 RGB1 和第 `r+32` 行的 RGB2。

## 参考

- STM32F103C8T6、1/32 扫描和 GPIO 接线。 [jhwi0424/stm32f103-hub75e-64x64](https://github.com/jhwi0424/stm32f103-hub75e-64x64/blob/16e4508602e31d4cd370549d0d1e349d2c3af0c5/docs/hardware.md)
- 128×128 拼屏、串联顺序和面板旋转。 [vamoosebbf/hub75e](https://github.com/vamoosebbf/hub75e/blob/e39acc246b3ecbf868f75b81d9228e49d75ac9cd/src/hub75e.c)
- 时钟、GPIO 和 TIM1 寄存器。 [ST RM0008](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101-103-105-107-stm32f100-series-armbased-32bit-mcus-stmicroelectronics.pdf)
