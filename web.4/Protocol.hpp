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

enum CODE{
    OK = 200,
    NOT_FOUNT = 404
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

        // 解析的正文
        string method;
        string uri;
        string version;
        string path;
        string query_string;

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
        int size; // 资源大小
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
        
    private:
        void RecvHttpRequestLine(){
            auto& line = http_request.requset_line;
            Util::ReadLine(sock, line);
            line.resize(line.size()-1); // 仅仅是为了输出美观，去掉'\n'
            LOG(INFO, http_request.requset_line);
        }
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
        // 读取报头
        void RecvHttpRequestHeader(){
            string line;
            while (true){
                line.clear();
                Util::ReadLine(sock, line);
                if (line == "\n"){
                    http_request.blank_line = '\n';
                    break;
                }
                line.resize(line.size()-1);
                http_request.request_header.push_back(line);
                LOG(INFO, line);
            }
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
        void RecvHttpRequestBody(){
            if (IsNeedRecvHttpRequestBody()){
                auto Content_Length = http_request.Content_Length;
                auto& request_body = http_request.request_body;
                char ch = 'X';
                while (Content_Length--){
                    ssize_t s = recv(sock, &ch, 1, 0);
                    if (s > 0){
                        request_body.push_back(ch);
                    }
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
        int ProcessNotCgi(int size){
            http_response.fd = open(http_request.path.c_str(), O_RDONLY);
            // cout << "path:" << http_request.path.c_str() << endl;
            if (http_response.fd >= 0){
                auto& line = http_response.response_line;
                line += HTTP_VERSION;
                line += " ";
                line += std::to_string(http_response.status_code);
                line += " ";
                line += CodeToMsg(http_response.status_code);
                line += LINE_END;
                http_response.size = size;

                string header_line = "Content-Type: ";
                // cout << "!!!!!!!!" << header_line .c_str() << endl;
                header_line += SuffixToDesc(http_response.suffix);
                header_line += LINE_END;
                http_response.response_header.push_back(header_line);
                // cout << "!!!!!!!!" << header_line .c_str() << endl;

                header_line = "Content-Length: ";
                header_line += std::to_string(size);
                header_line += LINE_END;
                http_response.response_header.push_back(header_line);
                // cout << "!!!!!!!!" << header_line .c_str() << endl;

                return OK;
            }
            return 404;
        }


        int ProcessCgi(){
            LOG(INFO, "cgi begin !!!");

            auto& bin = http_request.path;
            // 为了子进程可以用，必须在父进程定义
            int fread_cwrite[2];
            int cread_fwrite[2];
            // 建立匿名管道，两个
            if (pipe(cread_fwrite) < 0){
                LOG(ERROR, "input pipe error");
            }
            if (pipe(fread_cwrite) < 0){
                LOG(ERROR, "output pipe error");
            }
            pid_t pid = fork();
            if (pid == 0){
                // child
                // 子写
                close(fread_cwrite[0]);
                close(cread_fwrite[1]);

                // 重定向
                // 写 1 重定向为 fread_cwrite[1];
                // 读 0 重定向为 cread_fwrite[0];
                dup2(cread_fwrite[0], 0);
                dup2(fread_cwrite[1], 1);

                // 程序替换执行文件
                execl(bin.c_str(), bin.c_str(), nullptr);

                exit(1);
            }else if (pid < 0){
                LOG(ERROR, "fork error");
                return NOT_FOUNT;
            }else{
                // father
                // 父读
                close(fread_cwrite[1]);
                close(cread_fwrite[0]);

                waitpid(pid, nullptr, 0);
                close(fread_cwrite[0]);
                close(cread_fwrite[1]);
            }
            return OK;
        }

    // -----------------共公有----------------

    public:
        EndPoint(int _sock)
            :sock(_sock)
        {}
        // 读取
        void RecvHttpRequest(){
            RecvHttpRequestLine();
            RecvHttpRequestHeader();
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
            RecvHttpRequestBody();
        }

        // 构建响应
        void BuildHttpResponse(){
            auto& status_code = http_response.status_code;
            struct stat st;
            int size, pos;
            // cout << "--------" << http_request.method << endl;
            if (http_request.method != "POST" && http_request.method != "GET"){
                LOG(FATAL, "method warning");
                status_code = NOT_FOUNT;
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
            }else{
                LOG(WARNING, http_request.path += " NOT FOUND");
                status_code = NOT_FOUNT;
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
                status_code = ProcessNotCgi(size);
            }
END:
            // 不是OK就处理
            if (status_code != OK){

            }
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

            sendfile(sock, http_response.fd, nullptr, http_response.size);
            close(http_response.fd);
        }

        ~EndPoint()
        {
            close(sock);
        }
};

// #define DEBUG 1

// 入口类
class Entrance{
    public:
        static void* Handler(void *parm) // parameter
        {
            LOG(INFO, "########## get handler, request begin ... ##########");
            int sock = *(int*)parm;
            delete (int*)parm;

#ifdef DEBUG
            char buffer[1024];
            recv(sock, buffer, sizeof(buffer), 0);

            std::cout << "---------------new msg ----------------" << std::endl;
            std::cout << buffer << std::endl;
#else
            EndPoint *ep = new EndPoint(sock);  
            ep->RecvHttpRequest();
            ep->BuildHttpResponse();
            ep->SendHttpResponse();
            delete(ep);

#endif
            LOG(INFO, "########## handler request end ... ##########");
            return nullptr;
        }
};
