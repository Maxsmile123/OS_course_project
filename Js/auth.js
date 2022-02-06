$(function(){


    var myID = undefined;
    var userName = "Maxim";

    webSocket = new WebSocket("ws://localhost:9001");
    webSocket.onmessage = function(event){
        data = JSON.parse(event.data);
        processingMSG(data);
    };

    let processingMSG = function(data){
        if (data.command == "authorization"){
            if (data.result == "true"){
                // Проигрыш gifки
                userName = data.name;
                myID = data.user_id;
                webSocket.close();
                window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\messenger.html";
            } else if (data.result == "false"){

                let msgplace = $('.incorrent-login');
                if(msgplace[0].innerHTML == ""){
                    let msg = "Неверные логин или пароль!";
                    msgplace.append(msg);
                }
            }
        } else{
            alert("Произошла ошибка, попробуйте ещё раз!");
        }
    }
    function validate (input) {
        if(input.match(/^([a-zA-Z0-9_\-\.]+)@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.)|(([a-zA-Z0-9\-]+\.)+))([a-zA-Z]{1,5}|[0-9]{1,3})(\]?)$/) == null) {
            return false;
        } else {
            if(input == ''){
                return false;
            }
        } return true;
    }

    $("#login").click(function(){
        var login = document.getElementById('inputLogin');
        var password = document.getElementById('inputPass');
        if(!validate(login.value)){
            login.value = "";
            password.value = "";
            return;
        }
        let json = {
            "command": "authorization",
            "login": login.value,
            "password": password.value
        }
        login.value = "";
        password.value = "";
        webSocket.send(JSON.stringify(json));

    })
    $("#reg").click(function(){
        window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\registration.html";
    })




});