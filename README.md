# 语行 - AI 赋能的个性化英语学习应用

本项目为 电子科技大学《程序设计项目实践 (PBLF) 》课程项目。

### DEMO
- http://47.108.209.237/
- https://linggo.mkfs.tech/

### 构建
#### 编译安装 libhv
以下为在 GNU/Linux 环境下安装 libhv 的方法，其他平台请按照[官方文档](https://github.com/ithewei/libhv/blob/master/README-CN.md#%EF%B8%8F-%E6%9E%84%E5%BB%BA)配置。
```shell
git clone https://gitee.com/mirrors/libhv.git
cd libhv
mkdir build && cd build
cmake .. && sudo make install
```
#### 编译 linggo
```shell
git clone https://gitee.com/cmvy2020/linggo.git
cd linggo
mkdir build && cd build
cmake .. && make
```
#### 运行 linggo
```shell
./linggo ../res/config/config.json
```
此时可以浏览器打开 `localhost:8000` 

### 依赖
- [mdui](https://www.mdui.org/docs/)
- [libhv](https://github.com/ithewei/libhv)
- [json-parser](https://github.com/json-parser/json-parser)
- [json-builder](https://github.com/json-parser/json-builder)