#ifndef PONG_GAME_H
#define PONG_GAME_H

#include <vector>
#include <memory>
#include "Model.h"
#include "TextureAsset.h"
#include "Shader.h"

class PinballBackground {
public:
    PinballBackground();
    void update(float dt);
    void render(const Shader* shader) const;
    void createModels(AAssetManager* assetManager);
    
private:
    struct Bumper {
        Vector2 pos;
        float radius;
        float pulsePhase;
        Vector3 color;
    };
    
    struct Flipper {
        Vector2 pos;
        float angle;
        float targetAngle;
        bool isLeft;
    };
    
    struct Particle {
        Vector2 pos;
        Vector2 vel;
        float life;
        float maxLife;
        Vector3 color;
    };
    
    struct RenderModel {
        std::vector<Vertex> vertices;
        std::vector<Index> indices;
        std::shared_ptr<TextureAsset> texture;
    };
    
    std::vector<Bumper> bumpers_;
    std::vector<Flipper> flippers_;
    std::vector<Particle> particles_;
    std::vector<RenderModel> models_;
    RenderModel particleModel_;
    float time_ = 0;
};

class PongGame {
public:
    PongGame(float screenWidth, float screenHeight);
    ~PongGame();
    
    void update(float dt);
    void render(const Shader* shader) const;
    void handleTouch(float x, float y, bool isDown, bool isMove);
    void resize(float width, float height);
    
    void createModels(AAssetManager* assetManager);
    
private:
    void resetBall();
    void checkCollisions();
    void updatePaddles(float dt);
    void updateParticles(float dt);
    void spawnParticles(const Vector2& pos, const Vector3& color, int count);
    void renderDigit(const Shader* shader, int digit, float centerX, float centerY, float scale) const;
    
    float screenWidth_;
    float screenHeight_;
    float worldWidth_ = 8.0f;
    float worldHeight_;
    
    // Paddles
    Vector2 playerPos_;
    Vector2 aiPos_;
    float paddleWidth_ = 0.15f;
    float paddleHeight_ = 1.2f;
    float paddleSpeed_ = 5.0f;
    
    // Ball
    Vector2 ballPos_;
    Vector2 ballVel_;
    float ballRadius_ = 0.1f;
    float ballSpeed_ = 3.5f;
    float maxBallSpeed_ = 8.0f;
    
    // Score
    int playerScore_ = 0;
    int aiScore_ = 0;
    float scoreDisplayTime_ = 0;
    
    // Local two-player controls. Each player drags in their own half of the screen.
    float playerTouchY_ = 0;
    float opponentTouchY_ = 0;
    
    // Visual effects
    struct Particle {
        Vector2 pos;
        Vector2 vel;
        float life;
        float maxLife;
        Vector3 color;
    };
    std::vector<Particle> particles_;
    float screenShake_ = 0;
    
    // Models for rendering
    struct RenderModel {
        std::vector<Vertex> vertices;
        std::vector<Index> indices;
        std::shared_ptr<TextureAsset> texture;
        Vector3 color;
    };
    
    std::vector<RenderModel> models_;
    PinballBackground pinballBg_;
    
    // Touch handling
    bool isTouching_ = false;
};

#endif // PONG_GAME_H
