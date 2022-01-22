$(function(){

    var userName = "Maxim";
    var user_id_to = -1;


    webSocket = new WebSocket("ws://localhost:9001");
    webSocket.onmessage = function(event){
            data = JSON.parse(event.data);
            processingMSG(data);
    };

    let getTime = function(){
        var date = new Date();
        var dateString = (date.getHours() < 10 ? '0' : '') + date.getHours() + ":" + (date.getMinutes() < 10 ? '0' : '')
        + date.getMinutes() + " " + date.toLocaleDateString();
        return dateString;
    }

    let initChat = function(){
        loadMesseges();
        loadUsers();
    };

    let processingMSG = function(data){
        if (data.command == "authorization_status"){
            if (data.result){
                userName = data.name;
                initChat();
            } else{
                window.location.href = "http://localhost:10555/index.html"; // redirect on auth page
            }
        } else if (data.command == "change_name"){
            if (data.result){
                alert("Имя успешно изменено! :)");
            } else{
                alert("Невозможно изменить на введёное имя :(");
            }
        } else if (data.command == "private_msg"){
            addSendMSG(data);
        } else if (data.command == "get_id"){
            user_id_to = data.id;
        }
    

    }
        $("#send").click(function(){
            let time = getTime();
            let messagesList = $('.messages-list');
            var text = document.getElementById('textarea');
            var msg = text.value;
            if (msg == "") return;
            text.value = "";
            let jsonMSG = {
                "command": "private_msg",
                "user_id": user_id_to,
                "message": msg,
                "time": time
            }
            webSocket.send(JSON.stringify(jsonMSG));
            let messageItem = $('<div class="message-header-you">'
            + '[' + time + ' Вы]:' + '&nbsp' + '</div>'
            + '<div class="message">' + msg + '</div>');
            messagesList.append(messageItem);
        });

        let loadMesseges = function(){



        };

        let loadUsers = function(){

        };

        let getID = function(Nickname){
            var jsonID = {
                "command": "get_id",
                "name": Nickname
            }
            webSocket.send(JSON.stringify(jsonID));
        }

        let addSendMSG = function(data) {
            let messagesList = $('.messages-list');
            let messageItem = $('<div class="message-header">'
            + '[' + data.time + ' ' + data.name_from +  ' ]:' + '&nbsp' + '</div>'
            + '<div class="message">' + data.message + '</div>');
            messagesList.append(messageItem);
        }

        $('#CN').click(function(){
            let name = prompt('Введите новое имя пользователя:');
            userName = name;
            var jsonCN = {
                "command": "change_name",
                "new_name" : name
            }
            webSocket.send(JSON.stringify(jsonCN));
        });
        
        $(".NeedToClick").click(function(){
            let users_list = $(this);
            let cur_user_block = $('.current-user');
            let cur_user = users_list[0].innerText.substr(2);
            cur_user_block[0].innerHTML = cur_user;
            getID(cur_user);
        });


        let checkAuthStatus = function() {
            var jsonAuth = {
                "command": "authorization_status",
            }
            webSocket.send(JSON.stringify(jsonAuth));

        };

        checkAuthStatus();

})