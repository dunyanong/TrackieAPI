#include <WiFi.h>
#include <HTTPClient.h>;
#include <ArduinoJson.h>;

const char* ssid = "Shirley";
const char* password = "shirleyy";

void setup() {
  Serial.begin(115200);
  WiFi.begin("Shirley", "shirleyy");  // Replace with your Wi-Fi credentials
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if ((WiFi.status() == WL_CONNECTED)) //Check the current connection status
  {
    long rnd = random(1,10);
    HTTPClient client;

    client.begin("http://jsonplaceholder.typicode.com/comments?id=" + String(rnd));
    int httpCode = client.GET();

    if (httpCode > 0) {
      String payload = client.getString();
      Serial.println("\nStatuscode: " + String(httpCode));
      Serial.println(payload);

      char json[500];
      payload.replace(" ", "");
      payload.replace("\n", "");
      payload.trim();
      payload.remove(0,1);
      payload.toCharArray(json, 500);

      StaticJsonDocument<200> doc;
      deserializeJson(doc, json);

      int id = doc["id"];
      const char* email = doc["email"];

      Serial.println(String(id) + "-" + String(email) + "\n");

      client.end();
     
    }
    else {
      Serial.println("Ërror on HTTP request");
    }
  }
  else
  {
    Serial.println("Connection lost");
  }
  delay(10000);
}
  
