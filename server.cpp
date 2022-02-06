#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <queue>

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

SQLite::Database    db("database.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

queue <string> sessions_name;
queue <string> sessions_id;




struct PerSocketData {
    int user_id;
    string name;
};

map<int, PerSocketData*> activeUsers;

typedef uWS::WebSocket < false, true, PerSocketData> UWEBSOCK;

uWS::OpCode OpCode;


string status(PerSocketData* data, bool online)
{
    json request;
    request[COMMAND] = STATUS;
    request[NAME] = data->name;
    request[ONLINE] = online;
    return request.dump();
}


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
    if (command == CHECK) {
        if (sessions_name.empty() || sessions_id.empty()) {
            response[NAME] = "null";
            response[USER_ID] = "null";
        } else {
            response[NAME] = sessions_name.front();
            response[USER_ID] = sessions_id.front();
            sessions_name.pop();
            sessions_id.pop();
        }
        response[COMMAND] = CHECK;
        cout << response << endl;
        ws->send(response.dump(), OpCode);
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
            cout << response << endl;
            ws->send(response.dump(), OpCode);
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
        bool IS_EXIST = false;
        string login = parsed["login"];
        SQLite::Statement   query(db, "SELECT USER_ID FROM USERS WHERE LOGIN = ?");
        query.bind(1, login);
        while (query.executeStep())
        {
            IS_EXIST = true;
            break;

        }

        if (!IS_EXIST) {
            try {
                string SQL_RESPONSE = "INSERT INTO USERS(NAME, STATUS, LOGIN, PASSWORD, TIME_TO_GO_OFFLINE) VALUES (" + to_string(parsed[NAME]) + ","
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
            cout << response << endl;
            ws->send(response.dump(), OpCode);
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
            cout << response << endl;
            ws->send(response.dump(), OpCode);
        }
    }
    if (command == AUTHORIZATION)
    {
        bool IS_EXIST = false;
        SQLite::Statement   query(db, "SELECT NAME, USER_ID FROM USERS WHERE LOGIN = " + to_string(parsed["login"]) + "AND PASSWORD = "
         + to_string(parsed["password"]));
        try {
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
        query.reset();
        response[COMMAND] = AUTHORIZATION;
        if (IS_EXIST) {
            response[RESULT] = "true";
            sessions_name.push(response[NAME]);
            sessions_id.push(response[USER_ID]);
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
            /*
            try {
                queue<string> copy_name = sessions_name;
                queue<string> copy_id = sessions_id;
                while (copy_name.empty()) {
                    cout << copy_name.front() << endl;
                    copy_name.pop();
                }
                while (copy_id.empty()) {
                    cout << copy_id.front() << endl;
                    copy_id.pop();
                }
            } catch (std::exception& e) {
                cout << e.what() << std::endl;
                return;
                }
                */
            cout << "Succsessful connection" << endl;
            ws->subscribe(BROADCAST); //сообщения получают все пользователи
            ws->subscribe("UserN"); // личный канал
        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            OpCode = opCode;
            // ws->send(message, opCode, true);
            PerSocketData* data = ws->getUserData();
            cout << message << endl;

            processMessage(ws, message);
        },

        .close = [](auto* ws, int /*code*/, std::string_view /*message*/) {
            /* You may access ws->getUserData() here */
            PerSocketData* data = ws->getUserData();

            cout << "close" << endl;

            activeUsers.erase(data->user_id); // удалили из карты

        }
        }).listen(9001, [](auto* listen_socket) { //9001 - порт
            if (listen_socket) {
                std::cout << "Listening on port " << 9001 << std::endl;
            }
            }).run();
}