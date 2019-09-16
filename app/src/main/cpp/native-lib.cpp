#pragma clang diagnostic push
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <errno.h>


#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <unistd.h>
#include <android/sensor.h>

// Specific implementations
#include "gp_platform.h"
#include "gp_android.h"
#include "game.h"

#include <dlfcn.h>
ASensorManager* AcquireASensorManagerInstance(android_app* app) {

    if(!app)
        return nullptr;

    typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
    void* androidHandle = dlopen("libandroid.so", RTLD_NOW);
    PF_GETINSTANCEFORPACKAGE getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)
            dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
    if (getInstanceForPackageFunc) {
        JNIEnv* env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, NULL);

        jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
        jmethodID midGetPackageName = env->GetMethodID(android_content_Context,
                                                       "getPackageName",
                                                       "()Ljava/lang/String;");
        jstring packageName= (jstring)env->CallObjectMethod(app->activity->clazz,
                                                            midGetPackageName);

        const char *nativePackageName = env->GetStringUTFChars(packageName, 0);
        ASensorManager* mgr = getInstanceForPackageFunc(nativePackageName);
        env->ReleaseStringUTFChars(packageName, nativePackageName);
        app->activity->vm->DetachCurrentThread();
        if (mgr) {
            dlclose(androidHandle);
            return mgr;
        }
    }

    typedef ASensorManager *(*PF_GETINSTANCE)();
    PF_GETINSTANCE getInstanceFunc = (PF_GETINSTANCE)
            dlsym(androidHandle, "ASensorManager_getInstance");
    // by all means at this point, ASensorManager_getInstance should be available
    assert(getInstanceFunc);
    dlclose(androidHandle);

    return getInstanceFunc();
}
/******************************************************************
 * OpenGL context handler
 * The class handles OpenGL and EGL context based on Android activity life cycle
 * The caller needs to call corresponding methods for each activity life cycle
 *events as it's done in sample codes.
 *
 * Also the class initializes OpenGL ES3 when the compatible driver is installed
 *in the device.
 * getGLVersion() returns 3.0~ when the device supports OpenGLES3.0
 *
 * Thread safety: OpenGL context is expecting used within dedicated single
 *thread,
 * thus GLContext class is not designed as a thread-safe
 */
class GLContext {
private:
    // EGL configurations
    ANativeWindow *window_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLConfig config_;

    // Screen parameters
    int32_t screen_width_;
    int32_t screen_height_;
    int32_t color_size_;
    int32_t depth_size_;

    // Flags
    bool gles_initialized_;
    bool egl_context_initialized_;
    bool es3_supported_;
    float gl_version_;
    bool context_valid_;

    void InitGLES();

    void Terminate();

    bool InitEGLSurface();

    bool InitEGLContext();

    GLContext(GLContext const &);

    void operator=(GLContext const &);

    GLContext();

    virtual ~GLContext();

public:
    static GLContext *GetInstance() {
        // Singleton
        static GLContext instance;

        return &instance;
    }

    bool Init(ANativeWindow *window);

    EGLint Swap();

    bool Invalidate();

    void Suspend();

    EGLint Resume(ANativeWindow *window);

    ANativeWindow *GetANativeWindow(void) const { return window_; };

    int32_t GetScreenWidth() const { return screen_width_; }

    int32_t GetScreenHeight() const { return screen_height_; }

    int32_t GetBufferColorSize() const { return color_size_; }

    int32_t GetBufferDepthSize() const { return depth_size_; }

    float GetGLVersion() const { return gl_version_; }

    bool CheckExtension(const char *extension);

    EGLDisplay GetDisplay() const { return display_; }

    EGLSurface GetSurface() const { return surface_; }
};

GLContext::GLContext()
        : window_(nullptr),
          display_(EGL_NO_DISPLAY),
          surface_(EGL_NO_SURFACE),
          context_(EGL_NO_CONTEXT),
          screen_width_(0),
          screen_height_(0),
          gles_initialized_(false),
          egl_context_initialized_(false),
          es3_supported_(false) {}

void GLContext::InitGLES() {
    if (gles_initialized_) return;
    //
    // Initialize OpenGL ES 3 if available
    //
    /*const char* versionStr = (const char*)glGetString(GL_VERSION);
    if (strstr(versionStr, "OpenGL ES 3.")) {
        es3_supported_ = true;
        gl_version_ = 3.0f;
    } else {
        gl_version_ = 2.0f;
    }*/
    gl_version_ = 2.0f;
    gles_initialized_ = true;
}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
GLContext::~GLContext() { Terminate(); }

bool GLContext::Init(ANativeWindow *window) {
    if (egl_context_initialized_) return true;

    //
    // Initialize EGL
    //
    window_ = window;
    InitEGLSurface();
    InitEGLContext();
    InitGLES();

    egl_context_initialized_ = true;

    return true;
}

bool GLContext::InitEGLSurface() {
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display_, 0, 0);

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,  // Request opengl ES2.0
                              EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              24,
                              EGL_NONE};
    color_size_ = 8;
    depth_size_ = 24;

    EGLint num_configs;
    eglChooseConfig(display_, attribs, &config_, 1, &num_configs);

    if (!num_configs) {
        // Fall back to 16bit depth buffer
        const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                                  EGL_OPENGL_ES2_BIT,  // Request opengl ES2.0
                                  EGL_SURFACE_TYPE,
                                  EGL_WINDOW_BIT,
                                  EGL_BLUE_SIZE,
                                  8,
                                  EGL_GREEN_SIZE,
                                  8,
                                  EGL_RED_SIZE,
                                  8,
                                  EGL_DEPTH_SIZE,
                                  16,
                                  EGL_NONE};
        eglChooseConfig(display_, attribs, &config_, 1, &num_configs);
        depth_size_ = 16;
    }

    if (!num_configs) {
        android_logi("Unable to retrieve EGL config");
        return false;
    }

    surface_ = eglCreateWindowSurface(display_, config_, window_, NULL);
    eglQuerySurface(display_, surface_, EGL_WIDTH, &screen_width_);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &screen_height_);

    return true;
}

bool GLContext::InitEGLContext() {
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION,
                                      2,  // Request opengl ES2.0
                                      EGL_NONE};
    context_ = eglCreateContext(display_, config_, NULL, context_attribs);

    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE) {
        android_logi("Unable to eglMakeCurrent");
        return false;
    }

    context_valid_ = true;
    return true;
}

EGLint GLContext::Swap() {
    bool b = eglSwapBuffers(display_, surface_);
    if (!b) {
        EGLint err = eglGetError();
        if (err == EGL_BAD_SURFACE) {
            // Recreate surface
            InitEGLSurface();
            return EGL_SUCCESS;  // Still consider glContext is valid
        } else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT) {
            // Context has been lost!!
            context_valid_ = false;
            Terminate();
            InitEGLContext();
        }
        return err;
    }
    return EGL_SUCCESS;
}

void GLContext::Terminate() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }

        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }
        eglTerminate(display_);
    }

    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    window_ = nullptr;
    context_valid_ = false;
}

EGLint GLContext::Resume(ANativeWindow *window) {
    if (egl_context_initialized_ == false) {
        Init(window);
        return EGL_SUCCESS;
    }

    int32_t original_widhth = screen_width_;
    int32_t original_height = screen_height_;

    // Create surface
    window_ = window;
    surface_ = eglCreateWindowSurface(display_, config_, window_, NULL);
    eglQuerySurface(display_, surface_, EGL_WIDTH, &screen_width_);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &screen_height_);

    if (screen_width_ != original_widhth || screen_height_ != original_height) {
        // Screen resized
        android_logi("Screen resized");
    }

    if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_TRUE)
        return EGL_SUCCESS;

    EGLint err = eglGetError();
    //android_logw("Unable to eglMakeCurrent %d", err);
    android_logi("Unable to eglMakeCurrent %d");

    if (err == EGL_CONTEXT_LOST) {
        // Recreate context
        android_logi("Re-creating egl context");
        InitEGLContext();
    } else {
        // Recreate surface
        Terminate();
        InitEGLSurface();
        InitEGLContext();
    }

    return err;
}

void GLContext::Suspend() {
    if (surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
    }
}

bool GLContext::Invalidate() {
    Terminate();

    egl_context_initialized_ = false;
    return true;
}


struct android_app;

// @TODO refactor Engine. This is a mess.
class Engine {
    GLContext *gl_context_;

    State game_state;

    bool initialized_resources_;
    bool has_focus_;

    android_app *app_;

    ASensorManager* sensor_manager_;
    const ASensor* accelerometer_sensor_;
    ASensorEventQueue* sensor_event_queue_;



    void UpdateFPS(float fFPS);

    void ShowUI();

public:

    static void HandleCmd(struct android_app *app, int32_t cmd);

    static int32_t HandleInput(android_app *app, AInputEvent *event);

    Engine();

    ~Engine();

    void SetState(android_app *app);

    int InitDisplay(android_app *app, Engine *engine);

    void LoadResources(AAssetManager *asset_manager, Asset *assets, int total_assets);

    void UnloadResources(Asset *assets, int total_assets);

    void DrawFrame(Engine *engine, AAssetManager *asset_manager);

    void TermDisplay();

    void TrimMemory();

    bool IsReady();

    void UpdatePosition(AInputEvent *event, int32_t iIndex, float &fX, float &fY);

    void InitSensors();

    void ProcessSensors(int32_t id);

    void SuspendSensors();

    void ResumeSensors();
};

Engine::Engine()
        : initialized_resources_(false),
          has_focus_(false),
          app_(NULL) {
    gl_context_ = GLContext::GetInstance();
}

Engine::~Engine() {}

void Engine::LoadResources(AAssetManager *asset_manager, Asset *assets, int total_assets) {
    on_resources_loaded_game(assets, total_assets);
}

void Engine::UnloadResources(Asset *assets, int total_assets) {
    unload_resources_game(assets, total_assets);
}

/**
 * Initialize an EGL context for the current display.
 */
int Engine::InitDisplay(android_app *app, Engine *engine) {
    if (!initialized_resources_) {
        gl_context_->Init(app_->window);
        update_asset_manager(app->activity->assetManager);

        LOGI("InitDisplay ASSET_MANAGER IS NULL? %d", app->activity->assetManager == NULL);
        engine->game_state = init_state_game();

        LoadResources(app->activity->assetManager, engine->game_state.assets,
                      engine->game_state.total_assets);

        initialized_resources_ = true;
    } else if (app->window != gl_context_->GetANativeWindow()) {
        // Re-initialize ANativeWindow.
        // On some devices, ANativeWindow is re-created when the app is resumed
        assert(gl_context_->GetANativeWindow());
        UnloadResources(engine->game_state.assets, engine->game_state.total_assets);
        gl_context_->Invalidate();
        app_ = app;
        gl_context_->Init(app->window);
        LoadResources(app->activity->assetManager, engine->game_state.assets,
                      engine->game_state.total_assets);
        initialized_resources_ = true;
    } else {
        // initialize OpenGL ES and EGL
        if (EGL_SUCCESS == gl_context_->Resume(app_->window)) {
            UnloadResources(engine->game_state.assets, engine->game_state.total_assets);
            LoadResources(app->activity->assetManager, engine->game_state.assets,
                          engine->game_state.total_assets);
        } else {
            assert(0);
        }
    }

    ShowUI();

    // Initialize GL state.
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    init_game(&engine->game_state, gl_context_->GetScreenWidth(), gl_context_->GetScreenHeight());
    return 0;
}

void Engine::DrawFrame(Engine *engine, AAssetManager *asset_manager) {

    /*
     * float fps;
     * if (monitor_.Update(fps)) {
        UpdateFPS(fps);
    }
    renderer_.Update(monitor_.GetCurrentTime());
*/
    // Just fill the screen with a color.
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //renderer_.Render();
    render_game(&engine->game_state);

    // Swap
    if (EGL_SUCCESS != gl_context_->Swap()) {
        UnloadResources(engine->game_state.assets, engine->game_state.total_assets);
        update_asset_manager(asset_manager);
        LoadResources(asset_manager, engine->game_state.assets, engine->game_state.total_assets);
    }
}

/**
 * Tear down the EGL context currently associated with the display.
 */
void Engine::TermDisplay() { gl_context_->Suspend(); }

void Engine::TrimMemory() {
    android_logi("Trimming memory");
    gl_context_->Invalidate();
}

/**
 * Process the next input event.
 */
int32_t Engine::HandleInput(android_app *app, AInputEvent *event) {
    // Engine* eng = (Engine*)app->userData;
    return 0;
}

/**
 * Process the next main command.
 */
void Engine::HandleCmd(struct android_app *app, int32_t cmd) {
    Engine *eng = (Engine *) app->userData;


    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (app->window != NULL) {
                update_asset_manager(app->activity->assetManager);

                eng->InitDisplay(app, eng);
                eng->DrawFrame(eng, app->activity->assetManager);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            eng->TermDisplay();
            eng->has_focus_ = false;
            break;
        case APP_CMD_STOP:
            break;
        case APP_CMD_GAINED_FOCUS:
            eng->ResumeSensors();
            // Start animation
            eng->has_focus_ = true;
            break;
        case APP_CMD_LOST_FOCUS:
            update_asset_manager(app->activity->assetManager);
            eng->SuspendSensors();
            // Also stop animating.
            eng->has_focus_ = false;
            eng->DrawFrame(eng, app->activity->assetManager);
            break;
        case APP_CMD_LOW_MEMORY:
            // Free up GL resources
            eng->TrimMemory();
            break;
    }
}

void Engine::InitSensors() {
    sensor_manager_ = AcquireASensorManagerInstance(app_);
    accelerometer_sensor_ = ASensorManager_getDefaultSensor(
            sensor_manager_, ASENSOR_TYPE_ACCELEROMETER);
    sensor_event_queue_ = ASensorManager_createEventQueue(
            sensor_manager_, app_->looper, LOOPER_ID_USER, NULL, NULL);
}

void Engine::ProcessSensors(int32_t id) {
    // If a sensor has data, process it now.
    if (id == LOOPER_ID_USER) {
        if (accelerometer_sensor_ != NULL) {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(sensor_event_queue_, &event, 1) > 0) {
                // LOGI("Sensor: %f", event.acceleration.roll);
                update_input_game(event.acceleration.azimuth, event.acceleration.pitch, event.acceleration.roll);
            }
        }
    }
}

void Engine::ResumeSensors() {
    // When our app gains focus, we start monitoring the accelerometer.
    if (accelerometer_sensor_ != NULL) {
        ASensorEventQueue_enableSensor(sensor_event_queue_, accelerometer_sensor_);
        // We'd like to get 60 events per second (in us).
        ASensorEventQueue_setEventRate(sensor_event_queue_, accelerometer_sensor_,
                                       (1000L / 60) * 1000);
    }
}

void Engine::SuspendSensors() {
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    if (accelerometer_sensor_ != NULL) {
        ASensorEventQueue_disableSensor(sensor_event_queue_, accelerometer_sensor_);
    }
}

void Engine::SetState(android_app *state) {
    app_ = state;
/*    doubletap_detector_.SetConfiguration(app_->config);
    drag_detector_.SetConfiguration(app_->config);
    pinch_detector_.SetConfiguration(app_->config);*/
}

bool Engine::IsReady() {
    if (has_focus_) return true;

    return false;
}

void Engine::ShowUI() {
    /*JNIEnv* jni;
    app_->activity->vm->AttachCurrentThread(&jni, NULL);

    // Default class retrieval
    jclass clazz = jni->GetObjectClass(app_->activity->clazz);
    jmethodID methodID = jni->GetMethodID(clazz, "showUI", "()V");
    jni->CallVoidMethod(app_->activity->clazz, methodID);

    app_->activity->vm->DetachCurrentThread();*/
    return;
}

void Engine::UpdateFPS(float fFPS) {
/*    JNIEnv* jni;
    app_->activity->vm->AttachCurrentThread(&jni, NULL);

    // Default class retrieval
    jclass clazz = jni->GetObjectClass(app_->activity->clazz);
    jmethodID methodID = jni->GetMethodID(clazz, "updateFPS", "(F)V");
    jni->CallVoidMethod(app_->activity->clazz, methodID, fFPS);

    app_->activity->vm->DetachCurrentThread();*/
    return;
}

Engine g_engine;


/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {

    g_engine.SetState(state);

    // Init helper functions
    // JNIHelper::Init(state->activity, HELPER_CLASS_NAME);

    state->userData = &g_engine;
    state->onAppCmd = Engine::HandleCmd;
    state->onInputEvent = Engine::HandleInput;

//#ifdef USE_NDK_PROFILER
//    monstartup("libTeapotNativeActivity.so");
//#endif

    // Prepare to monitor accelerometer
    g_engine.InitSensors();

    // loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int id;
        int events;
        android_poll_source *source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, NULL, &events,
                                     (void **) &source)) >= 0) {
            // Process this event.
            if (source != NULL) source->process(state, source);

            g_engine.ProcessSensors(id);

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                g_engine.TermDisplay();
                return;
            }
        }

        if (g_engine.IsReady()) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            g_engine.DrawFrame(&g_engine, state->activity->assetManager);
        }
    }
}

//END_INCLUDE(all)
#pragma clang diagnostic pop