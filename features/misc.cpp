#include "misc.hpp"
#include "../util/config.hpp"
#include <atomic>         // For thread-safe boolean flags
#include <windows.h>      // For GetAsyncKeyState, mouse_event
#include <thread>         // For std::thread
#include <chrono>         // For timing (milliseconds, steady_clock)

// Initialize static variables
std::vector<misc::DamageData> misc::damageList;
std::atomic<float> misc::g_currentSpeed2D = { 0.0f };

namespace { // Anonymous namespace for internal linkage
    std::atomic<bool> g_autoBhopEnabled = false;
    std::atomic<bool> g_stopBhopThread = false;
    std::thread g_bhopThread;



    // Worker function for the bunny hop thread (keep it internal)
//     void bhopWorker(LocalPlayer localPlayer) {
//         while (!g_stopBhopThread) { // Loop until explicitly told to stop

// 			//int flags = localPlayer.getFlags();
// 			//bool onGround = (flags & FL_ONGROUND);
			
// 			//if (onGround) {
//                     mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
// 					int randomValue = getRandomInt(14444, 16000);

// 					//std::this_thread::sleep_for(std::chrono::microseconds(15626)); 
// 					std::this_thread::sleep_for(std::chrono::microseconds(randomValue)); 
// 					//std::this_thread::sleep_for(std::chrono::milliseconds(16));

//                 }
// 				//std::this_thread::sleep_for(std::chrono::microseconds(16625));
            
//       //  }


// } //nd anonymous namespace


void bhopWorker(LocalPlayer localPlayer) {

	//uintptr_t ctrl = localPlayer.getPlayerController();
	//uint32_t lastTick = MemMan.ReadMem<uint32_t>( ctrl + clientDLL::CBasePlayerController_["m_nTickBase"] );
	
    //auto lastTime = std::chrono::high_resolution_clock::now();

	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
	while (!g_stopBhopThread) { 
		Vector3 playerVelocity = MemMan.ReadMem<Vector3>(localPlayer.getPlayerPawn() + clientDLL::C_BaseEntity_["m_vecVelocity"]);

		//uint32_t curTick = MemMan.ReadMem<uint32_t>( ctrl + clientDLL::CBasePlayerController_["m_nTickBase"] );



		//if (curTick != lastTick) {
            // compute elapsed real time
            // auto now      = std::chrono::high_resolution_clock::now();
            // auto elapsed  = std::chrono::duration_cast<
            //                     std::chrono::microseconds
            //                 >(now - lastTime).count();

			// auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count();
			// Logger::info("… after {} μs", us);




			// if (playerVelocity.z  <= -145.0f || playerVelocity.z == 0.0f) {
			// 	std::this_thread::sleep_for(std::chrono::microseconds(15625));
			// 	mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
			// }


			if (playerVelocity.z  <= -250.0f || playerVelocity.z == 0.0f) {
				std::this_thread::sleep_for(std::chrono::microseconds(15625));
				mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
				//Logger::info("123");
			}
			// float speed2 = std::sqrt(playerVelocity.x * playerVelocity.x + playerVelocity.y * playerVelocity.y);
        	// std::string speedMessage = "Speed: " + std::to_string(speed2);
       		// Logger::info(speedMessage);
			float speed2D = std::sqrt(playerVelocity.x * playerVelocity.x + playerVelocity.y * playerVelocity.y);
            misc::g_currentSpeed2D.store(speed2D);



			

        //    lastTick = curTick;
       //     lastTime = now;
       // }





		//if (flags != -1) {
				//mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -120, 0);
				//std::this_thread::sleep_for(std::chrono::microseconds(15626)); 
				//std::this_thread::sleep_for(std::chrono::milliseconds(16));

		//	}
		
    }
	misc::g_currentSpeed2D.store(0.0f);

} 

}

bool misc::isGameWindowActive() {
	HWND hwnd = GetForegroundWindow();
	if (hwnd) {
		char windowTitle[256];
		GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
		return std::string(windowTitle).find("Counter-Strike 2") != std::string::npos;
	}
	return false;
}

static bool isBhopToggledActive = false;


static bool qKeyWasPressed = false;
static std::chrono::steady_clock::time_point qPressStartTime;
static const std::chrono::milliseconds holdDurationThreshold(17); // 200ms threshold


void misc::startBhopThread(LocalPlayer localPlayer) {
    if (!g_bhopThread.joinable()) { // Check if thread is not already running
        g_stopBhopThread = false;   // Reset the stop flag
        g_bhopThread = std::thread(bhopWorker, localPlayer); // Create and start the thread
        // Optional: Log thread start
        // Logger::info("[Misc] BunnyHop thread started.");
    }
}


void misc::stopBhopThread() {
    if (g_bhopThread.joinable()) {
		g_stopBhopThread = true; // Signal the thread to stop its loop
        try {
            g_bhopThread.join(); // Wait for the thread to finish execution
            // Optional: Log thread stop
            // Logger::info("[Misc] BunnyHop thread stopped and joined.");
        } catch (const std::system_error& e) {
            // Handle potential errors during join (e.g., if thread wasn't joinable)
        }
    }
     g_autoBhopEnabled = false; // Ensure state is off on exit
}



static std::chrono::steady_clock::time_point lastToggleTime = std::chrono::steady_clock::now(); // Time of the last successful toggle
static const std::chrono::milliseconds toggleCooldown(360);   // 50ms cooldown duration


void misc::bunnyHop(LocalPlayer localPlayer) {
    if (!isGameWindowActive()) return;

    bool isQPressedNow = (GetAsyncKeyState('Q') & 0x8000) != 0;
    if (!isQPressedNow) return;


    auto now = std::chrono::steady_clock::now();
    if (now - lastToggleTime < toggleCooldown) {
        return; 
    }
	else{

		lastToggleTime = now; 
	}

    // Cooldown passed, toggle
    g_autoBhopEnabled = !g_autoBhopEnabled;
    if (g_autoBhopEnabled) {
        startBhopThread(localPlayer);
    } else {
        stopBhopThread();
    }


}




void misc::droppedItem(C_CSPlayerPawn C_CSPlayerPawn, CGameSceneNode CGameSceneNode, view_matrix_t viewMatrix) {
	//startItemESPThread(C_CSPlayerPawn, CGameSceneNode, viewMatrix);

	if (!overlayESP::isMenuOpen()) {
		if (!misc::isGameWindowActive()) return;
	}

	for (int i = 65; i < 1024; i++) {
		// Entity
		C_CSPlayerPawn.value = i;
		C_CSPlayerPawn.getListEntry();
		if (C_CSPlayerPawn.listEntry == 0) continue;
		C_CSPlayerPawn.getPlayerPawn();
		if (C_CSPlayerPawn.playerPawn == 0) continue;
		if (C_CSPlayerPawn.getOwner() != -1) continue;

		// Entity name
		uintptr_t entity = MemMan.ReadMem<uintptr_t>(C_CSPlayerPawn.playerPawn + 0x10);
		uintptr_t designerNameAddy = MemMan.ReadMem<uintptr_t>(entity + 0x20);

		char designerNameBuffer[MAX_PATH]{};
		MemMan.ReadRawMem(designerNameAddy, designerNameBuffer, MAX_PATH);

		std::string name = std::string(designerNameBuffer);

		if (strstr(name.c_str(), "weapon_")) name.erase(0, 7);
		//else if (strstr(name.c_str(), "_projectile")) name.erase(name.length() - 11, 11);
		//else if (strstr(name.c_str(), "baseanimgraph")) name = "defuse kit";
		else continue;

		// Origin position of entity
		CGameSceneNode.value = C_CSPlayerPawn.getCGameSceneNode();
		CGameSceneNode.getOrigin();
		CGameSceneNode.origin = CGameSceneNode.origin.worldToScreen(viewMatrix);

		// Drawing


		if (CGameSceneNode.origin.z >= 0.01f) {
			ImVec2 textSize = ImGui::CalcTextSize(name.c_str());
			auto [horizontalOffset, verticalOffset] = getTextOffsets(textSize.x, textSize.y, 2.f);

			ImFont* gunText = {};
			if (std::filesystem::exists(DexterionSystem::weaponIconsTTF)) {
				gunText = imGuiMenu::weaponIcons;
				name = gunIcon(name);
			}
			else gunText = imGuiMenu::espNameText;

			ImGui::GetBackgroundDrawList()->AddText(gunText, 12, { CGameSceneNode.origin.x - horizontalOffset, CGameSceneNode.origin.y - verticalOffset }, ImColor(espConf.attributeColours[0], espConf.attributeColours[1], espConf.attributeColours[2]), name.c_str());
		}
	}
}





// Add damage for a player
void misc::addDamage(std::string name, int damage, uintptr_t playerHandle) {
	bool found = false;
	
	// Check if player already exists in damage list
	for (auto& entry : damageList) {
		if (entry.playerHandle == playerHandle) {
			// Add damage to existing player
			entry.damage += damage;
			found = true;
			break;
		}
	}
	
	// If player not found, add them to the list
	if (!found) {
		damageList.push_back(DamageData(name, damage, playerHandle));
	}
	
	// Sort the damage list (highest damage first)
	std::sort(damageList.begin(), damageList.end());
}

void misc::updatePlayerDamage(std::string name, int totalDamage, uintptr_t playerHandle) {
    bool found = false;
    
    // Check if player already exists in damage list
    for (auto& entry : damageList) {
        if (entry.playerHandle == playerHandle) {
            // Update with current total damage
            entry.damage = totalDamage;
            found = true;
            break;
        }
    }
    
    // If player not found, add them to the list
    if (!found) {
        damageList.push_back(DamageData(name, totalDamage, playerHandle));
    }
    
    // Sort the damage list (highest damage first)
    std::sort(damageList.begin(), damageList.end());
}

// Clear the damage list (for round reset)
void misc::clearDamageList() {
	damageList.clear();
}

// Display the damage list UI
void misc::displayDamageList() {
	// Skip if disabled or game window not active
	if (!miscConf.damageList) return;
	if (!overlayESP::isMenuOpen()) {
		if (!misc::isGameWindowActive()) return;
	}
	
	// Always show the window (like spectator list), even if empty
	// This allows users to see it during gameplay
	
	// Set window position and size
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	ImGui::SetNextWindowPos({ (float)GetSystemMetrics(SM_CXSCREEN) - 200.f, 200.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 180.f, 180.f }, ImGuiCond_FirstUseEver);
	
	// Create window
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
	if (ImGui::Begin("Damage List", nullptr, flags)) {
		// Header
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Damage This Round");
		ImGui::Separator();
		
		if (damageList.empty()) {
			// Show a message when no damage to display
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No damage recorded yet");
		} else {
			// Display each player and their damage
			int count = 0;
			for (const auto& player : damageList) {
				// Limit to top 5 players
				if (count >= 5) break;
				
				// Display player name
				ImGui::Text("%s:", player.playerName.c_str());
				ImGui::SameLine();
				
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%d", player.damage);
				
				count++;
			}
		}
		
		// Add clear button when in menu
		if (overlayESP::isMenuOpen()) {
			ImGui::Separator();
			if (ImGui::Button("Clear List")) {
				clearDamageList();
			}
		}
		
		ImGui::End();
	}
	ImGui::PopStyleVar();
}

