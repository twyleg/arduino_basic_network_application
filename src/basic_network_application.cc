// Copyright (C) 2022 twyleg
#include "basic_network_application.h"

#include <CLI.h>
#include <LittleFS.h>


namespace {

constexpr const char* CONFIG_PATH = "/config.json";
constexpr const int SERIAL_BAUD_RATE = 115200;


int handleCli(CLIClient* dev, int argc, char** argv, void* ptr) {

	NetworkApplication* networkApplication = reinterpret_cast<NetworkApplication*>(ptr);

	if (IS_COMMAND("help")) {
		return networkApplication->handleCliHelp(dev, argc, argv);
	}else if (IS_COMMAND("status")) {
		return networkApplication->handleCliStatus(dev, argc, argv);
	}else if (IS_COMMAND("connect_wifi")) {
		return networkApplication->handleCliConnectWifi(dev, argc, argv);
	}else if(IS_COMMAND("config")) {
		return networkApplication->handleCliConfig(dev, argc, argv);
	}else if(IS_COMMAND("config_set_parameter")) {
		return networkApplication->handleCliConfigSetParameter(dev, argc, argv);
	}

	return -1;
}

int handleCliConnect(CLIClient* dev, int argc, char** argv) {
	dev->println("");
	dev->println("Welcome to the CLI, type \"help\" to get available commands.");
	return 0;
}

}


BasicNetworkApplication::BasicNetworkApplication() :
	mWifiCliServer(2323),
	mNtp(mNtpWifiUdp),
	mNtpStarted(false),
	mJsonConfig(256){


}



void BasicNetworkApplication::setupSerial() {
	Serial.begin(SERIAL_BAUD_RATE);

	Serial.println("");
	Serial.printf("Setting up serial interface (baud=%d)...\n\r", SERIAL_BAUD_RATE);
}

void BasicNetworkApplication::setupCli() {
	CLI.setDefaultPrompt("> ");
	CLI.onConnect(handleCliConnect);

	CLI.addCommand("help", handleCli, this);
	CLI.addCommand("status", handleCli, this);
	CLI.addCommand("connect_wifi", handleCli, this);
	CLI.addCommand("config", handleCli, this);
	CLI.addCommand("config_set_parameter", handleCli, this);

	CLI.addClient(Serial);
}

void BasicNetworkApplication::setupFilesystem() {

	Serial.println("Setting up LittleFS...");

	if(!LittleFS.begin(true)){
		Serial.println("LittleFS Mount Failed!");
		return;
	}
}

void BasicNetworkApplication::setupWifi(Stream& stream) {

	stream.println("Setting up Wifi...");

	if(!mJsonConfig.containsKey("ssid")){
		stream.println("Unable to connect to wifi. \"ssid\" in config is missing.");
		return;
	}else if(!mJsonConfig.containsKey("password")){
		stream.println("Unable to connect to wifi. \"password\" in config is missing.");
		return;
	}

	const char* ssid = mJsonConfig["ssid"].as<const char*>();
	const char* password = mJsonConfig["password"].as<const char*>();

	stream.printf("Connecting to SSID: %s, Key: %s\n\r", ssid, password);

	if(WiFi.begin(ssid, password) == WL_CONNECT_FAILED) {
		stream.printf("Unable to connect with SSID: %s, Key: %s\n\r", ssid, password);
		return;
	}

	auto startTimestamp = millis();
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		stream.print(".");

		if((millis() - startTimestamp) > 10000) {
			stream.println("Connection timeout reached!");
			break;
		}
	}
	stream.println("");

	if (WiFi.status() == WL_CONNECTED) {
		stream.println("WiFi connected!");
		stream.printf("\tIPv4: %s\n\r", WiFi.localIP().toString().c_str());
		stream.printf("\tIPv6: %s", WiFi.localIPv6().toString().c_str());
	} else {
		stream.printf("Connection to network \"%s\" with password \"%s\" failed!", ssid, password);
	}

	stream.println("");
}

void BasicNetworkApplication::setupNtp(Stream& stream){

	if (WiFi.status() == WL_CONNECTED) {

		mNtp.ruleDST("CEST", Last, Sun, Mar, 2, 120); // last sunday in march 2:00, timetone +120min (+1 GMT + 1h summertime offset)
		mNtp.ruleSTD("CET", Last, Sun, Oct, 3, 60); // last sunday in october 3:00, timezone +60min (+1 GMT)
		mNtp.updateInterval(10 * 1000);
		mNtp.begin();
		mNtpStarted = true;

	}

}

int BasicNetworkApplication::handleCliHelp(CLIClient* dev, int, char**) {
	dev->println("Available commands:");
	dev->println("  help: Shows this help message");
	dev->println("  connect_wifi <SSID> <PASSWORD>: Shows this help message");
	dev->println("  config print|reset|delete: Prints, resets (set to \"{}\") or deletes the config file");
	dev->println("  config_set_parameter <KEY> <VALUE>: Sets key value pair in config");
	return 0;
}

int BasicNetworkApplication::handleCliStatus(CLIClient* dev, int, char**) {
	dev->println("Status:");
	dev->printf("  Uptime: %lu\n\r", millis());\
	dev->printf("  Time: %s\n\r", mNtp.formattedTime("%T - %F"));\
	dev->printf("  IPv4: %s\n\r", WiFi.localIP().toString().c_str());
	dev->printf("  IPv6: %s\n\r", WiFi.localIPv6().toString().c_str());
	return 0;
}

int BasicNetworkApplication::handleCliConnectWifi(CLIClient* dev, int argc, char** argv) {

	EXPECT_NUM_OF_ARGS(2);

	setConfigValue("ssid", argv[1]);
	setConfigValue("password", argv[2]);
	writeConfig();

	setupWifi(*dev);
	return 0;
}

int BasicNetworkApplication::handleCliConfig(CLIClient* dev, int argc, char** argv) {

	EXPECT_NUM_OF_ARGS(1);

	if (IS_SUBCOMMAND("print")) {
		char c;
		File file = LittleFS.open(CONFIG_PATH);
		dev->printf("Config file %s: ", CONFIG_PATH);
		while (file.readBytes(&c, sizeof(char))){
			dev->print(c);
		}
	} else if (IS_SUBCOMMAND("reset")) {
		dev->printf("Resetting config file %s to {}", CONFIG_PATH);
		File file = LittleFS.open(CONFIG_PATH, FILE_WRITE);
		file.print("{}");
	} else if (IS_SUBCOMMAND("delete")) {
		dev->printf("Deleting config file %s", CONFIG_PATH);
		LittleFS.remove(CONFIG_PATH);
	} else {
		dev->printf("Input Error: Subcommand \"%s\" unknown!", argv[1]);
	}

	dev->println("");

	return 0;
}

int BasicNetworkApplication::handleCliConfigSetParameter(CLIClient* dev, int argc, char** argv) {

	EXPECT_NUM_OF_ARGS(2);

	const char* key = argv[1];
	const char* value = argv[2];

	setConfigValue(key, value, *dev);
	writeConfig(*dev);

	return 0;
}


void BasicNetworkApplication::readConfig() {

	if(!LittleFS.exists(CONFIG_PATH)){
		File file = LittleFS.open(CONFIG_PATH, FILE_WRITE);
		file.print("{}");
		Serial.println("Creating empty config file");
	} else {
		File file = LittleFS.open(CONFIG_PATH);
		deserializeJson(mJsonConfig, file);
	}
}

void BasicNetworkApplication::writeConfig(Stream& stream) {

	char fileBuffer[256] = {0};
	serializeJson(mJsonConfig, fileBuffer);

	stream.printf("Writing config to file (\"%s\"): %s\n\r", CONFIG_PATH, fileBuffer);

	File file = LittleFS.open(CONFIG_PATH, FILE_WRITE);
	file.print(fileBuffer);
	file.flush();

	stream.println("Config written successfully!");

}

void BasicNetworkApplication::setConfigValue(const char* key, const char* value, Stream& stream){

	stream.printf("Setting config parameter: %s=%s", key, value);
	mJsonConfig[key] = value;

}

void BasicNetworkApplication::setup() {

	setupSerial();
	setupFilesystem();
	readConfig();
	setupWifi();
	setupNtp();
	setupCli();

	mWifiCliServer.begin();
}

void BasicNetworkApplication::loop() {

	if (mNtpStarted) {
		mNtp.update();
	}

	CLI.process();

	if (!mWifiCliClient) {
		mWifiCliClient= mWifiCliServer.available();
		if (mWifiCliClient) {
			CLI.addClient(mWifiCliClient);
		}
	}
}
