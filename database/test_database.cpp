#include <stdio.h>
#include <sqlite3.h>

int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open("mqtt_db", &db);
    if (rc) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    const char *sql = "SELECT ClientID, Username FROM DeviceInfo;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法准备 SQL 语句: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return rc;
    }

    printf("DeviceInfo 数据表内容:\n");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *clientID = sqlite3_column_text(stmt, 0);
        const unsigned char *username = sqlite3_column_text(stmt, 1);
        printf("ClientID: %s, Username: %s\n", clientID, username);
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "查询执行失败: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}
