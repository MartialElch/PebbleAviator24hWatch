Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL("http://deepshitnine.gmxhome.de/MartialElch/PebblAviator24hWatchConfigV1.3.html");
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
    var configuration = JSON.parse(decodeURIComponent(e.response));

    //Send to Pebble, persist there
    var message = {
      "KEY_SECONDS": configuration.seconds,
      "KEY_INVERT":  configuration.invert
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