#include "UnitUpdater.h"

int main()
{
	std::cout << "Hello CMake." << std::endl;

	// Todo - add settings file instance, 
	// Load settings file. 

	// Pass files into UnitUpdater

	UnitUpdater uu;
	bool broadcastReceived = false;

	if (uu.ListenForInterrupt())
	{
		broadcastReceived = true;
	}

	if (broadcastReceived)
	{
		// handle tcp stuffs and listen for transactions. 
	}

	// Close after timeout

	// proper clean up
	uu.Close();

	return 0;
}