//Listen for the watchface opening
Pebble.addEventListener('ready',
    function(e){
        console.log("PebbleKit JS ready!");
    }
);

//Listen for recieved AppMessage
Pebble.addEventListener('appmessage',
    function(e){
        var time_ms = e.payload.TIME_KEY;

        console.log(time_ms);
    }
);
