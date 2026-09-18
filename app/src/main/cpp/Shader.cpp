#include "Shader.h"

#include "AndroidOut.h"
#include "Model.h"
#include "Utility.h"

Shader *Shader::loadShader(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        const std::string &positionAttributeName,
        const std::string &uvAttributeName,
        const std::string &projectionMatrixUniformName) {
    Shader *shader = nullptr;

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return nullptr;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return nullptr;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

            if (logLength) {
                GLchar *log = new GLchar[logLength];
                glGetProgramInfoLog(program, logLength, nullptr, log);
                aout << "Failed to link program with:\n" << log << std::endl;
                delete[] log;
            }

            glDeleteProgram(program);
        } else {
            GLint positionAttribute = glGetAttribLocation(program, positionAttributeName.c_str());
            GLint uvAttribute = glGetAttribLocation(program, uvAttributeName.c_str());
            GLint projectionMatrixUniform = glGetUniformLocation(
                    program,
                    projectionMatrixUniformName.c_str());
            GLint modelViewMatrixUniform = glGetUniformLocation(program, "uModelView");
            GLint colorUniform = glGetUniformLocation(program, "uColor");
            GLint useTextureUniform = glGetUniformLocation(program, "uUseTexture");
            GLint alphaUniform = glGetUniformLocation(program, "uAlpha");

            if (positionAttribute != -1
                && uvAttribute != -1
                && projectionMatrixUniform != -1
                && modelViewMatrixUniform != -1
                && colorUniform != -1
                && useTextureUniform != -1
                && alphaUniform != -1) {

                shader = new Shader(
                        program,
                        positionAttribute,
                        uvAttribute,
                        projectionMatrixUniform,
                        modelViewMatrixUniform,
                        colorUniform,
                        useTextureUniform,
                        alphaUniform);
            } else {
                glDeleteProgram(program);
            }
        }
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shader;
}

GLuint Shader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    Utility::assertGlError();
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        auto *shaderRawString = (GLchar *) shaderSource.c_str();
        GLint shaderLength = shaderSource.length();
        glShaderSource(shader, 1, &shaderRawString, &shaderLength);
        glCompileShader(shader);

        GLint shaderCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

        if (!shaderCompiled) {
            GLint infoLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

            if (infoLength) {
                auto *infoLog = new GLchar[infoLength];
                glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
                aout << "Failed to compile with:\n" << infoLog << std::endl;
                delete[] infoLog;
            }

            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

void Shader::activate() const {
    glUseProgram(program_);
}

void Shader::deactivate() const {
    glUseProgram(0);
}

void Shader::drawModel(const Model &model, const float* modelViewMatrix, const float* color, float useTexture, float alpha) const {
    glVertexAttribPointer(
            position_,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            model.getVertexData()
    );
    glEnableVertexAttribArray(position_);

    glVertexAttribPointer(
            uv_,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            ((uint8_t *) model.getVertexData()) + sizeof(Vector3)
    );
    glEnableVertexAttribArray(uv_);

    if (modelViewMatrix) {
        glUniformMatrix4fv(modelViewMatrix_, 1, false, modelViewMatrix);
    }
    
    if (color) {
        glUniform4fv(color_, 1, color);
    }
    
    glUniform1f(useTexture_, useTexture);
    glUniform1f(alpha_, alpha);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.getTextureID());

    glDrawElements(GL_TRIANGLES, model.getIndexCount(), GL_UNSIGNED_SHORT, model.getIndexData());

    glDisableVertexAttribArray(uv_);
    glDisableVertexAttribArray(position_);
}

void Shader::setProjectionMatrix(float *projectionMatrix) const {
    glUniformMatrix4fv(projectionMatrix_, 1, false, projectionMatrix);
}

void Shader::setModelViewMatrix(float *modelViewMatrix) const {
    glUniformMatrix4fv(modelViewMatrix_, 1, false, modelViewMatrix);
}

void Shader::setColor(const float* color) const {
    glUniform4fv(color_, 1, color);
}

void Shader::setUseTexture(float useTexture) const {
    glUniform1f(useTexture_, useTexture);
}

void Shader::setAlpha(float alpha) const {
    glUniform1f(alpha_, alpha);
}
