$(function(){

    webSocket = new WebSocket("ws://localhost:9001");

    webSocket.onerror = function(error) {
        alert("Failed to connect to the server.\nCode Error: 1");
        window.location.reload();
    };

    webSocket.onmessage = function(event){
        data = JSON.parse(event.data);
        processingMSG(data);
    };

    let processingMSG = function(data){
        if (data.command == "authorization"){
            if (data.result == "true"){
                $('.container-login100').animate({
                    left: '2000px'
                }, 0);
                $('.container-login100L').animate({
                    left: '0px'
                }, 0);
                window.scrollTo(0, document.body.scrollHeight);

                setTimeout(() => {
                    window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\messenger.html";}, 15500);

            } else if (data.result == "false"){

                let msgplace = $('.incorrent-login');
                if(msgplace[0].innerHTML == ""){
                    let msg = "Invalid username or password!";
                    msgplace.append(msg);
                }
            }
        } else{
            alert("An error has occurred, try again! Error code: 5");
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