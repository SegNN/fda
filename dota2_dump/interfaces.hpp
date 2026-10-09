// Dota 2 runtime interfaces (resolved via CreateInterface)
#pragma once

namespace client_dll { // client.dll
    constexpr auto PanoramaUIClient001                      = 0x5BB42A0; // client.dll + rva
    constexpr auto PlayButtonService001                     = 0x5B96A38; // client.dll + rva
    constexpr auto DOTA_CLIENT_GCCLIENT                     = 0x63CA7C0; // client.dll + rva
    constexpr auto LegacyGameUI001                          = 0x5B89150; // client.dll + rva
    constexpr auto Source2ClientPrediction001               = 0x5B2F490; // client.dll + rva
    constexpr auto ClientToolsInfo_001                      = 0x5B2C8F0; // client.dll + rva
    constexpr auto Source2Client002                         = 0x6213E20; // client.dll + rva
    constexpr auto GameClientExports001                     = 0x5B28620; // client.dll + rva
    constexpr auto Source2ClientConfig001                   = 0x61B11A0; // client.dll + rva
    constexpr auto Source2ClientUI001                       = 0x5945BC0; // client.dll + rva
}
namespace engine2_dll { // engine2.dll
    constexpr auto SimpleEngineLoopService_001              = 0x5BE710; // engine2.dll + rva
    constexpr auto ClientServerEngineLoopService_001        = 0x8B5B00; // engine2.dll + rva
    constexpr auto KeyValueCache001                         = 0x5BE690; // engine2.dll + rva
    constexpr auto HostStateMgr001                          = 0x5BE5E0; // engine2.dll + rva
    constexpr auto GameEventSystemServerV001                = 0x8B57B0; // engine2.dll + rva
    constexpr auto GameEventSystemClientV001                = 0x8B5680; // engine2.dll + rva
    constexpr auto EngineServiceMgr001                      = 0x8B53A0; // engine2.dll + rva
    constexpr auto ClientServerSharedHandleSystem001        = 0x8B50E0; // engine2.dll + rva
    constexpr auto VProfService_001                         = 0x5BE490; // engine2.dll + rva
    constexpr auto ToolService_001                          = 0x5BE450; // engine2.dll + rva
    constexpr auto StatsService_001                         = 0x8B4920; // engine2.dll + rva
    constexpr auto SplitScreenService_001                   = 0x5BE350; // engine2.dll + rva
    constexpr auto SoundService_001                         = 0x5BE0D0; // engine2.dll + rva
    constexpr auto ScreenshotService001                     = 0x8B45E0; // engine2.dll + rva
    constexpr auto RenderService_001                        = 0x8B4320; // engine2.dll + rva
    constexpr auto NetworkService_001                       = 0x5BE090; // engine2.dll + rva
    constexpr auto NetworkServerService_001                 = 0x8B40B0; // engine2.dll + rva
    constexpr auto NetworkP2PService_001                    = 0x8B3F00; // engine2.dll + rva
    constexpr auto NetworkClientService_001                 = 0x8B3BC0; // engine2.dll + rva
    constexpr auto MapListService_001                       = 0x8B3A30; // engine2.dll + rva
    constexpr auto InputService_001                         = 0x874BC0; // engine2.dll + rva
    constexpr auto GameUIService_001                        = 0x8748E0; // engine2.dll + rva
    constexpr auto GameResourceServiceServerV001            = 0x5BDF20; // engine2.dll + rva
    constexpr auto GameResourceServiceClientV001            = 0x5BDEC0; // engine2.dll + rva
    constexpr auto BugBugService001                         = 0x5BDE80; // engine2.dll + rva
    constexpr auto BugService001                            = 0x874490; // engine2.dll + rva
    constexpr auto BenchmarkService001                      = 0x5BDD80; // engine2.dll + rva
    constexpr auto VENGINE_GAMEUIFUNCS_VERSION005           = 0x5BB820; // engine2.dll + rva
    constexpr auto EngineGameUI001                          = 0x5BB790; // engine2.dll + rva
    constexpr auto INETSUPPORT_001                          = 0x5B6D60; // engine2.dll + rva
    constexpr auto Source2EngineToServerStringTable001      = 0x5BB200; // engine2.dll + rva
    constexpr auto Source2EngineToServer001                 = 0x5BB1D8; // engine2.dll + rva
    constexpr auto Source2EngineToClientStringTable001      = 0x5BB160; // engine2.dll + rva
    constexpr auto Source2EngineToClient001                 = 0x5BB100; // engine2.dll + rva
}
namespace server_dll { // server.dll
    constexpr auto NavGameTest001                           = 0x4B19E08; // server.dll + rva
    constexpr auto Source2GameDirector001                   = 0x5165700; // server.dll + rva
    constexpr auto ServerToolsInfo_001                      = 0x4AA98D8; // server.dll + rva
    constexpr auto Source2GameClients001                    = 0x4AA8670; // server.dll + rva
    constexpr auto Source2GameEntities001                   = 0x4AA9080; // server.dll + rva
    constexpr auto Source2Server001                         = 0x4AA8ED0; // server.dll + rva
    constexpr auto Source2ServerConfig001                   = 0x5041F88; // server.dll + rva
    constexpr auto EntitySubclassUtilsV001                  = 0x48C61C0; // server.dll + rva
}
namespace schemasystem_dll { // schemasystem.dll
    constexpr auto SchemaSystem_001                         = 0x76610; // schemasystem.dll + rva
}
namespace resourcesystem_dll { // resourcesystem.dll
    constexpr auto ResourceSystem013                        = 0x970C0; // resourcesystem.dll + rva
}
namespace networksystem_dll { // networksystem.dll
    constexpr auto SerializedEntitiesVersion001             = 0x291210; // networksystem.dll + rva
    constexpr auto NetworkSystemVersion001                  = 0x291130; // networksystem.dll + rva
    constexpr auto NetworkMessagesVersion001                = 0x2A39E0; // networksystem.dll + rva
    constexpr auto FlattenedSerializersVersion001           = 0x2779E0; // networksystem.dll + rva
}
namespace vphysics2_dll { // vphysics2.dll
    constexpr auto VPhysics2_Interface_001                  = 0x472F10; // vphysics2.dll + rva
}
namespace particles_dll { // particles.dll
    constexpr auto ParticleSystemMgr003                     = 0x633FB0; // particles.dll + rva
}
namespace soundsystem_dll { // soundsystem.dll
    constexpr auto SoundBugBugService001_Client             = 0x536A10; // soundsystem.dll + rva
    constexpr auto SoundOpSystem001                         = 0x5368F0; // soundsystem.dll + rva
    constexpr auto SoundOpSystemEdit001                     = 0x536800; // soundsystem.dll + rva
    constexpr auto VMixEditTool001                          = 0xC9AC498; // soundsystem.dll + rva
    constexpr auto SoundSystem001                           = 0x64AF00; // soundsystem.dll + rva
}
namespace panorama_dll { // panorama.dll
    constexpr auto PanoramaUIEngine001                      = 0x586F60; // panorama.dll + rva
}
