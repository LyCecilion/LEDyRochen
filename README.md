# LEDyRochen

CH546 11x44 显示屏写入工具, presented by Limity'roChen and LyCecilion.

> To the ages long past, I bid my most devout farewell, even as the world doth bid farewell to me.

## 构建

进入 Nix 开发环境后，使用 CMake preset 配置并构建：

```console
cmake --preset dev
cmake --build --preset dev
```

生成的程序位于 `build/ledyrochen`，编译数据库位于 `build/compile_commands.json`，clangd 会自动读取后者。

如需让编译器把所有警告视为错误，可在配置时额外传入 `-DLEDYROCHEN_WARNINGS_AS_ERRORS=ON`。
