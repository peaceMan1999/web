#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

void QueryHandler(string& query_string){
        // 用2，01已经重定向
    string method = getenv("METHOD");
    cerr << "DEBUG : " << method.c_str() << endl;
    int Content_Length;
    if ("GET" == method){
        query_string = getenv("QUERY_STRING");
        cerr << "GET DEBUG : " << query_string.c_str() << endl;
    }else if ("POST" == method){
        cerr << "DEBUG : Content_Length = " << getenv("CONTENT_LENGTH") << endl;
        Content_Length = atoi(getenv("CONTENT_LENGTH"));
        char ch = 'X';
        while (Content_Length--){
            read(0, &ch, 1);
            query_string.push_back(ch);
        }
        cerr << "POST DEBUG : " << query_string.c_str() << endl;
    }
}

void CultString(string& line, string& str1, string& str2, string seq){
        size_t pos = line.find(seq);
        if (pos != string::npos){
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
    cout << "<html>" << endl;
    cout << "<head><meta charset =\"utf-8 \"></head>" << endl;
    cout << "<body>" << endl;
    cout << "<h1>--- 结果: ---</h1>" << endl;
    cout << "<h3>" << value1 << " + " << value2 << "="  << x+y << "</h3>" << endl;
    cout << "<h3>" << value1 << " - " << value2 << "="  << x-y << "</h3>" << endl;
    cout << "<h3>" << value1 << " * " << value2 << "="  << x*y << "</h3>" << endl;
    cout << "<h3>" << value1 << " / " << value2 << "="  << x/y << "</h3>" << endl;
    cout << "</body>" << endl;
    cout << "</html>" << endl;

    return 0;
}
