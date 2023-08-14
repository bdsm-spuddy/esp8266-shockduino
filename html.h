static const String bottom_frame_html = R"EOF(
<html>
<body>
Select an action above
</body>
</html>
)EOF";

static const String change_ap_html = R"EOF(
<html>
<body>

<form method=post enctype="multipart/form-data">
To set the WiFi network details, enter here:
<br>
WiFi SSID: <input name=ssid size=40>  (leave empty for no change)
<br>
WiFi Password: <input name=password size=40>  (leave empty for no change)
<br>
Collar name: <input name=collarname size=40 value="##collarname##">
<p>
Transmitter Key: <input name=key type=number min=0 max=131071 value="##key##"> (0 to 131071; default 22858)
<br>
GPIO pin to trigger collar (default 16): <input name=pin size=2 value="##pin##">
<br>
<input type=submit value="Update details" name=setwifi>
<br>
If the change is accepted, the shock controller will reboot after 5 seconds.
For reference these are the pins on a NodeMCU 12E board:
<hr>
<blockquote>
<pre>
GPIO16 == D0 (the default)
GPIO5  == D1
GPIO4  == D2
GPIO0  == D3
GPIO2  == D4
GPIO14 == D5
GPIO12 == D6
GPIO13 == D7
GPIO15 == D8
</pre>
</blockquote>
<hr>
<p>
<font size=-3>Software version: ##VERSION##</font>
</form>
</body>
</html>
)EOF";

static const String change_auth_html = R"EOF(
<html>
<body>

<form method=post action=setauth/ enctype="multipart/form-data">
To set the user name and password needed to access the collar controller:
<br>
Username: <input name=username size=40>
<br>
Password: <input name=password size=40>
<br>
<input type=submit value="Set Auth Details">
<hr>
If the change is accepted, you will need to login again.
</form>
</body>
</html>
)EOF";

static const String index_html = R"EOF(
<!DOCTYPE html>
<html>

<head>
<title>Shock Collar</title>
</head>

<frameset rows = "200px,*">
  <frame name = "top" src = "top_frame.html" />
  <frame name = "bottom" src = "bottom_frame.html" />

  <noframes>
     <body>Your browser does not support frames.</body>
  </noframes>

</frameset>

</html>
)EOF";

static const String top_frame_html = R"EOF(
<!DOCTYPE html>
<html>
<head>
  <base target="bottom">
</head>

<body>
<center>
  <form name=time action=send/ method=post enctype="multipart/form-data">
    Vibrate strength: <input name="v_str" size=3 type="number" min="0" max="100" value=100>
    Vibrate duration: <input name="v_dur" size=3 type="number" min="0" max="30" value=1>
    <input type=submit name=vibrate value=Vibrate>
<p>
    Shock strength: <input name="s_str" size=3 type="number" min="0" max="100" value=1>
    Shock duration: <input name="s_dur" size=3 type="number" min="0" max="30" value=1>
    <input type=submit name=shock value=Shock>
<p>
    Beep duration: <input name="b_dur" size=3 type="number" min="0" max="30" value=1>
    <input type=submit name=beep value=Beep>
<p>
</form>
</center>
<hr>
<center>
<a href="change_ap.html">Change WiFi or collar details</a>
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;
--
&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;
<a href="change_auth.html">Change authentication details</a>
</center>
</body>
</html>
)EOF";

