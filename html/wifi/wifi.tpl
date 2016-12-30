<html>
<head>
<title>WiFi connection</title>    
<meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link rel="stylesheet" type="text/css" href="style.min.css">
    <script type="text/javascript" src="140medley.min.js"></script>
    <script type="text/javascript">

var xhr=j();
var currAp="%sta_ssid%";

function createInputForAp(ap) {
	if (ap.essid=="" && ap.rssi==0) return;
	var div=document.createElement("div");
	div.id="apdiv";
	var rssi=document.createElement("div");
	var rssiVal=-Math.floor(ap.rssi/51)*32;
	rssi.className="icon";
	rssi.style.backgroundPosition="0px "+rssiVal+"px";
	var encrypt=document.createElement("div");
	var encVal="-64"; //assume wpa/wpa2
	if (ap.enc=="0") encVal="0"; //open
	if (ap.enc=="1") encVal="-32"; //wep
	encrypt.className="icon";
	encrypt.style.backgroundPosition="-32px "+encVal+"px";
	var input=document.createElement("input");
	input.type="radio";
	input.name="essid";
	input.value=ap.essid;
	if (currAp==ap.essid) 
        {
        input.checked="1";
        //document.getElementById("mqtt_device_attribute").value = ap.essid
	    }
    input.id="opt-"+ap.essid;
	var label=document.createElement("label");
	label.htmlFor="opt-"+ap.essid;
	label.textContent=ap.essid;
	div.appendChild(input);
	div.appendChild(rssi);
	div.appendChild(encrypt);
	div.appendChild(label);
	return div;
}

function getSelectedEssid() 
{
var e=document.forms.wifiform.elements;
for (var i=0; i<e.length; i++) 
    {
	if (e[i].type=="radio" && e[i].checked) 
        return e[i].value;
	}
return currAp;
}


function scanAPs() {
	xhr.open("GET", "wifiscan.cgi");
	xhr.onreadystatechange=function() {
		if (xhr.readyState==4 && xhr.status>=200 && xhr.status<300) {
			var data=JSON.parse(xhr.responseText);
			currAp=getSelectedEssid();
			if (data.result.inProgress=="0" && data.result.APs.length>1) {
				$("#aps").innerHTML="";
				for (var i=0; i<data.result.APs.length; i++) {
					if (data.result.APs[i].essid=="" && data.result.APs[i].rssi==0) continue;
					$("#aps").appendChild(createInputForAp(data.result.APs[i]));
				}
				window.setTimeout(scanAPs, 20000);
			} else {
				window.setTimeout(scanAPs, 1000);
			}
		}
	}
	xhr.send();
}


window.onload=function(e) {
	scanAPs();
};
  </script>
</head>
<body>
  <div class="row">
    <div class="small-12 medium-8 large-6 small-centered columns content">
	<div style="background-color:green; color:white"><h4>Access point and MQTT Setup<br/>Hackitt & Bodgitt 2016</h4>
	</div>
      <p>The current WiFi mode: %WiFiMode%</p>
      <form name="wifiform" action="connect.cgi" method="post">
      <p>To connect to a WiFi network<br/> please select one of the detected networks...</p>
      <p id="aps">Scanning...</p>
      <p>Current SSID = %sta_ssid%</p>
      <p>WiFi password, if needed</p>
      <input type="password" name="sta_pwd" value="%sta_pwd%" class="small-12" maxlength="31">
      <div class="form-input">
 
        <div>
          <label for="ssid2">Second SSID</label>
          <input type="text" name="ssid2" value="%ssid2%" class="small-12" maxlength="31">
        </div>        <div>
          <label for="pass2">Second PASS</label>
          <input type="password" name="pass2" value="%pass2%" class="small-12" maxlength="31">
        </div>
        <div>
          <label for="mqtt_host">MQTT Host</label>
          <input type="text" name="mqtt_host" value="%mqtt_host%" class="small-12" maxlength="31">
        </div>
        <div>
        <label for="mqtt_user">MQTT Username</label>
        <input type="text" name="mqtt_user" value="%mqtt_user%" class="small-12" maxlength="31">
        </div>
        <div>
        <label for="mqtt_pass">MQTT Password</label>
        <input type="password" name="mqtt_pass" value="%mqtt_pass%" class="small-12" maxlength="31">
        </div>
        <div>
          <label for="mqtt_port">MQTT Port</label>
          <input type="text" name="mqtt_port" value="%mqtt_port%" class="small-12" maxlength="6" >
        </div>
        <div>
          <label for="mqtt_device_name">MQTT Device Name</label>
          <input type="text" name="mqtt_device_name" value="%mqtt_device_name%" class="small-12" maxlength="31">
        </div>
        <div>
          <label for="mqtt_device_description">MQTT Description</label>
          <input type="text" name="mqtt_device_description" value="%mqtt_device_description%" class="small-12" maxlength="63">
        </div>
        <div>
          <label for="mqtt_device_attribute">MQTT Attributes</label>
          <input type="text" name="mqtt_device_attribute" value="%mqtt_device_attribute%" class="small-12" maxlength="31">
        </div>
        <div>
          <label for="ota_host">OTA Host</label>
          <input type="text" name="ota_host" value="%ota_host%" class="small-12" maxlength="63">
        </div>
        
        <div>
          <label for="ota_port">OTA Port</label>
          <input type="text" name="ota_port" value="%ota_port%" class="small-12" maxlength="6" >
        </div>

      <div>
          <label for="wifi_button">Wifi Button (2 or 0)</label>
          <input type="text" name="wifi_button" value="%wifi_button%" class="small-12" maxlength="1" >
        </div>

      <div>
          <label for="sonoff">Sonoff (1 or 0)</label>
          <input type="text" name="sonoff" value="%sonoff%" class="small-12" maxlength="1" >
        </div>


        
        <div>
          <p>Web Page Control:&nbsp;</p>
          <input type="radio" name="enable_webpage_control" id="enable_webpage_control_yes"  value="yes" %web_enabled_yes%>
          <label for="enable_webpage_control_yes">Enable</label>
          <input type="radio" name="enable_webpage_control" id="enable_webpage_control_no" value="no" %web_enabled_no%>
          <label for="enable_webpage_control_no">Disable</label>
        </div>
        <div>
          <label for="web_pass">Password</label>
          <input type="text" name="web_pass" value="%web_pass%" class="small-12" maxlength="31">
        </div>
      </div>
      <div class="submit">
        <input type="submit" name="connect" value="SAVE!">
      </div><br/>
	  <div style="background-color:yellow; color:black">
	  <center>Note: %WiFiapwarn% Save any changes FIRST.</center>
	  </div>
	  </div>
  </div>
</body>
</html>