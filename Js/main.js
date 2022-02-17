$(function(){

    var myID = undefined;
    var userName = undefined;
    var user_id_to = -1;

    webSocket = new WebSocket("ws://localhost:9001");


    webSocket.onmessage = function(event){
        data = JSON.parse(event.data);
        processingMSG(data);
    };


    webSocket.onclose = function(event){
        if (!event.wasClear){

            alert("Connection terminated... please log in again. \nCode Error: 2");
            window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\index.html"; // redirect on auth page
        } else{
            alert('Code Error: ' + event.code + ' Reason: ' + event.reason);
        }
    }

    let getTime = function(){
        var date = new Date();
        var dateString = (date.getHours() < 10 ? '0' : '') + date.getHours() + ":" + (date.getMinutes() < 10 ? '0' : '')
            + date.getMinutes() + " " + date.toLocaleDateString();
        return dateString;
    }

    let get_users_list = function(){
        let jsonUSERS = {
            "command": "load_users",
            "user_id": myID
        }
        let usersList = $('.users-list-ul');
        usersList[0].innerText = "";
        webSocket.send(JSON.stringify(jsonUSERS));
    }

    let initChat = function(){
        get_users_list();
        get_history();
        showYourName();

    };

    let get_history = function(){
        let jsonREQ = {
            "for_id" : user_id_to,
            "user_id" : myID,
            "command": "load_msg",
        }
        webSocket.send(JSON.stringify(jsonREQ));
        let messagesList = $('.messages-list');
        messagesList[0].innerText = "";
    }

    let processingMSG = function(data){
        if (data.command == "change_name"){
            if (data.result == "true"){
                console.log(data.new_name);
                userName = data.new_name;
                showYourName();
                alert("The name has been successfully changed! :)");
            } else{
                alert("Cannot be changed to the entered name :( \nError code: 3");
            }
        } else if (data.command == "private_msg"){
            addSendMSG(data);
        } else if (data.command == "get_id"){
            user_id_to = data.id;
        } else if (data.command == "check"){
            if (data.user_id == "NULL" || data.name == "NULL"){
                window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\index.html";
            }
            myID = data.user_id;
            userName = data.name;
            console.log(myID);
            console.log(userName);
        } else if (data.command == "load_users"){
            loadUsers(data);
        } else if (data.command == "load_msg"){
            loadMessages(data);
        } else if (data.command == "update"){
            let jsonUSERS = {
                "command": "load_users",
                "user_id": myID
            }
            let usersList = $('.users-list-ul');
            usersList[0].innerText = "";
            webSocket.send(JSON.stringify(jsonUSERS));
        } else if (data.command == "ping"){
            let jsonPING = {
                "command": "ping",
                "user_id": myID,
            }
            webSocket.send(JSON.stringify(jsonPING));

        }


    }
    $("#send").click(function(){
        let time = getTime();
        let messagesList = $('.messages-list');
        var text = document.getElementById('textarea');
        var msg = text.value.replace("\n", "");
        if (msg == "") return;
        text.value = "";
        let jsonMSG = {
            "command": "private_msg",
            "from_id": myID,
            "to_id" : user_id_to,
            "message": msg,
            "time": time
        }
        webSocket.send(JSON.stringify(jsonMSG));
        let headerItem = $('<span class="message-header-you">'
            + '[' + time + ' Вы]: <wbr>' + '</span>');
        messageItem = $('<span class="message">' + msg + '</span>' + '</br>');
        messagesList.append(headerItem);
        messagesList.append(messageItem);
        get_users_list();
    });

    let loadMessages = function(data){
        let messagesList = $('.messages-list');
        let messageItem;
        let headerItem;
        if(data.name == userName){
            headerItem = $('<span class="message-header-you">'
                + '[' + data.time + ' Вы]: <wbr> ' + '</span>');
            messageItem =  $('<span class="message">' + data.message  + '</span>' + '</br>');
        } else{
            headerItem = $('<span class="message-header">'
                + '[' + data.time + " " + data.name + ']: <wbr> ' + '</span>');
            messageItem =  $('<span class="message">' + data.message + '</span>' + '</br>');
        }
        messagesList.append(headerItem);
        messagesList.append(messageItem);
    };

    let loadUsers = function(data){
        let usersList = $('.users-list-ul');
        let user = undefined;
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
        data.message = data.message.replaceAll(/"/g, '');
        let headerItem = $('<span class="message-header">'
            + '[' + data.time + " " + data.name_from + ']: <wbr> ' + '</span>');
        let messageItem =  $('<span class="message">' + data.message + '</span>' + '</br>');
        messagesList.append(headerItem);
        messagesList.append(messageItem);
    }

    $('#CN').click(function(){
        let name = prompt('Enter a new username:');
        var jsonCN = {
            "command": "change_name",
            "old_name" : userName,
            "new_name" : name
        }
        webSocket.send(JSON.stringify(jsonCN));
    });

    $(".users-list-ul").on("click", ".NeedToClick",  function(){
        let users_list = $(this);
        let cur_user_block = $('.current-user');
        let cur_user = users_list[0].innerText.substr(3);
        cur_user_block[0].innerHTML = cur_user;
        getID(cur_user);
        setTimeout(() => {
            get_history();}, 500);
    });

    let showYourName = function(){
        let cur_user_block = $('.NameStyle');
        cur_user_block[0].innerText = userName.replaceAll(/"/g, '');
    }

    let checkAuthStatus = function(){
        var jsonCAS = {
            "command": "check",
        }
        webSocket.send(JSON.stringify(jsonCAS));
    }

    setTimeout(() => {
        checkAuthStatus();}, 500);

    setTimeout(() => {
        initChat();}, 1000);










})