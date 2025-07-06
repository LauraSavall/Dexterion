#include "misc.hpp"
#include "../util/config.hpp"
#include <atomic>	 // For thread-safe boolean flags
#include <windows.h> // For GetAsyncKeyState, mouse_event
#include <thread>	 // For std::thread
#include <chrono>	 // For timing (milliseconds, steady_clock)
#include "timeddraw.hpp"

// Initialize static variables
std::vector<misc::DamageData> misc::damageList;
std::atomic<float> misc::g_currentSpeed2D = {0.0f};

// Define the shared item ESP list and mutex
std::vector<misc::ItemESPData> misc::itemESPList;
std::mutex misc::itemESPListMutex;
std::mutex misc::itemESPFilterMutex; // Definition of the new mutex

namespace
{ // Anonymous namespace for internal linkage
	std::atomic<bool> g_autoBhopEnabled = false;
	std::atomic<bool> g_stopBhopThread = false;
	std::atomic<bool> g_stopItemESPThread = false;
	std::atomic<bool> isItemESPEnabled = false;
	std::thread g_bhopThread;
	std::thread g_speedInfoThread;
	std::thread g_itemESPThread;

	constexpr int FL_ONGROUND = (1 << 0); // Player is standing on ground.

	// Worker function for the bunny hop thread (keep it internal)
	//     void bhopWorker(LocalPlayer localPlayer) {
	//         while (!g_stopBhopThread) { // Loop until explicitly told to stop

	// 			//if (onGround) {
	//                     mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
	// 					int randomValue = getRandomInt(14444, 16000);

	// 					//std::this_thread::sleep_for(std::chrono::microseconds(15626));
	// 					std::this_thread::sleep_for(std::chrono::microseconds(randomValue));
	// 					//std::this_thread::sleep_for(std::chrono::milliseconds(16));

	//                 }
	// 				//std::this_thread::sleep_for(std::chrono::microseconds(16625));

	//       //  }

	void speedInfoWorker(LocalPlayer localPlayer)
	{
		std::vector<float> highSpeeds;
		highSpeeds.reserve(50000);

		while (!g_stopBhopThread)
		{
			Vector3 playerVelocity = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);
			float speed2D = std::sqrt(playerVelocity.x * playerVelocity.x + playerVelocity.y * playerVelocity.y);
			misc::g_currentSpeed2D.store(speed2D);
			if (speed2D > 251.0f)
			{
				highSpeeds.push_back(speed2D);
			}
			std::this_thread::sleep_for(std::chrono::microseconds(8333));
		}
		// misc::g_currentSpeed2D.store(0.0f);
		if (!highSpeeds.empty())
		{
			float sum = std::accumulate(highSpeeds.begin(), highSpeeds.end(), 0.0f);
			float averageSpeed = sum / highSpeeds.size();
			misc::g_currentSpeed2D.store(averageSpeed);
		}
		else
		{
			misc::g_currentSpeed2D.store(0.0f);
		}
	}

	void itemESPWorker(MemoryManagement::moduleData client)
	{

		while (!g_stopItemESPThread)
		{
			// std::string speedMessage = "is ON? " + std::to_string(g_stopItemESPThread) + "isItemESPEnabled: " + std::to_string(isItemESPEnabled);
			// Logger::info(speedMessage);
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
			// timeddraw::ProcessTimedDraws();
			C_CSPlayerPawn C_CSPlayerPawn(client.base);
			CGameSceneNode CGameSceneNode;
			view_matrix_t viewMatrix = MemMan.ReadMem<view_matrix_t>(client.base + offsets::clientDLL["dwViewMatrix"]);

			for (int i = 66; i < 6556; i++)
			{
				C_CSPlayerPawn.value = i;
				C_CSPlayerPawn.getListEntry();
				if (C_CSPlayerPawn.listEntry == 0)
					continue;
				C_CSPlayerPawn.getPlayerPawn();
				if (C_CSPlayerPawn.playerPawn == 0)
					continue;
				if (C_CSPlayerPawn.getOwner() != -1)
					continue;

				uintptr_t entity = MemMan.ReadMem<uintptr_t>(C_CSPlayerPawn.playerPawn + 0x10);
				uintptr_t designerNameAddy2 = MemMan.ReadMem<uintptr_t>(entity + 0x18);






				//if (lowerCaseName2.empty()) {
				//	continue;
				//}


				uintptr_t designerNameAddy = MemMan.ReadMem<uintptr_t>(entity + 0x20);
				char designerNameBuffer[MAX_PATH]{};
				MemMan.ReadRawMem(designerNameAddy, designerNameBuffer, MAX_PATH);
				std::string name = std::string(designerNameBuffer);
				std::string lowerCaseName = utils::toLower(name); 

				char designerNameBuffer2[MAX_PATH]{};
				MemMan.ReadRawMem(designerNameAddy2, designerNameBuffer2, MAX_PATH);

				std::string name2 = std::string(designerNameBuffer2);
				std::string lowerCaseName2 = utils::toLower(name2);

				if (!name2.empty()) {
					name = name + "_" + name2;
				}

				//bool isUsable = MemMan.ReadMem<bool>(C_CSPlayerPawn.playerPawn + 3372);
				//uintptr_t displayTextActualPtr = MemMan.ReadMem<uintptr_t>(C_CSPlayerPawn.playerPawn + 3376);
				//std::string displayText = "N/A";

				//if (displayTextActualPtr) {
				//	char displayTextBuffer[MAX_PATH]{};
				//	MemMan.ReadRawMem(displayTextActualPtr, displayTextBuffer, MAX_PATH);
				//	if (displayTextBuffer[0] != '\0') { // Check if string is not empty
				//		displayText = std::string(displayTextBuffer);
				//	}
				//}

				//if (isUsable || displayText != "N/A") {
				//	name = "weapon_" + name;
				//	lowerCaseName = "weapon_" + lowerCaseName;
				//}
			

				bool shouldProcess = false;
				{
					std::lock_guard<std::mutex> lock(misc::itemESPFilterMutex); // Protect access to miscConf.itemESPFilter
					for (const auto& filterStr : miscConf.itemESPFilter) {
						std::string lowerCaseFilterStr = utils::toLower(filterStr); // Convert filter string to lowercase
						if (lowerCaseName.find(lowerCaseFilterStr) != std::string::npos) { // If filter string IS found
							shouldProcess = true; // Mark for processing (inclusion)

							break;
						}
					}
				} // Lock is released here
				if (!shouldProcess) { // If NO filter string was found, then skip the item
					continue;
				}

				if (strstr(name.c_str(), "weapon_"))
				{
					name.erase(0, 7);
				}
				else
				{
					continue;
				}

				//if (isUsable || displayText != "N/A") {
				//	std::string buttonMessage = "Potential Usable Entity: DesignerName='" + name2 +
				//		"', DisplayText='" + displayText +
				//		"', IsUsable=" + (isUsable ? "true" : "false");
				//	Logger::info(buttonMessage);
				//}


				//int paintKit = MemMan.ReadMem<int>(entity + 5624);

				//if (paintKit > 0)
				//{
				//	continue;
				//}


				

				//int health = MemMan.ReadMem<int>(entity + 836);


		
					//std::string speedMessage = "add: " + std::to_string(entity) + ", " +name.c_str() + ", whast dat: " + lowerCaseName2;
					//Logger::info(speedMessage);


				

				

				CGameSceneNode.value = C_CSPlayerPawn.getCGameSceneNode();
				CGameSceneNode.getOrigin();
				CGameSceneNode.origin = CGameSceneNode.origin.worldToScreen(viewMatrix);

				if (CGameSceneNode.origin.z >= 0.01f)
				{
					// Collect item data into the shared list
					std::lock_guard<std::mutex> lock(misc::itemESPListMutex);
					// Collect item data into the shared list
					misc::ItemESPData itemData = {ImVec2(CGameSceneNode.origin.x, CGameSceneNode.origin.y), CGameSceneNode.getOrigin(), name};
					misc::itemESPList.push_back(itemData);
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			misc::itemESPList.clear();
		}
	}

	void bhopWorker(DWORD_PTR base, LocalPlayer localPlayer)
	{

		// int trig = 0;
		// uintptr_t ctrl = localPlayer.getPlayerController();
		// uint32_t lastTick = MemMan.ReadMem<uint32_t>( ctrl + clientDLL::CBasePlayerController_["m_nTickBase"] );

		// INPUT input;
		// input.type = INPUT_KEYBOARD;
		// input.ki.wVk = 0x36 ; // a lot of keys won't work, F keys work fine and F13-24 aren't used for anything
		// input.ki.dwExtraInfo = 0;
		// input.ki.time = 0;

		// 28 and 48 is the frame time

		// 6, 10 and 46
		int aaa = 0;

		uintptr_t globalVarsAddr = MemMan.ReadMem<uintptr_t>(base + offsets::clientDLL["dwGlobalVars"]);
		int lastTick = 0; // Keep track of the last tick we processed

		// mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
		while (!g_stopBhopThread)
		{
			int bbb = 0;
			// std::this_thread::sleep_for(std::chrono::microseconds(100));
			//  int currentTick = MemMan.ReadMem<int>(globalVarsAddr + 81);
			//  		//		std::string speedMessage = "tick rn: " + std::to_string(currentTick);
			//  	//	Logger::info(speedMessage);

			lastTick = MemMan.ReadMem<int>(globalVarsAddr + 81);

			//  int currentTick = MemMan.ReadMem<int>(globalVarsAddr + 81);
			//  		//		std::string speedMessage = "tick rn: " + std::to_string(currentTick);
			//  	//	Logger::info(speedMessage);

			// float frame_time = MemMan.ReadMem<float>(globalVarsAddr + 6);

			// 					constexpr float normalInterval = 1.0f / 64.0f;    // ≃0.015625
			// constexpr float lagThreshold  = 0.016f;           // anything above 20ms we count as lag

			// // scan offsets from 0 to, say, 128 bytes in 4-byte steps
			// for (int off = 0; off < 223; off ++) {
			// 	if(off == 64 || off == 60 ||off == 10 ||off == 7 ||off == 11 ||off == 47 ||off == 31 ||off == 6 ||off == 1 ||off == 25 ||off == 50 ||off == 54 ||off == 78 ||off == 73 ||off == 5 ||off == 9 ||off == 69 ||off == 46 ||off == 59 ||off == 62||off == 61||off == 30||off == 26||off == 79||off == 45||off == 33||off == 32||off == 51){
			// 		continue;
			// 	}
			//     float val = MemMan.ReadMem<float>(globalVarsAddr + off);
			//     if (val > lagThreshold && val < 0.070f) {
			//         // this field grows when the server is lagging
			// 		std::string speedMessage =  "Candidate lag-field at offset i:" + std::to_string(off) + " is  " + std::to_string(val) + "s";
			// 		Logger::info(speedMessage);
			//     }
			// }

			static int delta = 120;

			int flagss = localPlayer.getFlags();
			bool onGrounds = (flagss & FL_ONGROUND);
			// Vector3 playerVelocity = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);

			if (onGrounds)
			{
				int currentTicks = MemMan.ReadMem<int>(globalVarsAddr + 81);
				int one_or_zero = miscConf.trigg;
				float val = MemMan.ReadMem<float>(globalVarsAddr + 48);
				//	if(val > 0.016625){
				//		one_or_zero = 0;
				//	}
				while (lastTick + one_or_zero >= currentTicks)
				{
					// Logger::info("loop, " + std::to_string(bbb));
					bbb++;
					currentTicks = MemMan.ReadMem<int>(globalVarsAddr + 81);
					// std::this_thread::sleep_for(std::chrono::nanoseconds(1));
				}
				std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
				// mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);
				mouse_event(MOUSEEVENTF_WHEEL, 0, 0, delta, 0);
				std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
				delta = -delta;
				//	std::string speedMessage =  "Candidate lag-field at offset  " + std::to_string(val) + "s";
				aaa++;
				// Logger::info("num, " + std::to_string(aaa) + " OZ: " + std::to_string(one_or_zero));

				lastTick = currentTicks;
			}
			// std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			continue;

			// if (currentTick > lastTick) { // Check if it's a new, valid tick
			//     int flags = localPlayer.getFlags();
			//     bool onGround = (flags & FL_ONGROUND);

			//     if (onGround) {
			//         // Send jump input ONCE per tick when on ground
			//         // Using -120 for mouse wheel down (standard jump)
			// 		std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
			//         mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);
			//     }
			//     lastTick = currentTick; // Update the last processed tick
			// } else if (currentTick == 0 && lastTick != 0) {
			//      // Reset lastTick if the game resets tick count (e.g., map change)
			//      lastTick = 0;
			// }
			//         std::this_thread::sleep_for(std::chrono::microseconds(25));
			// 		continue;

			// int flags = localPlayer.getFlags();
			// bool onGround = (flags & FL_ONGROUND);

			// if (onGround) {
			// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
			// 	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
			// 	Logger::info("ad");
			// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
			// 	continue;
			// }
			// std::this_thread::sleep_for(std::chrono::microseconds(16625));

			//  }

			// int flags = localPlayer.getFlags();
			// bool onGround = (flags & FL_ONGROUND);

			Vector3 playerVelocitys = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);

			if (playerVelocitys.z == 0.0f)
			{
				int currentTick = MemMan.ReadMem<int>(globalVarsAddr + 81);
				if (currentTick > lastTick)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
					mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);
					lastTick = currentTick;
				}

				// std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
				//  Logger::warn("first jump?");
				//  std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
				//  mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);
				//  lastTime = std::chrono::high_resolution_clock::now();
				//  //mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);

				// while(onGround && elapsed < 31250){
				// //playerVelocity = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);
				// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleepForZero));
				// 	//lastTime = std::chrono::high_resolution_clock::now();
				// 	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 120, 0);
				// 	Logger::info("jmp sent");
				//  flags = localPlayer.getFlags();
				// onGround = (flags & FL_ONGROUND);
				// now = std::chrono::high_resolution_clock::now();
				// elapsed = std::chrono::duration_cast<
				// 	std::chrono::microseconds
				// >(now - lastTime).count();

				// }

				// 	//lastTime = std::chrono::high_resolution_clock::now();
				// 	Logger::success("too much, sleepin..");
				// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
				// 	continue;
			}

			// uint32_t curTick = MemMan.ReadMem<uint32_t>( ctrl + clientDLL::CBasePlayerController_["m_nTickBase"] );

			// if (curTick != lastTick) {
			//  compute elapsed real time
			//  auto now      = std::chrono::high_resolution_clock::now();
			//  auto elapsed  = std::chrono::duration_cast<
			//                      std::chrono::microseconds
			//                  >(now - lastTime).count();

			// auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count();
			// Logger::info("… after {} μs", us);

			// if (playerVelocity.z  <= -145.0f || playerVelocity.z == 0.0f) {
			// 	std::this_thread::sleep_for(std::chrono::microseconds(15625));
			// 	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
			// }

			// if (playerVelocity.z  <= miscConf.bhopJumpVelocityThreshold) {
			// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
			// 	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
			// 	std::this_thread::sleep_for(std::chrono::microseconds(miscConf.bhopSleep));
			// 	continue;
			// 	//SendInput(1, &input, sizeof(INPUT));

			// 	//continue;
			// 	//std::string speedMessage = "jmp: " + std::to_string(miscConf.bhopJumpVelocityThreshold);
			// 	//trig=1000;
			// 	//Logger::info(speedMessage);
			// }

			// float speed2 = std::sqrt(playerVelocity.x * playerVelocity.x + playerVelocity.y * playerVelocity.y);
			// std::string speedMessage = "Speed: " + std::to_string(speed2);
			// Logger::info(speedMessage);

			// Vector3 playerVelocity = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);

			//    lastTick = curTick;
			//     lastTime = now;
			// }

			// if (flags != -1) {
			// mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
			// std::this_thread::sleep_for(std::chrono::microseconds(15626));
			// std::this_thread::sleep_for(std::chrono::milliseconds(16));

			//	}
		}
	}

}

bool misc::isGameWindowActive()
{
	HWND hwnd = GetForegroundWindow();
	if (hwnd)
	{
		char windowTitle[256];
		GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
		return std::string(windowTitle).find("Counter-Strike 2") != std::string::npos;
	}
	return false;
}

static bool isBhopToggledActive = false;

static bool qKeyWasPressed = false;
static std::chrono::steady_clock::time_point qPressStartTime;
static const std::chrono::milliseconds holdDurationThreshold(17);

void misc::startBhopThread(DWORD_PTR base, LocalPlayer localPlayer)
{
	if (!g_bhopThread.joinable())
	{															   // Check if thread is not already running
		g_stopBhopThread = false;								   // Reset the stop flag
		g_bhopThread = std::thread(bhopWorker, base, localPlayer); // Create and start the thread
																   // Optional: Log thread start
																   // Logger::info("[Misc] BunnyHop thread started.");
	}
	if (!g_speedInfoThread.joinable())
	{																   // Check if thread is not already running
		g_stopBhopThread = false;									   // Reset the stop flag
		g_speedInfoThread = std::thread(speedInfoWorker, localPlayer); // Create and start the thread
																	   // Optional: Log thread start
																	   // Logger::info("[Misc] BunnyHop thread started.");
	}
}

void misc::startItemESPThread(MemoryManagement::moduleData client)
{
	if (!g_itemESPThread.joinable())
	{
		g_stopItemESPThread = false;
		g_itemESPThread = std::thread(itemESPWorker, client);
	}
}

void misc::stopItemESPThread()
{

	if (g_itemESPThread.joinable())
	{
		g_stopItemESPThread = true;
		try
		{

			g_itemESPThread.join(); // Wait for the thread to finish execution
									// Optional: Log thread stop
									// Logger::info("[Misc] BunnyHop thread stopped and joined.");
		}
		catch (const std::system_error &e)
		{
			// Handle potential errors during join (e.g., if thread wasn't joinable)
		}
	}
	isItemESPEnabled = false;
}

void misc::stopBhopThread()
{
	if (g_bhopThread.joinable())
	{
		g_stopBhopThread = true; // Signal the thread to stop its loop
		try
		{
			g_bhopThread.join(); // Wait for the thread to finish execution
								 // Optional: Log thread stop
								 // Logger::info("[Misc] BunnyHop thread stopped and joined.");
		}
		catch (const std::system_error &e)
		{
			// Handle potential errors during join (e.g., if thread wasn't joinable)
		}
	}
	if (g_speedInfoThread.joinable())
	{
		g_stopBhopThread = true; // Signal the thread to stop its loop
		try
		{
			g_speedInfoThread.join(); // Wait for the thread to finish execution
									  // Optional: Log thread stop
									  // Logger::info("[Misc] BunnyHop thread stopped and joined.");
		}
		catch (const std::system_error &e)
		{
			// Handle potential errors during join (e.g., if thread wasn't joinable)
		}
	}
	g_autoBhopEnabled = false; // Ensure state is off on exit
}

static std::chrono::steady_clock::time_point lastToggleTime = std::chrono::steady_clock::now(); // Time of the last successful toggle
static const std::chrono::milliseconds toggleCooldown(360);										// 50ms cooldown duration

void misc::bunnyHop(DWORD_PTR base, LocalPlayer localPlayer)
{
	if (!isGameWindowActive())
		return;

	bool isQPressedNow = (GetAsyncKeyState('Q') & 0x8000) != 0;
	if (!isQPressedNow)
		return;

	auto now = std::chrono::steady_clock::now();
	if (now - lastToggleTime < toggleCooldown)
	{
		return;
	}
	else
	{

		lastToggleTime = now;
	}

	// Cooldown passed, toggle
	g_autoBhopEnabled = !g_autoBhopEnabled;
	if (g_autoBhopEnabled)
	{
		startBhopThread(base, localPlayer);
	}
	else
	{
		stopBhopThread();
	}
}

void misc::droppedItem(C_CSPlayerPawn C_CSPlayerPawn, CGameSceneNode CGameSceneNode, view_matrix_t viewMatrix)
{
	// startItemESPThread(C_CSPlayerPawn, CGameSceneNode, viewMatrix);

	if (!overlayESP::isMenuOpen())
	{
		if (!misc::isGameWindowActive())
			return;
	}

	// std::filesystem::path desktopPath = std::filesystem::path(getenv("USERPROFILE")) / "Desktop";
	// std::filesystem::path logFilePath = desktopPath / "LOG1.txt";
	// std::ofstream logFile(logFilePath, std::ios_base::app); // Open in append mode

	// if (!logFile.is_open()) {
	// 	// Optionally, handle the error e.g., log to console that file couldn't be opened
	// 	 Logger::error("Failed to open LOG1.txt for writing.");
	// 	return;
	// }

	for (int i = 65; i < 1856; i++)
	{
		// std::this_thread::sleep_for(std::chrono::microseconds(10));
		//  Entity
		C_CSPlayerPawn.value = i;
		C_CSPlayerPawn.getListEntry();
		if (C_CSPlayerPawn.listEntry == 0)
			continue;
		C_CSPlayerPawn.getPlayerPawn();
		if (C_CSPlayerPawn.playerPawn == 0)
			continue;
		if (C_CSPlayerPawn.getOwner() != -1)
			continue;

		// Entity name
		uintptr_t entity = MemMan.ReadMem<uintptr_t>(C_CSPlayerPawn.playerPawn + 0x10);
		uintptr_t designerNameAddy = MemMan.ReadMem<uintptr_t>(entity + 0x20);

		char designerNameBuffer[MAX_PATH]{};
		MemMan.ReadRawMem(designerNameAddy, designerNameBuffer, MAX_PATH);

		std::string name = std::string(designerNameBuffer);
		// std::string speedMessage = "id: " + std::to_string(i) + ", name:" +name;
		// Logger::info(speedMessage);

		// auto now = std::chrono::system_clock::now();
		// auto in_time_t = std::chrono::system_clock::to_time_t(now);

		// Format time as string
		// std::tm buf{};
		// localtime_s(&buf, &in_time_t); // Use localtime_s for safety
		// std::ostringstream oss;
		// oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
		// std::string timestamp = oss.str();

		// std::string logMessage = timestamp + " - id: " + std::to_string(i) + ", name:" + name;
		// logFile << logMessage << std::endl;

		if (strstr(name.c_str(), "weapon_"))
		{
			name.erase(0, 7);
		}

		// else if (strstr(name.c_str(), "_projectile")) name.erase(name.length() - 11, 11);
		// else if (strstr(name.c_str(), "baseanimgraph")) name = "defuse kit";
		else
		{
			continue;
		}

		if (name.find("te") == std::string::npos)
		{
			continue;
		}

		// Origin position of entity
		CGameSceneNode.value = C_CSPlayerPawn.getCGameSceneNode();
		CGameSceneNode.getOrigin();
		CGameSceneNode.origin = CGameSceneNode.origin.worldToScreen(viewMatrix);

		// Drawing

		if (CGameSceneNode.origin.z >= 0.01f)
		{
			ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
			auto [horizontalOffset, verticalOffset] = getTextOffsets(textSize.x, textSize.y, 2.f);

			ImFont *gunText = {};
			if (std::filesystem::exists(DexterionSystem::weaponIconsTTF))
			{
				gunText = imGuiMenu::weaponIcons;
				name = gunIcon(name);
			}
			else
				gunText = imGuiMenu::espNameText;

			ImColor itemColor = ImColor(espConf.attributeColours[0], espConf.attributeColours[1], espConf.attributeColours[2]);
			itemColor = ImColor(255, 0, 0, 255); // RED color for "elite" items

			ImGui::GetBackgroundDrawList()->AddText(gunText, 12, {CGameSceneNode.origin.x - horizontalOffset, CGameSceneNode.origin.y - verticalOffset}, itemColor, name.c_str());
		}
	}
}

void misc::droppedItemSeparateThread(MemoryManagement::moduleData client)
{
	if (!isItemESPEnabled)
	{
		isItemESPEnabled = true;
		startItemESPThread(client);
	}
}

void misc::disableDroppedItemSeparateThread()
{
	if (isItemESPEnabled)
	{
		isItemESPEnabled = false;
		misc::itemESPList.clear();
		stopItemESPThread();
	}

	//}
}

// Add damage for a player
void misc::addDamage(std::string name, int damage, int hits, uintptr_t playerHandle)
{
	bool found = false;

	for (auto &entry : damageList)
	{
		if (entry.playerHandle == playerHandle)
		{
			entry.damage += damage;
			entry.hits += hits;
			found = true;
			break;
		}
	}

	if (!found)
	{
		damageList.push_back(DamageData(name, damage, hits, playerHandle));
	}

	std::sort(damageList.begin(), damageList.end());
}

void misc::updatePlayerDamage(std::string name, int totalDamage, int totalHits, uintptr_t playerHandle)
{
	bool found = false;

	for (auto &entry : damageList)
	{
		if (entry.playerHandle == playerHandle)
		{
			entry.damage = totalDamage;
			entry.hits = totalHits;
			found = true;
			break;
		}
	}

	if (!found)
	{
		damageList.push_back(DamageData(name, totalDamage, totalHits, playerHandle));
	}

	std::sort(damageList.begin(), damageList.end());
}

// Clear the damage list (for round reset)
void misc::clearDamageList()
{
	damageList.clear();
}

// Display the damage list UI
void misc::displayDamageList()
{
	// Skip if disabled or game window not active
	if (!miscConf.damageList)
		return;
	if (!overlayESP::isMenuOpen())
	{
		if (!misc::isGameWindowActive())
			return;
	}

	// Always show the window (like spectator list), even if empty
	// This allows users to see it during gameplay

	// Set window position and size
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	ImGui::SetNextWindowPos({(float)GetSystemMetrics(SM_CXSCREEN) - 200.f, 200.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({180.f, 180.f}, ImGuiCond_FirstUseEver);

	// Create window
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
	if (ImGui::Begin("Damage List", nullptr, flags))
	{
		// Header
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Damage This Round");
		ImGui::Separator();

		if (damageList.empty())
		{
			// Show a message when no damage to display
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No damage recorded yet");
		}
		else
		{
			// Display each player and their damage
			int count = 0;
			for (const auto &player : damageList)
			{
				// Limit to top 5 players
				// if (count >= 5)
				// 	break;

				// // Display player name
				// ImGui::Text("%s:", player.playerName.c_str());
				// ImGui::SameLine();

				// ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", player.damage);

				// count++;

				if (count >= 5)
					break;

				// ImGui::Text("%s:", player.playerName.c_str());
				ImGui::Text("%s:", player.playerName.substr(0, 12).c_str());
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d|%d", player.damage, player.hits);

				count++;
			}
		}

		// Add clear button when in menu
		if (overlayESP::isMenuOpen())
		{
			ImGui::Separator();
			if (ImGui::Button("Clear List"))
			{
				clearDamageList();
			}
		}

		ImGui::End();
	}
	ImGui::PopStyleVar();
}
