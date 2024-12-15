# 语行 - AI 赋能的个性化英语学习应用

本项目为《程序设计项目实践(PBLF)》课程项目。


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