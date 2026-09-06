# GitHub 提交清单

## 1. 仓库与分支

```text
远端：origin = https://github.com/Hao1937/health-terminal.git
当前分支：feature/liuyanming-oled
PR base：dev
```

正确流向：

```text
feature/liuyanming-oled → Pull Request → dev → 稳定后由组长合入 main
```

不要直接 push 到 `main`，也不要绕过 PR 把本次大批改动直接合入 `dev`。

## 2. 本人工作对应的源码清单

### 应用与 UI

```text
app/app_statemachine.c
app/ui_flow.c
app/ui_flow.h
```

### OLED、键盘、反应、存储、平衡

```text
modules/oled/oled.c
modules/oled/oled.h
modules/keypad/keypad.c
modules/reaction/reaction.c
modules/storage/storage.c
modules/mpu6050/mpu6050.c
```

相关 `.h` 如果当前没有 diff，不会产生提交内容，但仓库中仍是编译依赖。

### BLE 与 ESP32

```text
modules/ble/ble.c
modules/ble/ble.h
esp32/ble_gateway/ble_gateway.ino
esp32/ble_gateway/README.md
```

### Web

```text
web/dashboard.html
web/README.md
```

### 项目入口与联调文档

```text
README.md
docs/BLE联调.md
```

本次提交 manifest 固定包含 `submit/` 下这 9 份文档：

```text
submit/README.md
submit/01-工作说明与系统复现.md
submit/02-硬件接线与上电检查.md
submit/03-OLED菜单与矩阵键盘.md
submit/04-反应测试-Flash历史-平衡实验.md
submit/05-BLE与Web看板联调.md
submit/06-测试与验收记录.md
submit/07-GitHub提交清单.md
submit/08-本次修改-硬件接线-全流程测试与提交清单.md
```

当前阶段只保存在 `submit/`，不执行上传；用户后续明确要求上传时，将上述文档与本文件列出的源码一起提交。

## 3. 明确排除

绝对不要提交：

```text
core/此电脑.lnk
电子爱好者之家 ESP-WROOM-32 开发板 资料.zip
```

原因：

- `.lnk` 是本机个人快捷方式，可能泄露路径；
- ZIP 是第三方资料/驱动/安装包，约数百 MB，来源与再分发许可不明确，并超过 GitHub普通单文件限制；
- 项目只需在 README 写硬件型号、Core版本和驱动名称。

继续排除：

```text
build/
core/STM32CubeF1/
*.elf
*.bin
*.hex
*.map
*.o
*.obj
*.exe
.vscode/
.idea/
```

这些属于构建产物、外部依赖或个人配置。

## 4. 重复文档处理

当前根目录：

```text
BLE.md
```

与：

```text
docs/BLE联调.md
esp32/ble_gateway/README.md
web/README.md
```

内容重复。建议：

1. 通用链路、协议、联调顺序和排障放 `docs/BLE联调.md`；
2. ESP32烧录与网关细节放 `esp32/ble_gateway/README.md`；
3. 网页使用放 `web/README.md`；
4. 根目录 `BLE.md` 不提交，或确认独有内容已合并后删除。

## 5. 推荐原子提交

本次改动范围较大，建议拆成可评审的提交，而不是一个“全部完成”提交：

```text
feat(ui): 实现 OLED 多级菜单与矩阵键盘导航
feat(reaction): 实现 EXTI 反应时间测试与趋势
feat(storage): 实现片内 Flash 历史记录
feat(balance): 实现 MPU6050 平衡晃动指数
feat(ble): 完成 STM32 USART2 记录透传
feat(esp32): 新增 UART 到 BLE Notify 网关
fix(web): 兼容安卓 BLE 设备选择并修正通知切片
docs(liuyanming): 补充硬件复现、联调与验收说明
```

如果拆分时中间提交无法独立编译，可把紧密依赖的 UI 和状态机放在同一提交；每个提交结束后都应保证构建可用。

## 6. PR 建议

### 标题

```text
feat(liuyanming): 完成交互、历史、平衡与 BLE Web 链路
```

### PR 说明模板

```markdown
## 完成内容
- OLED SSD1306 菜单、反白、记录显示和趋势图
- 4×4 矩阵键盘扫描、消抖及导航键
- PB1/PB0 EXTI 反应时间测试
- 片内 Flash 历史记录
- MPU6050 平衡晃动指数
- STM32 USART2 二进制帧透传
- ESP32 UART↔FFE1 Notify/Write 双向网关
- Web Bluetooth 单页看板与安卓设备发现兼容
- 手机逐项修改 STM32 当前记录并可保存到 Flash 历史

## 复现资料
见 `submit/README.md`（如最终决定提交该目录）及模块 README。

## 验证
- [ ] host tests 全绿
- [ ] MODULE_SET=liuyanming 构建通过
- [ ] MODULE_SET=full 构建通过
- [ ] STM32 真机菜单/键盘/反应/Flash/MPU6050 验证
- [ ] ESP32 广播 FFE0/FFE1 验证
- [ ] 安卓 HTTPS Web Bluetooth 真机记录验证
- [ ] 手机 SET/APPLY/SAVE 与断电历史保持验证

## 已知限制
- 体脂若未由传感器/键盘产生，可由手机补录；空白仍为 `HS_VALUE_INVALID`
- BLE 主动记录帧仍无逐帧 ACK；手机 SET 命令逐项等待状态确认，错过主动帧可同步历史
- Flash 写满后整区擦除，无手动清空
- 睁眼/闭眼实验条件及每次原始值需人工记录
- Balance 即时页、History/Web 统一按 `balance_x10 / 10` 显示一位小数
```

## 7. 提交前检查

```powershell
cmake --preset host
cmake --build --preset host
ctest --preset host --output-on-failure

cmake --preset stm32 -DMODULE_SET=liuyanming
cmake --build --preset stm32

cmake --preset stm32 -DMODULE_SET=full
cmake --build --preset stm32
```

然后检查：

```powershell
git status --short
git diff --check
git diff --stat
```

确认：

- [ ] 没有 `.lnk`、ZIP、build产物；
- [ ] `app/ui_flow.h` 没有漏掉；
- [ ] ESP32网关两个文件都在；
- [ ] `dashboard.html` 的 `acceptAllDevices` 兼容修改在；
- [ ] 手机编辑区及 `SET/APPLY/SAVE` 协议在；
- [ ] PR diff 不含 `.github/workflows/pages.yml`（Pages 由独立 `gh-pages` 分支发布）；
- [ ] 不提交重复 `BLE.md`；
- [ ] 源码格式符合 clang-format（当前变更文件已通过 19.1.7 dry-run，最终提交集仍需复测）；
- [ ] 测试结果和真机证据已写入 PR；
- [ ] 公共契约若发生修改，已请至少一名其他成员 review。

## 8. 推送与 PR（仅在用户后续明确要求时执行）

首次推送个人分支：

```powershell
git push -u origin feature/liuyanming-oled
```

然后在 GitHub 创建：

```text
base: dev
compare: feature/liuyanming-oled
```

提交前应先完成本清单中的构建、格式和真机证据核对；本资料包不包含自动推送或创建 PR 的操作。
