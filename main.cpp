#include <windows.h>
#include <cmath>

// Sabitler (örnek adresler)
#define KO_PTR_GameProcMain 0x10B8A34
#define KO_PTR_FNC_TARGET_SELECT 0x7FAF70
#define KO_PTR_FNC_ATTACK 0x7FB000
#define KO_PTR_FNC_PICKUP_ITEM 0x7FC000
#define KO_PTR_FNC_MOVE_AWAY 0x7FD000
#define KO_PTR_FNC_USE_ITEM 0x7FE000
#define KO_PTR_FNC_MOVE_TO 0x7FD500
#define KO_PTR_FNC_USE_SKILL 0x7FE500
#define KO_PTR_FNC_INVITE_PARTY 0x7FF000
#define KO_PTR_ENTITY_LIST 0x10BA000
#define KO_PTR_ITEM_LIST 0x10B9000
#define KO_PTR_PLAYER_BASE 0x10BB000

// 1. Entity: Oyuncu verileri
struct PlayerEntity {
    DWORD baseAddress;
    int currentHP;
    int maxHP;
    int x;
    int y;

    void Update() {
        if (baseAddress != 0) {
            currentHP = *(int*)(baseAddress + 0x100); // HP offset
            maxHP = *(int*)(baseAddress + 0x104);     // max HP offset
            x = *(int*)(baseAddress + 0x10);          // X offset
            y = *(int*)(baseAddress + 0x14);          // Y offset
        }
    }
};

// 2. Repository: Oyun belleği ile iletişim (interface)
class IGameMemoryRepository {
public:
    virtual DWORD GetGameProcessPointer() = 0;
    virtual void SelectTarget(DWORD pTarget) = 0;
    virtual void AttackTarget() = 0;
    virtual void PickupItem(DWORD pItem) = 0;
    virtual void MoveAway() = 0;
    virtual void UseItem(DWORD itemId) = 0;
    virtual void UseSkill(DWORD skillId) = 0;
    virtual void MoveToPosition(int x, int y) = 0;
    virtual void InviteToParty(DWORD playerId) = 0;
    virtual DWORD FindNearestEnemy() = 0;
    virtual DWORD FindNextItem() = 0;
    virtual PlayerEntity GetPlayerEntity() = 0;
};

class GameMemoryRepository : public IGameMemoryRepository {
public:
    DWORD GetGameProcessPointer() override {
        return *(DWORD*)KO_PTR_GameProcMain;
    }

    void SelectTarget(DWORD pTarget) override {
        if (pTarget != 0) {
            DWORD pointer = GetGameProcessPointer();
            __asm {
                MOV ESI, pTarget
                PUSH ESI
                MOV ECX, pointer
                MOV EAX, KO_PTR_FNC_TARGET_SELECT
                CALL EAX
            }
        }
    }

    void AttackTarget() override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_ATTACK
            CALL EAX
        }
    }

    void PickupItem(DWORD pItem) override {
        if (pItem != 0) {
            DWORD pointer = GetGameProcessPointer();
            __asm {
                MOV ESI, pItem
                PUSH ESI
                MOV ECX, pointer
                MOV EAX, KO_PTR_FNC_PICKUP_ITEM
                CALL EAX
            }
        }
    }

    void MoveAway() override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_MOVE_AWAY
            CALL EAX
        }
    }

    void UseItem(DWORD itemId) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_USE_ITEM
            PUSH itemId
            CALL EAX
        }
    }

    void UseSkill(DWORD skillId) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ESI, skillId
            PUSH ESI
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_USE_SKILL
            CALL EAX
        }
    }

    void MoveToPosition(int x, int y) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            PUSH y
            PUSH x
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_MOVE_TO
            CALL EAX
        }
    }

    void InviteToParty(DWORD playerId) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ESI, playerId
            PUSH ESI
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_INVITE_PARTY
            CALL EAX
        }
    }

    DWORD FindNearestEnemy() override {
        DWORD entityList = *(DWORD*)KO_PTR_ENTITY_LIST;
        for (int i = 0; i < 20; i++) {
            DWORD entity = *(DWORD*)(entityList + i * 4);
            if (entity != 0) {
                int distance = *(int*)(entity + 0x10); // offset
                if (distance < 10) return entity;
            }
        }
        return 0;
    }

    DWORD FindNextItem() override {
        static int itemIndex = 0;
        DWORD itemList = *(DWORD*)KO_PTR_ITEM_LIST;
        DWORD item = *(DWORD*)(itemList + itemIndex * 4);
        itemIndex = (itemIndex + 1) % 10; // 10 eşya döngüsü
        return item;
    }

    PlayerEntity GetPlayerEntity() override {
        PlayerEntity entity;
        entity.baseAddress = *(DWORD*)KO_PTR_PLAYER_BASE;
        entity.Update();
        return entity;
    }
};

// 3. Use Cases: BusinessLogic
class AutoAttackUseCase {
    IGameMemoryRepository* repository;
public:
    AutoAttackUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute(DWORD pTarget) {
        if (pTarget != 0) {
            repository->SelectTarget(pTarget);
            repository->AttackTarget();
        }
    }
};

class AutoLootUseCase {
    IGameMemoryRepository* repository;
public:
    AutoLootUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        DWORD item = repository->FindNextItem();
        if (item != 0) {
            repository->PickupItem(item);
        }
    }
};

class AutoFleeUseCase {
    IGameMemoryRepository* repository;
public:
    AutoFleeUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        DWORD enemy = repository->FindNearestEnemy();
        if (enemy != 0) {
            repository->MoveAway();
        }
    }
};

class HealthManagementUseCase {
    IGameMemoryRepository* repository;
    DWORD potionId = 12345; // potion ID
public:
    HealthManagementUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        PlayerEntity player = repository->GetPlayerEntity();
        if (player.currentHP < player.maxHP * 0.3) {
            repository->UseItem(potionId);
        }
    }
};

class AutoPartyUseCase {
    IGameMemoryRepository* repository;
    DWORD targetPlayerId; // Davet edilecek oyuncunun ID’si
public:
    AutoPartyUseCase(IGameMemoryRepository* repo, DWORD targetId)
        : repository(repo), targetPlayerId(targetId) {
    }
    void Execute() {
        repository->InviteToParty(targetPlayerId);
    }
};

class SkillRotationUseCase {
    IGameMemoryRepository* repository;
    DWORD skills[3] = { 1001, 1002, 1003 }; // skill ID’leri
    int currentSkillIndex = 0;
public:
    SkillRotationUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        repository->UseSkill(skills[currentSkillIndex]);
        currentSkillIndex = (currentSkillIndex + 1) % 3; // 3 yetenek arasında döner
    }
};

class ZoneFarmingUseCase {
    IGameMemoryRepository* repository;
    int zoneX = 100; // Hedef bölge X koordinatı
    int zoneY = 200; // Hedef bölge Y koordinatı
    int zoneRadius = 20; // Bölge yarıçapı
public:
    ZoneFarmingUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        PlayerEntity player = repository->GetPlayerEntity();
        int distance = static_cast<int>(sqrt(pow(player.x - zoneX, 2) + pow(player.y - zoneY, 2)));
        if (distance > zoneRadius) {
            repository->MoveToPosition(zoneX, zoneY);
        }
    }
};

// 4. Controller: Tüm sistemi koordine eder
class BotController {
    IGameMemoryRepository* memoryRepo;
    AutoAttackUseCase* autoAttack;
    AutoLootUseCase* autoLoot;
    AutoFleeUseCase* autoFlee;
    HealthManagementUseCase* healthManagement;
    AutoPartyUseCase* autoParty;
    SkillRotationUseCase* skillRotation;
    ZoneFarmingUseCase* zoneFarming;

public:
    BotController() {
        memoryRepo = new GameMemoryRepository();
        autoAttack = new AutoAttackUseCase(memoryRepo);
        autoLoot = new AutoLootUseCase(memoryRepo);
        autoFlee = new AutoFleeUseCase(memoryRepo);
        healthManagement = new HealthManagementUseCase(memoryRepo);
        autoParty = new AutoPartyUseCase(memoryRepo, 5678); // Örnek player ID
        skillRotation = new SkillRotationUseCase(memoryRepo);
        zoneFarming = new ZoneFarmingUseCase(memoryRepo);
    }

    ~BotController() {
        delete zoneFarming;
        delete skillRotation;
        delete autoParty;
        delete healthManagement;
        delete autoFlee;
        delete autoLoot;
        delete autoAttack;
        delete memoryRepo;
    }

    void StartBot() {
        while (true) {
            DWORD target = memoryRepo->FindNearestEnemy();
            autoAttack->Execute(target);
            autoLoot->Execute();
            autoFlee->Execute();
            healthManagement->Execute();
            autoParty->Execute();
            skillRotation->Execute();
            zoneFarming->Execute();
            Sleep(200); // Genel döngü gecikmesi
        }
    }
};

// Thread fonksiyonu
DWORD WINAPI BotThread(LPVOID lpParam) {
    BotController* bot = new BotController();
    bot->StartBot();
    delete bot; // Thread bittiğinde temizlik
    return 0;
}

// DLL giriş noktası
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // DLL yüklendiğinde thread başlat
        CreateThread(NULL, 0, BotThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
