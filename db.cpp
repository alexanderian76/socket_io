#include "db.h"

void DbContext::addRow(int userId, int score)
{
    std::cout << "ADD ROW" << std::endl;
    char *SQL = new char;
    sprintf(SQL, "INSERT INTO USER_SCORE VALUES (%d,%d)", userId, score);
    char *err;
    std::cout << SQL << std::endl;
    if (sqlite3_exec(this->db, SQL, 0, 0, &err))
    {
        std::cout << "ERROR WHILE ADDING ROW: " << err << std::endl;
        sqlite3_free(err);
    }
    delete SQL;
}
