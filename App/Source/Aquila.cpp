#include "Application.h"
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    Editor::Application app;
    app.Run();

    _CrtDumpMemoryLeaks();

    return 0;
}