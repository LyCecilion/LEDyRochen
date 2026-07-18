# LEDyRochen

Fedora/Linux 原生的 `0416:5020 / wch.cn CH546` 11×44 LED 显示屏写入工具，presented by Limity'roChen and LyCecilion。

> To the ages long past, I bid my most devout farewell, even as the world doth bid farewell to me.

LEDyRochen 使用 C++20、libusb 与 stb_truetype。它支持商家 `ASC11`/`HZK11` 原版点阵字库，也能用普通 TTF/TTC 字体渲染；每个 64 字节 OUT 报告后都会读取 CH546 的 64 字节确认包。

## 构建与测试

进入 Nix 开发环境后：

```console
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

程序位于 `build/ledyrochen`。编译数据库位于 `build/compile_commands.json`，clangd 会自动读取。需要将所有编译器警告视为错误时，配置时加入 `-DLEDYROCHEN_WARNINGS_AS_ERRORS=ON`。

在允许 LeakSanitizer 检查进程的普通终端中运行完整 sanitizer 测试：

```console
cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

该 preset 独立构建到 `build/sanitizers`，启用 ASan、UBSan 与 LSan；任何地址错误、未定义行为或内存泄漏都会令测试失败。

## USB 权限

首次使用时安装仅匹配这块显示屏的 udev 规则：

```bash
sudo install -Dm644 udev/70-ledyrochen.rules /etc/udev/rules.d/70-ledyrochen.rules
sudo udevadm control --reload-rules
```

然后拔下并重新插入显示屏。

## 使用

列出设备：

```console
./build/ledyrochen --list-devices
```

预览并只编码、不写入：

```console
./build/ledyrochen --vendor-font-dir ../RocheLCD/vendor --preview --dry-run "洛汐"
```

写入一至八条循环消息：

```console
./build/ledyrochen --vendor-font-dir ../RocheLCD/vendor "洛汐" "LUOXI OK" "你好，世界"
```

常用选项：

```text
--device BUS:DEV:OUT:IN
--speed 1..8
--brightness 25|50|75|100
--mode left|right|up|down|still|animation|drop|curtain|laser
--blink
--border
--font /path/to/font.ttf
--font-size 11
--threshold 96
--vendor-font-dir /path/to/vendor
--no-vendor-font
```

工具会优先使用当前目录 `vendor/ASC11` 与 `vendor/HZK11`。原版 GBK 字库不包含某个字符时，该条消息自动回退到轮廓字体。字库来自商家安装包，不应在没有授权的情况下单独提交或再分发。

写入后若仍显示充电画面，拔掉 USB 或按 USB 接口旁的按钮。

## 实现结构

- `src/vendor_font.*`：原版 ASC11/HZK11 加载、校验与点阵渲染
- `src/truetype.*`：UTF-8 轮廓字体渲染与二值化
- `src/pack.*`、`src/protocol.*`：位图封包、协议头与 64 字节补齐
- `src/device.*`：只移动的 RAII libusb 设备与 CH546 OUT/ACK 握手
- `tests/tests.cpp`：UTF-8、字库、封包和协议回归测试
