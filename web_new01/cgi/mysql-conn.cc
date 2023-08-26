#include <iostream>
#include "mysql.h"

int main()
{
  std::cout << "version: " << mysql_get_client_info() << std::endl;
  // MYSQL* conn = mysql_init(nullptr);
  // mysql_set_character_set(conn, "utf8");
  // if(nullptr == mysql_real_connect(conn, "127.0.0.1" , "root", "root", "test", 3306, nullptr, 0)){
  //   std::cerr << "connect err" << std::endl;
  //   return 1;
  // }

  // std::cerr << "connect success" << std::endl;
  // mysql_query(conn, "set names utf8");
  // std::string sql = "insert into stu values(224, 'xixi')";
  // int ret = mysql_query(conn, sql.c_str());

  // std::cerr << "result: " << ret << std::endl;

  // mysql_close(conn);
  return 0;
}
