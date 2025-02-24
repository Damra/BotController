// BotDLL.cpp
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

// Sabitler
#define KO_PTR_GameProcMain 0x10B8A34
#define KO_PTR_FNC_TARGET_SELECT 0x7FAF70
#define KO_PTR_FNC_ATTACK 0x7FB000
#define KO_PTR_FNC_PICKUP_ITEM 0x7FC000
#define KO_PTR_FNC_MOVE_AWAY 0x7FD000
#define KO_PTR_FNC_USE_ITEM 0x7FE000
#define KO_PTR_FNC_MOVE_TO 0x7FD500
#define KO_PTR_FNC_USE_SKILL 0x7FE500
#define KO_PTR_FNC_INVITE_PARTY 0x7FF000
#define KO_PTR_FNC_INTERACT_NPC 0x7FF500
#define KO_PTR_FNC_SEND_CHAT 0x7FF800
#define KO_PTR_ENTITY_LIST 0x10BA000
#define KO_PTR_ITEM_LIST 0x10B9000
#define KO_PTR_PLAYER_BASE 0x10BB000
#define KO_PTR_BUFF_LIST 0x10BC000
#define KO_PTR_MINIMAP_BASE 0x10BD000
#define KO_PTR_MINIMAP_TEXTURE 0x10BE000

// Config
struct BotConfig {
    int zoneX = 100;
    int zoneY = 200;
    int zoneRadius = 20;
    DWORD potionId = 12345;
    DWORD targetPlayerId = 5678;
    DWORD skills[3] = { 1001, 1002, 1003 };
    DWORD buffSkillId = 2001;

    void LoadFromFile(const char* filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("zoneX=") == 0) zoneX = std::stoi(line.substr(6));
                else if (line.find("zoneY=") == 0) zoneY = std::stoi(line.substr(6));
                else if (line.find("zoneRadius=") == 0) zoneRadius = std::stoi(line.substr(11));
                else if (line.find("potionId=") == 0) potionId = std::stoul(line.substr(9));
                else if (line.find("targetPlayerId=") == 0) targetPlayerId = std::stoul(line.substr(15));
                else if (line.find("skill1=") == 0) skills[0] = std::stoul(line.substr(7));
                else if (line.find("skill2=") == 0) skills[1] = std::stoul(line.substr(7));
                else if (line.find("skill3=") == 0) skills[2] = std::stoul(line.substr(7));
                else if (line.find("buffSkillId=") == 0) buffSkillId = std::stoul(line.substr(12));
            }
            file.close();
        }
    }
};

// 1. Entity: Oyuncu, Buff ve Radar verileri
struct PlayerEntity {
    DWORD baseAddress;
    int currentHP;
    int maxHP;
    int x;
    int y;

    void Update() {
        if (baseAddress != 0) {
            currentHP = *(int*)(baseAddress + 0x100);
            maxHP = *(int*)(baseAddress + 0x104);
            x = *(int*)(baseAddress + 0x10);
            y = *(int*)(baseAddress + 0x14);
        }
    }
};

struct Buff {
    DWORD id;
    int duration;
};

struct RadarEntity {
    DWORD address;
    int x;
    int y;
    int type; // 1: Mob, 2: Oyuncu, 3: Obje
    char name[32];
};

struct MinimapInfo {
    int xOffset;
    int yOffset;
    int width;
    int height;
    float scale;
};

// DirectX Hook için global değişkenler
IDirect3DDevice9* g_pDevice = nullptr;
typedef HRESULT(WINAPI* EndScene_t)(IDirect3DDevice9*);
EndScene_t oEndScene;
LPD3DXFONT g_pFont = nullptr;
LPD3DXSPRITE g_pSprite = nullptr;
LPDIRECT3DTEXTURE9 g_pMapTexture = nullptr;

// 2. Repository: Oyun belleği ile iletişim
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
    virtual void InteractWithNPC(DWORD npcId) = 0;
    virtual void SendChatMessage(const char* message) = 0;
    virtual DWORD FindNearestEnemy() = 0;
    virtual DWORD FindNextItem() = 0;
    virtual PlayerEntity GetPlayerEntity() = 0;
    virtual std::vector<Buff> GetBuffList() = 0;
    virtual std::vector<RadarEntity> GetRadarEntities() = 0;
    virtual MinimapInfo GetMinimapInfo() = 0;
    virtual LPDIRECT3DTEXTURE9 GetMapTexture() = 0;
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

    void InteractWithNPC(DWORD npcId) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            MOV ESI, npcId
            PUSH ESI
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_INTERACT_NPC
            CALL EAX
        }
    }

    void SendChatMessage(const char* message) override {
        DWORD pointer = GetGameProcessPointer();
        __asm {
            PUSH message
            MOV ECX, pointer
            MOV EAX, KO_PTR_FNC_SEND_CHAT
            CALL EAX
        }
    }

    DWORD FindNearestEnemy() override {
        DWORD entityList = *(DWORD*)KO_PTR_ENTITY_LIST;
        for (int i = 0; i < 20; i++) {
            DWORD entity = *(DWORD*)(entityList + i * 4);
            if (entity != 0) {
                int distance = *(int*)(entity + 0x10);
                if (distance < 10) return entity;
            }
        }
        return 0;
    }

    DWORD FindNextItem() override {
        static int itemIndex = 0;
        DWORD itemList = *(DWORD*)KO_PTR_ITEM_LIST;
        DWORD item = *(DWORD*)(itemList + itemIndex * 4);
        itemIndex = (itemIndex + 1) % 10;
        return item;
    }

    PlayerEntity GetPlayerEntity() override {
        PlayerEntity entity;
        entity.baseAddress = *(DWORD*)KO_PTR_PLAYER_BASE;
        entity.Update();
        return entity;
    }

    std::vector<Buff> GetBuffList() override {
        std::vector<Buff> buffs;
        DWORD buffList = *(DWORD*)KO_PTR_BUFF_LIST;
        for (int i = 0; i < 5; i++) {
            DWORD buffAddr = *(DWORD*)(buffList + i * 8);
            if (buffAddr != 0) {
                Buff buff;
                buff.id = *(DWORD*)(buffAddr);
                buff.duration = *(int*)(buffAddr + 4);
                buffs.push_back(buff);
            }
        }
        return buffs;
    }

    std::vector<RadarEntity> GetRadarEntities() override {
        std::vector<RadarEntity> entities;
        DWORD entityList = *(DWORD*)KO_PTR_ENTITY_LIST;
        DWORD itemList = *(DWORD*)KO_PTR_ITEM_LIST;

        for (int i = 0; i < 20; i++) {
            DWORD entity = *(DWORD*)(entityList + i * 4);
            if (entity != 0) {
                RadarEntity radarEntity;
                radarEntity.address = entity;
                radarEntity.x = *(int*)(entity + 0x10);
                radarEntity.y = *(int*)(entity + 0x14);
                radarEntity.type = *(int*)(entity + 0x20);
                strncpy_s(radarEntity.name, (char*)(entity + 0x30), 32);
                entities.push_back(radarEntity);
            }
        }

        for (int i = 0; i < 10; i++) {
            DWORD item = *(DWORD*)(itemList + i * 4);
            if (item != 0) {
                RadarEntity radarEntity;
                radarEntity.address = item;
                radarEntity.x = *(int*)(item + 0x10);
                radarEntity.y = *(int*)(item + 0x14);
                radarEntity.type = 3;
                strncpy_s(radarEntity.name, "Item", 32);
                entities.push_back(radarEntity);
            }
        }

        return entities;
    }

    MinimapInfo GetMinimapInfo() override {
        MinimapInfo info;
        DWORD minimapBase = *(DWORD*)KO_PTR_MINIMAP_BASE;
        if (minimapBase != 0) {
            info.xOffset = *(int*)(minimapBase + 0x0);
            info.yOffset = *(int*)(minimapBase + 0x4);
            info.width = *(int*)(minimapBase + 0x8);
            info.height = *(int*)(minimapBase + 0xC);
            info.scale = *(float*)(minimapBase + 0x10);
        }
        else {
            info.xOffset = 50;
            info.yOffset = 50;
            info.width = 200;
            info.height = 200;
            info.scale = 0.1f;
        }
        return info;
    }

    LPDIRECT3DTEXTURE9 GetMapTexture() override {
        DWORD textureAddr = *(DWORD*)KO_PTR_MINIMAP_TEXTURE;
        return textureAddr ? (LPDIRECT3DTEXTURE9)textureAddr : nullptr;
    }
};

// DirectX Hook fonksiyonu
HRESULT WINAPI HookedEndScene(IDirect3DDevice9* pDevice) {
    if (!g_pDevice) {
        g_pDevice = pDevice;
        D3DXCreateSprite(pDevice, &g_pSprite);
        D3DXCreateFont(pDevice, 12, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial", &g_pFont);
    }

    return oEndScene(pDevice);
}

void HookDirectX() {
    HWND hWnd = FindWindow(NULL, L"Knight Online Client");
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return;

    D3DPRESENT_PARAMETERS d3dpp = { 0 };
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;

    IDirect3DDevice9* pDummyDevice = nullptr;
    pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
    if (!pDummyDevice) {
        pD3D->Release();
        return;
    }

    void** vTable = *(void***)pDummyDevice;
    oEndScene = (EndScene_t)vTable[42];

    DWORD oldProtect;
    VirtualProtect(&vTable[42], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    vTable[42] = HookedEndScene;
    VirtualProtect(&vTable[42], sizeof(void*), oldProtect, &oldProtect);

    pDummyDevice->Release();
    pD3D->Release();
}

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
    DWORD potionId;
public:
    HealthManagementUseCase(IGameMemoryRepository* repo, DWORD id) : repository(repo), potionId(id) {}
    void Execute() {
        PlayerEntity player = repository->GetPlayerEntity();
        if (player.currentHP < player.maxHP * 0.3) {
            repository->UseItem(potionId);
        }
    }
};

class AutoPartyUseCase {
    IGameMemoryRepository* repository;
    DWORD targetPlayerId;
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
    DWORD skills[3];
    int currentSkillIndex = 0;
public:
    SkillRotationUseCase(IGameMemoryRepository* repo, DWORD skill1, DWORD skill2, DWORD skill3)
        : repository(repo) {
        skills[0] = skill1;
        skills[1] = skill2;
        skills[2] = skill3;
    }
    void Execute() {
        repository->UseSkill(skills[currentSkillIndex]);
        currentSkillIndex = (currentSkillIndex + 1) % 3;
    }
};

class ZoneFarmingUseCase {
    IGameMemoryRepository* repository;
    int zoneX;
    int zoneY;
    int zoneRadius;
public:
    ZoneFarmingUseCase(IGameMemoryRepository* repo, int x, int y, int radius)
        : repository(repo), zoneX(x), zoneY(y), zoneRadius(radius) {
    }
    void Execute() {
        PlayerEntity player = repository->GetPlayerEntity();
        int distance = static_cast<int>(sqrt(pow(player.x - zoneX, 2) + pow(player.y - zoneY, 2)));
        if (distance > zoneRadius) {
            repository->MoveToPosition(zoneX, zoneY);
        }
    }
};

class AutoNPCUseCase {
    IGameMemoryRepository* repository;
    DWORD npcId = 9999;
public:
    AutoNPCUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        repository->InteractWithNPC(npcId);
    }
};

class ChatBotUseCase {
    IGameMemoryRepository* repository;
public:
    ChatBotUseCase(IGameMemoryRepository* repo) : repository(repo) {}
    void Execute() {
        static int counter = 0;
        if (counter % 50 == 0) {
            repository->SendChatMessage("Bot aktif!");
        }
        counter++;
    }
};

class BuffManagementUseCase {
    IGameMemoryRepository* repository;
    DWORD buffSkillId;
public:
    BuffManagementUseCase(IGameMemoryRepository* repo, DWORD skillId)
        : repository(repo), buffSkillId(skillId) {
    }
    void Execute() {
        std::vector<Buff> buffs = repository->GetBuffList();
        bool hasBuff = false;
        for (const Buff& buff : buffs) {
            if (buff.id == buffSkillId && buff.duration > 0) {
                hasBuff = true;
                break;
            }
        }
        if (!hasBuff) {
            repository->UseSkill(buffSkillId);
        }
    }
};

class RadarUseCase {
    IGameMemoryRepository* repository;
    DWORD selectedTarget = 0;

    void DrawEntity(IDirect3DDevice9* pDevice, const RadarEntity& entity, MinimapInfo minimap, PlayerEntity player, D3DCOLOR color) {
        int mapX = minimap.xOffset + static_cast<int>((entity.x * minimap.scale));
        int mapY = minimap.yOffset + static_cast<int>((entity.y * minimap.scale));
        float distance = sqrt(pow(entity.x - player.x, 2) + pow(entity.y - player.y, 2));
        D3DXVECTOR3 pos;
        g_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
        switch (entity.type) {
        case 1: // Mob (kırmızı daire)
            pos = D3DXVECTOR3(mapX - 4, mapY - 4, 0);
            g_pSprite->Draw(NULL, NULL, NULL, &pos, D3DCOLOR_ARGB(255, 255, 0, 0));
            break;
        case 2: // Oyuncu (mavi üçgen)
            pos = D3DXVECTOR3(mapX - 5, mapY - 5, 0);
            g_pSprite->Draw(NULL, NULL, NULL, &pos, D3DCOLOR_ARGB(255, 0, 0, 255));
            break;
        case 3: // Eşya (sarı yıldız)
            pos = D3DXVECTOR3(mapX - 3, mapY - 3, 0);
            g_pSprite->Draw(NULL, NULL, NULL, &pos, D3DCOLOR_ARGB(255, 255, 255, 0));
            break;
        }

        char label[64];
        sprintf_s(label, "%s (%.0f)", entity.name, distance);
        RECT textRect = { mapX + 5, mapY, 0, 0 };
        g_pFont->DrawTextA(g_pSprite, label, -1, &textRect, DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));

        g_pSprite->End();
    }

    bool CheckClick(int mouseX, int mouseY, const RadarEntity& entity, MinimapInfo minimap) {
        int mapX = minimap.xOffset + static_cast<int>((entity.x * minimap.scale));
        int mapY = minimap.yOffset + static_cast<int>((entity.y * minimap.scale));
        return (mouseX >= mapX - 5 && mouseX <= mapX + 5 && mouseY >= mapY - 5 && mouseY <= mapY + 5);
    }

public:
    RadarUseCase(IGameMemoryRepository* repo) : repository(repo) {}

    void Execute() {
        if (!g_pDevice || !g_pSprite || !g_pFont) return;

        std::vector<RadarEntity> entities = repository->GetRadarEntities();
        PlayerEntity player = repository->GetPlayerEntity();
        MinimapInfo minimap = repository->GetMinimapInfo();
        LPDIRECT3DTEXTURE9 mapTexture = repository->GetMapTexture();

        g_pDevice->BeginScene();

        if (mapTexture) {
            g_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
            D3DXVECTOR3 mapPos((float)minimap.xOffset, (float)minimap.yOffset, 0);
            g_pSprite->Draw(mapTexture, NULL, NULL, &mapPos, D3DCOLOR_ARGB(255, 255, 255, 255));
            g_pSprite->End();
        }

        int playerMapX = minimap.xOffset + static_cast<int>((player.x * minimap.scale));
        int playerMapY = minimap.yOffset + static_cast<int>((player.y * minimap.scale));
        g_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
        D3DXVECTOR3 playerPos(playerMapX - 5, playerMapY - 5, 0);
        g_pSprite->Draw(NULL, NULL, NULL, &playerPos, D3DCOLOR_ARGB(255, 0, 255, 0));
        g_pSprite->End();

        for (const RadarEntity& entity : entities) {
            switch (entity.type) {
            case 1: DrawEntity(g_pDevice, entity, minimap, player, D3DCOLOR_ARGB(255, 255, 0, 0)); break;
            case 2: DrawEntity(g_pDevice, entity, minimap, player, D3DCOLOR_ARGB(255, 0, 0, 255)); break;
            case 3: DrawEntity(g_pDevice, entity, minimap, player, D3DCOLOR_ARGB(255, 255, 255, 0)); break;
            }
        }

        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            POINT mouse;
            GetCursorPos(&mouse);
            ScreenToClient(FindWindow(NULL, L"Knight Online Client"), &mouse);
            for (const RadarEntity& entity : entities) {
                if (CheckClick(mouse.x, mouse.y, entity, minimap)) {
                    selectedTarget = entity.address;
                    repository->SelectTarget(selectedTarget);
                    break;
                }
            }
        }

        g_pDevice->EndScene();
    }

    ~RadarUseCase() {
        if (g_pFont) g_pFont->Release();
        if (g_pSprite) g_pSprite->Release();
        if (g_pMapTexture) g_pMapTexture->Release();
    }
};

// 4. Controller: Bot sınıfı
class KOBotController {
    IGameMemoryRepository* memoryRepo;
    AutoAttackUseCase* autoAttack;
    AutoLootUseCase* autoLoot;
    AutoFleeUseCase* autoFlee;
    HealthManagementUseCase* healthManagement;
    AutoPartyUseCase* autoParty;
    SkillRotationUseCase* skillRotation;
    ZoneFarmingUseCase* zoneFarming;
    AutoNPCUseCase* autoNPC;
    ChatBotUseCase* chatBot;
    BuffManagementUseCase* buffManagement;
    RadarUseCase* radar;
    BotConfig config;
    HWND guiWindow;
    bool botEnabled = true;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        KOBotController* self = (KOBotController*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) self->botEnabled = !self->botEnabled;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

public:
    KOBotController() {
        config.LoadFromFile("bot_config.ini");
        memoryRepo = new GameMemoryRepository();
        autoAttack = new AutoAttackUseCase(memoryRepo);
        autoLoot = new AutoLootUseCase(memoryRepo);
        autoFlee = new AutoFleeUseCase(memoryRepo);
        healthManagement = new HealthManagementUseCase(memoryRepo, config.potionId);
        autoParty = new AutoPartyUseCase(memoryRepo, config.targetPlayerId);
        skillRotation = new SkillRotationUseCase(memoryRepo, config.skills[0], config.skills[1], config.skills[2]);
        zoneFarming = new ZoneFarmingUseCase(memoryRepo, config.zoneX, config.zoneY, config.zoneRadius);
        autoNPC = new AutoNPCUseCase(memoryRepo);
        chatBot = new ChatBotUseCase(memoryRepo);
        buffManagement = new BuffManagementUseCase(memoryRepo, config.buffSkillId);
        radar = new RadarUseCase(memoryRepo);

        HookDirectX();

        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"BotGUI";
        RegisterClass(&wc);
        guiWindow = CreateWindowW(L"BotGUI", L"Bot Kontrol", WS_OVERLAPPEDWINDOW, 0, 0, 300, 100, NULL, NULL, wc.hInstance, NULL);
        SetWindowLongPtr(guiWindow, GWLP_USERDATA, (LONG_PTR)this);
        CreateWindowW(L"BUTTON", L"Botu Aç/Kapa", WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, guiWindow, (HMENU)1, wc.hInstance, NULL);
        ShowWindow(guiWindow, SW_SHOW);
        UpdateWindow(guiWindow);
    }

    ~KOBotController() {
        DestroyWindow(guiWindow);
        delete radar;
        delete buffManagement;
        delete chatBot;
        delete autoNPC;
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
        MSG msg;
        while (true) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) break;
            }

            if (botEnabled) {
                DWORD target = memoryRepo->FindNearestEnemy();
                autoAttack->Execute(target);
                autoLoot->Execute();
                autoFlee->Execute();
                healthManagement->Execute();
                autoParty->Execute();
                skillRotation->Execute();
                zoneFarming->Execute();
                autoNPC->Execute();
                chatBot->Execute();
                buffManagement->Execute();
                radar->Execute();
            }
            Sleep(200);
        }
    }
};

// Thread
DWORD WINAPI BotThread(LPVOID lpParam) {
    KOBotController* bot = new KOBotController();
    bot->StartBot();
    delete bot;
    return 0;
}

// DLL Entrypoint
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, BotThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
