#include "UnitUpdater.h"

int main()
{
	// Create unit updater instance
	UnitUpdater uu;

	// Welcome message
	std::cout << "------------------------------------\n";
	std::cout << "-           Unit Updater           -\n";
	std::cout << "------------------------------------\n" << std::endl;

	// Setup
	uu.Setup("C:\\Users\\cbrommer\\Desktop\\DesktopItems\\settings.json");

	std::cout << "Listening for Broadcast!\n";

	bool interruptReceived = false;

	// Listen for boot interrupt. 
	if (uu.ListenForInterrupt())
	{
		interruptReceived = true;
	}

	// If interrupt received, start the server for communication. else notify and close
	if (!interruptReceived)
	{
		std::cout << "\nNOTICE: \tBroadcast not found!\n";
		std::cout << "\t\tStarting OFS!\n" << std::endl;
	}
	else
	{
		uu.StartServer();
	}

	// proper clean up
	uu.Close();

	return 0;
}