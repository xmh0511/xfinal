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
   /*可以通过serve.static_path("./static") 改变目录 同时会自动改变静态资源的url的标识*/
   /*内置处理方式默认设置了static  用户可以通过localhost:8080/static/abc.png 来访问静态资源文件*/
   /*对于多媒体等文件默认支持断点续传功能*/
   serve.run();
}
````

## xfinal 为你的接口提供拦截器
````
#include "http_server.hpp"
using namespace xfinal;
struct http_interceptor{
   bool before(request& req,response& res){  //这里的返回值用来告诉框架是否还继续执行用户接下来的拦截器以及注册的路由逻辑
       auto id = req.query("id");
       if(id=="0"){
          return true;
       }else{
         return false;
       }
   }
   
   bool after(request& req,response& res){
      res.add_header("Access-Control-Allow-Origin","*");
      return true;
   }
}
int main()
{
   http_server serve(4) //线程数
   serve.router<POST>("/interceptor",[](request& req,response& res){
       res.write_string("OK");
   },http_interceptor{}); //可以注册多个拦截器 定义如示例中的http_interceptor
   serve.run();
}
````

## xfinal 支持项目结构分层
````
////Test.hpp
#pragma once
#include "http_server.hpp"
class Test{
  public:
    void shop(request& req,response& res){
        res.write_string("shop !");
    }
} 
////Test.hpp
#include "http_server.hpp"
#include "Test.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   /*接口不记录信息的*/
   serve.router<GET,POST>("/shop",&Test::shop,nullptr);
   /*这里可以记录下每次请求的一些信息，用于下次请求使用*/
   Test t;
   serve.router<GET,POST>("/shop",&Test::shop,t);
   serve.run();
}

或者可以继承xfinal 的Controller

///shop.hpp
#pragma once
#include "http_server.hpp"
class Shop:public Controller{
  public:
    void go(){
        this->get_response().write_string("go shopping!");
    }
}
///shop.hpp

#include "http_server.hpp"
#include "shop.hpp"
using namespace xfinal;
int main()
{
   http_server serve(4) //线程数
   /*不记录信息的*/
   serve.router<GET,POST>("/shop",&Shop::go,nullptr);
   
    /*这里可以记录下每次请求的一些信息，用于下次请求使用*/
   Shop t;
   serve.router<GET,POST>("/shop",&Shop::go,t);
   serve.run();
}
````

