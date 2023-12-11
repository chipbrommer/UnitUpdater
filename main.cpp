#include "UnitUpdater.h"

int main()
{
	// Create unit updater instance
	UnitUpdater uu;

	std::string pcPath = "C:\\Users\\chipb\\source\\repos\\chipbrommer\\UnitUpdater\\test_files\\settings.json";
	std::string wkPath = "C:\\Users\\cbrommer\\source\\repos\\UnitUpdater\\test_files\\settings.json";

	// Setup
	if (uu.Setup(wkPath) < 0)
	{
		std::cout << "Failed to setup Updater. Closing\n";
		uu.Close();
		return -1;
	}

	std::cout << "Listening for Broadcast!\n";

	bool interruptReceived = false;

	// Listen for boot interrupt. 
	if (uu.ListenForInterrupt() == 1)
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