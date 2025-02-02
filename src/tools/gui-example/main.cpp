#include "AppExample.h"
#include "tools/core/TryCatch.h"

using namespace tools;

int main(int argc, char *argv[])
{
    TOOLS_try
    {
        // Instantiate the application using the predefined configuration
        AppExample app;
        app.init();

        while( !app.should_stop() )
        {
            app.update();
            app.draw();
        }
        app.shutdown();
    }
    TOOLS_catch
    return 0;
}
