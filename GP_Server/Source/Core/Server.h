#pragma once

class Server
{
public:
	static Server& GetInst()
	{
		static Server inst;
		return inst;
	}
	~Server() {}

	bool Init();
	void Run();
	void Shutdown();

protected:
	void InitSocket(SOCKET& socket, DWORD dwFlags);

	void NetworkWorkerLoop();
	void HandleCompletionError(ExpOver* ex_over, int32 _id);
	void DoAccept();

protected:
	bool _bRunning = true;
	SOCKET _listenSocket;
	SOCKET _acceptSocket;

	ExpOver _acceptOver;
};
