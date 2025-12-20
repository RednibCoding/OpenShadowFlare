# OpenShadowFlare - Radare2 Analysis Script
# Run with: r2 -i shadowflare-init.r2 ShadowFlare.exe
# Then save project: Ps /path/to/OpenShadowFlare/radare2/shadowflare

# Analyze all
aaa

# === Initialization Functions ===
afn CheckSingleInstance @ 0x00401550
CC "Mutex-based single instance check" @ 0x00401550

afn ProcessCommandLine @ 0x004014a0
CC "Parse command line args" @ 0x004014a0

afn LoadConfig @ 0x00401eb0
CC "Load SFlare.Cfg configuration" @ 0x00401eb0

afn InitGame @ 0x004016b0
CC "Initialize all game subsystems" @ 0x004016b0

afn WinMain @ 0x004022e0
CC "Entry point, message loop" @ 0x004022e0

afn CreateGameWindow @ 0x00402520
CC "Register class, create window" @ 0x00402520

afn Shutdown @ 0x00401b90
CC "Release all subsystems" @ 0x00401b90

# === Window/Input Handling ===
afn WndProc @ 0x00402b30
CC "Window message handler (1142 bytes)" @ 0x00402b30

afn KeyDownHandler @ 0x00402840
CC "VK_RETURN, VK_SNAPSHOT handling" @ 0x00402840

afn KeyUpHandler @ 0x004026d0
CC "Key release (stub)" @ 0x004026d0

afn LeftClickHandler @ 0x004028a0
CC "Mouse left button" @ 0x004028a0

afn RightClickHandler @ 0x00402900
CC "Mouse right/middle button" @ 0x00402900

# === Game State Machine ===
afn UpdateGameState @ 0x004023d0
CC "State machine dispatcher (states 0,1,2)" @ 0x004023d0

afn State0_Init @ 0x00420c40
CC "Initialize title/menu state" @ 0x00420c40

afn State0Handler @ 0x00420df0
CC "Title screen / Main menu" @ 0x00420df0

afn State1_Init @ 0x00421a00
CC "Initialize loading state" @ 0x00421a00

afn State1Handler @ 0x00421bf0
CC "Loading / Transition" @ 0x00421bf0

afn State2_Init @ 0x0041d3f0
CC "Initialize gameplay state" @ 0x0041d3f0

afn State2Handler @ 0x0041d880
CC "Main gameplay dispatch" @ 0x0041d880

afn State2Handler_Main @ 0x0041d970
CC "Main gameplay update (2936 bytes)" @ 0x0041d970

# === Scenario/Loading ===
afn LoadScenario @ 0x00426200
CC "Main scenario loader (4846 bytes)" @ 0x00426200

afn LoadScenarioData @ 0x00426160
CC "Helper for scenario loading" @ 0x00426160

afn LoadScenarioMct @ 0x00427b50
CC "Scenario.Mct loader - magic MCED DATA v0000 (6780 bytes)" @ 0x00427b50

# === Save/Load ===
afn FindSaveSlot @ 0x004021b0
CC "Find next free save slot" @ 0x004021b0

afn LoadSaveSlotInfo @ 0x004239b0
CC "Read save slot headers" @ 0x004239b0

afn SaveGame @ 0x0044b580
CC "Write save file - magic ShadowFlare0005, XOR encrypted (5429 bytes)" @ 0x0044b580

# === Major Game Systems ===
afn CommandDispatcher @ 0x00429ec0
CC "Command/Event dispatcher - huge switch (20119 bytes)" @ 0x00429ec0

afn ScriptInterpreter @ 0x00430f80
CC "Script interpreter with 75 opcodes (13677 bytes)" @ 0x00430f80

afn LoadItemData @ 0x00462f80
CC "Item data loader - magic SFItemDataV0000, XOR encrypted (9247 bytes)" @ 0x00462f80

afn OptionsMenu @ 0x004103c0
CC "Options/Settings menu (7773 bytes)" @ 0x004103c0

afn ObjectNpcDisplay @ 0x00414990
CC "Object/NPC display via RKC_RPGSCRN_OBJECTDISP (7221 bytes)" @ 0x00414990

afn StatusScreenDisplay @ 0x00409a60
CC "Status screen display (5212 bytes)" @ 0x00409a60

afn CharacterStatusDisplay @ 0x00405750
CC "Character status/element display - shows element bonuses (4936 bytes)" @ 0x00405750

afn CharacterCreation @ 0x00421e10
CC "Character creation - class/gender select (2710 bytes)" @ 0x00421e10

# === Global Variables ===
f g_hInstance @ 0x00482770
CC "HINSTANCE - Application instance handle" @ 0x00482770

f g_lParam @ 0x00482774
CC "Last lParam from WM_CREATE" @ 0x00482774

f g_SFWindow @ 0x00482778
CC "SFWindow object (~0xB100 bytes)" @ 0x00482778

f g_pUpdib @ 0x00482D10
CC "RKC_UPDIB* - Sprite system" @ 0x00482D10

f g_pDbfControl @ 0x00482D18
CC "RKC_DBFCONTROL* - Double buffer control" @ 0x00482D18

f g_pCharAnim @ 0x00482D20
CC "RKC_RPGSCRN_CHARANIMBLOCK* - Character animation" @ 0x00482D20

f g_pNetwork @ 0x00482D24
CC "RKC_NETWORK* - Network system" @ 0x00482D24

f g_pDsound @ 0x00482D28
CC "RKC_DSOUND* - Sound system" @ 0x00482D28

f g_GameMode @ 0x00482DB0
CC "int - Game mode (0=SP, 1=Client, 2=Server)" @ 0x00482DB0

f g_ScreenshotFlag @ 0x0048D71C
CC "int - Screenshot requested flag" @ 0x0048D71C

f g_WindowStyle @ 0x0048D8B8
CC "int - Window style index" @ 0x0048D8B8

f g_ImeEnabled @ 0x0048D8CC
CC "int - IME enabled flag" @ 0x0048D8CC

f g_CursorVisible @ 0x0048D8D4
CC "int - Cursor visibility flag" @ 0x0048D8D4

# === Newly Discovered Functions (Session 2) ===

afn LoadGame @ 0x0044cac0
CC "Load save file - counterpart to SaveGame, XOR decryption (7527 bytes)" @ 0x0044cac0

afn ItemStatsDisplay @ 0x0040aed0
CC "Display item/equipment stats (Attack, Defense, etc.) (6758 bytes)" @ 0x0040aed0

afn ScenarioLoader_Phase2 @ 0x00423ca0
CC "Scenario loading phase 2, called from State1Handler (4306 bytes)" @ 0x00423ca0

afn NetworkServerHandler @ 0x0041afc0
CC "Network server packet handling via RKC_NETWORK (3680 bytes)" @ 0x0041afc0

afn SpritePacketSetup @ 0x004039f0
CC "Sprite rendering via RKC_UPDIB::SetPacket (3437 bytes)" @ 0x004039f0

afn MagicWindowDisplay @ 0x00407a60
CC "Magic/spell window - shows all spells with Lv/Exp/MP stats (3394 bytes)" @ 0x00407a60

afn PlayerLoginHandler @ 0x00418640
CC "Handle player login notification (266 bytes)" @ 0x00418640

afn NetworkConnectionHandler @ 0x00419f60
CC "Server connection management via RKC_NETWORK_SERVER (1251 bytes)" @ 0x00419f60

afn ObjectDisplayHandler @ 0x00441c00
CC "Object display via RKC_RPGSCRN_OBJECTDISP (3462 bytes)" @ 0x00441c00

afn TableDataLookup @ 0x00443cb0
CC "Table data lookup via RKC_RPG_TABLE (3069 bytes)" @ 0x00443cb0

# Done - now save the project manually:
# Ps /home/micha/DEV/projects/OpenShadowFlare/radare2/shadowflare
