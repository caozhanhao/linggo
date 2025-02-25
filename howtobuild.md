# 语行 - AI 赋能的个性化英语学习网站

### 在线示例

以下是部署至云端的示例

- http://47.108.209.237/

### 项目结构
- GitHub 仓库: https://github.com/caozhanhao/linggo

| 文件                 | 描述                               |
|--------------------|----------------------------------|
| include            | C 语言头文件                          |
| src                | C 语言源代码                          |
| res                | 网页资源文件                           |
| thr                | 第三方依赖库                           |
| bin-windows-x86_64 | 编译好的 X86_64 架构下 Windows 平台的可执行文件 |
| bin-linux-x86_64   | 编译好的 X86_64 架构下 Linux 平台的可执行文件   |
| CMakeLists.txt     | CMake 构建配置                       |
| README.md          | GitHub Readme 文件                 |
| LICENSE            | MIT 开源协议                         |
| .gitignore         | Git 相关文件                         |

### 运行

本项目跨平台，已经在 x86_64 架构下的 Windows / Linux 经过测试，并有打包好的可执行文件(bin-windows-x86_64 和
bin-linux-x86_64)。点击即可执行。
运行时注意全部解压后运行，尽量不要移动文件夹的相对位置（移动后须在 config.json 中重写配置）

- Linux 下 `./linggo-linux-x86_64 ../res/config/config.json`, 如果遇到权限问题请先 `chmod +x linggo-linux-x86_64`
  后再次之前上述命令。
- Windows下 **可以** 在 cmd 中输入 `linggo-windows-x86_64.exe ../res/config/config.json`，**也可以**
  双击运行后输入 `../res/config/config.json`

此时可以浏览器打开 `127.0.0.1:8000`

### 构建

#### 编译安装 libhv

以下为在 GNU/Linux 环境下安装 libhv 的方法。
Windows 平台下可以使用 Vcpkg 安装 libhv。
其他平台请按照[官方文档](https://github.com/ithewei/libhv/blob/master/README-CN.md#%EF%B8%8F-%E6%9E%84%E5%BB%BA)配置。

```shell
git clone https://gitee.com/mirrors/libhv.git
cd libhv
mkdir build && cd build
cmake .. && sudo make install
```

#### 编译 linggo

```shell
git clone https://github.com/caozhanhao/linggo
cd linggo
mkdir build && cd build
cmake .. && make
# 也可以用 gcc:
# gcc ../src/* ../thr/json-builder/json-builder.c ../thr/json-parser/json.c libhv_static.a -I../include -I../thr/ -lm
```

### 配置

linggo 服务器的配置文件如下

```json
{
  "listen_address": "0.0.0.0",
  "listen_port": 8000,
  "resource_path": "../res",
  "python": "python3",
  "version": "1.0.0"
}
```

#### listen_address

表示 linggo 服务器的监听地址，默认 `0.0.0.0` 即可。

#### listen_port

表示 linggo 服务器的监听端口，如 `8000` `8080` `80` 等。

#### resource_path

表示 linggo 服务器资源文件夹的路径，可以用相对路径，具体请根据实际情况。

#### python

注意：AI 模块需要 Python 的支持，但基本功能不需要该配置，不配置该项也可正常使用除了 AI 模块的功能。

该配置项为 Python 可执行文件的路径。例如：（以下为在我的机器上的路径，配置时请根据实际情况）

- Windows 平台下可能为 `C:\\Users\\cao20\\AppData\\Local\\Programs\\Python\\Python312\\python.exe`
- GNU/Linux 平台下可能为 `/usr/bin/python3`

也可配置 PATH， 具体操作不在赘述。

AI 模块还需调用 Spark AI 的模块，需用 pip 安装

```shell
pip install spark_ai_python
pip install sparkai
```

此外如果遇到找不到安装的模块，还需要指定 Python 运行的参数或环境变量，可在这里一并指定。如：

- Windows
  平台下可能为 `set PYTHONPATH=%PYTHONPATH%;C:\\Users\\cao20\\ClionProjects\\linggo\\venv\\lib\\site-packages && C:\\Users\\cao20\\AppData\\Local\\Programs\\Python\\Python312\\python.exe`
- GNU/Linux 平台下可能为 `export PYTHONPATH=$PYTHONPATH:'xxx/site-packages'; python3`