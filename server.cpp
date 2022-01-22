#include <nlohmann/json.hpp>
#include <uWebSockets/App.h>
#include <iostream>
#include <string>

using json = nlohmann::json;
using namespace std;


// ws - объект/параметр вебсокета

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
const string AUTHORIZATION_STATUS = "authorization_status";
const string AUTHORIZATION = "authorization";
const string LOAD_MSG = "load_msg";
const string LOAD_USERS = "load_users";


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



void processMessage(UWEBSOCK* ws, std::string_view message)
{
    PerSocketData* data = ws->getUserData();
    auto parsed = json::parse(message);
    string command = parsed[COMMAND];

    if (command == PRIVATE_MSG)
    {
        try {
            string user_id = to_string(parsed[USER_ID]);
            string user_msg = parsed[MESSAGE];
            int sender = data->user_id;
            json response;
            response[COMMAND] = PRIVATE_MSG;
            response[NAME_FROM] = data->name;
            response[MESSAGE] = user_msg;
            response[TIME] = parsed[TIME];
            ws->publish("UserN" + user_id, response.dump());
        }
        catch(...) {
            cout << "[-] Error to send msg!\n";
        }


    }
    if (command == SET_NAME)
    {
        data->name = parsed[NAME];
        ws->publish(BROADCAST, status(data, true));
    }
}

int main() {
    int latest_id = 10;

    uWS::App().ws<PerSocketData>("/*", {
        .idleTimeout = 9999, /*
        .maxBackpressure = 1 * 1024 * 1024,
        .closeOnBackpressureLimit = false,
        .resetIdleTimeoutOnSend = false,
        .sendPingsAutomatically = true,
        */
        /* Handlers */
        .open = [&latest_id](auto* ws) {

            PerSocketData* data = ws->getUserData();
            data->user_id = latest_id++;

            cout << "User " << data->user_id << "has connected" << endl;



            ws->publish(BROADCAST, status(data, true));

            ws->subscribe(BROADCAST); //сообщения получают все пользователи
            ws->subscribe("UserN" + to_string(data->user_id)); // личный канал


            for (auto entry : activeUsers)
            {
                ws->send(status(entry.second, true), uWS::OpCode::TEXT);
            }

            activeUsers[data->user_id] = data;


        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            // ws->send(message, opCode, true);
            PerSocketData* data = ws->getUserData();

            cout << message << endl;

            processMessage(ws, message);
        },

        .close = [](auto* ws, int /*code*/, std::string_view /*message*/) {
            /* You may access ws->getUserData() here */
            PerSocketData* data = ws->getUserData();

            cout << "close" << endl;
            ws->publish(BROADCAST, status(data, false));

            activeUsers.erase(data->user_id);

        }
    }).listen(9001, [](auto* listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}