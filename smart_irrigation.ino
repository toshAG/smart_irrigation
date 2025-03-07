#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "DHT.h"

const char* ssid = "V5SJ4FS";
const char* password = "012345678Q";

// Pin assignments
const int in1 = 27;      // Replace with appropriate ESP32 pin
const int in2 = 26;      // Replace with appropriate ESP32 pin
const int sensor1 = 34;  // Moisture sensor 1 (ADC)
const int sensor2 = 35;  // Moisture sensor 2 (ADC)
const int sensorPin = A0; // General sensor pin (if used elsewhere)
const int valveControlPin = 32; // Pin connected to valve control
const int flowPin = 14;   // Pin connected to flow sensor
const int delayTime = 8000; // Delay in milliseconds

// DHT sensor setup
#define DHTPIN 4    // DHT sensor connected to pin 4
#define DHTTYPE DHT22   // DHT 22 sensor
DHT dht(DHTPIN, DHTTYPE);

// Flow sensor counter
volatile int flowCount = 0;  // Variable to store the flow count

void IRAM_ATTR pulseCounter() {
    flowCount++;
}

// Constants for moisture sensor range (adjust these values based on calibration)
const int sensor1Min = 0;
const int sensor1Max = 4025;
const int sensor2Min = 0;
const int sensor2Max = 4025;

// Define the HTML content (unchanged interface with link and YouTube embed)
const char* index_html PROGMEM = R"=====( 
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>smart nurserybed</title>
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
        }

        .container {
            width: 80%;
            max-width: 1200px;
            background-color: #6be8b4;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        h1, h2 {
            margin-bottom: 20px;
            color: white;
            background-color: green;
            padding: 10px;
            border-radius: 8px;
            font-weight: bold;
            text-align: center;
        }

        .parameter-container {
            display: flex;
            justify-content: space-around;
            width: 100%;
            margin-bottom: 20px;
        }

        .parameter-box {
            padding: 15px;
            border-radius: 8px;
            width: 18%;
            text-align: center;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }

        .parameter-box span {
            font-weight: bold;
            display: block;
            margin-top: 5px;
        }

        .sensor1 {
            background-color: #FFDDC1;
        }

        .sensor2 {
            background-color: #FFABAB;
        }

        .flow-rate {
            background-color: #FF6F61;
        }

        .temperature {
            background-color: #6B5B95;
        }

        .humidity {
            background-color: #8FC1E3;
        }

        button {
            width: 20%;
            padding: 10px;
            margin-bottom: 10px;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
        }

        button.open {
            background-color: #e74c3c;
            color: white;
        }

        button.close {
            background-color: #3498db;
            color: white;
        }

        button.open:hover,
        button.close:hover {
            background-color: rgb(7, 80, 7);
        }

        iframe {
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>The kinondo School</h1>
        <h2> PRESENTERS:CELINA JULIUS AND HUSSEIN RAMA</h2>
       <h3> CATEGORY: AGRICULTURE FROM KWALE</h3>
        <div class="parameter-container">
            <div class="parameter-box sensor1">
                <p>MOISTURE SENSOR IN NURSERY 1:</p>
                <span id="sensor1">N/A</span>
            </div>
            <div class="parameter-box sensor2">
                <p>MOISTURE SENSOR IN NURSERY 2:</p>
                <span id="sensor2">N/A</span>
            </div>
            <div class="parameter-box flow-rate">
                <p>Flow Rate:</p>
                <span id="flowRate">N/A</span> L/min
            </div>
            <div class="parameter-box temperature">
                <p>Temperature:</p>
                <span id="temperature">N/A</span> °C
            </div>
            <div class="parameter-box humidity">
                <p>Humidity:</p>
                <span id="humidity">N/A</span> %
            </div>
        </div>

        <button class="open" onclick="openValve()">Open Valve</button>
        <button class="close" onclick="closeValve()">Close Valve</button>      

        <p>For more information, <a href="https://nation.africa/kenya/">click here</a>.</p>
        <iframe width="400" height="200" src="https://www.youtube.com/embed/vlH54GRmSfk" title="Sensor Based Irrigation System Demo at Giaki Farm Meru" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
    </div>

    <script>
        function updateSensorReadings(sensor1, sensor2, flowRate, temperature, humidity) {
            document.getElementById("sensor1").innerText = sensor1 + '%';
            document.getElementById("sensor2").innerText = sensor2 + '%';
            document.getElementById("flowRate").innerText = flowRate.toFixed(2);
            document.getElementById("temperature").innerText = temperature.toFixed(2);
            document.getElementById("humidity").innerText = humidity.toFixed(2);
        }

        function openValve() {
            console.log('Valve opened');
        }

        function closeValve() {
            console.log('Valve closed');
        }

        setInterval(function() {
            fetch('/updateReadings')
                .then(response => response.json())
                .then(data => {
                    updateSensorReadings(data.sensor1, data.sensor2, data.flowRate, data.temperature, data.humidity);
                })
                .catch(error => console.error('Error updating sensor readings:', error));
        }, 5000); 
    </script>
</body>
</html>
)=====";

AsyncWebServer server(80);

void handleToggleValve(AsyncWebServerRequest *request) {
    int sensorValue = analogRead(sensorPin);
    bool isOpen = (sensorValue > 3000);
    digitalWrite(valveControlPin, isOpen ? HIGH : LOW);

    String valveStatus = isOpen ? "Open" : "Closed";
    String json = "{\"valveStatus\": \"" + valveStatus + "\", \"sensor1\": 75, \"sensor2\": 50}";
    request->send(200, "application/json", json);
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing the system");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup pins
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(sensor1, INPUT);
    pinMode(sensor2, INPUT);
    pinMode(sensorPin, INPUT);
    pinMode(valveControlPin, OUTPUT);
    pinMode(flowPin, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, RISING); 

    dht.begin(); // Start the DHT sensor

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/updateReadings", HTTP_GET, [](AsyncWebServerRequest *request){
        // Read temperature and humidity from DHT sensor
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();

        // If readings fail, set to -1 (or use an error code)
        if (isnan(humidity) || isnan(temperature)) {
            humidity = -1;
            temperature = -1;
        }

        // Read moisture sensors (raw ADC values)
        int rawVal1 = analogRead(sensor1);
        int rawVal2 = analogRead(sensor2);

        // Convert raw values to percentages using calibration range
        int sensor1Percent = map(rawVal1, sensor1Min, sensor1Max, 0, 100);
        int sensor2Percent = map(rawVal2, sensor2Min, sensor2Max, 0, 100);

        // For debugging: print values to the Serial Monitor
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.print("%, Sensor1: ");
        Serial.print(sensor1Percent);
        Serial.print("%, Sensor2: ");
        Serial.print(sensor2Percent);
        Serial.print("%, Flow Count: ");
        Serial.println(flowCount);

        // Prepare JSON with moisture percentages, flow rate, temperature, and humidity
        String json = "{\"sensor1\": " + String(sensor1Percent) +
                      ", \"sensor2\": " + String(sensor2Percent) +
                      ", \"flowRate\": " + String(flowCount) +
                      ", \"temperature\": " + String(temperature) +
                      ", \"humidity\": " + String(humidity) + "}";
        flowCount = 0; // Reset flow count after sending data
        request->send(200, "application/json", json);
    });

    server.begin();
}

void loop() {
    // No additional loop code needed
}
