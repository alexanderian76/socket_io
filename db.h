#include <sqlite3.h>
#include <iostream>

class DbContext
{
public:
    DbContext()
    {
        const char *SQL = "CREATE TABLE IF NOT EXISTS USER_SCORE(\"ID\" INTEGER PRIMARY KEY ASC,\"SCORE\" INTEGER);";
        db = 0; // хэндл объекта соединение к БД
        char *err = 0;

        // открываем соединение
        if (sqlite3_open("io_db.db", &db))
            fprintf(stderr, "Ошибка открытия/создания БД: %s\n", sqlite3_errmsg(db));
        else if (sqlite3_exec(db, SQL, 0, 0, &err))
        {
            fprintf(stderr, "Ошибка SQL: %sn", err);
            sqlite3_free(err);
        }
        // закрываем соединение
       // sqlite3_close(db);
    }

    void addRow(int, int);
    sqlite3 *db;
};