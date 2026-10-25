# YZU CampusNet Login C++ write
参考前两个开源项目
+ [py版本](https://github.com/TerraceCN/yzu-campusnet-login)
+ [go版本](https://github.com/luoboQAQ/yzu-campusnet-login)
# 项目使用指北
- 提供 Qt 图形界面 `netool-gui` 和轻量命令行程序 `netool`
- 读取 `.env` 或系统环境变量
- 定时检测是否已经联网
- 断网后自动登录校园网
- 使用 `-logout` 一键下线当前账号的在线客户端

## 依赖

- C++17 编译器
- CMake 3.16+
- libcurl 开发库
- Qt 6 或 Qt 5 Widgets 开发库（仅图形界面需要）

Ubuntu/Debian 示例：

```bash
sudo apt install build-essential cmake libcurl4-openssl-dev qt6-base-dev
# 也可使用 Qt 5：将 qt6-base-dev 替换为 qtbase5-dev
```

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

启动图形界面：

```bash
./build/netool-gui
```

默认同时生成命令行程序 `./build/netool`。不需要 Qt 的服务器或路由器可仅构建命令行版本：

```bash
cmake -S . -B build-cli -DBUILD_GUI=OFF
cmake --build build-cli -j
```

已同时安装 Qt 5/6 时默认优先 Qt 6；使用 `-DQT_DIR=/path/to/Qt5/lib/cmake/Qt5` 指定 Qt 5 的 CMake 包目录。

## 图形界面

1. 填写账号、密码，选择学校、移动、联通、电信或校内免费服务。
2. 点击「立即登录」执行一次登录；「检测网络」无需填写账号即可使用。
3. 点击「开启自动重连」，程序会先检测连接，断网时等待指定时间后登录，完成后按检测间隔继续检查；失败也会等待间隔，避免连续重试。
4. 「停止」取消等待、后续检测和登录；正在执行的网络请求会等待结束（单次超时 5 秒）。停止不会主动下线设备。
5. 「下线全部设备…」确认后调用原有自助服务，下线此账号的全部在线客户端。

窗口显示连接状态和带时间的日志。网络请求在后台线程中执行，关闭窗口会停止任务并在当前请求结束后退出；关闭后不再自动重连。

启动时读取当前工作目录的 `.env` 和系统环境变量，也可通过「导入 .env」选择其他文件。同名系统环境变量优先于文件。缺少账号或密码时可直接在界面补填。`USER_AGENT` 可通过配置文件或环境变量设置。

程序使用 Qt 应用设置记住账号、服务、检测间隔和登录延迟；密码只在内存中使用，不写入应用设置。已有 `.env` 中的密码仍由用户自行管理。自动重连运行时先点击「停止」再修改配置。调试模式会在检测已联网时仍尝试登录。互联网检测未通过并不一定表示校园网认证失败，校内免费服务也可能无法访问互联网检测地址。

## 测试

安装对应版本的 Qt Test 开发组件后：

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

测试使用离屏 Qt 平台，覆盖配置解析、环境变量优先级、非法时间、表单校验、密码显示以及等待登录时停止/退出，不发送真实登录或下线请求。真实认证需要在扬州大学校园网环境中验证。

## 配置

可以使用系统环境变量，也可以在当前目录放 `.env` 文件。

```dotenv
SSO_USERNAME=你的学号或账号
SSO_PASSWORD=你的密码
CAMPUSNET_SERVICE=学校互联网服务
CHECK_INTERVAL=60
START_DELAY=5
DEBUG=false
```

`CHECK_INTERVAL` 和 `START_DELAY` 支持纯数字秒数，也支持 `s`、`m`、`h` 后缀，例如 `60`、`60s`、`1m`。检测间隔必须大于 0，登录延迟可为 0；两者最大为 2147483 秒。

## 运行

```bash
./build/netool
```

指定环境变量文件：

```bash
./build/netool -e .env.router
```

下线当前账号的全部在线客户端：

```bash
./build/netool -logout
```
# 免责说明
YZU CampusNet Login C++ write（以下简称“本脚本”）为便于作者个人生活的脚本，本脚本所用的方法均为对正常登录过的模拟，不得用于任何商业用途。

本脚本之著作权归脚本作者所有。用户可以自由选择是否使用本脚本。如果用户下载、安装、使用本脚本，即表明用户信任该脚本作者，脚本作者对因使用项目而造成的损失不承担任何责任。
