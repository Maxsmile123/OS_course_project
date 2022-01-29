$(function(){

    var myID = undefined;
    var userName = "Maxim";
    var user_id_to = -1;




    webSocket = new WebSocket("ws://localhost:9001");
    webSocket.onmessage = function(event){
        data = JSON.parse(event.data);
        processingMSG(data);
    };

    webSocket.onclose = function(event){
        if (!event.wasClear){
            alert("Соединение прервано... пожалуйста, авторизируйтесь заново.");
            window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\index.html"; // redirect on auth page
        } else{
            alert('Код: ' + event.code + ' причина: ' + event.reason);
        }
    }

    let getTime = function(){
        var date = new Date();
        var dateString = (date.getHours() < 10 ? '0' : '') + date.getHours() + ":" + (date.getMinutes() < 10 ? '0' : '')
            + date.getMinutes() + " " + date.toLocaleDateString();
        return dateString;
    }

    let initChat = function(){

    };

    let get_history = function(){
        let jsonREQ = {
            "from_id" : user_id_to,
            "command": "load_msg",
        }
        webSocket.send(JSON.stringify(jsonREQ));
    }

    let processingMSG = function(data){
        if (data.command == "change_name"){
            if (data.result){
                alert("Имя успешно изменено! :)");
            } else{
                alert("Невозможно изменить на введёное имя :(");
            }
        } else if (data.command == "private_msg"){
            addSendMSG(data);
        } else if (data.command == "get_id"){
            user_id_to = data.id;
        } else if (data.command == "registration"){
            if (data.result == "true"){
                alert("Регистрация успешно пройдена!") // Вместо алерт сделать это ввиде вывода на экране. Сверстать страницу регистрации и авторизации
                window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\index.html";
            } else if (data.result == "false"){
                alert(data.reason); // / Вместо алерт сделать это ввиде вывода на экране.
            }

        } else if (data.command == "authorization"){
            if (data.result == "true"){
                alert("Авторизация успешно пройдена!") // Вместо алерт сделать это ввиде вывода на экране. Сверстать страницу регистрации и авторизации
                userName = data.name;
                myID = data.user_id;
                window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\messenger.html";
            } else if (data.result == "false"){
                alert("Неверные логин или пароль :("); // / Вместо алерт сделать это ввиде вывода на экране.
            }
        } else if (data.command == "load_users"){
            loadUsers(data);
        } else if (data.command == "load_msg"){
            loadMessages(data);
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

    let loadMessages = function(data){
        let messagesList = $('.messages-list');
        let messageItem;
        if(data.user_from == userName){
            messageItem = $('<div class="message-header-you">'
                + '[' + data.time + ' Вы]:' + '&nbsp' + '</div>'
                + '<div class="message">' + data.message + '</div>');
        } else{
            messageItem = $('<div class="message-header">'
                + '[' + data.time + data.user_from + ']:' + '&nbsp' + '</div>'
                + '<div class="message">' + data.message + '</div>');
        }
        messagesList.append(messageItem);
    };

    let loadUsers = function(data){
        let usersList = $('.users-list-ul');
        let user;
        if(data.status == "0"){
            user = $('<li><a class ="NeedToClick" href="#">' + '🔴 ' + data.name + '</a></li>');
        } else if (data.status == "1"){
            user = $('<li><a class ="NeedToClick" href="#">' + '🟢 ' + data.name + '</a></li>');
        }
        usersList.append(user);

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
        get_history();
    });

    initChat();




})