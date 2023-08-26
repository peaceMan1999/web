#include <iostream>
#include <cstdlib>
#include <unistd.h>

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
        // cerr << "DEBUG : Content_Length = " << getenv("CONTENT_LENGTH") << endl;
        Content_Length = atoi(getenv("CONTENT_LENGTH"));
        char ch = 'X';
        while (Content_Length--)
        {
            read(0, &ch, 1);
            query_string.push_back(ch);
        }
        // cerr << "POST DEBUG : " << query_string.c_str() << endl;
    }
}

void CultString(string &line, string &str1, string &str2, string seq)
{
    size_t pos = line.find(seq);
    if (pos != string::npos)
    {
        str1 = line.substr(0, pos);
        str2 = line.substr(pos + seq.size());
    }
}

int main()
{
    string query_string;

    // 对正文处理
    QueryHandler(query_string);

    string str1, str2;
    CultString(query_string, str1, str2, "&");

    string key1, key2, value1, value2;
    CultString(str1, key1, value1, "=");
    CultString(str2, key2, value2, "=");

    // cout << "key1 : " << key1 << " = " << value1 << endl;
    // cout << "key2 : " << key2 << " = " << value2 << endl;

    int x = atoi(value1.c_str());
    int y = atoi(value2.c_str());

    // cerr << "DEBUG : " << key1 << " = " << value1 << endl;
    // cerr << "DEBUG : " << key2 << " = " << value2 << endl;

    // 输出页面
    // cout << "<html>" << endl;
    // cout << "<head><meta charset =\"utf-8 \"></head>" << endl;
    // cout << "<body>" << endl;
    // cout << "<h1>--- 结果: ---</h1>" << endl;
    // cout << "<h3>" << value1 << " + " << value2 << "="  << x+y << "</h3>" << endl;
    // cout << "<h3>" << value1 << " - " << value2 << "="  << x-y << "</h3>" << endl;
    // cout << "<h3>" << value1 << " * " << value2 << "="  << x*y << "</h3>" << endl;
    // cout << "<h3>" << value1 << " / " << value2 << "="  << x/y << "</h3>" << endl;
    // cout << "</body>" << endl;
    // cout << "</html>" << endl;

    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
          <meta charset="UTF-8">
          <title>运行1</title>
        <style>
            body {
            font-family: Arial, sans-serif;
            background-color: #000033; /* 设置背景颜色为藏蓝色 */
            margin: 0;
            padding: 0;
            }

            .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #D3D3D3; /* 设置内容框背景颜色为灰色 */
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            }

            h1 {
            color: #D3D3D3;
            text-align: center;
            }

            p {
            color: #666;
            line-height: 1.5;
            }

            .btn {
            display: inline-block;
            padding: 10px 20px;
            background-color: #007bff;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            transition: background-color 0.3s ease;
            }

            .btn:hover {
            background-color: #0056b3;
            }
        </style>
        </head>
        <body>
            <h1>--- 结果: ---</h1>

          <div class="container">
            <h3>)" + value1 +
                       " + " + value2 + " = " + std::to_string(x + y) + R"(</h3>
            <h3>)" + value1 +
                       " - " + value2 + " = " + std::to_string(x - y) + R"(</h3>
            <h3>)" + value1 +
                       " * " + value2 + " = " + std::to_string(x * y) + R"(</h3>
            <h3>)" + value1 +
                       " / " + value2 + " = " + std::to_string(x / y) + R"(</h3>
          </div>
        </body>
        </html>
    )";

    std::cout << html << std::endl;
    return 0;
}
