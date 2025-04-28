#pragma once

#include "include.hpp"
#include "../gui/overlay.hpp"
#include "../util/config.hpp"
#include "../gui/menu.hpp"

#include <algorithm>

namespace misc {

	inline namespace sharedData {
		inline int bhopInAir = (1 << 0);
	};

	void droppedItem(C_CSPlayerPawn C_CSPlayerPawn, CGameSceneNode CGameSceneNode, view_matrix_t viewMatrix);
	bool isGameWindowActive();
	void bunnyHop(LocalPlayer localPlayer);
	
	// Damage tracking structure (similar to CSGO implementation)
// In misc.hpp
struct DamageData {
    std::string playerName;
    int damage;
    uintptr_t playerHandle;
    
    DamageData(std::string name, int dmg, uintptr_t handle) : 
        playerName(name), damage(dmg), playerHandle(handle) {}
    
    bool operator<(const DamageData& other) const {
        return damage > other.damage;
    }
};
	
	// Storage for damage data
	extern std::vector<DamageData> damageList;
	extern int lastUpdateTime;
	
	// Damage list functions
	void displayDamageList();                                  // Display the damage list UI
	void addDamage(std::string name, int damage, uintptr_t playerHandle); // Add damage for a player
	void clearDamageList();                                   // Clear the damage list (for round reset)
	void updatePlayerDamage(std::string name, int totalDamage, uintptr_t playerHandle);
	void updateDamageList(MemoryManagement::moduleData client);

    
    void startBhopThread(LocalPlayer localPlayer); // Function to initialize and start the thread
	void stopBhopThread();  // Function to signal the thread to stop and join it

    void startItemESPThread(C_CSPlayerPawn C_CSPlayerPawn, CGameSceneNode CGameScene, view_matrix_t viewMatrix);

    void stopItemESPThread();



	// Utility to get current timestamp in seconds
	inline int getCurrentTimestamp() {
		return static_cast<int>(std::time(nullptr));
	}



struct CDamageRecord {
    // First validate the structure layout and offsets
    // Use minimal definition at first
    uintptr_t m_PlayerDamager;         // 0x00
    uintptr_t m_PlayerRecipient;       // 0x08
    uintptr_t m_PlayerControllerDamager;   // 0x10
    uintptr_t m_PlayerControllerRecipient; // 0x18
    // Skip to the fields we need
    char pad[0x30];                    // Adjust this padding based on actual memory layout
    char m_szPlayerDamagerName[64];    // Actual offset uncertain - verify!
    char m_szPlayerRecipientName[64];  
    uint64_t m_DamagerXuid;
    uint64_t m_RecipientXuid;
    int32_t m_iBulletsDamage;
    int32_t m_iDamage;                 // This is what we need
};

// Define the NetworkedVector structure (simplified)
struct NetworkedVector {
    // CS2 uses a different memory layout for vectors
    uintptr_t m_data;         // 0x00: Pointer to data array
    int32_t m_size;           // 0x08: Current number of elements
    int32_t m_capacity;       // 0x0C: Allocated capacity
    int32_t m_growSize;       // 0x10: Elements to add when growing
    // There may be additional fields
};

}