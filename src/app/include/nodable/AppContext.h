#pragma once

namespace Nodable
{
    // forward decl
    class App;
    namespace Asm
    {
        class VM;
    }
    class Settings;
    class Language;
    class TextureManager;

    /**
     * @brief Simple structure to store application context.
     * (in order to avoid singleton I use a context instance I pass to the application and to any instance requiring it)
     */
    class AppContext
    {
    friend class App;
    public:
        AppContext(App* _app)
            : settings(nullptr)
            , app(_app)
            {}

        ~AppContext();

        Settings* settings;
        App*      app;
        Asm::VM*  vm;
        const Language* language;
        TextureManager* texture_manager;
    private:
        static AppContext* create_default(App* _app);
    };


}