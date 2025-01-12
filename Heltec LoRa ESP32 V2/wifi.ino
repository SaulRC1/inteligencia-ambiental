WiFi.mode(WIFI_STA);
WiFi.persistent(false);
WiFi.setAutoReconnect(true);
WiFi.setSleep(false);

Serial.println("\nConnecting");
WiFi.begin(ssid, password);

int timeout_counter = 0;

while (WiFi.status() != WL_CONNECTED)
{
    // failed, retry
    Serial.print(".");
    delay(500);
    timeout_counter++;
    if (timeout_counter >= CONNECTION_TIMEOUT * 5)
    {
        ESP.restart();
    }
}

Serial.println("You're connected to the network");
Serial.print("Connected, IP address: ");
Serial.println(WiFi.localIP());
Serial.println(WiFi.getHostname());
Serial.println();
delay(1000);
bool success = Ping.ping("www.google.com", 3);
if (!success)
{
    Serial.println("Ping failed");
    return;
}
Serial.println("Ping succesful.");
delay(1000);