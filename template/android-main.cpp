#include "imgui/imgui.h"
#include "imgui/imgui_impl_android.h"
#include "imgui/imgui_impl_opengl3.h"
#include "IconsMaterialDesign.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <string>

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
static ImRad::IOUserData    g_IOUserData;
static int                  g_ImeType = 0;

// Forward declarations of helper functions
static void Init(struct android_app* app);
static void Shutdown();
static void MainLoopStep();
static int ShowSoftKeyboardInput(int mode);
static int GetAssetData(const char* filename, void** out_data);
static void GetDisplayInfo();

//-----------------------------------------------------------------

void Draw()
{
	// TODO: Add your drawing code here
}

//-----------------------------------------------------------------

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_OnKeyboardShown(JNIEnv *env, jobject thiz, jboolean b) {
    if (!b)
        g_ImeType = 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_OnScreenRotation(JNIEnv *env, jobject thiz, jint angle) {
    switch (angle) {
        case 0:
            g_IOUserData.displayRectMinOffset = { 0, 0 };
            g_IOUserData.displayRectMaxOffset = { 0, (float)g_NavBarHeight };
            break;
        case 90:
            g_IOUserData.displayRectMinOffset = { 0, 0 };
            g_IOUserData.displayRectMaxOffset = { (float)g_NavBarHeight, 0 };
            break;
        case 270:
            g_IOUserData.displayRectMinOffset = { (float)g_NavBarHeight, 0 };
            g_IOUserData.displayRectMaxOffset = { 0, 0 };
            break;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_OnInputCharacter(JNIEnv *env, jobject thiz, jint ch) {
    ImGui::GetIO().AddInputCharacter(ch);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_MainActivity_OnSpecialKey(JNIEnv *env, jobject thiz, jint code) {
    ImGui::GetIO().AddKeyEvent(ImGuiKey_AppForward, true);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_AppForward, false);
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
            Shutdown();
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
        while (ALooper_pollAll(g_Initialized ? 0 : -1, nullptr, &out_events, (void**)&out_data) >= 0)
        {
            // Process one event
            if (out_data != nullptr)
                out_data->process(app, out_data);

            // Exit the app by returning from within the infinite loop
            if (app->destroyRequested != 0)
            {
                // shutdown() should have been called already while processing the
                // app command APP_CMD_TERM_WINDOW. But we play save here
                if (!g_Initialized)
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
        // Load ImRAD style including fonts:
        // ImRad::LoadStyle(fname, g_IOUserData.dpiScale);
        // Alternatively, setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
        ImGui::GetStyle().ScaleAllSizes(g_IOUserData.dpiScale);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        // - Android: The TTF files have to be placed into the assets/ directory (android/app/src/main/assets), we use our GetAssetData() helper to retrieve them.
        void *roboto_data, *material_data;
        int roboto_size, material_size;
        ImFont *font;
        //font_data_size = GetAssetData("DroidSans.ttf", &font_data);
        //font = io.Fonts->AddFontFromMemoryTTF(font_data, font_data_size, 16.0f);
        //IM_ASSERT(font != nullptr);
        roboto_size = GetAssetData("Roboto-Regular.ttf", &roboto_data);
        material_size = GetAssetData(FONT_ICON_FILE_NAME_MD, &material_data);
        static ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};

        ImFontConfig cfg;
        font = io.Fonts->AddFontFromMemoryTTF(roboto_data, roboto_size,
                                              g_IOUserData.dpiScale * 17.0f);
        IM_ASSERT(font != nullptr);
        cfg.MergeMode = true;
        cfg.GlyphOffset.y = 17.f * g_IOUserData.dpiScale / 5;
        font = io.Fonts->AddFontFromMemoryTTF(material_data, material_size,
                                              g_IOUserData.dpiScale * 17.0f, &cfg, icons_ranges);
        IM_ASSERT(font != nullptr);

        // TODO 
        //someActivity.Open();
    }
}

// this is called during app switching so no need to destroy everything
void Shutdown()
{
    if (g_EglSurface != EGL_NO_SURFACE)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(g_EglDisplay, g_EglSurface);
        g_EglSurface = EGL_NO_SURFACE;
    }


    /*if (!g_Initialized)
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
    ANativeWindow_release(g_App->window);

    g_Initialized = false;*/
}

void MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    // Our state
    static ImVec4 clear_color = ImVec4(0.f, 0.f, 0.0f, 1.00f);

    // Open on-screen (soft) input if requested by Dear ImGui
    int newImeType = io.WantTextInput ? g_IOUserData.imeType : 0;
    if (newImeType != g_ImeType) {
        g_ImeType = newImeType;
        ShowSoftKeyboardInput(g_ImeType);
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

// Unfortunately, there is no way to show the on-screen input from native code.
// Therefore, we call ShowSoftKeyboardInput() of the main activity implemented in MainActivity.kt via JNI.
static int ShowSoftKeyboardInput(int mode)
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    jint jni_return = java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return -1;

    jni_return = java_vm->AttachCurrentThread(&java_env, nullptr);
    if (jni_return != JNI_OK)
        return -2;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == nullptr)
        return -3;

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "showSoftInput", "(I)V");
    if (method_id == nullptr)
        return -4;

    java_env->CallVoidMethod(g_App->activity->clazz, method_id, mode);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return -5;

    return 0;
}

// Helper to retrieve data placed into the assets/ directory (android/app/src/main/assets)
static int GetAssetData(const char* filename, void** outData)
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

// Retrieve some display related information like DPI
static void GetDisplayInfo()
{
    JavaVM* java_vm = g_App->activity->vm;
    JNIEnv* java_env = nullptr;

    jint jni_return = java_vm->GetEnv((void**)&java_env, JNI_VERSION_1_6);
    if (jni_return == JNI_ERR)
        return;

    jni_return = java_vm->AttachCurrentThread(&java_env, nullptr);
    if (jni_return != JNI_OK)
        return;

    jclass native_activity_clazz = java_env->GetObjectClass(g_App->activity->clazz);
    if (native_activity_clazz == nullptr)
        return;

    jmethodID method_id = java_env->GetMethodID(native_activity_clazz, "getDpi", "()I");
    if (method_id == nullptr)
        return;

    jint dpi = java_env->CallIntMethod(g_App->activity->clazz, method_id);
    g_NavBarHeight = 48 * dpi / 160.0; //android dp definition
    g_IOUserData.dpiScale = dpi / 140.0; //relative to laptop screen DPI;
    ImGui::GetIO().UserData = &g_IOUserData;

    method_id = java_env->GetMethodID(native_activity_clazz, "getRotation", "()I");
    if (method_id == nullptr)
        return;

    jint rot = java_env->CallIntMethod(g_App->activity->clazz, method_id);
    Java_com_example_myapplication_MainActivity_OnScreenRotation(java_env, g_App->activity->clazz, rot);

    jni_return = java_vm->DetachCurrentThread();
    if (jni_return != JNI_OK)
        return;
}
