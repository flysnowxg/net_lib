# net_lib  
作者：flysnowxg  
email:flysnowxg@qq.com  
net_lib(一个简单的基于tcp的windows环境下的网络库)  
项目地址：https://github.com/flysnowxg/net_lib/
  
这是一个很简洁的网络库，暂时就命名为net_lib吧。  
net_lib基于windows的完成端口模型，net_lib并没有对socket做完整的封装，只是对tcp读写进行了封装  
这个库的目标是能提供高效的消息处理，和非常灵活的消息扩展机制  
目前消息扩展机制做的还不错，非常灵活。可以简单的定义消息，注册消息处理函数就能处理一个消息了。  
同时net_lib也为消息的序列化提供了一个高效灵活的库。  
消息收发时充分利用了完成端口模型的优点，避免了所有不必要的线程切换。  
但是对内存的申请和释放上目前还有很大的优化空间，有时间来做一下。  
  
另外这个库还提供session的管理机制，可以管理每个session相关的运行时数据。  
另外还提供了一个认证协议的框架，同时还提供了一个简单类似于widows的NTLM的简单的认证协议。  
