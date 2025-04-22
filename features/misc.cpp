#include "misc.hpp"
#include "../util/config.hpp"

// Initialize static variables
std::vector<misc::DamageData> misc::damageList;
int misc::lastUpdateTime = 0;

bool misc::isGameWindowActive() {
	HWND hwnd = GetForegroundWindow();
	if (hwnd) {
		char windowTitle[256];
		GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
		return std::string(windowTitle).find("Counter-Strike 2") != std::string::npos;
	}
	return false;
}

void misc::droppedItem(C_CSPlayerPawn C_CSPlayerPawn, CGameSceneNode CGameSceneNode, view_matrix_t viewMatrix) {
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
		else if (strstr(name.c_str(), "_projectile")) name.erase(name.length() - 11, 11);
		else if (strstr(name.c_str(), "baseanimgraph")) name = "defuse kit";
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

