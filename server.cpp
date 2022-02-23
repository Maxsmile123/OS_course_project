#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <algorithm>
#include <regex>

using json = nlohmann::json;

const char* SQL_CREATE_USERS_TABLE = "CREATE TABLE IF NOT EXISTS USERS("  \
"USER_ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
"NAME TEXT NOT NULL UNIQUE," \
"STATUS BIT," \
"LOGIN TEXT NOT NULL UNIQUE,"  \
"PASSWORD TEXT NOT NULL);";


const char* SQL_CREATE_MSG_TABLE = "CREATE TABLE IF NOT EXISTS MSG("  \
"TIME TEXT NOT NULL," \
"FROM_USER INTEGER NOT NULL," \
"TO_USER INTEGER NOT NULL," \
"MESSAGE TEXT NOT NULL);";

const std::string COMMAND = "command";
const std::string PRIVATE_MSG = "private_msg";
const std::string CHANGE_NAME = "change_name";
const std::string USER_ID = "user_id";
const std::string TIME = "time";
const std::string USER_FROM = "user_from";
const std::string NAME_FROM = "name_from";
const std::string MESSAGE = "message";
const std::string NAME = "name";
const std::string ONLINE = "online";
const std::string STATUS = "status";
const std::string BROADCAST = "broadcast";
const std::string REGISTRATION = "registration";
const std::string AUTHORIZATION = "authorization";
const std::string LOAD_MSG = "load_msg";
const std::string RESULT = "result";
const std::string LOAD_USERS = "load_users";
const std::string GET_ID = "get_id";
const std::string FROM_ID = "from_id";
const std::string TO_ID = "to_id";
const std::string CHECK = "check";
const std::string PING = "ping";
const std::string FOR_ID = "for_id";
const std::string UPDATE = "update";
const std::string NEW_NAME = "new_name";
const std::string REASON = "reason";
const std::string ID = "id";

SQLite::Database    db("database.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

std::queue <std::string> sessions_name;
std::queue <std::string> sessions_id;

// The data of current socket
struct PerSocketData {
    std::string user_id = "NULL";
};

std::ofstream LOG("Log.txt", std::ios::app);


typedef uWS::WebSocket < false, true, PerSocketData> UWEBSOCK;

uWS::OpCode OpCode;



std::string get_id(std::string name) {
    try {
        SQLite::Statement   query(db, "SELECT USER_ID FROM USERS WHERE NAME = " + name);
        while (query.executeStep())
        {
            std::string id = query.getColumn(0);
            return id;
        }
        return "NULL";
    } catch (std::exception& e) {
        std::cout << "[-] Error to get_id!\n";
        std::cout << "SQLite exception: " << e.what() << std::endl;
        LOG << "[-] Error to get_id!\n";
        LOG << "SQLite exception: " << e.what() << std::endl;
    }
}

std::string get_name(std::string id) {
    try {
        SQLite::Statement   query(db, "SELECT NAME FROM USERS WHERE USER_ID = " + id);
        while (query.executeStep())
        {
            std::string name = query.getColumn(0);
            return name;
        }
        return "NULL";
    } catch (std::exception& e) {
        std::cout << "[-] Error to get_name!\n";
        std::cout << "SQLite exception: " << e.what() << std::endl;
        LOG << "[-] Error to get_name!\n";
        LOG << "SQLite exception: " << e.what() << std::endl;
    }
}



void processMessage(UWEBSOCK* ws, std::string_view message)
{
    auto parsed = json::parse(message);
    std::string command = parsed[COMMAND];
    json response;
    std::string SQL_RESPONSE;
    // Send message
    if (command == PRIVATE_MSG)
    {
        try {
            std::string sender = to_string(parsed[FROM_ID]);
            std::string user_msg = to_string(parsed[MESSAGE]);
            std::string to_id = to_string(parsed[TO_ID]);
            response[COMMAND] = PRIVATE_MSG;
            response[NAME_FROM] = get_name(sender);
            response[MESSAGE] = user_msg;
            response[TIME] = parsed[TIME];
            if (to_id == "\"-1\"") {
                ws->publish(BROADCAST, response.dump());
            } else {
                ws->publish("UserN" + to_id, response.dump());
            }
            std::cout << "[+] Send data: " <<  response << std::endl;
            LOG << "[+] Send data: " << response << std::endl;
            SQL_RESPONSE = "INSERT INTO MSG(TIME, FROM_USER, TO_USER, MESSAGE) VALUES(" + to_string(response[TIME]) + ", " + sender + ", " +
                to_id + ", " + user_msg + ");";
            db.exec(SQL_RESPONSE);
        }
        catch (std::exception& e) {
            std::cout << "[-] Error to send msg!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error to send msg!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
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
        std::cout << "[+] Send data: " << response << std::endl;
        LOG << "[+] Send data: " << response << std::endl;
        ws->send(response.dump(), OpCode);
        LOG << "[+] Succsessful connection user # " << data->user_id << std::endl;
        std::cout << "[+] Succsessful connection user # " << data->user_id << std::endl;
    }
    if (command == CHANGE_NAME)
    {
        try {
            std::string new_name = to_string(parsed["new_name"]);
            std::string old_name = to_string(parsed["old_name"]);
            SQL_RESPONSE = "UPDATE USERS SET NAME = " + new_name + " WHERE NAME = " + old_name + ";";
            db.exec(SQL_RESPONSE);
            response[NEW_NAME] = new_name;
            response[COMMAND] = CHANGE_NAME;
            response[RESULT] = "true";
            std::cout << "[+] Send data: " << response << std::endl;
            LOG << "[+] Send data: " << response << std::endl;
            ws->send(response.dump(), OpCode);
            response[COMMAND] = UPDATE;
            ws->publish(BROADCAST, response.dump());
        }
        catch (std::exception& e) {
            std::cout << "[-] Error to change name!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error to change name!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
            response[RESULT] = "false";
            response[COMMAND] = CHANGE_NAME;
            std::cout << "[+] Send data: " << response << std::endl;
            LOG << "[+] Send data: " << response << std::endl;
            ws->send(response.dump(), OpCode);
        }
    }
    if (command == REGISTRATION)
    {
        bool IS_EXIST = false;
        try {
            SQL_RESPONSE = "SELECT USER_ID FROM USERS WHERE LOGIN = ?";
            std::string login = parsed["login"];
            response[COMMAND] = REGISTRATION;
            SQLite::Statement   query(db, SQL_RESPONSE);
            query.bind(1, login);
            while (query.executeStep())
            {
                IS_EXIST = true;
                break;
            }
        } catch (std::exception& e) {
            std::cout << "[-] Error registration!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error registration!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
        }
        if (!IS_EXIST) {
            try {
                SQL_RESPONSE = "INSERT INTO USERS(NAME, STATUS, LOGIN, PASSWORD) VALUES (" + to_string(parsed[NAME]) + ","
                    + "0 ," + to_string(parsed["login"]) + ", " + to_string(parsed["password"]) + ");";
                db.exec(SQL_RESPONSE);
            }
            catch (std::exception& e) {
                std::cout << "[-] Error registration!\n";
                std::cout << "SQLite exception: " << e.what() << std::endl;
                LOG << "[-] Error registration!\n";
                LOG << "SQLite exception: " << e.what() << std::endl;
                response[RESULT] = "false";
                response[REASON] = "This name is already exist!";
                ws->send(response.dump(), OpCode);
                return;
            }
            response[RESULT] = "true";
            response[REASON] = "none";
            std::cout << "[+] Send data: " << response << std::endl;
            LOG << "[+] Send data: " << response << std::endl;
            ws->send(response.dump(), OpCode);
        } else {
            response[RESULT] = "false";
            response[REASON] = "This login is already exist!";
            std::cout << "[+] Send data: " << response << std::endl;
            LOG << "[+] Send data: " << response << std::endl;
            ws->send(response.dump(), OpCode);
        }
    }
    // load msgs history
    if (command == LOAD_MSG)
    {
        try {
            if (to_string(parsed[FOR_ID]) == "\"-1\"") {
                SQL_RESPONSE = "SELECT * FROM MSG WHERE TO_USER = -1";
                std::cout << "we are here";
            } else {
                SQL_RESPONSE = "SELECT * FROM MSG WHERE (FROM_USER = " + to_string(parsed[FOR_ID]) + " AND TO_USER = " + to_string(parsed[USER_ID]) +
                    ") OR (FROM_USER = " + to_string(parsed[USER_ID]) + " AND TO_USER = " + to_string(parsed[FOR_ID]) +
                    ")";
            }
            SQLite::Statement   query(db, SQL_RESPONSE);
            while (query.executeStep())
            {
                response[COMMAND] = LOAD_MSG;
                response[TIME] = query.getColumn(0);
                response[FROM_ID] = query.getColumn(1);
                response[TO_ID] = query.getColumn(2);
                response[NAME] = get_name(to_string(response[FROM_ID]));
                response[MESSAGE] = query.getColumn(3);
                std::cout << "[+] Send data: " << response << std::endl;
                LOG << "[+] Send data: " << response << std::endl;
                ws->send(response.dump(), OpCode);
            }
        } catch (std::exception& e){
            std::cout << "[-] Error load_msgs!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error load_msgs!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
        }
    }
    // load users list
    if (command == LOAD_USERS)
    {
        try {
            SQL_RESPONSE = "SELECT NAME, STATUS FROM USERS WHERE USER_ID != " + to_string(parsed[USER_ID]) + ";";
            SQLite::Statement   query(db, SQL_RESPONSE);
            while (query.executeStep())
            {
                response[COMMAND] = LOAD_USERS;
                response[NAME] = query.getColumn(0);
                response[STATUS] = query.getColumn(1);
                std::cout << "[+] Send data: " << response << std::endl;
                LOG << "[+] Send data: " << response << std::endl;
                ws->send(response.dump(), OpCode);
            }
        } catch (std::exception& e) {
            std::cout << "[-] Error load_users!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error load_users!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
        }
    }
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
            std::cout << "[-] Error " + AUTHORIZATION + "!\n";
            std::cout << "SQLite exception: " << e.what() << std::endl;
            LOG << "[-] Error " + AUTHORIZATION + "!\n";
            LOG << "SQLite exception: " << e.what() << std::endl;
        }
        response[COMMAND] = AUTHORIZATION;
        json req;
        if (IS_EXIST) {
            response[RESULT] = "true";
            sessions_name.push(response[NAME]);
            sessions_id.push(response[USER_ID]);
            try {
                SQL_RESPONSE = "UPDATE USERS SET STATUS = 1 WHERE USER_ID = " + to_string(response[USER_ID]) + ";";
                db.exec(SQL_RESPONSE);
            } catch (std::exception& e)
            {
                std::cout << "[-] Error to update user's status, auth!\n";
                std::cout << "SQLite exception: " << e.what() << std::endl;
                LOG << "[-] Error to update user's status, auth!\n";
                LOG << "SQLite exception: " << e.what() << std::endl;
            }
            req[COMMAND] = UPDATE;
            ws->publish(BROADCAST, req.dump());
        } else {
            response[RESULT] = "false";
        }
        std::cout << "[+] Send data: " << response << std::endl;
        LOG << "[+] Send data: " << response << std::endl;
        ws->send(response.dump(), OpCode);
    }
    if (command == GET_ID)
    {
        response[COMMAND] = GET_ID;
        response[ID] = get_id(to_string(parsed[NAME]));
        std::cout << "[+] Send data: " << response << std::endl;
        LOG << "[+] Send data: " << response << std::endl;
        ws->send(response.dump(), OpCode);
    }
}

int main() {
    if (!LOG) {
        std::cerr << "[-] Impossible to open file Log.txt\n";
    } else {
        LOG << "----------------------------------------------------------------\n";
    }
    try {
        db.exec(SQL_CREATE_USERS_TABLE);
        db.exec(SQL_CREATE_MSG_TABLE);
    } catch (std::exception& e)
    {
        LOG << "[-] SQLite exception: " << e.what() << std::endl;
        std::cout << "[-] SQLite exception: " << e.what() << std::endl;
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
            OpCode = opCode;

            std::cout << "[+] Input data: " << message << std::endl;
            LOG << "[+] Input data: " << message << std::endl;

            processMessage(ws, message);
        },

        .close = [](auto* ws, int /*code*/, std::string_view message) {
            PerSocketData* data = ws->getUserData();
            if (data->user_id != "NULL") {
                try {
                    std::string SQL_RESPONSE = "UPDATE USERS SET STATUS = 0 WHERE USER_ID = " + data->user_id + ";";
                    db.exec(SQL_RESPONSE);
                }
                catch (std::exception& e)
                {
                    std::cout << "[-] SQLite exception: " << e.what() << std::endl;
                    LOG << "[-] SQLite exception: " << e.what() << std::endl;
                }
            }
            if (data->user_id != "NULL") {
                std::cout << "[+] User " + data->user_id + " is go to sleep" << std::endl;
                LOG << "[+] User " + data->user_id + " is go to sleep" << std::endl;
            }

        }
    }).listen(9001, [](auto* listen_socket) { //9001 - порт
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
            LOG << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}