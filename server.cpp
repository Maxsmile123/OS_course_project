#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include "windows.h"

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
const string CHECK = "check";
const string PING = "ping";
const string FOR_ID = "for_id";
const string UPDATE = "update";

SQLite::Database    db("database.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

queue <string> sessions_name;
queue <string> sessions_id;




struct PerSocketData {
    string user_id = "NULL";
};


typedef uWS::WebSocket < false, true, PerSocketData> UWEBSOCK;

uWS::OpCode OpCode;



string get_id(string name) {
    try {
        SQLite::Statement   query(db, "SELECT USER_ID FROM USERS WHERE NAME = " + name);
        while (query.executeStep())
        {
            string id = query.getColumn(0);
            return id;
        }
        return "NULL";
    } catch (std::exception& e) {
        cout << "SQLite exception: " << e.what() << std::endl;
        cout << "[-] Error to get_id!\n";
    }
}

string get_name(string id) {
    SQLite::Statement   query(db, "SELECT NAME FROM USERS WHERE USER_ID = " + id);
    while (query.executeStep())
    {
        string name = query.getColumn(0);
        return name;
    }
    return "NULL";
}



void processMessage(UWEBSOCK* ws, std::string_view message)
{
    auto parsed = json::parse(message);
    string command = parsed[COMMAND];
    string sender = to_string(parsed[FROM_ID]);
    json response;
    string SQL_RESPONSE;
    // Send message
    if (command == PRIVATE_MSG)
    {
        try {
            string user_msg = to_string(parsed[MESSAGE]);
            string user_id = to_string(parsed[TO_ID]);
            response[COMMAND] = PRIVATE_MSG;
            response[NAME_FROM] = get_name(to_string(parsed[FROM_ID]));
            response[MESSAGE] = user_msg;
            response[TIME] = parsed[TIME];
            if (user_id == "-1") {
                ws->publish(BROADCAST, response.dump());
            }
            cout << response << endl;
            ws->publish("UserN" + user_id, response.dump());
            SQL_RESPONSE = "INSERT INTO MSG(TIME, FROM_USER, TO_USER, MESSAGE) VALUES(" + to_string(response[TIME]) + ", " + sender + ", " +
                user_id + ", " + user_msg + ");";
            db.exec(SQL_RESPONSE);
        }
        catch (std::exception& e) {
            cout << "SQLite exception: " << e.what() << std::endl;
            cout << "[-] Error to send msg!\n";
        }


    }
    // Check auth status
    if (command == CHECK) {
        PerSocketData* data = ws->getUserData();
        if (sessions_name.empty() || sessions_id.empty()) {
            response[NAME] = "NULL";
            response[USER_ID] = "NULL";
        } else {
            response[NAME] = sessions_name.front();
            response[USER_ID] = sessions_id.front();
            sessions_name.pop();
            sessions_id.pop();
        }
        data->user_id = to_string(response[USER_ID]);
        ws->subscribe("UserN" + data->user_id);
        ws->subscribe(BROADCAST);
        response[COMMAND] = CHECK;
        cout << response << endl;
        ws->send(response.dump(), OpCode);
    }
    if (command == CHANGE_NAME)
    {
        try {
            string new_name = to_string(parsed["new_name"]);
            string old_name = to_string(parsed["old_name"]);
            SQL_RESPONSE = "UPDATE USERS SET NAME = " + new_name + " WHERE NAME = " + old_name + ";";
            db.exec(SQL_RESPONSE);
            response["new_name"] = new_name;
            response[RESULT] = "true";
            response[COMMAND] = CHANGE_NAME;
            cout << response << endl;
            ws->send(response.dump(), OpCode);
            response[COMMAND] = UPDATE;
            ws->publish(BROADCAST, response.dump());
        }
        catch (std::exception& e) {
            cout << "SQLite exception: " << e.what() << std::endl;
            cout << "[-] Error to change name!\n";
            response[RESULT] = "false";
            response[COMMAND] = CHANGE_NAME;
            cout << response << endl;
            ws->send(response.dump(), OpCode);
        }
    }
    if (command == REGISTRATION)
    {
        SQL_RESPONSE = "SELECT USER_ID FROM USERS WHERE LOGIN = ?";
        bool IS_EXIST = false;
        string login = parsed["login"];
        SQLite::Statement   query(db, SQL_RESPONSE);
        query.bind(1, login);
        while (query.executeStep())
        {
            IS_EXIST = true;
            break;

        }

        if (!IS_EXIST) {
            try {
                SQL_RESPONSE = "INSERT INTO USERS(NAME, STATUS, LOGIN, PASSWORD, TIME_TO_GO_OFFLINE) VALUES (" + to_string(parsed[NAME]) + ","
                    + "0 ," + to_string(parsed["login"]) + ", " + to_string(parsed["password"]) + ", " + "123);";
                db.exec(SQL_RESPONSE);
            }
            catch (std::exception& e) {
                cout << "SQLite exception: " << e.what() << std::endl;
                return;
            }
            response[COMMAND] = REGISTRATION;
            response[RESULT] = "true";
            response["reason"] = "none";
            cout << response << endl;
            ws->send(response.dump(), OpCode);
        }
        else {
            response[COMMAND] = REGISTRATION;
            response[RESULT] = "false";
            response["reason"] = "This login is already exist!";
            cout << response << endl;
            ws->send(response.dump(), OpCode);
        }

    }
    // not work
    if (command == LOAD_MSG)
    {
        try {
            SQL_RESPONSE = "SELECT * FROM MSG WHERE (FROM_USER = " + to_string(parsed[FOR_ID]) + " AND TO_USER = " + to_string(parsed[USER_ID]) +
                ") OR (FROM_USER = " + to_string(parsed[USER_ID]) + " AND TO_USER = " + to_string(parsed[FOR_ID]) +
                ")";
            SQLite::Statement   query(db, SQL_RESPONSE);
            while (query.executeStep())
            {
                response[COMMAND] = LOAD_MSG;
                response[TIME] = query.getColumn(0);
                response[FROM_ID] = query.getColumn(1);
                response[TO_ID] = query.getColumn(2);
                response[NAME] = get_name(to_string(response[FROM_ID]));
                response[MESSAGE] = query.getColumn(3);
                cout << response << endl;
                ws->send(response.dump(), OpCode);
            }
        }catch (std::exception& e)
        {
            cout << e.what() << std::endl;
        }
    }
    // work
    if (command == LOAD_USERS)
    {
        SQL_RESPONSE = "SELECT NAME, STATUS FROM USERS WHERE USER_ID != " + to_string(parsed[USER_ID]) + ";";
        SQLite::Statement   query(db, SQL_RESPONSE);
        while (query.executeStep())
        {
            response[COMMAND] = LOAD_USERS;
            response[NAME] = query.getColumn(0);
            response[STATUS] = query.getColumn(1);
            cout << response << endl;
            ws->send(response.dump(), OpCode);
        }
    }
    // work
    if (command == AUTHORIZATION)
    {
        PerSocketData* data = ws->getUserData();
        bool IS_EXIST = false;
        try {
            SQL_RESPONSE = "SELECT NAME, USER_ID FROM USERS WHERE LOGIN = " + to_string(parsed["login"]) + "AND PASSWORD = "
                + to_string(parsed["password"]);

            SQLite::Statement   query(db, SQL_RESPONSE);

            while (query.executeStep())
            {
                IS_EXIST = true;
                response[NAME] = query.getColumn(0);
                response[USER_ID] = query.getColumn(1);
                break;
            }
        }
        catch (std::exception& e)
        {
            cout << e.what() << std::endl;
        }
        response[COMMAND] = AUTHORIZATION;
        json req;
        if (IS_EXIST) {
            response[RESULT] = "true";
            sessions_name.push(response[NAME]);
            sessions_id.push(response[USER_ID]);

            SQL_RESPONSE = "UPDATE USERS SET STATUS = 1 WHERE USER_ID = " + to_string(response[USER_ID]) + ";";
            db.exec(SQL_RESPONSE);
            req[COMMAND] = "update";
            ws->publish(BROADCAST, req.dump());


        } else {
            response[RESULT] = "false";
        }
        cout << response << endl;
        ws->send(response.dump(), OpCode);
    }
    if (command == GET_ID)
    {
        response[COMMAND] = GET_ID;
        response["id"] = get_id(to_string(parsed[NAME]));
        cout << response << endl;
        ws->send(response.dump(), OpCode);
    }
}

/*
void checkStatus(UWEBSOCK* ws) {
    cout << "we are in checkStatus" << endl;
    string SQL_RESPONSE;
    json response;
    response[COMMAND] = PING;
    while (1) {
        try {
            SQL_RESPONSE = "SELECT USER_ID FROM USERS";
            SQLite::Statement   query(db, SQL_RESPONSE);

            while (query.executeStep())
            {
                string user_id = query.getColumn(0);
                users_list_for_status.push_back(user_id);
                cout << response << user_id << endl;
                ws->publish("UserN" + user_id, response.dump());
            }
        } catch (std::exception& e)
        {
            cout << "SQLite exception: " << e.what() << std::endl;
        }
        Sleep(60000);
        while (!users_list_for_status.empty()) {
            try {
                SQL_RESPONSE = "UPDATE USERS SET STATUS = 0 WHERE USER_ID = " + users_list_for_status.back() + ";";
                users_list_for_status.pop_back();
                db.exec(SQL_RESPONSE);
            }
            catch (std::exception& e)
            {
                cout << "SQLite exception: " << e.what() << std::endl;
            }
        }
    }
}
*/


int main() {
    cout << "SQLite database file '" << db.getFilename().c_str() << "' opened successfully\n";
    int last_connect = 0;
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
        .open = [&last_connect](auto* ws) {
            cout << "Succsessful connection" << endl;
        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            OpCode = opCode;
            PerSocketData* data = ws->getUserData();
            cout << message << endl;

            processMessage(ws, message);
        },

        .close = [](auto* ws, int /*code*/, std::string_view message) {
            PerSocketData* data = ws->getUserData();
            if (data->user_id != "NULL") {
                try {
                    string SQL_RESPONSE = "UPDATE USERS SET STATUS = 0 WHERE USER_ID = " + data->user_id + ";";
                    db.exec(SQL_RESPONSE);
                }
                catch (std::exception& e)
                {
                    cout << "SQLite exception: " << e.what() << std::endl;
                }
            }


            cout << "user № " + data->user_id + " is go to sleep" << endl;

        }
    }).listen(9001, [](auto* listen_socket) { //9001 - порт
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}