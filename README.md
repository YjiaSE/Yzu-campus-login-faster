# YZU CampusNet Login C++ write
参考前两个开源项目
+ [py版本](https://github.com/TerraceCN/yzu-campusnet-login)
+ [go版本](https://github.com/luoboQAQ/yzu-campusnet-login)
# 项目使用指北
- 此项目的可执行程序仅有272KB大小
- 读取 `.env` 或系统环境变量
- 定时检测是否已经联网
- 断网后自动登录校园网
- 使用 `-logout` 一键下线当前账号的在线客户端

## 依赖

- C++17 编译器
- CMake 3.16+
- libcurl 开发库

Ubuntu/Debian 示例：

```bash
sudo apt install build-essential cmake libcurl4-openssl-dev
```

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

生成的程序位于：

```bash
./build/netool
```

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

`CHECK_INTERVAL` 和 `START_DELAY` 支持纯数字秒数，也支持 `s`、`m`、`h` 后缀，例如 `60`、`60s`、`1m`。

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
