#pragma once
namespace commserver
{
	bool CreateCommThread();
	void CloseCommThread();
	bool CreateEvents();
	void CloseHandles();
	int WaitUntilReady(const char* deviceName, unsigned long timeout);
};

