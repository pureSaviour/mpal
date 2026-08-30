# MPAL

MPAL（Multi-Precision Arithmetic Library）是一个使用 C++23 开发的高性能大整数运算库。项目当前提供 `uint128_t`、`int128_t` 和仍在开发中的 `BigInt`，后续计划加入多线程与 CUDA 加速后端。

目前已经实现的主要内容：

- `uint128_t`：无符号 128 位整数、算术、位运算、比较、字符串转换和边界处理。
- `int128_t`：有符号 128 位整数，采用补码和模 2^128 回绕语义。
- 公共 128 位除法内核：包含 128÷64、规范化 128÷128、二的幂快速路径和编译期回退。
- 通用任意精度流式输入输出工具：提供可变长度和固定长度 limb 解析，以及与具体整数类型解耦的格式化输出能力。
- 符合 C++ 格式化整数规则的 `uint128_t`、`int128_t` 流式输入输出。
- `BigInt`：任意精度整数的基础实现，仍在持续完善。
- GoogleTest 边界与随机差分测试。
- Google Benchmark 微基准测试。

## 环境要求

- CMake 3.25 或更高版本（`CMakePresets.json` 的最低要求）。
- 支持 C++23 的 GCC、Clang 或 MSVC。
- Ninja。
- Git。测试和 benchmark 首次配置时会通过 Git 浅克隆 GoogleTest 与 Google Benchmark。

CUDA 当前尚未作为构建语言启用；相关忽略规则已经预留，以便后续增加 CUDA 后端。

## 构建

项目提供四个相互独立的 workflow preset：

| Preset | 构建类型 | 构建内容 |
| --- | --- | --- |
| `debug` | Debug | MPAL 库和主程序 |
| `release` | Release | MPAL 库和主程序 |
| `test` | Debug | MPAL 库和测试，不构建主程序与 benchmark |
| `benchmark` | Release | MPAL 库和 benchmark，不构建主程序与测试 |

构建并运行测试：

```bash
cmake --workflow --preset test
```

构建 Release benchmark：

```bash
cmake --workflow --preset benchmark
```

构建 Debug 或 Release 主程序：

```bash
cmake --workflow --preset debug
cmake --workflow --preset release
```

默认输出目录分别为：

```text
out/build/test/
out/build/benchmark/
out/build/debug/
out/build/release/
```

## 128 位后端

CMake 选项 `MPAL_USE_NATIVE_INT128` 控制是否优先使用编译器原生 128 位整数，默认开启：

```bash
cmake --preset benchmark -DMPAL_USE_NATIVE_INT128=ON
```

- GCC/Clang 在定义 `__SIZEOF_INT128__` 的目标上使用原生 `__int128`/`unsigned __int128` 后端。
- MSVC 没有对应的标准扩展类型，因此自动使用由两个 64 位 limb 组成的自定义后端，并在 x64 上使用 `_umul128`、`_udiv128` 等 intrinsic。
- 要在 GCC/Clang 上测试自定义后端，可配置 `-DMPAL_USE_NATIVE_INT128=OFF`。

切换后端或编译器时应使用新的构建目录，或者重新运行对应 configure preset，避免复用另一套编译器生成的 CMake 缓存。

## 流式输入输出

`include/utils_stream.h` 提供与具体整数类型解耦的通用整数流工具。输入端可以将字符流解析为由 32 位 limb 组成的任意精度结果，并提供两种存储方式：

- `read_integer(stream)`：使用可变长度存储，适合位宽不固定的任意精度整数。
- `read_integer<CharT, Traits, N>(stream)`：使用包含 `N` 个 limb 的固定长度存储，同时报告溢出，适合 `uint128_t`、`int128_t` 等固定位宽整数。
- `write_integer(...)`：统一处理符号、进制前缀、本地化数字分组、字段宽度、填充字符和对齐方式。

`uint128_t` 和 `int128_t` 在这些工具之上实现了适用于窄字符流与宽字符流的模板化 `operator<<` 和 `operator>>`。其行为遵循 C++ 格式化整数输入输出的主要规则，包括：

- 通过 `std::dec`、`std::oct`、`std::hex` 或 `std::setbase(0)` 决定输入进制；默认 `std::dec` 不会因为 `0` 或 `0x` 前缀自动切换进制，`std::setbase(0)` 才会启用前缀识别。
- 支持 `std::showbase`、`std::showpos`、`std::uppercase`、`std::setw`、`std::setfill` 以及 `left`、`right`、`internal` 对齐。
- 使用流的 locale 处理千位分隔与数字分组，并正确设置 `eofbit`、`failbit` 等流状态。
- 对十进制输出保留有符号数的符号；非十进制输出按照标准整数流的方式展示 `int128_t` 的 128 位补码表示。

示例：

```cpp
#include <iomanip>
#include <sstream>
#include "uint128.h"
#include "int128.h"

uint128_t value("340282366920938463463374607431768211455");

std::ostringstream output;
output << std::showbase << std::uppercase << std::hex << value;
// output.str() == "0XFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"

std::istringstream input("052 0x2a -42");
uint128_t octal;
uint128_t hexadecimal;
int128_t signed_value;
input >> std::setbase(0) >> octal >> hexadecimal >> signed_value;
// octal == 42, hexadecimal == 42, signed_value == -42
```

## Benchmark

benchmark 覆盖加法、减法、乘法、除法、取模、移位、比较、十进制解析与字符串转换。运行完整基准：

### 性能展示

以下结果于 2026-08-29 在同一台 Windows x64 机器上采集（32 个逻辑处理器，基准程序报告频率为 2495 MHz）。三组均使用 Release 构建、Google Benchmark 1.9.4、每项重复 5 次；表中为 5 次运行的平均墙钟时间，数值越低越好。

- GCC：GCC 15.1.0，`MPAL_USE_NATIVE_INT128=ON`，使用原生 `__int128` 后端。
- GCC：GCC 15.1.0，`MPAL_USE_NATIVE_INT128=OFF`，使用 MPAL 双 64 位 limb 后端。
- MSVC：MSVC 19.51.36252，`MPAL_USE_NATIVE_INT128=OFF`，使用 MPAL 双 64 位 limb 后端及 x64 intrinsic。

无符号 `uint128_t`：

| Benchmark | GCC 原生后端 | GCC 自定义后端 | MSVC 自定义后端 |
| --- | ---: | ---: | ---: |
| Add | 0.765 ns | 0.503 ns | 4.97 ns |
| Subtract | 0.783 ns | 0.503 ns | 4.97 ns |
| Multiply | 2.01 ns | 1.62 ns | 5.49 ns |
| Divide by 64-bit | 18.1 ns | 10.0 ns | 16.4 ns |
| Divide by 128-bit | 21.2 ns | 7.88 ns | 23.2 ns |
| Modulo 128-bit | 21.5 ns | 16.0 ns | 23.7 ns |
| Shift | 2.77 ns | 6.28 ns | 5.29 ns |
| Compare | 1.93 ns | 1.33 ns | 4.91 ns |
| Parse decimal | 990 ns | 899 ns | 884 ns |
| ToString | 1235 ns | 1335 ns | 3585 ns |

有符号 `int128_t`：

| Benchmark | GCC 原生后端 | GCC 自定义后端 | MSVC 自定义后端 |
| --- | ---: | ---: | ---: |
| Add | 1.18 ns | 0.700 ns | 5.05 ns |
| Subtract | 1.10 ns | 0.678 ns | 4.53 ns |
| Multiply | 2.25 ns | 2.17 ns | 5.01 ns |
| Divide positive | 22.8 ns | 9.57 ns | 18.7 ns |
| Divide negative | 23.8 ns | 10.3 ns | 23.5 ns |
| Modulo | 25.1 ns | 17.8 ns | 24.7 ns |
| Arithmetic shift | 3.64 ns | 3.23 ns | 5.99 ns |
| Compare | 2.10 ns | 1.20 ns | 5.34 ns |
| Parse decimal | 1072 ns | 956 ns | 901 ns |
| ToString max | 813 ns | 757 ns | 1134 ns |
| ToString min fast path | 67.9 ns | 94.2 ns | 86.0 ns |

这些结果展示的是“编译器 + 后端 + 当前硬件”的组合性能，并不是仅针对算法本身的隔离比较。GCC 与 MSVC 的代码生成、ABI 和 intrinsic 不同，因此不能根据该表断言某个后端在所有平台上都更快；如需严格比较自定义后端与原生后端，应在同一 GCC/Clang 版本下分别使用 `MPAL_USE_NATIVE_INT128=ON/OFF` 构建并测试。

```bash
./out/build/benchmark/benchmarks/mpal_benchmarks
```

Windows：

```powershell
.\out\build\benchmark\benchmarks\mpal_benchmarks.exe
```

只运行除法和取模并进行多次重复：

```powershell
.\out\build\benchmark\benchmarks\mpal_benchmarks.exe `
  --benchmark_filter="(Divide|Modulo)" `
  --benchmark_repetitions=10 `
  --benchmark_report_aggregates_only=true
```

### GCC/Clang 原生后端

在 GCC 或 Clang 环境中执行：

```bash
cmake --preset benchmark -DMPAL_USE_NATIVE_INT128=ON
cmake --build --preset benchmark
./out/build/benchmark/benchmarks/mpal_benchmarks
```

该结果衡量编译器原生 `__int128` 后端。若要和 MPAL 自定义 limb 后端在同一编译器、同一机器上比较，重新配置为：

```bash
cmake --preset benchmark -DMPAL_USE_NATIVE_INT128=OFF
cmake --build --preset benchmark
```

### MSVC 自定义后端

先打开 Visual Studio x64 Developer Command Prompt，再执行：

```powershell
cmake --preset benchmark -DMPAL_USE_NATIVE_INT128=OFF
cmake --build --preset benchmark
.\out\build\benchmark\benchmarks\mpal_benchmarks.exe
```

MSVC 构建使用自定义 limb 后端；x64 除法会进入 `_udiv128` 优化路径。比较结果时应使用相同机器、Release 配置、动态操作数和足够多的重复次数。Debug 下的除法结果不适合代表最终性能。

## 项目结构

```text
include/       公开头文件和 128 位公共后端
  utils_stream.h  通用任意精度整数流解析与格式化工具
src/           MPAL 库实现
tests/         GoogleTest 测试
benchmarks/    Google Benchmark 基准
main.cpp       示例主程序
```

第三方测试依赖不会加入源码树。启用相应 preset 后，CMake `FetchContent` 会通过 Git 获取固定版本：

- GoogleTest `v1.14.0`
- Google Benchmark `v1.9.4`
