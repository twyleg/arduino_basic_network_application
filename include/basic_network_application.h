// Copyright (C) 2022 twyleg
#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <NTP.h>
#include <CLI.h>

#include <ArduinoJson.h>

#define IS_COMMAND(cmd) strcmp(argv[0], cmd) == 0
#define IS_SUBCOMMAND(scmd) strcmp(argv[1], scmd) == 0

#define EXPECT_NUM_OF_ARGS(num) if(argc != (num)+1) {\
	dev->printf("Input error: command expects %d argument(s), got %d\n\r", num, argc-1);\
	return 0;\
	}

class BasicNetworkApplication {

private:

	void setupSerial();
	void setupCli();
	void setupFilesystem();
	void setupWifi(Stream& = Serial);
	void setupNtp(Stream & = Serial);

	void readConfig();

	WiFiUDP mNtpWifiUdp;
	WiFiServer mWifiCliServer;
	WiFiClient mWifiCliClient;

protected:
	NTP mNtp;
	bool mNtpStarted;

	DynamicJsonDocument mJsonConfig;

public:

	BasicNetworkApplication();

	virtual int handleCliHelp(CLIClient* dev, int argc, char** argv);
	virtual int handleCliStatus(CLIClient* dev, int argc, char** argv);
	int handleCliConnectWifi(CLIClient* dev, int argc, char** argv);
	int handleCliConfig(CLIClient* dev, int argc, char** argv);
	int handleCliConfigSetParameter(CLIClient* dev, int argc, char** argv);

	void writeConfig(Stream& = Serial);
	void setConfigValue(const char* key, const char* value, Stream& = Serial);

	void setup();
	void loop();

};
