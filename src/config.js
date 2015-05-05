Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL("http://deepshitnine.gmxhome.de/MartialElch/PebblAviator24hWatchConfigV1.2.html");
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log('JSON: ', JSON.stringify(configuration));

    // save local data
    localStorage.setItem("seconds", configuration.seconds);

    //Send to Pebble, persist there
    var message = {
      "KEY_SECONDS": configuration.seconds
    };
    Pebble.sendAppMessage(
      message,
      function(e) {
        console.log("Sending settings data...");
      },
      function(e) {
        console.log("Sending settings failed!");
      }
    );
  }
);

Pebble.addEventListener('ready', function(e) {
  loadLocalData();
});

function loadLocalData() {
  var config = {
    "KEY_SECONDS": localStorage.getItem("seconds")
  };
  
  Pebble.sendAppMessage (
    config,
      function(e) {
        console.log("Sending config data...");
      },
      function(e) {
        console.log("Sending config failed!");
      }
  )
}
