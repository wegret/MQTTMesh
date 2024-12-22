#ifndef DATABASE_T_H
#define DATABASE_T_H

#include <sqlite3.h>
#include <string>

class database_t
{
private:
    sqlite3 *db;
    std::string db_path;

public:
    database_t(const std::string &path = "mqtt_db") : db(nullptr), db_path(path)
    {
        int rc = sqlite3_open(db_path.c_str(), &db);
        if (rc)
        {
            fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
            db = nullptr;
        }
        else
            fprintf(stderr, "数据库连接已建立。\n");
    }

    ~database_t()
    {
        if (db)
            sqlite3_close(db);
        fprintf(stderr, "数据库连接已关闭。\n");
    }

    void insert_client(const char *ClientID, const char *Username, const char *IPAddress)
    {
        if (!db)
            return;

        const char *sql = "INSERT INTO DeviceInfo (ClientID, Username, IPAddress, ConnectionTime, SessionActive) VALUES (?, ?, ?, datetime('now', 'localtime'), 1);";

        sqlite3_stmt *stmt;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "无法准备 SQL 语句: %s\n", sqlite3_errmsg(db));
            return;
        }

        sqlite3_bind_text(stmt, 1, ClientID, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, Username, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, IPAddress, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
            fprintf(stderr, "插入数据失败: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "成功插入客户端信息。\n");
        sqlite3_finalize(stmt);
    }

    void update_client_status(const char *ClientID, bool is_active)
    {
        if (!db)
            return;

        const char *sql = "UPDATE DeviceInfo SET SessionActive = ? WHERE ClientID = ?;";
        sqlite3_stmt *stmt;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "无法准备 SQL 语句: %s\n", sqlite3_errmsg(db));
            return;
        }

        sqlite3_bind_int(stmt, 1, is_active ? 1 : 0);
        sqlite3_bind_text(stmt, 2, ClientID, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
            fprintf(stderr, "更新数据失败: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "成功更新客户端状态。\n");

        sqlite3_finalize(stmt);
    }

    // void update_sensor(const char *ClientID, const char *SensorType, double Value)
    // {
    //     if (!db)
    //         return;

    //     // 首先检查是否存在该传感器的数据
    //     const char *select_sql = "SELECT DataID FROM SensorData WHERE ClientID = ? AND SensorType = ?;";
    //     sqlite3_stmt *stmt_select;

    //     int rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt_select, NULL);
    //     if (rc != SQLITE_OK)
    //     {
    //         fprintf(stderr, "无法准备查询语句: %s\n", sqlite3_errmsg(db));
    //         return;
    //     }

    //     sqlite3_bind_text(stmt_select, 1, ClientID, -1, SQLITE_STATIC);
    //     sqlite3_bind_text(stmt_select, 2, SensorType, -1, SQLITE_STATIC);

    //     rc = sqlite3_step(stmt_select);

    //     if (rc == SQLITE_ROW)
    //     {
    //         // 存在，执行更新操作
    //         int data_id = sqlite3_column_int(stmt_select, 0);
    //         sqlite3_finalize(stmt_select);

    //         const char *update_sql = "UPDATE SensorData SET Value = ?, UpdateTime = datetime('now') WHERE DataID = ?;";
    //         sqlite3_stmt *stmt_update;

    //         rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt_update, NULL);
    //         if (rc != SQLITE_OK)
    //         {
    //             fprintf(stderr, "无法准备更新语句: %s\n", sqlite3_errmsg(db));
    //             return;
    //         }

    //         sqlite3_bind_double(stmt_update, 1, Value);
    //         sqlite3_bind_int(stmt_update, 2, data_id);

    //         rc = sqlite3_step(stmt_update);
    //         if (rc != SQLITE_DONE)
    //             fprintf(stderr, "更新传感器数据失败: %s\n", sqlite3_errmsg(db));
    //         else
    //             fprintf(stderr, "成功更新传感器数据。\n");

    //         sqlite3_finalize(stmt_update);
    //     }
    //     else if (rc == SQLITE_DONE)
    //     {
    //         // 不存在，执行插入操作
    //         sqlite3_finalize(stmt_select);

    //         const char *insert_sql = "INSERT INTO SensorData (ClientID, SensorType, Value, UpdateTime) VALUES (?, ?, ?, datetime('now'));";
    //         sqlite3_stmt *stmt_insert;

    //         rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt_insert, NULL);
    //         if (rc != SQLITE_OK)
    //         {
    //             fprintf(stderr, "无法准备插入语句: %s\n", sqlite3_errmsg(db));
    //             return;
    //         }

    //         sqlite3_bind_text(stmt_insert, 1, ClientID, -1, SQLITE_STATIC);
    //         sqlite3_bind_text(stmt_insert, 2, SensorType, -1, SQLITE_STATIC);
    //         sqlite3_bind_double(stmt_insert, 3, Value);

    //         rc = sqlite3_step(stmt_insert);
    //         if (rc != SQLITE_DONE)
    //             fprintf(stderr, "插入传感器数据失败: %s\n", sqlite3_errmsg(db));
    //         else
    //             fprintf(stderr, "成功插入传感器数据。\n");

    //         sqlite3_finalize(stmt_insert);
    //     }
    //     else
    //     {
    //         fprintf(stderr, "查询传感器数据失败: %s\n", sqlite3_errmsg(db));
    //         sqlite3_finalize(stmt_select);
    //     }
    // }
    void update_sensor(const char *ClientID, const char *SensorType, double Value)
    {
        if (!db)
            return;

        // 每次直接插入新数据
        const char *insert_sql =
            "INSERT INTO SensorData (ClientID, SensorType, Value, UpdateTime) "
            "VALUES (?, ?, ?, datetime('now','localtime'));";
        sqlite3_stmt *stmt_insert;

        int rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt_insert, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "无法准备插入语句: %s\n", sqlite3_errmsg(db));
            return;
        }

        sqlite3_bind_text(stmt_insert, 1, ClientID, -1, SQLITE_STATIC);   // 绑定ClientID
        sqlite3_bind_text(stmt_insert, 2, SensorType, -1, SQLITE_STATIC); // 绑定SensorType
        sqlite3_bind_double(stmt_insert, 3, Value);                       // 绑定传感器值

        rc = sqlite3_step(stmt_insert);
        if (rc != SQLITE_DONE)
            fprintf(stderr, "插入传感器数据失败: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "成功插入传感器数据。\n");

        sqlite3_finalize(stmt_insert);

        // 清理多余的数据，保持每种传感器最多100条记录
        const char *cleanup_sql =
            "DELETE FROM SensorData "
            "WHERE DataID IN ("
            "   SELECT DataID FROM SensorData "
            "   WHERE ClientID = ? AND SensorType = ? "
            "   ORDER BY UpdateTime DESC "
            "   LIMIT -1 OFFSET 100"
            ");";

        sqlite3_stmt *stmt_cleanup;

        rc = sqlite3_prepare_v2(db, cleanup_sql, -1, &stmt_cleanup, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "无法准备清理数据语句: %s\n", sqlite3_errmsg(db));
            return;
        }

        sqlite3_bind_text(stmt_cleanup, 1, ClientID, -1, SQLITE_STATIC);   // 绑定ClientID
        sqlite3_bind_text(stmt_cleanup, 2, SensorType, -1, SQLITE_STATIC); // 绑定SensorType

        rc = sqlite3_step(stmt_cleanup);
        if (rc != SQLITE_DONE)
            fprintf(stderr, "清理数据失败: %s\n", sqlite3_errmsg(db));
        else
            fprintf(stderr, "成功清理多余传感器数据。\n");

        sqlite3_finalize(stmt_cleanup);
    }
};

#endif // DATABASE_T_H
