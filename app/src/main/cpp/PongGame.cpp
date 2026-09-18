#include "PongGame.h"
#include "Renderer.h"
#include "Shader.h"
#include "TextureAsset.h"
#include "Utility.h"
#include "AndroidOut.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <memory>

static std::mt19937 rng(std::random_device{}());
static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
static std::uniform_real_distribution<float> distNeg11(-1.0f, 1.0f);

float randFloat(float min, float max) {
    return min + dist01(rng) * (max - min);
}

Vector2 makeVector2(float x, float y) {
    Vector2 v;
    v.x = x;
    v.y = y;
    return v;
}

Vector3 makeVector3(float x, float y, float z) {
    Vector3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

Vector2 vec2Add(const Vector2& a, const Vector2& b) {
    return makeVector2(a.x + b.x, a.y + b.y);
}

Vector2 vec2Sub(const Vector2& a, const Vector2& b) {
    return makeVector2(a.x - b.x, a.y - b.y);
}

Vector2 vec2Mul(const Vector2& a, float s) {
    return makeVector2(a.x * s, a.y * s);
}

Vector2 vec2Div(const Vector2& a, float s) {
    return makeVector2(a.x / s, a.y / s);
}

float vec2Length(const Vector2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vector2 vec2Normalize(const Vector2& v) {
    float len = vec2Length(v);
    if (len == 0) return makeVector2(0, 0);
    return makeVector2(v.x / len, v.y / len);
}

void vec2AddAssign(Vector2& a, const Vector2& b) {
    a.x += b.x;
    a.y += b.y;
}

void vec2MulAssign(Vector2& a, float s) {
    a.x *= s;
    a.y *= s;
}

// Matrix helpers
void mat4Identity(float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1;
}

void mat4Translate(float* m, float x, float y, float z) {
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

void mat4Scale(float* m, float x, float y, float z) {
    m[0] *= x; m[1] *= x; m[2] *= x; m[3] *= x;
    m[4] *= y; m[5] *= y; m[6] *= y; m[7] *= y;
    m[8] *= z; m[9] *= z; m[10] *= z; m[11] *= z;
}

void mat4RotateZ(float* m, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5];
    m[0] = m0 * c - m4 * s;
    m[1] = m1 * c - m5 * s;
    m[4] = m0 * s + m4 * c;
    m[5] = m1 * s + m5 * c;
}

PinballBackground::PinballBackground() {
    bumpers_.push_back({makeVector2(-2.0f, 1.5f), 0.3f, 0, makeVector3(1, 0.3f, 0.1f)});
    bumpers_.push_back({makeVector2(2.0f, 1.5f), 0.3f, 1, makeVector3(0.1f, 0.8f, 1)});
    bumpers_.push_back({makeVector2(0.0f, 0.0f), 0.4f, 2, makeVector3(1, 0.9f, 0.1f)});
    bumpers_.push_back({makeVector2(-2.5f, -1.0f), 0.25f, 3, makeVector3(0.8f, 0.2f, 1)});
    bumpers_.push_back({makeVector2(2.5f, -1.0f), 0.25f, 4, makeVector3(0.2f, 1, 0.4f)});
    
    flippers_.push_back({makeVector2(-3.0f, -2.0f), -0.5f, -0.5f, true});
    flippers_.push_back({makeVector2(3.0f, -2.0f), 0.5f, 0.5f, false});
}

void PinballBackground::update(float dt) {
    time_ += dt;
    
    for (auto& bumper : bumpers_) {
        bumper.pulsePhase += dt * 3.0f;
    }
    
    for (auto& flipper : flippers_) {
        float target = flipper.isLeft ? -0.5f + std::sin(time_ * 0.7f) * 0.3f : 0.5f + std::sin(time_ * 0.7f + 3.14159f) * 0.3f;
        flipper.angle += (target - flipper.angle) * dt * 2.0f;
    }
    
    for (auto it = particles_.begin(); it != particles_.end();) {
        it->pos = vec2Add(it->pos, vec2Mul(it->vel, dt));
        it->vel.y -= 2.0f * dt;
        it->life -= dt;
        if (it->life <= 0) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
    
    if (dist01(rng) < 0.02f) {
        Vector2 pos = makeVector2(randFloat(-3.5f, 3.5f), randFloat(-2.5f, 2.5f));
        Vector3 colors[] = {
            makeVector3(1,0.3f,0.1f), 
            makeVector3(0.1f,0.8f,1), 
            makeVector3(1,0.9f,0.1f), 
            makeVector3(0.8f,0.2f,1), 
            makeVector3(0.2f,1,0.4f)
        };
        Vector3 color = colors[std::uniform_int_distribution<int>(0,4)(rng)];
        particles_.push_back({pos, 
                             makeVector2(randFloat(-0.5f,0.5f), randFloat(0.5f,1.5f)), 
                             randFloat(1.0f, 2.0f), randFloat(1.0f, 2.0f), color});
    }
}

void PinballBackground::createModels(AAssetManager* assetManager) {
    auto createCircle = [](float radius, int segments) {
        std::vector<Vertex> vertices;
        std::vector<Index> indices;
        
        vertices.push_back(Vertex(Vector3{0, 0, 0.05f}, Vector2{0.5f, 0.5f}));
        
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2 * 3.14159f;
            float x = std::cos(angle) * radius;
            float y = std::sin(angle) * radius;
            vertices.push_back(Vertex(Vector3{x, y, 0.05f}, Vector2{0.5f + std::cos(angle) * 0.5f, 0.5f + std::sin(angle) * 0.5f}));
        }
        
        for (int i = 1; i <= segments; i++) {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i % segments + 1);
        }
        
        return std::make_pair(vertices, indices);
    };
    
    auto [circleVerts, circleIndices] = createCircle(1.0f, 16);
    
    for (const auto& bumper : bumpers_) {
        std::vector<Vertex> verts = circleVerts;
        for (auto& v : verts) {
            v.position.x *= bumper.radius;
            v.position.y *= bumper.radius;
        }
        models_.push_back({verts, circleIndices, nullptr});
    }
    
    std::vector<Vertex> flipperVerts = {
        Vertex(Vector3{-0.05f, 0, 0.05f}, Vector2{0, 0}),
        Vertex(Vector3{0.05f, 0, 0.05f}, Vector2{1, 0}),
        Vertex(Vector3{0.05f, 1.5f, 0.05f}, Vector2{1, 1}),
        Vertex(Vector3{-0.05f, 1.5f, 0.05f}, Vector2{0, 1})
    };
    std::vector<Index> flipperIndices = {0, 1, 2, 0, 2, 3};
    
    for (int i = 0; i < 2; i++) {
        models_.push_back({flipperVerts, flipperIndices, nullptr});
    }
    
    std::vector<Vertex> particleVerts = {
        Vertex(Vector3{-0.05f, -0.05f, 0.2f}, Vector2{0, 0}),
        Vertex(Vector3{0.05f, -0.05f, 0.2f}, Vector2{1, 0}),
        Vertex(Vector3{0.05f, 0.05f, 0.2f}, Vector2{1, 1}),
        Vertex(Vector3{-0.05f, 0.05f, 0.2f}, Vector2{0, 1})
    };
    std::vector<Index> particleIndices = {0, 1, 2, 0, 2, 3};
    particleModel_ = {particleVerts, particleIndices, nullptr};
}

void PinballBackground::render(const Shader* shader) const {
    float modelView[16];
    float color[4];
    
    // Render bumpers
    for (size_t i = 0; i < bumpers_.size() && i < models_.size(); i++) {
        const auto& bumper = bumpers_[i];
        float pulse = 1.0f + std::sin(bumper.pulsePhase) * 0.15f;
        
        mat4Identity(modelView);
        mat4Translate(modelView, bumper.pos.x, bumper.pos.y, 0);
        mat4Scale(modelView, pulse, pulse, 1);
        
        color[0] = bumper.color.x;
        color[1] = bumper.color.y;
        color[2] = bumper.color.z;
        color[3] = 0.8f;
        
        shader->setModelViewMatrix(modelView);
        shader->setColor(color);
        shader->setUseTexture(0.0f);
        shader->setAlpha(0.8f);
        shader->drawModel(
            Model(models_[i].vertices, models_[i].indices, models_[i].texture),
            modelView, color, 0.0f, 0.8f
        );
    }
    
    // Render flippers
    for (size_t i = 0; i < flippers_.size(); i++) {
        const auto& flipper = flippers_[i];
        size_t modelIdx = bumpers_.size() + i;
        if (modelIdx >= models_.size()) continue;
        
        mat4Identity(modelView);
        mat4Translate(modelView, flipper.pos.x, flipper.pos.y, 0);
        mat4RotateZ(modelView, flipper.angle);
        
        color[0] = 0.6f; color[1] = 0.6f; color[2] = 0.7f; color[3] = 1.0f;
        
        shader->setModelViewMatrix(modelView);
        shader->setColor(color);
        shader->setUseTexture(0.0f);
        shader->setAlpha(1.0f);
        shader->drawModel(
            Model(models_[modelIdx].vertices, models_[modelIdx].indices, models_[modelIdx].texture),
            modelView, color, 0.0f, 1.0f
        );
    }
}

PongGame::PongGame(float screenWidth, float screenHeight) 
    : screenWidth_(screenWidth), screenHeight_(screenHeight) {
    worldHeight_ = worldWidth_ * (screenHeight_ / screenWidth_);
    
    playerPos_ = makeVector2(-worldWidth_ * 0.5f + 0.5f, 0);
    aiPos_ = makeVector2(worldWidth_ * 0.5f - 0.5f, 0);
    resetBall();
}

PongGame::~PongGame() {}

void PongGame::resetBall() {
    ballPos_ = makeVector2(0, 0);
    float angle = randFloat(-0.7f, 0.7f);
    if (dist01(rng) < 0.5f) angle += 3.14159f;
    ballVel_ = vec2Mul(makeVector2(std::cos(angle), std::sin(angle)), ballSpeed_);
    ballSpeed_ = 3.5f;
}

void PongGame::update(float dt) {
    pinballBg_.update(dt);
    updatePaddles(dt);
    
    ballPos_ = vec2Add(ballPos_, vec2Mul(ballVel_, dt));
    
    checkCollisions();
    updateParticles(dt);
    
    if (screenShake_ > 0) {
        screenShake_ -= dt * 10.0f;
        if (screenShake_ < 0) screenShake_ = 0;
    }
    
    if (scoreDisplayTime_ > 0) {
        scoreDisplayTime_ -= dt;
    }
}

void PongGame::updatePaddles(float dt) {
    const float minY = -worldHeight_ * 0.5f + paddleHeight_ * 0.5f;
    const float maxY = worldHeight_ * 0.5f - paddleHeight_ * 0.5f;
    const float follow = std::min(1.0f, dt * paddleSpeed_ * 4.0f);

    playerPos_.y += (std::clamp(playerTouchY_, minY, maxY) - playerPos_.y) * follow;
    aiPos_.y += (std::clamp(opponentTouchY_, minY, maxY) - aiPos_.y) * follow;
}

void PongGame::checkCollisions() {
    float topWall = worldHeight_ * 0.5f - ballRadius_;
    float bottomWall = -worldHeight_ * 0.5f + ballRadius_;
    
    if (ballPos_.y > topWall) {
        ballPos_.y = topWall;
        ballVel_.y = -std::abs(ballVel_.y);
        spawnParticles(makeVector2(ballPos_.x, topWall), makeVector3(0.5f, 0.8f, 1), 8);
    } else if (ballPos_.y < bottomWall) {
        ballPos_.y = bottomWall;
        ballVel_.y = std::abs(ballVel_.y);
        spawnParticles(makeVector2(ballPos_.x, bottomWall), makeVector3(0.5f, 0.8f, 1), 8);
    }
    
    // Player paddle collision
    float playerLeft = playerPos_.x - paddleWidth_ * 0.5f;
    float playerRight = playerPos_.x + paddleWidth_ * 0.5f;
    float playerTop = playerPos_.y + paddleHeight_ * 0.5f;
    float playerBottom = playerPos_.y - paddleHeight_ * 0.5f;
    
    float ballLeft = ballPos_.x - ballRadius_;
    float ballRight = ballPos_.x + ballRadius_;
    float ballTop = ballPos_.y + ballRadius_;
    float ballBottom = ballPos_.y - ballRadius_;
    
    if (ballVel_.x < 0 && ballRight > playerLeft && ballLeft < playerRight && 
        ballTop > playerBottom && ballBottom < playerTop) {
        float hitPos = (ballPos_.y - playerPos_.y) / (paddleHeight_ * 0.5f);
        hitPos = std::max(-1.0f, std::min(1.0f, hitPos));
        
        float angle = hitPos * 1.2f;
        float speed = vec2Length(ballVel_);
        ballVel_ = vec2Mul(makeVector2(std::cos(angle), std::sin(angle)), speed);
        ballVel_.x = std::abs(ballVel_.x);
        
        ballPos_.x = playerPos_.x + paddleWidth_ * 0.5f + ballRadius_;
        
        ballSpeed_ = std::min(maxBallSpeed_, ballSpeed_ * 1.05f);
        ballVel_ = vec2Mul(vec2Normalize(ballVel_), ballSpeed_);
        
        spawnParticles(makeVector2(playerPos_.x + paddleWidth_ * 0.5f, ballPos_.y), makeVector3(0.2f, 1, 0.4f), 12);
        screenShake_ = 0.15f;
    }
    
    // AI paddle collision
    float aiLeft = aiPos_.x - paddleWidth_ * 0.5f;
    float aiRight = aiPos_.x + paddleWidth_ * 0.5f;
    float aiTop = aiPos_.y + paddleHeight_ * 0.5f;
    float aiBottom = aiPos_.y - paddleHeight_ * 0.5f;
    
    if (ballVel_.x > 0 && ballRight > aiLeft && ballLeft < aiRight && 
        ballTop > aiBottom && ballBottom < aiTop) {
        float hitPos = (ballPos_.y - aiPos_.y) / (paddleHeight_ * 0.5f);
        hitPos = std::max(-1.0f, std::min(1.0f, hitPos));
        
        float angle = 3.14159f - hitPos * 1.2f;
        float speed = vec2Length(ballVel_);
        ballVel_ = vec2Mul(makeVector2(std::cos(angle), std::sin(angle)), speed);
        ballVel_.x = -std::abs(ballVel_.x);
        
        ballPos_.x = aiPos_.x - paddleWidth_ * 0.5f - ballRadius_;
        
        ballSpeed_ = std::min(maxBallSpeed_, ballSpeed_ * 1.05f);
        ballVel_ = vec2Mul(vec2Normalize(ballVel_), ballSpeed_);
        
        spawnParticles(makeVector2(aiPos_.x - paddleWidth_ * 0.5f, ballPos_.y), makeVector3(1, 0.3f, 0.2f), 12);
        screenShake_ = 0.15f;
    }
    
    // Scoring
    if (ballPos_.x < -worldWidth_ * 0.5f - ballRadius_) {
        aiScore_++;
        scoreDisplayTime_ = 2.0f;
        spawnParticles(makeVector2(-worldWidth_ * 0.5f, ballPos_.y), makeVector3(1, 0.2f, 0.2f), 20);
        screenShake_ = 0.3f;
        resetBall();
    } else if (ballPos_.x > worldWidth_ * 0.5f + ballRadius_) {
        playerScore_++;
        scoreDisplayTime_ = 2.0f;
        spawnParticles(makeVector2(worldWidth_ * 0.5f, ballPos_.y), makeVector3(0.2f, 1, 0.4f), 20);
        screenShake_ = 0.3f;
        resetBall();
    }
}

void PongGame::updateParticles(float dt) {
    for (auto it = particles_.begin(); it != particles_.end();) {
        it->pos = vec2Add(it->pos, vec2Mul(it->vel, dt));
        it->vel.y -= 3.0f * dt;
        vec2MulAssign(it->vel, 0.98f);
        it->life -= dt;
        if (it->life <= 0) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
}

void PongGame::spawnParticles(const Vector2& pos, const Vector3& color, int count) {
    for (int i = 0; i < count; i++) {
        float angle = dist01(rng) * 2 * 3.14159f;
        float speed = randFloat(2.0f, 5.0f);
        Vector2 vel = makeVector2(std::cos(angle) * speed, std::sin(angle) * speed);
        particles_.push_back({pos, vel, randFloat(0.5f, 1.2f), randFloat(0.5f, 1.2f), color});
    }
}

void PongGame::render(const Shader* shader) const {
    float modelView[16];
    float color[4];

    // Classic Pong: a clean field, paddles, ball, center line and score only.
    const float scoreY = worldHeight_ * 0.5f - 0.85f;
    renderDigit(shader, playerScore_ % 10, -1.0f, scoreY, 1.0f);
    renderDigit(shader, aiScore_ % 10, 1.0f, scoreY, 1.0f);
    
    // Render player paddle (left)
    mat4Identity(modelView);
    mat4Translate(modelView, playerPos_.x, playerPos_.y, 0);
    
    color[0] = 0.2f; color[1] = 0.8f; color[2] = 1.0f; color[3] = 1.0f;
    shader->setModelViewMatrix(modelView);
    shader->setColor(color);
    shader->setUseTexture(0.0f);
    shader->setAlpha(1.0f);
    if (models_.size() > 0) {
        shader->drawModel(
            Model(models_[0].vertices, models_[0].indices, models_[0].texture),
            modelView, color, 0.0f, 1.0f
        );
    }
    
    // Render AI paddle (right)
    mat4Identity(modelView);
    mat4Translate(modelView, aiPos_.x, aiPos_.y, 0);
    
    color[0] = 1.0f; color[1] = 0.3f; color[2] = 0.2f; color[3] = 1.0f;
    shader->setModelViewMatrix(modelView);
    shader->setColor(color);
    shader->setUseTexture(0.0f);
    shader->setAlpha(1.0f);
    if (models_.size() > 1) {
        shader->drawModel(
            Model(models_[1].vertices, models_[1].indices, models_[1].texture),
            modelView, color, 0.0f, 1.0f
        );
    }
    
    // Render ball with screen shake
    mat4Identity(modelView);
    float shakeX = (randFloat(-1, 1) * screenShake_);
    float shakeY = (randFloat(-1, 1) * screenShake_);
    mat4Translate(modelView, ballPos_.x + shakeX, ballPos_.y + shakeY, 0.1f);
    
    color[0] = 1.0f; color[1] = 1.0f; color[2] = 0.2f; color[3] = 1.0f;
    shader->setModelViewMatrix(modelView);
    shader->setColor(color);
    shader->setUseTexture(0.0f);
    shader->setAlpha(1.0f);
    if (models_.size() > 2) {
        shader->drawModel(
            Model(models_[2].vertices, models_[2].indices, models_[2].texture),
            modelView, color, 0.0f, 1.0f
        );
    }
    
    // Render particles
    if (models_.size() > 3) {
        for (const auto& particle : particles_) {
            mat4Identity(modelView);
            mat4Translate(modelView, particle.pos.x, particle.pos.y, 0.2f);
            float alpha = particle.life / particle.maxLife;
            float size = 0.08f * alpha;
            mat4Scale(modelView, size, size, 1);
            
            color[0] = particle.color.x;
            color[1] = particle.color.y;
            color[2] = particle.color.z;
            color[3] = alpha * 0.8f;
            
            shader->setModelViewMatrix(modelView);
            shader->setColor(color);
            shader->setUseTexture(0.0f);
            shader->setAlpha(alpha * 0.8f);
            shader->drawModel(
                Model(models_[3].vertices, models_[3].indices, models_[3].texture),
                modelView, color, 0.0f, alpha * 0.8f
            );
        }
    }
    
    // Render center line
    mat4Identity(modelView);
    mat4Scale(modelView, 0.02f, worldHeight_, 1);
    color[0] = 1; color[1] = 1; color[2] = 1; color[3] = 0.3f;
    shader->setModelViewMatrix(modelView);
    shader->setColor(color);
    shader->setUseTexture(0.0f);
    shader->setAlpha(0.3f);
    if (models_.size() > 0) {
        shader->drawModel(
            Model(models_[0].vertices, models_[0].indices, models_[0].texture),
            modelView, color, 0.0f, 0.3f
        );
    }
}

void PongGame::renderDigit(
        const Shader* shader, int digit, float centerX, float centerY, float scale) const {
    if (models_.empty() || digit < 0 || digit > 9) return;

    float modelView[16];

    // Seven-segment digits: top, upper-left, upper-right, middle, lower-left, lower-right, bottom.
    static constexpr bool segments[10][7] = {
        {true,  true,  true,  false, true,  true,  true }, // 0
        {false, false, true,  false, false, true,  false}, // 1
        {true,  false, true,  true,  true,  false, true }, // 2
        {true,  false, true,  true,  false, true,  true }, // 3
        {false, true,  true,  true,  false, true,  false}, // 4
        {true,  true,  false, true,  false, true,  true }, // 5
        {true,  true,  false, true,  true,  true,  true }, // 6
        {true,  false, true,  false, false, true,  false}, // 7
        {true,  true,  true,  true,  true,  true,  true }, // 8
        {true,  true,  true,  true,  false, true,  true }  // 9
    };
    static constexpr float positions[7][2] = {
        { 0.0f,  0.42f}, {-0.28f,  0.21f}, { 0.28f,  0.21f},
        { 0.0f,  0.00f}, {-0.28f, -0.21f}, { 0.28f, -0.21f},
        { 0.0f, -0.42f}
    };

    const float color[] = {0.95f, 0.98f, 1.0f, 1.0f};
    for (int segment = 0; segment < 7; ++segment) {
        if (!segments[digit][segment]) continue;

        mat4Identity(modelView);
        mat4Translate(modelView,
                      centerX + positions[segment][0] * scale,
                      centerY + positions[segment][1] * scale,
                      0.3f);
        const bool horizontal = segment == 0 || segment == 3 || segment == 6;
        mat4Scale(modelView,
                  horizontal ? 3.6f * scale : 0.48f * scale,
                  horizontal ? 0.06f * scale : 0.38f * scale,
                  1.0f);
        shader->drawModel(
                Model(models_[0].vertices, models_[0].indices, models_[0].texture),
                modelView, color, 0.0f, 1.0f);
    }
}

void PongGame::handleTouch(float x, float y, bool isDown, bool isMove) {
    float worldX = (x / screenWidth_) * worldWidth_ - worldWidth_ * 0.5f;
    float worldY = -(y / screenHeight_) * worldHeight_ + worldHeight_ * 0.5f;
    
    isTouching_ = isDown || isMove;
    const float minY = -worldHeight_ * 0.5f + paddleHeight_ * 0.5f;
    const float maxY = worldHeight_ * 0.5f - paddleHeight_ * 0.5f;
    if (worldX < 0) {
        playerTouchY_ = std::clamp(worldY, minY, maxY);
    } else {
        opponentTouchY_ = std::clamp(worldY, minY, maxY);
    }
}

void PongGame::resize(float width, float height) {
    screenWidth_ = width;
    screenHeight_ = height;
    worldHeight_ = worldWidth_ * (screenHeight_ / screenWidth_);
}

void PongGame::createModels(AAssetManager* assetManager) {
    pinballBg_.createModels(assetManager);
    
    std::vector<Vertex> paddleVerts = {
        Vertex(Vector3{-paddleWidth_ * 0.5f, -paddleHeight_ * 0.5f, 0}, Vector2{0, 0}),
        Vertex(Vector3{paddleWidth_ * 0.5f, -paddleHeight_ * 0.5f, 0}, Vector2{1, 0}),
        Vertex(Vector3{paddleWidth_ * 0.5f, paddleHeight_ * 0.5f, 0}, Vector2{1, 1}),
        Vertex(Vector3{-paddleWidth_ * 0.5f, paddleHeight_ * 0.5f, 0}, Vector2{0, 1})
    };
    std::vector<Index> paddleIndices = {0, 1, 2, 0, 2, 3};
    
    std::vector<Vertex> ballVerts;
    std::vector<Index> ballIndices;
    ballVerts.push_back(Vertex(Vector3{0, 0, 0.1f}, Vector2{0.5f, 0.5f}));
    int segments = 16;
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / segments * 2 * 3.14159f;
        float x = std::cos(angle) * ballRadius_;
        float y = std::sin(angle) * ballRadius_;
        ballVerts.push_back(Vertex(Vector3{x, y, 0.1f}, Vector2{0.5f + std::cos(angle) * 0.5f, 0.5f + std::sin(angle) * 0.5f}));
    }
    for (int i = 1; i <= segments; i++) {
        ballIndices.push_back(0);
        ballIndices.push_back(i);
        ballIndices.push_back(i % segments + 1);
    }
    
    std::shared_ptr<TextureAsset> paddleTexture = TextureAsset::loadAsset(assetManager, "android_robot.png");
    
    models_.push_back({paddleVerts, paddleIndices, paddleTexture, makeVector3(0.2f, 0.8f, 1)});
    models_.push_back({paddleVerts, paddleIndices, paddleTexture, makeVector3(1, 0.3f, 0.2f)});
    models_.push_back({ballVerts, ballIndices, paddleTexture, makeVector3(1, 1, 0.2f)});
    
    std::vector<Vertex> particleVerts = {
        Vertex(Vector3{-0.08f, -0.08f, 0.2f}, Vector2{0, 0}),
        Vertex(Vector3{0.08f, -0.08f, 0.2f}, Vector2{1, 0}),
        Vertex(Vector3{0.08f, 0.08f, 0.2f}, Vector2{1, 1}),
        Vertex(Vector3{-0.08f, 0.08f, 0.2f}, Vector2{0, 1})
    };
    std::vector<Index> particleIndices = {0, 1, 2, 0, 2, 3};
    models_.push_back({particleVerts, particleIndices, paddleTexture, makeVector3(1, 1, 1)});
}
