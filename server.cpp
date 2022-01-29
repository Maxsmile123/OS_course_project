#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>

using json = nlohmann::json;
using namespace std;


// ws - обхект/параметр вебсокета

const char* SQL_CREATE_USERS_TABLE = "CREATE TABLE IF NOT EXISTS USERS("  \
"USER_ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
"NAME TEXT NOT NULL UNIQUE," \
"STATUS BIT," \
"LOGIN TEXT NOT NULL UNIQUE,"  \
"PASSWORD TEXT NOT NULL,"  \
"TIME_TO_GO_OFFLINE TEXT);";


const char* SQL_CREATE_MSG_TABLE = "CREATE TABLE IF NOT EXISTS MSG("  \
"TIME TEXT NOT NULL," \
"FROM_USER INTEGER NOT NULL," \
"TO_USER INTEGER NOT NULL," \
"MESSAGE TEXT NOT NULL);";

const string COMMAND = "command";
const string PRIVATE_MSG = "private_msg";
const string CHANGE_NAME = "change_name";
const string USER_ID = "user_id";
const string TIME = "time";
const string USER_FROM = "user_from";
const string NAME_FROM = "name_from";
const string MESSAGE = "message";
const string NAME = "name";
const string ONLINE = "online";
const string STATUS = "status";
const string BROADCAST = "broadcast";
const string REGISTRATION = "registration";
const string AUTHORIZATION = "authorization";
const string LOAD_MSG = "load_msg";
const string RESULT = "result";
const string LOAD_USERS = "load_users";
const string GET_ID = "get_id";
const string FROM_ID = "from_id";
const string TO_ID = "to_id";

SQLite::Database    db("database.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);




struct PerSocketData {
    int user_id;
    string name;
};

map<int, PerSocketData*> activeUsers;

typedef uWS::WebSocket < false, true, PerSocketData> UWEBSOCK;


string status(PerSocketData* data, bool online)
{
    json request;
    request[COMMAND] = STATUS;
    request[NAME] = data->name;
    request[ONLINE] = online;
    return request.dump();
}


int get_id(string name) {
    SQLite::Statement   query(db, "SELECT USER_ID FROM USERS WHERE NAME = ?");
    query.bind(1, name);
    while (query.executeStep())
    {
        int id = query.getColumn(0);
        return id;
    }
    return -2;
}

string get_name(string id) {
    SQLite::Statement   query(db, "SELECT NAME FROM USERS WHERE USER_ID = ?");
    query.bind(1, id);
    while (query.executeStep())
    {
        string name = query.getColumn(0);
        return name;
    }
    return "None";
}



void processMessage(UWEBSOCK* ws, std::string_view message)
{
    auto parsed = json::parse(message);
    string command = parsed[COMMAND];
    string sender = to_string(parsed[FROM_ID]);
    json response;

    if (command == PRIVATE_MSG)
    {
        try {
            string user_msg = parsed[MESSAGE];
            string user_id = to_string(parsed[TO_ID]);
            response[COMMAND] = PRIVATE_MSG;
            response[NAME_FROM] = get_name(user_id);
            response[MESSAGE] = user_msg;
            response[TIME] = parsed[TIME];
            if (user_id == "-1") {
                ws->publish(BROADCAST, response.dump());
            }
            ws->publish("UserN" + user_id, response.dump());
            string SQL_RESPONSE = "INSERT INTO MSG(TIME, FROM_USER, TO_USER, MESSAGE) VALUES(" + to_string(response[TIME]) + ", " + sender + ", " +
                user_id + ", " + user_msg + ");";
            db.exec(SQL_RESPONSE);
        }
        catch (std::exception& e) {
            cout << "SQLite exception: " << e.what() << std::endl;
            cout << "[-] Error to send msg!\n";
        }


    }
    if (command == CHANGE_NAME)
    {
        try {
            string new_name = to_string(parsed["new_name"]);
            string old_name = to_string(parsed["old_name"]);
            string SQL_RESPONSE = "UPDATE USERS SET NAME = " + new_name + " WHERE NAME = " + old_name + ";";
            db.exec(SQL_RESPONSE);
            response[RESULT] = "true";
            response[COMMAND] = CHANGE_NAME;
            ws->publish("UserN" + sender, response.dump());
        }
        catch (std::exception& e) {
            cout << "SQLite exception: " << e.what() << std::endl;
            cout << "[-] Error to change name!\n";
            response[RESULT] = "false";
            response[COMMAND] = CHANGE_NAME;
            ws->publish("UserN" + sender, response.dump());
        }
    }
    if (command == REGISTRATION)
    {
        bool IS_EXIST = false;
        string login = parsed["login"];
        SQLite::Statement   query(db, "SELECT USER_ID FROM USERS WHERE LOGIN = ?");
        query.bind(1, login);
        while (query.executeStep())
        {
            cout << query.getColumn(0) << endl;
            IS_EXIST = true;
            break;

        }
        if (!IS_EXIST) {
            string SQL_RESPONSE = "INSERT INTO USERS(NAME, STATUS, LOGIN, PASSWORD, TIME_TO_GO_OFFLINE) VALUES (" + to_string(parsed[NAME]) + ","
                + "0 ," + to_string(parsed["login"]) + ", " + to_string(parsed["password"]) + ", " + "NONE);";
            db.exec(SQL_RESPONSE);
            response[COMMAND] = REGISTRATION;
            response[RESULT] = "true";
            response["reason"] = "none";
            ws->publish("UserN" + sender, response.dump());
        }
        else {
            response[COMMAND] = REGISTRATION;
            response[RESULT] = "false";
            response["reason"] = "This login is already exist!";
            ws->publish("UserN" + sender, response.dump());
        }

    }
    if (command == LOAD_MSG)
    {
        SQLite::Statement   query(db, "SELECT * FROM MSG WHERE FROM_USER = ? OR TO_USER = ?;");
        query.bind(1, to_string(parsed[FROM_ID]));
        query.bind(2, to_string(parsed[FROM_ID]));
        while (query.executeStep())
        {
            response[COMMAND] = LOAD_MSG;
            response[TIME] = query.getColumn(0);
            response[FROM_ID] = query.getColumn(1);
            response[TO_ID] = query.getColumn(2);
            response[MESSAGE] = query.getColumn(3);
            ws->publish("UserN" + sender, response.dump());
        }
    }
    if (command == LOAD_USERS)
    {
        string SQL_RESPONSE = "SELECT NAME, STATUS FROM USERS;";
        SQLite::Statement   query(db, SQL_RESPONSE);
        while (query.executeStep())
        {
            response[COMMAND] = LOAD_USERS;
            response[NAME] = query.getColumn(0);
            response[STATUS] = query.getColumn(1);
            ws->publish("UserN" + sender, response.dump());
        }
    }
    if (command == AUTHORIZATION)
    {
        bool IS_EXIST = false;
        SQLite::Statement   query(db, "SELECT NAME, USER_ID FROM USERS WHERE LOGIN = ? AND PASSWORD = ?;");
        query.bind(1, to_string(parsed["login"]));
        query.bind(2, to_string(parsed["password"]));
        while (query.executeStep())
        {
            IS_EXIST = true;
            response[NAME] = query.getColumn(0);
            response[USER_ID] = query.getColumn(1);
            break;
        }
        response[AUTHORIZATION] = AUTHORIZATION;
        if (IS_EXIST) {
            response[RESULT] = "true";
        } else {
            response[RESULT] = "false";
        }
        ws->publish("UserN" + sender, response.dump());
    }
    if (command == GET_ID)
    {
        response[COMMAND] = GET_ID;
        response["id"] = get_id(parsed[NAME]);
        ws->publish("UserN" + sender, response.dump());
    }
}


int main() {
    cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";
    try {
        db.exec(SQL_CREATE_USERS_TABLE);
        db.exec(SQL_CREATE_MSG_TABLE);
    }
    catch (std::exception& e)
    {
        cout << "SQLite exception: " << e.what() << std::endl;
        return EXIT_FAILURE; // unexpected error : exit the example program
    }

    uWS::App().ws<PerSocketData>("/*", {
        .idleTimeout = 9999, /*
        .maxBackpressure = 1 * 1024 * 1024,
        .closeOnBackpressureLimit = false,
        .resetIdleTimeoutOnSend = false,
        .sendPingsAutomatically = true,
        */
        /* Handlers */
        .open = [](auto* ws) {

        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            // ws->send(message, opCode, true);
            PerSocketData* data = ws->getUserData();

            processMessage(ws, message);
        },

        .close = [](auto* ws, int /*code*/, std::string_view /*message*/) {
            /* You may access ws->getUserData() here */
            PerSocketData* data = ws->getUserData();

            cout << "close" << endl;
            ws->publish(BROADCAST, status(data, false));

            activeUsers.erase(data->user_id); // удалили из карты

        }
        }).listen(9001, [](auto* listen_socket) { //9001 - порт
            if (listen_socket) {
                std::cout << "Listening on port " << 9001 << std::endl;
            }
            }).run();
}