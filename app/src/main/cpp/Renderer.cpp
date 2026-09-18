#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>
#include <chrono>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"
#include "PongGame.h"

#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

#define CORNFLOWER_BLUE 0.1f, 0.05f, 0.15f, 1

static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform mat4 uProjection;
uniform mat4 uModelView;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * uModelView * vec4(inPosition, 1.0);
}
)vertex";

static const char *fragment = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform float uUseTexture;
uniform float uAlpha;

out vec4 outColor;

void main() {
    vec4 texColor = texture(uTexture, fragUV);
    vec4 color = mix(uColor, texColor, uUseTexture);
    color.a *= uAlpha;
    outColor = color;
}
)fragment";

Renderer::Renderer(android_app *pApp) :
        app_(pApp),
        display_(EGL_NO_DISPLAY),
        surface_(EGL_NO_SURFACE),
        context_(EGL_NO_CONTEXT),
        width_(0),
        height_(0),
        shaderNeedsNewProjectionMatrix_(true),
        lastFrameTime_(std::chrono::steady_clock::now()) {
    initRenderer();
}

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    width_ = -1;
    height_ = -1;

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection"));
    assert(shader_);

    shader_->activate();

    glClearColor(CORNFLOWER_BLUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    createModels();
    
    lastFrameTime_ = std::chrono::steady_clock::now();
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
        shaderNeedsNewProjectionMatrix_ = true;
        
        if (game_) {
            game_->resize(width, height);
        }
    }
}

void Renderer::createModels() {
    game_ = std::make_unique<PongGame>(width_ > 0 ? width_ : 800, height_ > 0 ? height_ : 600);
    auto assetManager = app_->activity->assetManager;
    game_->createModels(assetManager);
}

void Renderer::updateGame(float dt) {
    if (game_) {
        game_->update(dt);
    }
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        return;
    }

    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                if (game_) {
                    game_->handleTouch(x, y, true, false);
                }
                break;
            case AMOTION_EVENT_ACTION_CANCEL:
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                if (game_) {
                    game_->handleTouch(x, y, false, false);
                }
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    if (game_) {
                        game_->handleTouch(x, y, true, true);
                    }
                }
                break;
            default:
                break;
        }
    }
    android_app_clear_motion_events(inputBuffer);

    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        aout << "Key: " << keyEvent.keyCode <<" ";
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                aout << "Key Down";
                break;
            case AKEY_EVENT_ACTION_UP:
                aout << "Key Up";
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                aout << "Multiple Key Actions";
                break;
            default:
                aout << "Unknown KeyEvent Action: " << keyEvent.action;
        }
        aout << std::endl;
    }
    android_app_clear_key_events(inputBuffer);
}

void Renderer::render() {
    updateRenderArea();

    auto now = std::chrono::steady_clock::now();
    float dt = 0.016f; // Default 60fps
    if (!firstFrame_) {
        dt = std::chrono::duration<float>(now - lastFrameTime_).count();
        dt = std::min(dt, 0.05f); // Cap at 20fps minimum
    }
    firstFrame_ = false;
    lastFrameTime_ = now;

    updateGame(dt);

    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16] = {0};
        const float aspect = float(width_) / height_;
        Utility::buildOrthographicMatrix(
                projectionMatrix,
                4.0f / aspect, // Keeps the game's 8-unit-wide world visible on every screen.
                aspect,
                -10.0f,
                10.0f);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader_->activate();

    if (game_) {
        game_->render(shader_.get());
    }

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}
