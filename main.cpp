#include "UnitUpdater.h"

int main()
{
	std::cout << "Hello CMake." << std::endl;

	// Todo - add settings file instance, 
	// Load settings file. 

	// Pass files into UnitUpdater

	UnitUpdater uu;
	uu.Setup(8000, 8005);
	int start = uu.Start();

	if (start < 0)
	{
		std::cout << "Failed to start!\n";
	}

	bool broadcastReceived = false;

	if (uu.ListenForInterrupt())
	{
		broadcastReceived = true;
	}

	if (broadcastReceived)
	{
		// handle tcp stuffs and listen for transactions. 
	}
	else
	{
		std::cout << "NOTICE: Broadcast not found!\n";
	}

	// Close after timeout

	// proper clean up
	uu.Close();

	return 0;
}