var interactive = function(){
        $('.menu').click(function() {
            $('.menu-section').animate({
                left: '0px'
            }, 200);
            $('.icon-section').animate({
                left: '550px'
            }, 300);   

        });

        $('.exit').click(function() {
            $('.menu-section').animate({
                left: '-265px'
            }, 200);
            $('.icon-section').animate({
                left: '550px'
            }, 300);   
        });
    }




$(document).ready(interactive);

