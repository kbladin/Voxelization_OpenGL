#include "../include/SimpleGraphicsEngine.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gl/glew.h>
#include <gl/glfw3.h>

#include "../include/ShaderLoader.h"

using namespace SGE;

ShaderManager* ShaderManager::instance_;

ShaderManager::ShaderManager()
{
}

void ShaderManager::loadShader(
  std::string name,
  const char* vs_src,
  const char* tcs_src,
  const char* tes_src,
  const char* gs_src,
  const char* fs_src)
{
  shader_program_IDs.insert(
    std::pair<std::string,GLuint>(name,
    ShaderLoader::loadShaders(
      vs_src, // Vertex shader file path
      tcs_src, // Tesselation control shader file path
      tes_src, // Tesselation evaluation shader file path
      gs_src, // Geometry shader file path
      fs_src))); // Fragment shader file path
}

ShaderManager::~ShaderManager()
{
  for (std::map<std::string, GLuint>::const_iterator it = shader_program_IDs.begin(); it != shader_program_IDs.end(); it++) {
    glDeleteProgram(it->second);
  }
}

ShaderManager* ShaderManager::instance()
{
  if (!instance_) {
    instance_ = new ShaderManager();
  }
  return instance_;
}

GLuint ShaderManager::getShader(std::string name)
{
  GLuint program_ID = shader_program_IDs[name];
  if (!program_ID)
  {
    std::cout << "ERROR : This name, " << name << " is not a valid shader program name!" << std::endl;
  }
  return program_ID;
}

void Object3D::addChild(Object3D *child)
{
  children.push_back(child);
}

void Object3D::removeChild(Object3D *child)
{
  children.erase(std::remove(children.begin(), children.end(), child), children.end());
  for (int i=0; i<children.size(); i++) {
    children[i]->removeChild(child);
  }
}

void Object3D::render(glm::mat4 M, GLuint program_ID)
{
  for (std::vector<Object3D*>::const_iterator iter = children.begin(); iter != children.end(); iter++) {
    (*iter)->render(M * transform_matrix_, program_ID);
  }
}

AbstractMesh::AbstractMesh()
{
}

AbstractMesh::~AbstractMesh()
{
  // Cleanup VBO
  glDeleteBuffers(1, &vertex_buffer_);
  glDeleteVertexArrays(1, &vertex_array_ID_);
}

void AbstractMesh::initialize()
{  
  glGenVertexArrays(1, &vertex_array_ID_);
  glBindVertexArray(vertex_array_ID_);
  
  glGenBuffers(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(glm::vec3), &vertices_[0], GL_STATIC_DRAW);
}

TriangleMesh::TriangleMesh() : AbstractMesh()
{
  initialize();
}

TriangleMesh::TriangleMesh(
           std::vector<glm::vec3> vertices,
           std::vector<glm::vec3> normals,
           std::vector<unsigned short> elements) : AbstractMesh()
{
  vertices_ = vertices;
  normals_ = normals;
  elements_ = elements;
  
  initialize();
}

TriangleMesh::~TriangleMesh()
{
  glDeleteBuffers(1, &element_buffer_);
  glDeleteBuffers(1, &normal_buffer_);
}

void TriangleMesh::initialize()
{
  AbstractMesh::initialize();
  
  glGenBuffers(1, &normal_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_);
  glBufferData(GL_ARRAY_BUFFER, normals_.size() * sizeof(glm::vec3), &normals_[0], GL_STATIC_DRAW);
  
  glGenBuffers(1, &element_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements_.size() * sizeof(unsigned short), &elements_[0] , GL_STATIC_DRAW);
}

void TriangleMesh::render(glm::mat4 M, GLuint program_ID)
{
  Object3D::render(M, program_ID);
  //material_->render();
  
  glm::mat4 total_transform = M * transform_matrix_;

  // Use our shader
  glUseProgram(program_ID);
  
  glUniformMatrix4fv(glGetUniformLocation(program_ID, "M"), 1, GL_FALSE, &total_transform[0][0]);
  
  glBindVertexArray(vertex_array_ID_);
  
  // 1rst attribute buffer : vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glVertexAttribPointer(
                        0,                  // attribute
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        (void*)0            // array buffer offset
                        );
  
  // 2nd attribute buffer : normals
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_);
  glVertexAttribPointer(
                        1,                  // attribute
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        (void*)0            // array buffer offset
                        );
  
  // Index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
  
  // Draw the triangles !
  glDrawElements(
                 GL_TRIANGLES,      // mode
                 elements_.size(),    // count
                 GL_UNSIGNED_SHORT,   // type
                 (void*)0           // element array buffer offset
                 );
  
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

AbstractCamera::AbstractCamera(GLFWwindow* window)
{
  window_ = window;  
  transform_matrix_ = glm::lookAt(
                                glm::vec3(0.0f,0.0f,3.0f),
                                glm::vec3(0.0f,0.0f,-1.0f),
                                glm::vec3(0.0f,1.0f,0.0f));
}

void AbstractCamera::render(glm::mat4 M, GLuint program_ID)
{
  Object3D::render(M * transform_matrix_, program_ID);

  glm::mat4 V = M * transform_matrix_;
  
  glUseProgram(program_ID);

  glUniformMatrix4fv(glGetUniformLocation(program_ID, "V"), 1, GL_FALSE, &V[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(program_ID, "P"), 1, GL_FALSE, &projection_transform_matrix_[0][0]);
}

PerspectiveCamera::PerspectiveCamera(GLFWwindow* window) :
AbstractCamera(window)
{
  int width;
  int height;
  glfwGetWindowSize(window_, &width, &height);
  float aspect = float(width)/height;
  projection_transform_matrix_ = glm::perspective(45.0f, aspect, 0.1f, 100.0f);
}

void PerspectiveCamera::render(glm::mat4 M, GLuint program_ID)
{
  int width;
  int height;
  glfwGetWindowSize(window_, &width, &height);
  float aspect = float(width)/height;
  projection_transform_matrix_ = glm::perspective(45.0f, aspect, 0.1f, 100.0f);
  
  AbstractCamera::render(M, program_ID);
}

OrthoCamera::OrthoCamera(GLFWwindow* window) :
  AbstractCamera(window)
{
  int width;
  int height;
  glfwGetWindowSize(window_, &width, &height);
  float aspect = float(width)/height;
  projection_transform_matrix_ = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -100.0f, 100.0f);
}

void OrthoCamera::render(glm::mat4 M, GLuint program_ID)
{
  int width;
  int height;
  glfwGetWindowSize(window_, &width, &height);
  float aspect = float(width)/height;
  projection_transform_matrix_ = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -100.0f, 100.0f);
  
  AbstractCamera::render(M, program_ID);
}

LightSource::LightSource()
{
  intensity_ = 5.0f;
  color_ = glm::vec3(1.0, 1.0, 1.0);
}

void LightSource::render(glm::mat4 M, GLuint program_ID)
{
  //Object3D::render(M * transform_.getMatrix());
  
  //glm::vec4 position = M * transform_.getMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  
  glUseProgram(program_ID);
  //glUniform3f(glGetUniformLocation(program_ID_, "lightPosition"),position.x,position.y, position.z);
  glUniform1f(glGetUniformLocation(program_ID, "lightIntensity"), intensity_);
  glUniform3f(glGetUniformLocation(program_ID, "lightColor"), color_.r, color_.g, color_.b);
}

Object3D* SimpleGraphicsEngine::camera_;
Object3D* SimpleGraphicsEngine::viewspace_ortho_camera_;

SimpleGraphicsEngine::SimpleGraphicsEngine()
{
  initialize();
}

SimpleGraphicsEngine::~SimpleGraphicsEngine()
{
  glfwTerminate();
  delete scene_;
  delete view_space_;
  delete background_space_;
    
  delete camera_;
  delete basic_cam_;
  delete viewspace_ortho_camera_;
  //delete background_ortho_cam_;
}

bool SimpleGraphicsEngine::initialize()
{
  time_ = glfwGetTime();
  // Initialize the library
  if (!glfwInit())
    return -1;
  // Modern OpenGL
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // Create a windowed mode window and its OpenGL context
  window_ = glfwCreateWindow(720, 480, "Model Viewer", NULL, NULL);
  if (!window_)
  {
    glfwTerminate();
    return false;
  }
  // Make the window's context current
  glfwMakeContextCurrent(window_);
  printf("%s\n", glGetString(GL_VERSION));
  
  glewExperimental=true; // Needed in core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return false;
  }
  
  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);
  // Cull triangles which normal is not towards the camera
  glEnable(GL_CULL_FACE);
  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  // Instantiate (needs to be done after creating OpenGL context)
  ShaderManager::instance();

  scene_ = new Object3D();
  view_space_ = new Object3D();
  background_space_ = new Object3D();
  
  camera_ = new Object3D();
  viewspace_ortho_camera_ = new Object3D();
  basic_cam_ = new PerspectiveCamera(window_);
  //background_ortho_cam_ = new OrthoCamera(ShaderManager::instance()->getShader("SHADER_BACKGROUND"), window_);
  
  camera_->addChild(basic_cam_);
  //viewspace_ortho_camera_->addChild(background_ortho_cam_);
  scene_->addChild(camera_);

  view_space_->addChild(viewspace_ortho_camera_);
  return true;
}

void SimpleGraphicsEngine::run()
{
  while (!glfwWindowShouldClose(window_))
  {
    
    update();

    render();
    
    glfwSwapBuffers(window_);
    glfwPollEvents();
  }
}

void SimpleGraphicsEngine::update()
{
  dt_ = glfwGetTime() - time_;
  time_ = glfwGetTime();
  
  int width;
  int height;
  glfwGetWindowSize(window_, &width, &height);
  glViewport(0,0,width * 2,height * 2);
}


void SimpleGraphicsEngine::render()
{

}

void FBO::CHECK_FRAMEBUFFER_STATUS()
{
  GLenum status;
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    fprintf(stderr, "Framebuffer not complete\n");
}

FBO::FBO(int width, int height, int int_method)
{
  width_ = width;
  height_ = height;

  // create objects
  glGenFramebuffers(1, &fb_); // frame buffer id
  glBindFramebuffer(GL_FRAMEBUFFER, fb_);
  glGenTextures(1, &texid_);
  fprintf(stderr, "%i \n", texid_);
  glBindTexture(GL_TEXTURE_2D, texid_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if (int_method == 0)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid_, 0);

  // Renderbuffer
  // initialize depth renderbuffer
    glGenRenderbuffers(1, &rb_);
    glBindRenderbuffer(GL_RENDERBUFFER, rb_);
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_ );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_ );
    CHECK_FRAMEBUFFER_STATUS();

  fprintf(stderr, "Framebuffer object %d\n", fb_);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FBO::~FBO()
{
   //Delete resources
   glDeleteTextures(1, &texid_);
   glDeleteRenderbuffersEXT(1, &rb_);
   //Bind 0, which means render to back buffer, as a result, fb is unbound
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glDeleteFramebuffersEXT(1, &fb_);
}

static int lastw = 0;
static int lasth = 0;

// Obsolete
void updateScreenSizeForFBOHandler(int w, int h)
{
  lastw = w;
  lasth = h;
}

// choose input (textures) and output (FBO)
void FBO::useFBO(FBO *out, FBO *in1, FBO *in2)
{
  GLint curfbo;

// This was supposed to catch changes in viewport size and update lastw/lasth.
// It worked for me in the past, but now it causes problems to I have to
// fall back to manual updating.
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &curfbo);
  if (curfbo == 0)
  {
    GLint viewport[4] = {0,0,0,0};
    GLint w, h;
    glGetIntegerv(GL_VIEWPORT, viewport);
    w = viewport[2] - viewport[0];
    h = viewport[3] - viewport[1];
    if ((w > 0) && (h > 0) && (w < 65536) && (h < 65536)) // I don't believe in 64k pixel wide frame buffers for quite some time
    {
      lastw = viewport[2] - viewport[0];
      lasth = viewport[3] - viewport[1];
    }
  }
  
  if (out != nullptr)
    glViewport(0, 0, out->width_, out->height_);
  else
    glViewport(0, 0, lastw, lasth);

  if (out != nullptr)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, out->fb_);
    glViewport(0, 0, out->width_, out->height_);
  }
  else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glActiveTexture(GL_TEXTURE1);
  if (in2 != nullptr)
    glBindTexture(GL_TEXTURE_2D, in2->texid_);
  else
    glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  if (in1 != nullptr)
    glBindTexture(GL_TEXTURE_2D, in1->texid_);
  else
    glBindTexture(GL_TEXTURE_2D, 0);
}














void FBO3D::CHECK_FRAMEBUFFER_STATUS()
{
  GLenum status;
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    fprintf(stderr, "Framebuffer not complete\n");
}

FBO3D::FBO3D(int size)
{
  size_ = size;
  
  // create objects
  glGenFramebuffers(1, &fb_); // frame buffer id
  glBindFramebuffer(GL_FRAMEBUFFER, fb_);
  glGenTextures(1, &texid_);
  fprintf(stderr, "%i \n", texid_);
  glBindTexture(GL_TEXTURE_3D, texid_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, size, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texid_, 0, 0);

  // Renderbuffer
  // initialize depth renderbuffer
  glGenRenderbuffers(1, &rb_);
  glBindRenderbuffer(GL_RENDERBUFFER, rb_);
  glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size_, size_ );
  glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_ );
  CHECK_FRAMEBUFFER_STATUS();

  fprintf(stderr, "Framebuffer object %d\n", fb_);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FBO3D::~FBO3D()
{
   //Delete resources
   glDeleteTextures(1, &texid_);
   glDeleteRenderbuffersEXT(1, &rb_);
   //Bind 0, which means render to back buffer, as a result, fb is unbound
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
   glDeleteFramebuffersEXT(1, &fb_);
}

// choose input (textures) and output (FBO)
void FBO3D::useFBO(FBO3D *out, FBO3D *in1, FBO3D *in2)
{
  GLint curfbo;

// This was supposed to catch changes in viewport size and update lastw/lasth.
// It worked for me in the past, but now it causes problems to I have to
// fall back to manual updating.
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &curfbo);
  if (curfbo == 0)
  {
    GLint viewport[4] = {0,0,0,0};
    GLint w, h;
    glGetIntegerv(GL_VIEWPORT, viewport);
    w = viewport[2] - viewport[0];
    h = viewport[3] - viewport[1];
    if ((w > 0) && (h > 0) && (w < 65536) && (h < 65536)) // I don't believe in 64k pixel wide frame buffers for quite some time
    {
      lastw = viewport[2] - viewport[0];
      lasth = viewport[3] - viewport[1];
    }
  }
  
  if (out != nullptr)
    glViewport(0, 0, out->size_, out->size_);
  else
    glViewport(0, 0, lastw, lasth);

  if (out != nullptr)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, out->fb_);
    glViewport(0, 0, out->size_, out->size_);
  }
  else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glActiveTexture(GL_TEXTURE1);
  if (in2 != nullptr)
    glBindTexture(GL_TEXTURE_3D, in2->texid_);
  else
    glBindTexture(GL_TEXTURE_3D, 0);
  glActiveTexture(GL_TEXTURE0);
  if (in1 != nullptr)
    glBindTexture(GL_TEXTURE_3D, in1->texid_);
  else
    glBindTexture(GL_TEXTURE_3D, 0);
}