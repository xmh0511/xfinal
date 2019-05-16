# xfinal
一个用于快速使用c++ 开发web服务器的框架
# xfinal简介
高性能易用的http框架,基于c++14标准开发

1. 统一而简单的接口
2. header-only
3. 跨平台
5. 支持拦截器功能

# 如何使用

## 编译依赖
使用支持c++14标准的c++ 编译器即可编译允许

# 快速示例

## Get请求 服务器返回Hello,world
````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   serve.listen("0.0.0.0","8080");
   serve.router<GET>("/hello",[](request& req,response& res){
      res.write_string("hello,world");
   });
   serve.run();
}
````
## url form 表单请求
````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   serve.listen("0.0.0.0","8080");
   serve.router<GET>("/form",[](request& req,response& res){
        auto name = req.query("name");  /*query接口用于查询 name="xxx" 的值*/ 
        res.write_string(view2str(name));
   });
   serve.run();
}
````
## multipart form 表单请求
````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   serve.listen("0.0.0.0","8080");
   serve.router<GET>("/form",[](request& req,response& res){
        auto name = req.query("name");  /*query接口用于查询 name="xxx" 的值 */ 
        auto& file = req.file("img"); /*file 接口用于查询 name="xxx" 的文件 */ 
        res.write_string(std::to_string(file.size()));
   });
   serve.run();
}
````
## octet-stream 请求处理

````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   serve.listen("0.0.0.0","8080");
   serve.router<GET>("/form",[](request& req,response& res){
        auto& file = req.file();  /*octet-stream 只能提交单个二进制流数据 所以这里的file没有参数 */
        res.write_string(std::to_string(file.size()));
   });
   serve.run();
}
````


## 文件请求
````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   serve.listen("0.0.0.0","8080");
   serve.router<GET>("/file",[](request& req,response& res){
      res.write_file("./static/need.bmp",/*这里有个bool类型参数 可以用来告诉接口 是直接返回文件内容(false) 还是以chunked的方式返回文件  
      (true)*/);
      /*chunked 方式支持断点续传 */
   });
   serve.run();
}
````
## xfinal 框架内置了静态文件处理
````
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   /*可以通过serve.static_path("./static") 改变目录 已经请求的标识*/
   /*内置处理方式默认设置了static  用户可以通过localhost:8080/static/abc.png 来访问静态资源文件*/
   http_server.run();
}
````
## xfinal 支持项目结构分层
````
#pragma once
class Test{
  public:
    void shop(request& req,response& res){
        res.write_string("shop !");
    }
}
#include "http_server.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   Test t;
   serve.router<GET,POST>("/shop",&Test::shop,nullptr);
    serve.router<GET,POST>("/shop",&Test::shop,t);
   http_server.run();
}
````

