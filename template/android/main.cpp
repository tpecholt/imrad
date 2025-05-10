#include "imrad.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"
//#include "IconsMaterialDesign.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>

// TODO: include activity headers here

// Data
static EGLDisplay           g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface           g_EglSurface = EGL_NO_SURFACE;
static EGLContext           g_EglContext = EGL_NO_CONTEXT;
static EGLConfig            g_EglConfig;
static struct android_app*  g_App = nullptr;
static bool                 g_Initialized = false;
static char                 g_LogTag[] = "ImGuiExample";
static std::string          g_IniFilename = "";
static int                  g_NavBarHeight = 0;
static int                  g_StatusBarHeight = 0;
static int                  g_KbdHeight = 0;
static int                  g_RotAngle = 0;
static ImRad::IOUserData    g_IOUserData;
static int                  g_ImeType = 0;

// Forward declarations of helper functions
static void Init(struct android_app* app);
static void Shutdown();
static void FreeMem();
static void MainLoopStep();
static int ShowSoftKeyboardInput(int mode);
static void PerformHapticFeedback(int kind);
/*static*/ int GetAssetData(const char* filename, void** out_data);
static void GetDisplayInfo();
static void UpdateScreenRect();

//-----------------------------------------------------------------

void Draw()
{
    // TODO: Call your drawing code here
    //mainActivity.Draw();
    
    bool isOpen;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { ImGui::GetTextLineHeight()/2, ImGui::GetTextLineHeight()/2 });
    ImGui::SetNextWindowPos(g_IOUserData.WorkRect().Min);
    ImGui::SetNextWindowSize(g_IOUserData.WorkRect().GetSize());
    if (ImGui::Begin("TODO", &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
    {
        ImRad::ScrollWhenDragging(false);
        ImGui::TextWrapped("Welcome to android development with ImRAD!");
        ImGui::Spacing();
        ImGui::TextWrapped("Your next steps:");
        ImGui::Spacing();
        ImGui::TextWrapped("1. Load suitable fonts");
        ImGui::Indent();
        ImGui::Bullet();
        ImGui::TextWrapped("Create assets directory and put Roboto and Material fonts there");
        ImGui::Bullet();
        ImGui::TextWrapped("Uncomment code to load fonts");
        ImGui::Unindent();
        ImGui::Spacing();
        ImGui::TextWrapped("2. Patch imgui_impl_android.cpp");
        ImGui::Indent();
        ImGui::Bullet();
        ImGui::TextWrapped("As reported in #6627 the stock version leaves all buttons in pushed state after a press");
        ImGui::Button(" Test here ");
        ImGui::Unindent();
        ImGui::Spacing();
        ImGui::TextWrapped("3. Add your first activity to the project");
        ImGui::Indent();
        ImGui::Bullet();
        ImGui::TextWrapped("Design activity in ImRAD");
        ImGui::Bullet();
        ImGui::TextWrapped("Uncomment code to use it");
        ImGui::Unindent();
        ImGui::Spacing();
        ImGui::TextWrapped("Have fun!");

        ImGui::End();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

//-----------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL
Java_${JNI_PACKAGE}_MainActivity_OnKeyboardShown(JNIEnv *env, jobject thiz, jint height)
{
    g_KbdHeight = height;
    g_IOUserData.kbdShown = height / g_IOUserData.dpiScale > 100;
    if (!g_IOUserData.kbdShown)
        g_ImeType = 0;
    UpdateScreenRect();
}

extern "C"
JNIEXPORT void JNICALL
Java_${JNI_PACKAGE}_MainActivity_OnScreenRotation(JNIEnv *env, jobject thiz, jint angle)
{
    g_RotAngle = angle;
    UpdateScreenRect();
}

extern "C"
JNIEXPORT void JNICALL
Java_${JNI_PACKAGE}_MainActivity_OnInputCharacter(JNIEnv *env, jobject thiz, jint ch)
{
    ImGui::GetIO().AddInputCharacter(ch);
}

extern "C"
JNIEXPORT void JNICALL
Java_${JNI_PACKAGE}_MainActivity_OnSpecialKey(JNIEnv *env, jobject thiz, jint code)
{
    ImGuiKey key = code == 4 ? ImGuiKey_AppBack : ImGuiKey_AppForward;
    ImGui::GetIO().AddKeyEvent(key, true);
    ImGui::GetIO().AddKeyEvent(key, false);
}

// Main code
static void handleAppCmd(struct android_app* app, int32_t appCmd)
{
    switch (appCmd)
    {
        case APP_CMD_SAVE_STATE:
            break;
        case APP_CMD_INIT_WINDOW:
            Init(app);
            break;
        case APP_CMD_TERM_WINDOW:
            FreeMem();
            break;
        case APP_CMD_GAINED_FOCUS:
        case APP_CMD_LOST_FOCUS:
            break;
    }
}

static int32_t handleInputEvent(struct android_app* app, AInputEvent* inputEvent)
{
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}

void android_main(struct android_app* app)
{
    app->onAppCmd = handleAppCmd;
    app->onInputEvent = handleInputEvent;

    while (true)
    {
        int out_events;
        struct android_poll_source* out_data;

        // Poll all events. If the app is not visible, this loop blocks until g_Initialized == true.
        while (ALooper_pollOnce(g_Initialized ? 0 : -1, nullptr, &out_events, (void**)&out_data) >= 0)
        {
            // Process one event
            if (out_data != nullptr)
                out_data->process(app, out_data);

            // Exit the app by returning from within the infinite loop
            if (app->destroyRequested != 0)
            {
                if (g_Initialized)
                    Shutdown();

                return;
            }
        }

        // Initiate a new frame
        MainLoopStep();
    }
}

void Init(struct android_app* app)
{
    g_App = app;
    ANativeWindow_acquire(g_App->window);

    // Initialize EGL
    // This is mostly boilerplate code for EGL...
    if (g_EglDisplay == EGL_NO_DISPLAY)
    {
        g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_EglDisplay == EGL_NO_DISPLAY)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s",
                                "eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");

        if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s",
                                "eglInitialize() returned with an error");

        const EGLint egl_attributes[] = {EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                                         EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                         EGL_NONE};
        EGLint num_configs = 0;
        if (eglChooseConfig(g_EglDisplay, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s",
                                "eglChooseConfig() returned with an error");
        if (num_configs == 0)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s",
                                "eglChooseConfig() returned 0 matching config");

        // Get the first matching config
        eglChooseConfig(g_EglDisplay, egl_attributes, &g_EglConfig, 1, &num_configs);
        EGLint egl_format;
        eglGetConfigAttrib(g_EglDisplay, g_EglConfig, EGL_NATIVE_VISUAL_ID, &egl_format);
        ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);
    }
    if (g_EglContext == EGL_NO_CONTEXT)
    {
        const EGLint egl_context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        g_EglContext = eglCreateContext(g_EglDisplay, g_EglConfig, EGL_NO_CONTEXT,
                                        egl_context_attributes);

        if (g_EglContext == EGL_NO_CONTEXT)
            __android_log_print(ANDROID_LOG_ERROR, g_LogTag, "%s",
                                "eglCreateContext() returned EGL_NO_CONTEXT");
    }
    if (g_EglSurface == EGL_NO_SURFACE)
    {
        g_EglSurface = eglCreateWindowSurface(g_EglDisplay, g_EglConfig, g_App->window, nullptr);
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);
    }

    if (!g_Initialized)
    {
        g_Initialized = true;
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        // Redirect loading/saving of .ini file to our location.
        // Make sure 'g_IniFilename' persists while we use Dear ImGui.
        g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
        io.IniFilename = g_IniFilename.c_str();

        // Setup Platform/Renderer backends
        ImGui_ImplAndroid_Init(g_App->window);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        GetDisplayInfo();
        io.MouseDragThreshold *= g_IOUserData.dpiScale;
        // Load ImRAD style including fonts:
        // ImRad::LoadStyle(fname, g_IOUserData.dpiScale);
        // Alternatively, setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
        ImGui::GetStyle().ScaleAllSizes(g_IOUserData.dpiScale);

        // TODO: Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // - Android: The TTF files have to be placed into the assets/ directory (android/app/src/main/assets), we use our GetAssetData() helper to retrieve them

        ImFontConfig cfg;
        cfg.SizePixels = g_IOUserData.dpiScale * 18.0f;
        io.Fonts->AddFontDefault(&cfg);

        /*void *roboto_data, *material_data;
        int roboto_size, material_size;
        ImFont *font;
        roboto_size = GetAssetData("Roboto-Regular.ttf", &roboto_data);
        material_size = GetAssetData(FONT_ICON_FILE_NAME_MD, &material_data);
        static ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};

        font = io.Fonts->AddFontFromMemoryTTF(roboto_data, roboto_size,
                                              g_IOUserData.dpiScale * 20.0f);
        IM_ASSERT(font != nullptr);
        cfg.MergeMode = true;
        cfg.GlyphOffset.y = 20.f * g_IOUserData.dpiScale / 5;
        font = io.Fonts->AddFontFromMemoryTTF(material_data, material_size,
                                              g_IOUserData.dpiScale * 20.0f, &cfg, icons_ranges);
        IM_ASSERT(font != nullptr);*/
    }
}

// this is called during app switching so no need to destroy everything
void FreeMem()
{
    if (g_EglSurface != EGL_NO_SURFACE)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(g_EglDisplay, g_EglSurface);
        g_EglSurface = EGL_NO_SURFACE;
    }
}

void Shutdown()
{
    if (!g_Initialized)
        return;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);

        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);

        eglTerminate(g_EglDisplay);
    }

    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;
    
    if (g_App->window)
        ANativeWindow_release(g_App->window);

    g_Initialized = false;
}

void MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_EglDisplay == EGL_NO_DISPLAY || g_EglSurface == EGL_NO_SURFACE)
        return;

    // Our state
    static ImVec4 clear_color = ImVec4(0.f, 0.f, 0.0f, 1.00f);

    // Open on-screen (soft) input if requested by Dear ImGui
    int newImeType = io.WantTextInput ? g_IOUserData.imeType : 0;
    if (newImeType != g_ImeType) {
        g_ImeType = newImeType;
        ShowSoftKeyboardInput(g_ImeType);
    }
    if (g_IOUserData.longPressID) {
        g_IOUserData.longPressID = 0;
        PerformHapticFeedback(0);
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    Draw();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

// Helper functions

// Helper to retrieve data placed into the assets/ directory (android/app/src/main/assets)
/*static*/ int GetAssetData(const char* filename, void** outData)
{
    int num_bytes = 0;
    AAsset* asset_descriptor = AAssetManager_open(g_App->activity->assetManager, filename, AASSET_MODE_BUFFER);
    if (asset_descriptor)
    {
        num_bytes = AAsset_getLength(asset_descriptor);
        *outData = IM_ALLOC(num_bytes);
        int64_t num_bytes_read = AAsset_read(asset_descriptor, *outData, num_bytes);
        AAsset_close(asset_descriptor);
        IM_ASSERT(num_bytes_read == num_bytes);
    }
    return num_bytes;
}

struct JniBlock 
{
    JniBlock()
    {
        JavaVM* java_vm = g_App->activity->vm;

        jint jni_return = java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6);
        if (jni_return == JNI_ERR)
            return;

        jni_return = java_vm->AttachCurrentThread(&java_env, nullptr);
        if (jni_return != JNI_OK)
            return;

        native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
        if (native_activity_clazz == nullptr)
            return;
    }
    ~JniBlock()
    {
        if (!java_env)
            return;
        JavaVM* java_vm = g_App->activity->vm;
        jint jni_return = java_vm->DetachCurrentThread();
        if (jni_return != JNI_OK)
            return;
    }
    JNIEnv* operator-> ()
    {
        return java_env;
    }

    JNIEnv* java_env = nullptr;
    jclass native_activity_clazz = nullptr;
};

// Unfortunately, there is no way to show the on-screen input from native code.
// Therefore, we call ShowSoftKeyboardInput() of the main activity implemented in MainActivity.kt via JNI.
static int ShowSoftKeyboardInput(int mode)
{
    JniBlock bl;
    jmethodID method_id = bl->GetMethodID(bl.native_activity_clazz, "showSoftInput", "(I)V");
    if (method_id == nullptr)
        return -4;
    bl->CallVoidMethod(g_App->activity->clazz, method_id, mode);
    return 0;
}

static void PerformHapticFeedback(int kind)
{
    JniBlock bl;
    jmethodID method_id = bl->GetMethodID(bl.native_activity_clazz, "performHapticFeedback", "(I)V");
    if (!method_id)
        return;
    bl->CallVoidMethod(g_App->activity->clazz, method_id, kind);
}

// Retrieve some display related information like DPI
static void GetDisplayInfo()
{
    JniBlock bl;

    jmethodID method_id = bl->GetMethodID(bl.native_activity_clazz, "getDpi", "()I");
    if (method_id == nullptr)
        return;

    jint dpi = bl->CallIntMethod(g_App->activity->clazz, method_id);
    //g_NavBarHeight = 48 * dpi / 160.0; //android dp definition
    g_StatusBarHeight = 40 * dpi / 160.0;
    g_IOUserData.dpiScale = dpi / 140.0; //relative to laptop screen DPI;
    //round dpiScale otherwise when using box sizers floating point errors in imgui
    //accumulate and cause the window contentRegionRect to grow continuously
    g_IOUserData.dpiScale = std::round(1000 * g_IOUserData.dpiScale) / 1000.0;
    ImGui::GetIO().UserData = &g_IOUserData;

    method_id = bl->GetMethodID(bl.native_activity_clazz, "getRotation", "()I");
    if (method_id == nullptr)
        return;

    g_RotAngle = bl->CallIntMethod(g_App->activity->clazz, method_id);
    UpdateScreenRect();
}

void UpdateScreenRect()
{
    switch (g_RotAngle) {
        case 0:
            g_IOUserData.displayOffsetMin = { 0, (float)g_StatusBarHeight };
            g_IOUserData.displayOffsetMax = { 0, (float)g_NavBarHeight + (float)g_KbdHeight };
            break;
        case 90:
            g_IOUserData.displayOffsetMin = { 0, (float)g_StatusBarHeight };
            g_IOUserData.displayOffsetMax = { (float)g_NavBarHeight, (float)g_KbdHeight };
            break;
        case 270:
            g_IOUserData.displayOffsetMin = { (float)g_NavBarHeight, (float)g_StatusBarHeight };
            g_IOUserData.displayOffsetMax = { 0, (float)g_KbdHeight };
            break;
    }
}
