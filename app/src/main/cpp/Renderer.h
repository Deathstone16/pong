#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>
#include <chrono>

#include "Model.h"
#include "Shader.h"
#include "PongGame.h"

struct android_app;

class Renderer {
public:
    Renderer(android_app *pApp);
    virtual ~Renderer();

    void handleInput();
    void render();

private:
    void initRenderer();
    void updateRenderArea();
    void createModels();
    void updateGame(float dt);

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;
    std::unique_ptr<PongGame> game_;
    
    std::chrono::steady_clock::time_point lastFrameTime_;
    bool firstFrame_ = true;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H