// Copyright (C) 2022 twyleg
#pragma once

#include <arduino-timer.h>
#include <WebServer.h>

#include "basic_network_application.h"

class ExampleApplication: public NetworkApplication, public RequestHandler {

private:

	int mCount = 0;

	WebServer mWebServer;
	Timer<1> mTimer;

public:

	ExampleApplication();

	int handleCliHelp(CLIClient* dev, int argc, char** argv) override;
	int handleCliStatus(CLIClient* dev, int argc, char** argv) override;
	int handleCliCount(CLIClient* dev, int, char**);

	bool canHandle(HTTPMethod, String) override { return true; };
	bool handle(WebServer& server, HTTPMethod requestMethod, String requestUri) override;

	bool handleIncreaseTimer();

	void setup();
	void loop();

};

