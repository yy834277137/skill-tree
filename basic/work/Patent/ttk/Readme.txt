1、测试工具集缩写“dspttk”，模块内非static接口以该命名开头 
2、该测试工具集以网页方式访问，建议使用Chrome浏览器，网页缩放比例不超过120% 
3、未避免重复错误，测试模块中代码是独立的： 
     ① 测试模块内仅能访问DSP组件源码工程内的无接口定义类头文件，比如sal_macro.h和sal_type.h
     ② 测试模块编译链接时，仅能访问DSP组件成果out/目录内资源

=====================【test目录结构】=====================
.
├── Makefile                    // test源码Makefile
├── config                      // 可执行程序dspttk运行依赖的配置文件
│   ├── devcfg                  // 各机型的默认配置文件，json格式
│   └── index.html              // Web服务的网页总入口
├── inc                         // 外部资源头文件
├── lib                         // 外部依赖库
├── src                         // test源码目录
│   ├── comm                    // 通用接口封装，比如进程间通信、文件操作等
│   ├── dspttk_devcfg           // 设备配置文件相关操作
│   ├── dspttk_main             // dspttk的主函数
│   ├── sample                  // DSP接口的示例代码
│   │   ├── dspttk_callback     // 回调接口的实现
│   │   ├── dspttk_cmd_disp     // 显示业务的实现
│   │   ├── dspttk_cmd_xray     // XRAY/XSP业务的实现
│   │   ├── dspttk_cmd_enc      // 编码业务的实现
│   │   ├── dspttk_cmd_dec      // 解码业务的实现
│   │   └── dspttk_init         // DSP初始化的实现
│   └── web                     // Web服务与动态网页生成
└── start.sh                    // dspttk的启动脚本
