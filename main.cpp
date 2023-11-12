#include "UnitUpdater.h"

int main()
{
	std::cout << "------------------------------------\n";
	std::cout << "-           Unit Updater           -\n";
	std::cout << "------------------------------------\n" << std::endl;
	// Todo - add settings file instance, 
	// Load settings file. 

	// Pass files into UnitUpdater

	UnitUpdater uu;
	uu.Setup(8000, 8005);

	std::cout << "Listening for Broadcast!\n";

	bool interruptReceived = false;

	if (uu.ListenForInterrupt())
	{
		interruptReceived = true;
	}

	if (interruptReceived)
	{
		// handle tcp stuffs and listen for transactions. 
		std::cout << "\nHELLO: Broadcast found!\n";
	}
	else
	{
		std::cout << "\nNOTICE: \tBroadcast not found!\n";
		std::cout << "\t\tStarting OFS!\n" << std::endl;
	}

	// Close after timeout

	// proper clean up
	uu.Close();

	return 0;
}