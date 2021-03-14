#include "otaupdate.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void OTAUpdate::addRoutes() {
    httpServer.on("/", HTTP_GET, [&]() {
        httpServer.client().setNoDelay(true);
        const char compile_date[] = __DATE__ " " __TIME__;
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'></head>") +
            "<body><div class='container'><h1>" + settings.device_name + "</h1>" +
            "Version: " + String(compile_date) +
            "<br>Device address: " + settings.device_name + ".local (" + WiFi.localIP().toString() + ")" + 
            "<br>MQTT server: " + settings.mqtt_server + ":" + settings.mqtt_port +
            "<br>MQTT User: " + settings.mqtt_user +
            "</p><p><a href='update' class='btn btn-primary'>OTA-Update</a> <a href='config' class='btn btn-primary'>Configuration</a> " +
            "<a href='reboot' class='btn btn-warning' onclick=\"return confirm('Are you sure to reboot?')\">Device reboot</a> " +
            "<a href='reset' class='btn btn-danger' onclick=\"return confirm('Are you sure to factory-reset?')\">Factory-Reset</a></p>" +
            "</div></body></html>";

        httpServer.send(200, F("text/html"), tmp);
        delay(100);
        httpServer.client().stop();
    });

    httpServer.on("/reset", HTTP_GET, [&]() {
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>") +
            "<meta http-equiv='refresh' content='10; /'>" +
            "</head><body><div class='container'><h1>" + settings.device_name + "</h1>" +
            "<div class='alert alert-danger'>Reset of H801 was successful, you can reconfigure it. </div>" + 
            "</div></body></html>";
        httpServer.send(200, F("text/html"), tmp);
        delay(1000);
        httpServer.client().stop();
        wifi.resetSettings();
        settings.remove();
        delay(1000);
        ESP.restart();
    });

    httpServer.on("/reboot", HTTP_GET, [&]() {
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>") +
            "<meta http-equiv='refresh' content='10; /'>" +
            "</head><body><div class='container'><h1>" + settings.device_name + "</h1>" +
            "<div class='alert alert-danger'>Device is rebooting.</div>" + 
            "</div></body></html>";
        httpServer.send(200, F("text/html"), tmp);
        httpServer.client().stop();
        ESP.restart();
    });

    httpServer.on("/config", HTTP_GET, [&]() {
        String tmp = String("<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css'>") +
            "</head><body><div class='container'><h1>Config for " + settings.device_name + "</h1><form method='POST'>" +
            "<div class='mb-3'><label for='1' class='form-label'>Mqtt server</label><input type='text' class='form-control' id='1' name='mqtt_server' value='" + settings.mqtt_server + "'></div>" + 
            "<div class='mb-3'><label for='2' class='form-label'>Mqtt server port</label><input type='text' class='form-control' id='2' name='mqtt_port' value='" + settings.mqtt_port + "'></div>" + 
            "<div class='mb-3'><label for='3' class='form-label'>Mqtt user</label><input type='text' class='form-control' id='3' name='mqtt_user' value='" + settings.mqtt_user + "'></div>" + 
            "<div class='mb-3'><label for='4' class='form-label'>Mqtt password</label><input type='password' class='form-control' id='4' name='mqtt_pass' value='" + settings.mqtt_pass + "'></div>" + 
            "<div class='mb-3'><label for='5' class='form-label'>Device name</label><input type='text' class='form-control' id='5' name='device_name' value='" + settings.device_name + "'></div>" + 
            "<button type='submit' class='btn btn-primary'>Submit</button>" + 
            "</form></div></body></html>";
        httpServer.send(200, F("text/html"), tmp);
    });

    httpServer.on("/config", HTTP_POST, [&]() {
        // check if all config args are there
        if ( ! httpServer.hasArg("mqtt_server") || ! httpServer.hasArg("mqtt_port") 
            || ! httpServer.hasArg("mqtt_user") || ! httpServer.hasArg("mqtt_pass") 
            || ! httpServer.hasArg("device_name") ) {
            httpServer.send(400, F("text/plain"), F("400: Invalid Request")); // The request is invalid, so send HTTP status 400
            return;
        }

        strcpy(settings.mqtt_server, httpServer.arg("mqtt_server").c_str());
        strcpy(settings.mqtt_port, httpServer.arg("mqtt_port").c_str());
        strcpy(settings.mqtt_user, httpServer.arg("mqtt_user").c_str());
        strcpy(settings.mqtt_pass, httpServer.arg("mqtt_pass").c_str());
        strcpy(settings.device_name, httpServer.arg("device_name").c_str());
        settings.save();

        httpServer.send(200, F("text/html"), F("<META http-equiv='refresh' content='10;URL=/'>Ok. Rebooting!"));

        httpServer.client().stop();
        ESP.restart();
    });
}

void OTAUpdate::loop() {
    httpServer.handleClient();
}

void OTAUpdate::init() {
    httpUpdater.setup(&httpServer);
    this->addRoutes();
    httpServer.begin();
}

OTAUpdate otaUpdate;
