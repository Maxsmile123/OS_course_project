function blink()
{
    if(document.querySelector("blink"))
    {
        var d = document.querySelector("blink") ;
        d.style.visibility= (d.style.visibility=='hidden'?'visible':'hidden');
        setTimeout('blink()', 500);
    }
}

$(function(){
    webSocket = new WebSocket("ws://localhost:9001");
    webSocket.onerror = function(error) {
        alert("Failed to connect to the server.\nCode Error: 1");
        window.location.reload();
    };
    webSocket.onmessage = function(event){
        console.log(event);
        data = JSON.parse(event.data);
        processingMSG(data);
    };
    let processingMSG = function(data){
        console.log(data);
        if (data.command == "registration"){
            if (data.result == "true"){
                window.location.href = "C:\\Users\\Sysoe\\VSCodeProjects\\Frontend motherfacker\\index.html";
            } else if (data.result == "false"){
                let msgplace = $('.incorrent-login');
                if(msgplace[0].innerHTML == ""){
                    let msg = data.reason;
                    msgplace.append(msg);
                }
            }
        } else{
            alert("An error has occurred, try again! Error code: 4");
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

    $("#Register").click(function(){
        var login = document.getElementById('inputLogin');
        var name = document.getElementById('inputName');
        var password = document.getElementById('inputPass');
        if(!validate(login.value)){
            login.value = "";
            name.value = "";
            password.value = "";
            return;
        }
        let json = {
            "command": "registration",
            "name": name.value,
            "login": login.value,
            "password": password.value
        }
        login.value = "";
        name.value = "";
        password.value = "";
        webSocket.send(JSON.stringify(json));
    })


});

