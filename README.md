> [!WARNING]
> 仅个人使用的zsh配置和prompt，还没有达到starship、p10k、omp的高度。


```
g++ -std=c++17 -O3 main.cpp modules.cpp -o ~/.local/bin/prompt
```

也可以使用 Makefile：

```
make
make install
make test
```

`make install` 默认安装到 `~/.local/bin/prompt`，可用 `PREFIX`、`BINDIR`
或 `TARGET` 覆盖。

`make test` 会构建轻量级 C++ 测试程序，并创建临时目录 / Git 仓库覆盖宽度计算、Git 状态解析、语言检测和窄终端渲染。

### zsh 接入方式

```zsh
setopt PROMPT_SUBST
PROMPT='$(~/.local/bin/prompt $PROMPT_EXIT_CODE "$PROMPT_CMD_DURATION" $COLUMNS)'
RPROMPT=''
```

<p align="center">
  <img src="https://raw.githubusercontent.com/Archirithm/my_zsh_prompt/main/Screenshot1.png" width="500">
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/Archirithm/my_zsh_prompt/main/Screenshot2.png" width="500">
</p>

### features
- 时间戳 （命令错误显红色）
- 当前目录脚本语言
- 当前目录git状态
- 当前目录路径显示
- 计时器
- 简洁的历史记录
- 自动适应窗口宽度
