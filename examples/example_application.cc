// Copyright (C) 2022 twyleg
#include "example_application.h"

namespace {

int handleCli(CLIClient* dev, int argc, char** argv, void* ptr) {
	ExampleApplication* exampleApplication = reinterpret_cast<ExampleApplication*>(ptr);

	if (IS_COMMAND("count")) {
		return exampleApplication->handleCliCount(dev, argc, argv);
	}

	return -1;
}

bool handleIncreaseTimer(void* ptr) {
	ExampleApplication* exampleApplication = reinterpret_cast<ExampleApplication*>(ptr);
	return exampleApplication->handleIncreaseTimer();
}

}

ExampleApplication::ExampleApplication() :
	mWebServer(80)
{
}

int ExampleApplication::handleCliHelp(CLIClient* dev, int argc, char** argv) {
	NetworkApplication::handleCliHelp(dev, argc, argv);

	dev->println("  count: Print the current counter value");
	return 0;
}

int ExampleApplication::handleCliStatus(CLIClient* dev, int argc, char** argv) {
	NetworkApplication::handleCliStatus(dev, argc, argv);

	dev->printf("  count: %d\n\r", mCount);
	return 0;
}

int ExampleApplication::handleCliCount(CLIClient* dev, int, char**) {
	dev->printf("Count: %d\n\r", mCount);
	return 0;
}

bool ExampleApplication::handle(WebServer& server, HTTPMethod requestMethod, String requestUri) {
	char buf[32];
	sprintf(buf, "Count: %d", mCount);
	server.send(200, "text/plain", buf);
	return true;
}

bool ExampleApplication::handleIncreaseTimer() {
	mCount++;
	return true;
}

void ExampleApplication::setup() {
	NetworkApplication::setup();

	CLI.addCommand("count", ::handleCli, this);

	mTimer.every(1000, ::handleIncreaseTimer, this);

	mWebServer.addHandler(this);
	mWebServer.begin();
}

void ExampleApplication::loop() {
	NetworkApplication::loop();

	mTimer.tick();
	mWebServer.handleClient();
}
