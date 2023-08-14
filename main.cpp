#include <cstdio>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

struct Shader
{
  unsigned int ID;
  Shader(const char* path)
  {
    FILE *fp = fopen(path, "rb");
    if(!fp) return;

    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    rewind(fp);
    auto buf = new char[size + 1];
    fread(buf, size, 1, fp);
    buf[size] = 0;
    fclose(fp);


    unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &buf, NULL);
    glCompileShader(compute);

    ID = glCreateProgram();

    glAttachShader(ID, compute);
    glLinkProgram(ID);

    delete[] buf;

    printf("%s Compiled\n", path);
  }

  void Use()
  {
    glUseProgram(ID);
  }
};

struct Image
{
  unsigned int ID;
  Image(unsigned int w, unsigned int h)
  {
    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0);
    //need Max Level 0 OR min filter nearest/linear OR generate mipmap
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, w, h);
    glBindImageTexture(0, ID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
  }
};

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam );

int main(int argc, char **argv)
{
  if(!glfwInit()) return 1;
  puts("GLFW Initialized");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  auto win = glfwCreateWindow(640, 480, "Raymarch", NULL, NULL);
  if(!win) return 1;
  puts("Created Window");

  glfwMakeContextCurrent(win);
  if(glewInit() != GLEW_OK) return 1;
  puts("GLEW Initialized");

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  Shader shade("march.comp");
  Image img(640, 480);

  shade.Use();

  unsigned int fb;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    img.ID,
    0
  );
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  while(!glfwWindowShouldClose(win))
  {
    glUniform1f(0, glfwGetTime());

    glDispatchCompute(640, 480, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBlitNamedFramebuffer(
      fb, 0,
      0,0,640,480,
      0,0,640,480,
      GL_COLOR_BUFFER_BIT,
      GL_NEAREST
    );

    glfwSwapBuffers(win);

    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
    ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
    type, severity, message );
}
