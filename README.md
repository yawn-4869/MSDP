### 环境配置说明
```
虚拟机：CentOS7

gcc: 9.3.1 

cmake: 3.8.0

protobuf: 3.19.4

依赖库：

Eigen -- 卡尔曼滤波

rocket -- rpc调用

tinyxml -- rocket读取配置文件需要
```

步骤如下

1. 在命令行安装gcc（若自带gcc则可跳过，需要root权限）
```
sudo yum install gcc
```

2. 在命令行安装gcc-c++（需要root权限）
```
sudo yum install gcc-c++
```

3. 安装cmake（需要root权限）
```
解压压缩包：
tar -zxvf cmake-3.8.0.tar.gz 
进入解压后的文件夹：
cd cmake-3.3.2
输入:
./bootstrap && make -j4 && sudo make install
```

4. 安装eigen（需要root权限）
```
解压压缩包：
tar -zxvf eigen-3.3.9.tar.gz 
进入解压后的文件夹：
cd eigen-3.3.9
输入以下指令:
mkdir build  # 新建一个build文件夹
cd build  # 进入build文件夹
cmake ..  # 用cmake生成Makefile
make install  # 安装
创建软连接：
ln -s /usr/local/include/eigen3/Eigen /usr/local/include/Eigen
ln -s /usr/local/include/eigen3/unsupported /usr/local/include/unsupported 
```

5. 安装protobuf
```
tar -xzvf protobuf-cpp-3.19.4.tar.gz
我们需要指定 安装路径在 `/usr` 目录下:
cd protobuf-cpp-3.19.4
./configure -prefix=/usr/local
make -j4 
sudo make install
```

输入命令查看protobuf版本
```
protoc --version
```
显示版本号 `libprotoc 3.19.4` 代表安装成功

安装完成后，可以找到头文件将位于 `/usr/local/include/google` 下，库文件将位于 `/usr/local/lib` 下。

6. 项目编译及运行
```
进入项目文件夹MSDP_V0.1, 输入以下命令: 
mkdir build
cd build
cmake ..
make

等待编译完成, 输入: 
./msdp_app
项目开始运行
```

### 相关配置说明
融合程序处理周期：4s

心跳包发送周期：1s

节点检查周期：2s

心跳包未接收到最大忍耐时间：2s

连续4次未接收到心跳包视为掉线