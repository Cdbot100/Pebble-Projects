function fetchWeather(latitude, longitude) {
    var response;
    var req = new XMLHttpRequest();
    req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
             "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
    console.log("get");
    req.onload = function(e) {
        if (req.readyState == 4) {
            if(req.status == 200) {
                //console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, city;
                if (response) {
                    temperature = Math.round(response.main.temp - 273.15);
                    temperature = Math.round(((temperature*1.8)+32));
                    city = response.weather[0].description;
                    console.log(temperature + "\u00B0F");
                    console.log(city);
                    Pebble.sendAppMessage({
                                          "temperature":temperature + "\u00B0F",
                                          "city":city});
                }
                
            } else {
                //console.log(req.status, "status");
            }
        }//console.log(req.readyState, "state");
    }
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    console.warn('location error (' + err.code + '): ' + err.message);
    Pebble.sendAppMessage({
                          "city":"Loc Unavailable",
                          "temperature":"N/A"
                          });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };


Pebble.addEventListener("ready",
                        function(e) {
                        console.log("connect! " + e.ready);
                        locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                        console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                        window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
                        console.log(e.type);
                        console.log(e.payload.temperature);
                        console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                        function(e) {
                        console.log("webview closed");
                        console.log(e.type);
                        console.log(e.response);
                        });

