#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
 
int main() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;
 
    // 创建数据库
    rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
 
    // 创建表
    const char *create_table_sql = "CREATE TABLE IF NOT EXISTS user (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);";
    rc = sqlite3_exec(db, create_table_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
 
    // 插入数据
    const char *insert_sql = "INSERT INTO user (name, age) VALUES ('Alice', 30);";
    rc = sqlite3_exec(db, insert_sql, NULL, NULL, &err_msg);
 
    // 删除数据
    const char *delete_sql = "DELETE FROM user WHERE id = 1;";
    rc = sqlite3_exec(db, delete_sql, NULL, NULL, &err_msg);
 
    // 更新数据
    const char *update_sql = "UPDATE user SET age = 35 WHERE id = 2;";
    rc = sqlite3_exec(db, update_sql, NULL, NULL, &err_msg);
 
    // 关闭数据库
    sqlite3_close(db);
    return 0;
}
