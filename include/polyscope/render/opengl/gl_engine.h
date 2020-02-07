// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/options.h"
#include "polyscope/render/engine.h"
#include "polyscope/utilities.h"

#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#include "GLFW/glfw3.h"
#else
#include "glad/glad.h"
// glad must come first
#include "GLFW/glfw3.h"
#endif


// Note: DO NOT include this header throughout polyscope, and do not directly make openGL calls. This header should only
// be used to construct an instance of Engine. engine.h gives the render API, all render calls should pass through that.

// Macro to construct shader strings. Unforunately, it eats line numbers.
#define POLYSCOPE_GLSL(version, shader) "#version " #version "\n" #shader

namespace polyscope {
namespace render {

// Some very nice typdefs
typedef GLuint TextureBufferHandle;
typedef GLuint RenderBufferHandle;
typedef GLuint FrameBufferHandle;
typedef GLuint ShaderHandle;
typedef GLuint ProgramHandle;
typedef GLuint AttributeHandle;
typedef GLuint VertexBufferHandle;

typedef GLint UniformLocation;
typedef GLint AttributeLocation;
typedef GLint TextureLocation;

class GLTextureBuffer : public TextureBuffer {
public:
  // create a 1D texture from data
  GLTextureBuffer(TextureFormat format, unsigned int size1D, unsigned char* data);
  GLTextureBuffer(TextureFormat format, unsigned int size1D, float* data);

  // create a 2D texture from data
  GLTextureBuffer(TextureFormat format, unsigned int sizeX_, unsigned int sizeY_, unsigned char* data = nullptr);

  ~GLTextureBuffer() override;


  // Resize the underlying buffer (contents are lost)
  void resize(unsigned int newLen) override;
  void resize(unsigned int newX, unsigned int newY) override;

  void setFilterMode(FilterMode newMode) override;
  void* getNativeHandle() override;


  void bind();
  TextureBufferHandle getHandle() const { return handle; }

protected:
  TextureBufferHandle handle;
};

class GLRenderBuffer : public RenderBuffer {
public:
  GLRenderBuffer(RenderBufferType type, unsigned int sizeX_, unsigned int sizeY_);
  ~GLRenderBuffer() override;

  void resize(unsigned int newX, unsigned int newY) override;

  void bind();
  RenderBufferHandle getHandle() const { return handle; }

protected:
  RenderBufferHandle handle;
};


class GLFrameBuffer : public FrameBuffer {

public:
  GLFrameBuffer();
  ~GLFrameBuffer() override;

  // Bind to this framebuffer so subsequent draw calls will go to it
  // If return is false, binding failed and the framebuffer should not be used.
  bool bindForRendering() override;

  // Clear to redraw
  void clear() override;

  // Bind to textures/renderbuffers for output
  void addColorBuffer(std::shared_ptr<RenderBuffer> renderBuffer) override;
  void addColorBuffer(std::shared_ptr<TextureBuffer> textureBuffer) override;
  void addDepthBuffer(std::shared_ptr<RenderBuffer> renderBuffer) override;
  void addDepthBuffer(std::shared_ptr<TextureBuffer> textureBuffer) override;

  void setDrawBuffers() override;

  // Query pixel
  std::array<float, 4> readFloat4(int xPos, int yPos) override;

  // Getters
  FrameBufferHandle getHandle() const { return handle; }

protected:
  FrameBufferHandle handle;

  void bind();
};


class GLShaderProgram : public ShaderProgram {

public:
  GLShaderProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm, unsigned int nPatchVertices = 0);
  ~GLShaderProgram() override;

  // === Store data
  // If update is set to "true", data is updated rather than allocated (must be allocated first)

  // Uniforms
  bool hasUniform(std::string name) override;
  void setUniform(std::string name, int val) override;
  void setUniform(std::string name, unsigned int val) override;
  void setUniform(std::string name, float val) override;
  void setUniform(std::string name, double val) override; // WARNING casts down to float
  void setUniform(std::string name, float* val) override;
  void setUniform(std::string name, glm::vec2 val) override;
  void setUniform(std::string name, glm::vec3 val) override;
  void setUniform(std::string name, glm::vec4 val) override;
  void setUniform(std::string name, std::array<float, 3> val) override;
  void setUniform(std::string name, float x, float y, float z, float w) override;

  // = Attributes
  // clang-format off
  bool hasAttribute(std::string name) override;
  void setAttribute(std::string name, const std::vector<glm::vec2>& data, bool update = false, int offset = 0, int size = -1) override;
  void setAttribute(std::string name, const std::vector<glm::vec3>& data, bool update = false, int offset = 0, int size = -1) override;
  void setAttribute(std::string name, const std::vector<glm::vec4>& data, bool update = false, int offset = 0, int size = -1) override;
  void setAttribute(std::string name, const std::vector<double>& data, bool update = false, int offset = 0, int size = -1) override;
  void setAttribute(std::string name, const std::vector<int>& data, bool update = false, int offset = 0, int size = -1) override; 
  void setAttribute(std::string name, const std::vector<uint32_t>& data, bool update = false, int offset = 0, int size = -1) override;
  // clang-format on

  // Convenience method to set an array-valued attrbute, such as 'in vec3 vertexVal[3]'. Applies interleaving then
  // forwards to the usual setAttribute
  template <typename T, unsigned int C>
  void setAttribute(std::string name, const std::vector<std::array<T, C>>& data, bool update = false, int offset = 0,
                    int size = -1);


  // Indices
  void setIndex(std::vector<std::array<unsigned int, 3>>& indices) override;
  void setIndex(std::vector<unsigned int>& indices) override;
  void setPrimitiveRestartIndex(unsigned int restartIndex) override;

  // Textures
  void setTexture1D(std::string name, unsigned char* texData, unsigned int length) override;
  void setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height,
                    bool withAlpha = true, bool useMipMap = false, bool repeat = false) override;
  void setTextureFromColormap(std::string name, const ValueColorMap& colormap, bool allowUpdate = false) override;
  void setTextureFromBuffer(std::string name, TextureBuffer* textureBuffer) override;

  // Draw!
  void draw() override;
  void validateData() override;

protected:
  // Classes to keep track of attributes and uniforms
  struct GLShaderUniform {
    std::string name;
    DataType type;
    bool isSet; // has a value been assigned to this uniform?
    UniformLocation location;
  };

  struct GLShaderAttribute {
    std::string name;
    DataType type;
    int arrayCount;
    long int dataSize; // the size of the data currently stored in this attribute (-1 if nothing)
    AttributeLocation location;
    VertexBufferHandle VBOLoc;
  };

  struct GLShaderTexture {
    std::string name;
    int dim;
    unsigned int index;
    bool isSet;
    GLTextureBuffer* textureBuffer;
    std::shared_ptr<GLTextureBuffer> textureBufferOwned; // might be empty, if texture isn't owned
    TextureLocation location;
  };

  // Lists of attributes and uniforms that need to be set
  std::vector<GLShaderUniform> uniforms;
  std::vector<GLShaderAttribute> attributes;
  std::vector<GLShaderTexture> textures;

  void addUniqueAttribute(ShaderSpecAttribute attribute);
  void addUniqueUniform(ShaderSpecUniform uniform);
  void addUniqueTexture(ShaderSpecTexture texture);

private:
  // Setup routines
  void compileGLProgram(const std::vector<ShaderStageSpecification>& stages);
  void setDataLocations();
  void createBuffers();

  void deleteAttributeBuffer(GLShaderAttribute& attribute);

  // Drawing related
  void activateTextures();

  // GL pointers for various useful things
  ProgramHandle programHandle = 0;
  AttributeHandle vaoHandle;
  AttributeHandle indexVBO;

  int nPatchVertices;
};


class GLEngine : public Engine {
public:
  GLEngine();

  // High-level control
  void checkError(bool fatal = false) override;

  void clearDisplay() override;
  void bindDisplay() override;

  // Manage render state
  void pushActiveRenderBuffer() override;
  void popActiveRenderBuffer() override;

  // === Factory methods

  // create textures
  std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                       unsigned char* data) override; // 1d
  std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int size1D,
                                                       float* data) override; // 1d
  std::shared_ptr<TextureBuffer> generateTextureBuffer(TextureFormat format, unsigned int sizeX_, unsigned int sizeY_,
                                                       unsigned char* data = nullptr) override; // 2d

  // create render buffers
  std::shared_ptr<RenderBuffer> generateRenderBuffer(RenderBufferType type, unsigned int sizeX_,
                                                     unsigned int sizeY_) override;

  // create frame buffers
  std::shared_ptr<FrameBuffer> generateFrameBuffer() override;

  // create shader programs
  std::shared_ptr<ShaderProgram> generateShaderProgram(const std::vector<ShaderStageSpecification>& stages, DrawMode dm,
                                                       unsigned int nPatchVertices = 0) override;

protected:
  // Helpers
  void allocateGlobalBuffersAndPrograms();

  std::vector<FrameBufferHandle> activeRenderBufferStack;
};

} // namespace render
} // namespace polyscope
