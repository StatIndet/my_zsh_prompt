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

### Git 状态栏

Git 状态栏显示在当前目录 pill 后面。非 Git 目录、Git 命令超时或 Git 命令异常时，不显示 Git pill。

显示格式：

```text
󰘬 <branch> <operation> <sync> <changes>
```

示例：

```text
󰘬 main
󰘬 main ⇡1
󰘬 main ⇣2
󰘬 main ⇕1/2
󰘬 main +2 ~3 ?1
󰘬 main MERGE !1 +2
󰘬 detached@a1b2c3d ~2
```

字段含义：

- `branch`：当前分支名，过长时按可见宽度截断；detached HEAD 显示为 `detached@<short-hash>`。
- `operation`：当前 Git 操作状态，只显示一个，优先级为 `REBASE`、`MERGE`、`PICK`、`REVERT`。
- `sync`：与上游分支的同步状态，`⇡N` 表示 ahead，`⇣N` 表示 behind，`⇕A/B` 表示 diverged。
- `changes`：工作区 / 暂存区变化，按 `!N`、`+N`、`~N`、`-N`、`»N`、`?N` 的顺序显示，最多显示 4 类。

变化符号：

- `!N`：冲突文件数量。
- `+N`：已暂存变更数量。
- `~N`：已修改但未暂存数量。
- `-N`：删除数量。
- `»N`：重命名数量。
- `?N`：未跟踪文件数量。

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
