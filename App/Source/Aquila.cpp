// #include "Application.h"

// int main()
// {
//     if (Core::Platform::Initialize()) {
//         const auto& info = Core::Platform::GetPlatformInfo();
//         Debug::Log("Platform initialized successfully");
//         Debug::Log("Running on: " + std::string(info.name) + " (" + std::string(info.version) + ")");
//         Debug::Log("CPU Cores: " + std::to_string(info.cpuCores));
//         Debug::Log("Total Memory: " + std::to_string(info.totalMemory / (1024 * 1024)) + " MB");

//         ThreadHandle* mainThread = Threading::CreateThread([](void*) -> int {
//             Debug::Log("Thread started");
            
//             Debug::Log("Counter initialized");
//             int counter = 0;

//             while (counter < 10) {
//                 Threading::Sleep(1000);
//                 Debug::Log("Thread is running...");
//                 if (counter == 5) {
//                     Debug::Log("Counter reached 5");
//                 }
//                 counter++;
//             }
//             Debug::Log("Thread finished");
//             return 0;
//         }, "MainThread");

//         Threading::DetachThread(mainThread); 

//         Editor::Application app;
//         app.Run();


//         Core::Platform::Shutdown();

//     }
//     return 0;



//     // return 0;
// }