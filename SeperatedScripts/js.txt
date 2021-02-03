        var gateway = `ws://${window.location.hostname}/ws`;  //entry point to websocket interface. window.location.home finds current address, which will be web server ip address
        var websocket;    
        window.addEventListener('load', onLoad);  //onLoad called when web page loads
        function initWebSocket() {    //function to initialize websocket connection to server
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway); //uses the gateway address to initialize
            websocket.onopen    = onOpen;       //self explanatory, onOpen called when websocket connection opened, onClose on connection closed and onMessage when recieving message from server
            websocket.onclose   = onClose;
            websocket.onmessage = onMessage; 
        }
        function onOpen(event) {
            console.log('Connection opened');   //lets us know the connection worked
        }
        function onClose(event) {
            console.log('Connection closed');       //lets us know connection failed
            setTimeout(initWebSocket, 2000);        //tries to connect again every 2000ms (2sec)
        }
        function onMessage(event) { //reacts to message sent from server which tells us what position the valve is in or the moisture percentage
            var state;
            var perc;
            if(event.data == "-1" || event.data == "-2" || event.data == "-3"){
              if(event.data == "-1"){
                  state = "Valve Open";
              }
              else if(event.data == "-2"){
                  state = "Valve Closed";
              }
              else{
                  state = "Error";
              }
  
              document.getElementById('state').innerHTML = state; //replaces the text placeholder for valve state
            }
            else{
              var y = "Last Updated Moisture Percent = ";
              var z = "%";
              perc = y + event.data+z;
              document.getElementById('perc').innerHTML = perc;   //replaces the text placeholder for moisture percentage
            }
            
            
            
        }
        function onLoad(event) {  //called when we load in. sets up connection and then sets the functions for our buttons with initButton()
            initWebSocket();
            initButton();
        }
        function initButton() {   //sets buttons up


            document.getElementsByClassName('openButton')[0].addEventListener('click', openValve);   //when the button is clicked we call openValve function in js

            document.getElementsByClassName('closeButton')[0].addEventListener('click', closeValve);      
                       
            document.getElementsByClassName('getButton')[0].addEventListener('click', getMoisture);      


        }
      
        function openValve(){websocket.send('openValve');}  //the functions will simply send a string (in this case openValve) to the server (our esp32) which will react depending on the contents of the message
        function closeValve(){websocket.send('closeValve');}
        function getMoisture(){websocket.send('getMoisture');}
