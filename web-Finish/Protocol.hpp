#pragma once

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sstream>
#include <unordered_map>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <fcntl.h>
#include <sys/sendfile.h>

#include "Util.hpp"
#include "Log.hpp"

using std::cout;
using std::endl;
using std::string;

#define SEQ ": "
#define WWWROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"
#define PAGE_404 "404.html"

enum CODE{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUNT = 404,
    SERVER_ERROR = 500 // 服务器错误。请求无关
};

// 状态码状态
static string CodeToMsg(int code){
    string msg;
    switch(code){
        case 200:
            msg = "OK";
            break;
        case 404:
            msg = "NOT_FOUNT";
            break;
        default:
            break;
    }
    return msg;
}

// 后缀映射方式
static string SuffixToDesc(string suffix){
    static std::unordered_map<string, string> suff2D{ // to
        {".html","text/html"},
        {".js","application/x-javascript"},
        {".jpg","application/x-jpg"},
        {".css","text/css"}
        // 后续可以再加
    };

    auto iter = suff2D.find(suffix);
    if (iter != suff2D.end()){
        return iter->second;
    }
    return "text/html";
}


// 接收请求
class HttpRequest{
    public:
        string requset_line; // 请求行
        std::vector<string> request_header; // 请求属性
        string blank_line;  // 空行
        string request_body; //正文
        int Content_Length; // 正文字节数
        int size; // 资源大小


        // 解析的正文
        string method;
        string uri;
        string version;
        string path;
        string query_string; // GET 的参数

        // 哈希表
        std::unordered_map<string, string> header_kv;

        // cgi机制
        bool cgi;
    public:
        HttpRequest():Content_Length(0), cgi(false){};
        ~HttpRequest(){};
};

// 响应
class HttpResponse{
    public:
        string response_line; // 请求行
        std::vector<string> response_header; // 请求属性
        string blank_line;  // 空行
        string response_body; // 正文
        string suffix; // 后缀

        int status_code; // 状态码
        int fd; // 文件描述符
    public:
        HttpResponse():blank_line(LINE_END), status_code(OK), fd(-1){};
        ~HttpResponse(){};
};

// 对端
class EndPoint{
    private:
        int sock;
        HttpRequest http_request;   // 接收类
        HttpResponse http_response; // 响应类
        bool stop; // 判断读取错误
        
    public:
        bool IsStop(){
            return stop;
        }

    private:
        // 读取行
        bool RecvHttpRequestLine(){
            auto& line = http_request.requset_line;
            if (Util::ReadLine(sock, line) > 0){
                line.resize(line.size()-1); // 仅仅是为了输出美观，去掉'\n'
                LOG(INFO, http_request.requset_line);
            }else{
                stop = true;
            }
            return stop;
        }
        // 读取属性
        bool RecvHttpRequestHeader(){
            string line;
            while (true){
                line.clear();
                if (Util::ReadLine(sock, line)<= 0){
                    stop = true;
                    break;
                }       
                if (line == "\n"){
                    http_request.blank_line = '\n';
                    break;
                }
                line.resize(line.size()-1);
                http_request.request_header.push_back(line);
                // LOG(INFO, line);
            }
            return stop;
        }
        // 判断有无正文
        bool IsNeedRecvHttpRequestBody(){
            if (http_request.method == "POST"){
                auto& kv = http_request.header_kv;
                auto iter = kv.find("Content-Length");
                if (iter != kv.end()){
                    http_request.Content_Length = atoi(iter->second.c_str());
                    return true;
                }
            }
            return false;
        }
        // 读取正文
        bool RecvHttpRequestBody(){
            if (IsNeedRecvHttpRequestBody()){
                auto Content_Length = http_request.Content_Length;
                auto& request_body = http_request.request_body;
                char ch = 'X';
                while (Content_Length--){
                    ssize_t s = recv(sock, &ch, 1, 0);
                    if (s > 0){
                        request_body.push_back(ch);
                    }else{
                        stop = true;
                        break;
                    }
                }
            }
            return stop;
        }
        // 解析
        void ParseHttpRequestLine(){
            auto& line = http_request.requset_line;
            std::stringstream ss(line);
            ss >> http_request.method >> http_request.uri >> http_request.version;
            auto& method = http_request.method;
            // 统一大写！！！ 用C语言的
            std::transform(method.begin(), method.end(), method.begin(), ::toupper);

            // LOG(INFO, http_request.method);
            // LOG(INFO, http_request.uri);
            // LOG(INFO, http_request.version);
        }
        // 分析属性
        void ParseHttpRequestHeader(){
            string key;
            string value;
            for (auto& it : http_request.request_header){
                if (Util::CultString(it, key, value, SEQ)){
                    http_request.header_kv.insert({key, value});
                    // cout << "DEBUG : " << key << " -- " << value << endl;
                }else{
                    LOG(FATAL, "request header error");
                }
            }
        }
        // 添加web根目录前缀
        void AddRoot(string& path){
            string _before = path;
            path = WWWROOT;
            path += _before;
        }
        // 非cgi处理
        int ProcessNotCgi(){
            http_response.fd = open(http_request.path.c_str(), O_RDONLY);
            // cout << "path:" << http_request.path.c_str() << endl;
            if (http_response.fd >= 0){
                return OK;
            }
            return 404;
        }
        // cgi处理
        int ProcessCgi(){
            // LOG(INFO, "cgi begin !!!");
            int code = OK;

            auto& bin = http_request.path;
            auto& query_string = http_request.query_string; // get
            auto& request_body = http_request.request_body; // post
            auto& method = http_request.method;
            auto& response_body = http_response.response_body;
            string content_length_env;

            // 为了子进程可以用，必须在父进程定义
            int fread_cwrite[2];
            int cread_fwrite[2];
            // 建立匿名管道，两个
            if (pipe(cread_fwrite) < 0){
                LOG(ERROR, "input pipe error");
                code = SERVER_ERROR;
                return code;
            }
            if (pipe(fread_cwrite) < 0){
                LOG(ERROR, "output pipe error");
                code = SERVER_ERROR;
                return code;
            }
            pid_t pid = fork();
            if (pid == 0){
                // child
                // 子写
                close(fread_cwrite[0]);
                close(cread_fwrite[1]);

                // 导入环境变量
                string method_env = "METHOD=";
                method_env += method.c_str();
                putenv((char*)method_env.c_str());

                if ("GET" == method){
                    string q_s = "QUERY_STRING=";
                    q_s += query_string.c_str();
                    putenv((char*)q_s.c_str());
                    LOG(INFO, q_s.c_str());
                }else if ("POST" == method){
                    content_length_env = "CONTENT_LENGTH=";
                    content_length_env += std::to_string(http_request.Content_Length);
                    putenv((char*)content_length_env.c_str());
                    LOG(INFO, content_length_env.c_str());
                }

                // 重定向 -------- 注意！！！这里的重定向不能只能贴着execl前一条执行，否则前面有什么输出都会到管道
                // 写 1 重定向为 fread_cwrite[1];
                // 读 0 重定向为 cread_fwrite[0];
                dup2(cread_fwrite[0], 0);
                dup2(fread_cwrite[1], 1);

                // 程序替换执行文件
                execl(bin.c_str(), bin.c_str(), nullptr);

                exit(1);
            }else if (pid < 0){
                LOG(ERROR, "fork error");
                code = SERVER_ERROR;
                return code;
            }else{
                // father
                // 父读
                close(fread_cwrite[1]);
                close(cread_fwrite[0]);

                // 通过管道传递给子进程
                if ("POST" == method){
                    ssize_t size = 0;
                    ssize_t pos = 0, b_size = request_body.size();
                    ssize_t content_length = http_request.Content_Length;
                    while ((pos < content_length) && (size = write(cread_fwrite[1], request_body.c_str() + pos, b_size - pos)) > 0){
                        pos += size;
                    }
                }

                // 读取子进程传来的结果
                char ch = 'X';
                while (read(fread_cwrite[0], &ch, 1) > 0){
                    response_body.push_back(ch);
                }
                // LOG(WARNING, response_body.c_str());
                // LOG(WARNING, std::to_string(response_body.size()).c_str());


                int status = 0;
                pid_t ret = waitpid(pid, &status, 0);
                if (ret == pid){
                    if (WIFEXITED(status)){
                        if (WEXITSTATUS(status) == 0){
                            code = OK;
                        }else{
                            code = BAD_REQUEST; // 客户端数据有问题
                        }
                    }else{
                        code = SERVER_ERROR;
                    }
                }
                close(fread_cwrite[0]);
                close(cread_fwrite[1]);
            }
            return code;
        }
        void HandlerError(string page){
            // 因为sendfile又分cgi和非cgi，所以主要一错误，我们就设置为false
            http_request.cgi = false;

            http_response.fd = open(page.c_str(), O_RDONLY);
            if (http_response.fd > 0){
                struct stat st;
                // 选定文件
                stat(page.c_str(), &st);
                http_request.size = st.st_size;
                string line = "Content-Type: text/html";
                LOG(INFO, line.c_str());
                line += LINE_END;
                http_response.response_header.push_back(line);

                line = "Content-Length: ";
                line += std::to_string(http_request.size);
                LOG(INFO, line.c_str());
                line += LINE_END;
                http_response.response_header.push_back(line);
            }
        }
        void BuildOKResponse(){
            string header_line = "Content-Type: ";
            header_line += SuffixToDesc(http_response.suffix);
            header_line += LINE_END;
            http_response.response_header.push_back(header_line);

            header_line = "Content-Length: ";
            if (http_request.cgi){    // cgi
                header_line += std::to_string(http_response.response_body.size());
                // LOG(WARNING, std::to_string(http_response.response_body.size()).c_str());
            }else{  // 非cgi
                header_line += std::to_string(http_request.size); 
            }
            header_line += LINE_END;
            http_response.response_header.push_back(header_line);

        }

        // 构建响应
        void BuildHttpResponseHelper(){
            auto& code = http_response.status_code;
            auto& line = http_response.response_line;
            line += HTTP_VERSION;
            line += " ";
            line += std::to_string(code);
            line += " ";
            line += CodeToMsg(code);
            line += LINE_END;

            string path = WWWROOT;
            path += "/";
            switch (code)
            {
            case OK:
                BuildOKResponse();
                break;
            case NOT_FOUNT:
                path += PAGE_404;
                HandlerError(path); 
                break;
            case BAD_REQUEST:
                path += PAGE_404;
                // HandlerError(PAGE_404);
                break;
            case SERVER_ERROR:
                path += PAGE_404;
                // HandlerError(PAGE_404);
                break;
            default:
                break;
            }

        }


    // -----------------共公有----------------

    public:
        EndPoint(int _sock)
            :sock(_sock)
            ,stop(false)
        {}
        // 读取
        void RecvHttpRequest(){
            if ( (!RecvHttpRequestLine()) && (!RecvHttpRequestHeader()) ){
                // 如果错误，返回true，都不为真，说明顺利
                ParseHttpRequestLine();
                ParseHttpRequestHeader();
                RecvHttpRequestBody();          
            }
        }

        // 构建响应
        void BuildHttpResponse(){
            auto& status_code = http_response.status_code;
            struct stat st;
            int size, pos;
            // cout << "--------" << http_request.method << endl;
            if (http_request.method != "POST" && http_request.method != "GET"){
                // 非法请求
                LOG(FATAL, "method warning");
                status_code = BAD_REQUEST;
                goto END;    
            }
            if (http_request.method == "GET"){
                // 因为GET也可以在url处带参，所以要查询一下
                auto& uri = http_request.uri;
                size_t pos = uri.find("?");
                if (pos != string::npos){
                    // 分离提取
                    Util::CultString(uri, http_request.path, http_request.query_string, "?");
                    http_request.cgi = true;
                }else{
                    http_request.path = uri;
                }
            }else if (http_request.method == "POST"){
                http_request.cgi = true;
                http_request.path = http_request.uri;
            }else{
                // 其他
            }
            AddRoot(http_request.path);
            // 判断是否是根目录
            if (http_request.path[http_request.path.size()-1] == '/'){
                // 默认显示首页
                http_request.path += HOME_PAGE;
            }
            // 判断是否是存在
            if (stat(http_request.path.c_str(), &st) == 0){
                // 判断是否是目录
                
                if (S_ISDIR(st.st_mode)){
                    http_request.path += "/";
                    http_request.path += HOME_PAGE;
                    // 因为已经不是原来的目录了，所以要更新st
                    stat(http_request.path.c_str(), &st);
                }
                // 判断是否是可执行文件，按位与
                if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)){
                    // 特殊处理 .exe
                    http_request.cgi = true;
                }
                size = st.st_size;
                http_request.size = size;
            }else{
                LOG(WARNING, http_request.path += " NOT FOUND");
                status_code = NOT_FOUNT;
                goto END;
            }
            // 查找后缀
            pos = http_request.path.rfind(".");
            if (pos == string::npos){
                http_response.suffix = ".html";
            }else{
                http_response.suffix = http_request.path.substr(pos);
            }
            
            // cout << "DDDD" << http_request.uri << endl;
            // cout << "DDDD" << http_request.path << endl;
            // cout << "DDDD" << http_request.query_string << endl;
            if (http_request.cgi){
                status_code = ProcessCgi();
            }else{
                // cout << "------------------------ppppp-----------" << endl;
                status_code = ProcessNotCgi();
            }
END:
            // 不是OK就处理
            BuildHttpResponseHelper(); 
            // LOG(WARNING, std::to_string(http_response.response_body.size()).c_str());
            // OK直接返回
            return;
        }

        // 发送响应
        void SendHttpResponse(){
            send(sock, http_response.response_line.c_str(), http_response.response_line.size(), 0);
            for (auto& iter : http_response.response_header){
                // cout << "??????????????" << iter.c_str() << endl;
                send(sock, iter.c_str(), iter.size(), 0);
            }

            send(sock, http_response.blank_line.c_str(), http_response.blank_line.size(), 0);

            if (http_request.cgi){         
                auto& body = http_response.response_body;
                ssize_t pos = 0, size = 0;
                const char *start = body.c_str();
                
                while (pos < body.size() && (size = send(sock, start + pos, body.size() - pos, 0)) > 0){
                    pos += size;
                }
                // auto &response_body = http_response.response_body;
                // size_t total = 0;
                // size_t size = 0;
                // const char *start = response_body.c_str();
                // while( total < response_body.size() && (size = send(sock, start + total, response_body.size() - total, 0)) > 0)
                // {
                //     total += size;
                // }  
            }else{
                sendfile(sock, http_response.fd, nullptr, http_request.size);
                close(http_response.fd);
            }


        }

        ~EndPoint()
        {
            close(sock);
        }
};

// #define DEBUG 1

// 入口类
// class Entrance{
class CallBack{ // 回调函数--仿函数
    public:
        CallBack(){}

        void operator()(int sock){
            Handler(sock);
        }

        void Handler(int sock) // parameter
        {
            LOG(INFO, "########## get handler, request begin ... ##########");
            // int sock = *(int*)parm;
            // delete (int*)parm;

#ifdef DEBUG
            char buffer[1024];
            recv(sock, buffer, sizeof(buffer), 0);

            std::cout << "---------------new msg ----------------" << std::endl;
            std::cout << buffer << std::endl;
#else
            EndPoint *ep = new EndPoint(sock);  
            ep->RecvHttpRequest();
            if (!ep->IsStop()){
                LOG(INFO, "RECV NON ERROR, BUILD RESPONSE");
                ep->BuildHttpResponse();
                ep->SendHttpResponse();
            }else{
                LOG(WARNING, "RECV ERROR, STOP");
            }

            delete(ep);

#endif
            LOG(INFO, "########## handler request end ... ##########");
        }
        
        ~CallBack(){}

};
