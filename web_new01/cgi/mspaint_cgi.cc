#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include "mysql.h"

using namespace std;

void QueryHandler(string &query_string)
{
    // 用2，01已经重定向
    string method = getenv("METHOD");
    cerr << "DEBUG : " << method.c_str() << endl;
    int Content_Length;
    if ("GET" == method)
    {
        query_string = getenv("QUERY_STRING");
        cerr << "GET DEBUG : " << query_string.c_str() << endl;
    }
    else if ("POST" == method)
    {
        cerr << "DEBUG : Content_Length = " << getenv("CONTENT_LENGTH") << endl;
        Content_Length = atoi(getenv("CONTENT_LENGTH"));
        char ch = 'X';
        while (Content_Length--)
        {
            read(0, &ch, 1);
            query_string.push_back(ch);
        }
        cerr << "POST DEBUG222 : " << query_string.c_str() << endl;
    }
}

void Insert(string &query_string){
    // std::cerr << "version: " << mysql_get_client_info() << std::endl;

    // 登录
    MYSQL* conn = mysql_init(nullptr);
    if(nullptr == mysql_real_connect(conn, "127.0.0.1" , "root", "root", "web", 3306, nullptr, 0)){
        std::cerr << "connect err" << std::endl;
        return;
    }
    std::cerr << "connect success" << std::endl;

    // // 插入
    // std::string sql = "insert into stu values(1,"+ query_string +")";
    // int ret = mysql_query(conn, sql.c_str());
    // std::cerr << "result: " << ret << std::endl;

    // 先查询主键是否存在
    std::string select_sql = "SELECT * FROM DataURL WHERE id = 1";
    int ret = mysql_query(conn, select_sql.c_str());

    if (ret == 0) {
        MYSQL_RES *result = mysql_store_result(conn);
        int num_rows = mysql_num_rows(result);

        if (num_rows == 0) {
            // 主键不存在，执行插入操作
            std::string sql = "insert into DataURL values(1,'"+ query_string +"')";
            ret = mysql_query(conn, sql.c_str());

            if (ret == 0) {
                // 插入成功
                std::cerr << "Insert success!" << std::endl;
                // 输出提示消息
                std::cout << "<script>alert('保存成功');</script>";
            } else {
                // 插入失败
                std::cerr << "Insert failed!" << std::endl;
            }
        } else {
            // 主键已存在，不执行插入操作
            std::cerr << "Primary key already exists!" << std::endl;
            // 输出提示消息
            std::cout << "<script>alert('已经有图片了');</script>";
        }

        mysql_free_result(result);
    } else {
        // 查询失败
        std::cerr << "Query failed!" << std::endl;
    }

    mysql_close(conn);
}

int main()
{
    string query_string;

    // 对正文处理
    QueryHandler(query_string);
    Insert(query_string);
    std::ifstream file("/home/yz/http/web-server/web/output/wwwroot/mspaint.html");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::cout << content;


    return 0;
}
