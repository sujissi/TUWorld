#include "pch.h"
#include "DummyClinetManager.h"

int main()
{
	SetConsoleCtrlHandler([](DWORD) -> BOOL {
		DummyClientManager::GetInst().Shutdown();
		return TRUE;
	}, TRUE);

	auto& TestMgr = DummyClientManager::GetInst();
	if (!TestMgr.Init())
		return -1;
	try
	{
		TestMgr.Run();
	}
	catch (...)
	{
		TestMgr.Shutdown();
	}
	return 0;
}