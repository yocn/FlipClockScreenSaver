# Classic Flip Clock

Windows 10 / 11（x64）经典翻页时钟屏幕保护程序。黑色背景、白色数字、机械翻页效果，无网络请求、无声音，不需要安装 .NET 或浏览器运行环境。

![启用日期与星期后的翻页时钟效果预览](preview/calendar-screenshot.png)

> 图片和 `preview/index.html` 是浏览器视觉预览；实际屏保使用 Windows 原生绘图，字体和翻页透视会略有不同。

## 安装

1. 在本仓库 **Actions → Build Windows screensaver** 中打开一次成功的构建，下载 `ClassicFlipClock-Windows-x64` artifact 并解压。
2. 双击 `Install.cmd`，屏保会复制到 `%LOCALAPPDATA%\ClassicFlipClock\ClassicFlipClock.scr`，然后打开 Windows 屏幕保护程序设置。
3. 设置等待时间，按需勾选“在恢复时显示登录屏幕”，点击“应用”。

也可将 `ClassicFlipClock.scr` 放到固定目录，右键选择“安装”。安装后请勿移动该文件。

这是未签名的自编译程序。若组织策略禁止运行未签名程序，请先按照组织的软件安装流程处理。

## 使用

- 默认 24 小时制，显示小时、分钟、秒；只对变化的卡片播放翻页动画。
- 在 Windows 屏保设置中点击“设置”，可隐藏秒数、开启 12 小时制（AM/PM）、显示日期、显示星期、关闭位置微移。
- `Show date`（显示日期）与 `Show day of week`（显示星期）可独立开关，也可同时开启，显示在时钟下方；日期格式和星期名称跟随系统区域设置，中文环境显示“星期三”等名称。默认关闭，升级保留原有设置。
- 默认每分钟轻微调整位置；这不能保证避免 OLED 烧屏，长期闲置仍应让 Windows 关闭显示器。
- 每台显示器独立居中显示。全屏时移动鼠标超过 6 像素、点击鼠标或按键退出。
- 初始 800 毫秒忽略鼠标位移，避免启动时被残留移动立即关闭。
- 锁屏、密码验证、等待时间和休眠均交由 Windows 管理。程序不会改变这些策略或阻止显示器休眠。
- 配置保存在 `HKCU\Software\ClassicFlipClock`。

屏保标准命令：

```text
ClassicFlipClock.scr /s          全屏运行
ClassicFlipClock.scr /c          打开设置（无参数也打开设置）
ClassicFlipClock.scr /c:12345     带父窗口的设置
ClassicFlipClock.scr /p 12345     嵌入 Windows 的小预览窗口
```

卸载：先在 Windows 屏保设置中选择“无”或其他屏保，再删除 `%LOCALAPPDATA%\ClassicFlipClock` 文件夹。可选删除上述应用配置注册表项。

## 直接看效果

双击 `preview/index.html`，可在 macOS、Windows 或其他平台的现代浏览器查看视觉效果，页面下方提供全屏和设置按钮。这个页面不会注册系统屏保。

## 构建

### Windows（Visual Studio C++ 桌面开发工具）

在 **x64 Native Tools Command Prompt** 中执行：

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1
```

输出：`dist\ClassicFlipClock.scr`。`Install.cmd` / `Install.ps1` 在 `packaging` 目录，分发时与 `.scr` 放在同一目录。

### macOS / Linux（MinGW-w64）

```sh
# macOS 首次需要安装交叉编译器
brew install mingw-w64
./test.sh
./build.sh
```

核心逻辑测试使用本机 C++17 编译器；屏保使用 `x86_64-w64-mingw32-g++` 生成 Windows PE64 文件。构建为静态 C++ 运行时，仅依赖 Windows 系统 DLL。

## 验证范围

- 可移植测试：命令行解析（含无效/溢出 HWND）、午夜/中午、12/24 小时制、屏幕布局和翻页阶段。
- 浏览器预览：设置、持久化、横屏/竖屏/小尺寸布局。
- GitHub Actions：在 Windows 上使用 MSVC 编译并运行核心测试；检查无效参数、嵌入式预览、父窗口销毁、设置窗口和全屏按键消息退出，再上传屏保包。
- 首次验证：[Windows 构建与运行检查全部通过](https://github.com/yocn/FlipClockScreenSaver/actions/runs/41821647326)。
- 仍需 Windows 交互式验收：系统屏保预览、混合 DPI 多屏、真实字体/动画、鼠标退出、待机启动、锁屏恢复。浏览器预览和编译成功不能替代这些检查。

## 文件结构

```text
src/core.h            可移植的参数、时间、布局和动画计算
src/renderer.h        GDI+ 卡片缓存、双缓冲与翻页绘制
src/main.cpp          Windows 屏保生命周期与配置
src/app.rc            设置窗口和版本资源
tests/core_test.cpp   核心行为验证
preview/              可独立打开的浏览器视觉预览
packaging/            当前用户安装助手
```

屏保参数和安装方式参考 [Microsoft 屏保示例](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms686421(v=vs.85)) 与 [SCRNSAVE.EXE 文档](https://learn.microsoft.com/en-us/windows/win32/devnotes/scrnsave-exe)。
