<html>
  <head>
    <title>Test</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link rel="stylesheet" type="text/css" href="style.min.css">
  </head>
  <body>
    <div class="row">
      <div class="small-12 medium-6 small-centered columns content">
        <h1>GPIO Control Unit</h1>
        <h1>%mqtt_device_name%</h1>
        <p>Temperature = %temperature%<p>
        <p>Humidity = %humidity%</p>
        <p>Time %time%, date %date%</p>
        <p>Analog Input = %analog%</p>
        <div class="row">
          <div class="small-12 small-centered columns">
          <form method="post" action="led.cgi">
            <div class="form-input">
              <label for="web_pass">Password</label>
              <input type="password" name="web_pass" value="%web_pass%" class="small-12 medium-6">
            </div>
            <div class="form-input">
              <label for="led">LED state is %ledstate%</label>
              <input type="submit" name="led" value="1" class="on"><input type="submit" name="led" value="0" class="off">
            </div> 
            <div class="form-input">
              <label for="relay">RELAY state is %relaystate%</label>
              <input type="submit" name="relay" value="1" class="on"><input type="submit" name="relay" value="0" class="off">
            </div>
          </form>
        </div>
      </div>
    </div>
  </div>
</body>
</html>

