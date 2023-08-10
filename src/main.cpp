#include <cstddef>
#include <glm/fwd.hpp>
#include <iostream>
#include <math.h>
#include <string>

#include <glm/glm.hpp>
#include <vector>
#include "glm/gtc/matrix_transform.hpp"
#include "renderer.hh"
#include "buffers.hh"
#include "shaders.hh"
#include "camera.hh"
#include "mesh.hh"
#include "materials.hh"
#include "lights.hh"
#include "vendor/obj_loader/obj_loader.h"

#include "containers.hh"

#define TAU 6.28

// settings
int width = 1280;
int height = 720;
bool first_mouse_call = true;
float mouse_last_x = width / 2.0f;
float mouse_last_y = height / 2.0f;

// setup free camera
Camera camera(glm::vec3(0.0f, 0.0f, 12.0f));

void viewport_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
// triagle positions and incidices - vertex collection

glm::vec3 cube_word_positions[] = {glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
                                   glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
                                   glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
                                   glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
                                   glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

int main(void) {
  char window_name[] = "Engine3D";
  camera.set_moviment_speed(8.0f);

  Renderer render(window_name, width, height);
  render.set_swap_interval(true);
  render.set_depth_test();
  render.set_mouse_moviment_callback((void *)mouse_callback);
  render.set_mouse_scroll_callback((void *)scroll_callback);
  render.set_viewport_size_callback((void *)viewport_size_callback);
  // render.set_mouse_button_callback((void *)mouse_button_callback);

  // load shader source code
  Shader shader("res/shaders/material.shader");
  Shader shader_light("res/shaders/light.shader");

  // imgui
  ImGuiRenderer imgui(render.get_window());
  ImGuiIO &imgui_io = ImGui::GetIO();

  // So, basically MVP:
  // Model matrix: defines position, rotation and scale of the vertices of the model in the world.
  // View matrix: defines position and orientation of the "camera".
  // Projection matrix: Maps what the "camera" sees to NDC, taking carX1e of aspect ratio and perspective.
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;

  // light
  PointLight point_light = {
      glm::vec3(0.7f, 0.2f, 2.0f),

      glm::vec3(0.4f, 0.4f, 0.4f),
      glm::vec3(0.5f, 0.5f, 0.5f),
      glm::vec3(1.0f, 1.0f, 1.0f),

      1.0f,
      0.09f,
      0.032f,
  };
  DirLight directional_light = {
      glm::vec3(-0.2f, -1.0f, -0.3f),

      glm::vec3(0.5f, 0.5f, 0.5f),
      glm::vec3(0.4f, 0.4f, 0.4f),
      glm::vec3(0.5f, 0.5f, 0.5f),
  };

  Mesh light(Container::vertices, Container::indices);

  std::vector<Mesh *> meshes;
  Material *material;
  {
    objl::Loader loader;
    std::string path_models = "res/models/";
    std::string obj = "backpack";
    std::string obj_name = path_models + obj;
    if (!loader.LoadFile(obj_name + "/" + obj + ".obj")) {
      std::cout << "[ERROR] Fail to load model" << std::endl;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // int m = 0;

    for (size_t m = 0; m < loader.LoadedMeshes.size(); m++) {
      for (size_t i = 0; i < loader.LoadedMeshes[m].Vertices.size(); i++) {
        Vertex v;
        v.position.x = loader.LoadedMeshes[m].Vertices[i].Position.X;
        v.position.y = loader.LoadedMeshes[m].Vertices[i].Position.Y;
        v.position.z = loader.LoadedMeshes[m].Vertices[i].Position.Z;

        v.normal.x = loader.LoadedMeshes[m].Vertices[i].Normal.X;
        v.normal.y = loader.LoadedMeshes[m].Vertices[i].Normal.Y;
        v.normal.z = loader.LoadedMeshes[m].Vertices[i].Normal.Z;

        v.tex_coord.x = loader.LoadedMeshes[m].Vertices[i].TextureCoordinate.X;
        v.tex_coord.y = loader.LoadedMeshes[m].Vertices[i].TextureCoordinate.Y;
        vertices.push_back(v);
      }
    }
    for (size_t i = 0; i < loader.LoadedIndices.size(); i++) {
      indices.push_back(loader.LoadedIndices[i]);
    }
    meshes.push_back(new Mesh(vertices, indices));
    material = new Material(obj_name + "/" + loader.LoadedMaterials[0].map_Kd,
                            obj_name + "/" + loader.LoadedMaterials[0].map_Ks,
                            obj_name + "/" + loader.LoadedMaterials[0].map_Ke, 1.0f, 1.0f, false);
  }

  /* Loop until the user closes the window */
  while (render.running()) {
    imgui.clear();
    render.clear();

    // handler key
    {
      if (glfwGetKey(render.get_window(), GLFW_KEY_W) == GLFW_PRESS)
        camera.process_keyboard(camera_direction_t::FORWARD, render.get_deltatime());
      if (glfwGetKey(render.get_window(), GLFW_KEY_S) == GLFW_PRESS)
        camera.process_keyboard(camera_direction_t::BACKWARD, render.get_deltatime());
      if (glfwGetKey(render.get_window(), GLFW_KEY_A) == GLFW_PRESS)
        camera.process_keyboard(camera_direction_t::LEFT, render.get_deltatime());
      if (glfwGetKey(render.get_window(), GLFW_KEY_D) == GLFW_PRESS)
        camera.process_keyboard(camera_direction_t::RIGHT, render.get_deltatime());
    }

    // imgui panel
    {
      ImGui::Begin("Menu");
      if (ImGui::TreeNode("Point Light")) {
        ImGui::ColorEdit3("Ambiant light", &directional_light.ambient.x);
        ImGui::ColorEdit3("Diffuse light", &directional_light.diffuse.x);
        ImGui::ColorEdit3("Specular light", &directional_light.specular.x);
        ImGui::TreePop();
      }
      if (ImGui::TreeNode("Directional Light")) {
        ImGui::ColorEdit3("Ambiant light", &point_light.ambient.x);
        ImGui::ColorEdit3("Diffuse light", &point_light.diffuse.x);
        ImGui::ColorEdit3("Specular light", &point_light.specular.x);
        ImGui::SliderFloat("Constant", &point_light.constant, 0.0f, 1.0f);
        ImGui::SliderFloat("Linear", &point_light.linear, 0.0f, 1.0f);
        ImGui::SliderFloat("Quadratic", &point_light.quadratic, 0.0f, 1.0f);
        ImGui::TreePop();
      }
      // TODO move to material debug option
      // if (ImGui::TreeNode("Material")) {
      //   ImGui::SliderFloat("Shininess material", &material.shininess, 0.0f, 1.0f);
      //   ImGui::SliderFloat("Emission material", &material.emissioness, 0.0f, 1.0f);
      //   ImGui::Checkbox("Emission mask?", &material.emission_mask);
      //   ImGui::TreePop();
      // }
      // ImGui::Checkbox("Rotate?", &rotate);
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / imgui_io.Framerate, imgui_io.Framerate);
      ImGui::End();
    }

    /* draw here */
    view = camera.get_camera_matrix();
    proj = glm::perspective(glm::radians(camera.get_fov()), (float)render.get_width() / (float)render.get_height(),
                            0.1f, 100.0f);

    shader.bind();
    model = glm::translate(glm::mat4(1.0f), cube_word_positions[0]);  // multiply z to use positive bar in gui
    model = glm::scale(model, glm::vec3(1.0f));

    shader.set_uniform_vec3("u_DirLight.direction", directional_light.direction);
    shader.set_uniform_vec3("u_DirLight.ambient", directional_light.ambient);
    shader.set_uniform_vec3("u_DirLight.diffuse", directional_light.diffuse);
    shader.set_uniform_vec3("u_DirLight.specular", directional_light.specular);

    shader.set_uniform_vec3("u_PointLight.position", point_light.position);
    shader.set_uniform_vec3("u_PointLight.ambient", point_light.ambient);
    shader.set_uniform_vec3("u_PointLight.diffuse", point_light.diffuse);
    shader.set_uniform_vec3("u_PointLight.specular", point_light.specular);
    shader.set_uniform_float1("u_PointLight.constant", point_light.constant);
    shader.set_uniform_float1("u_PointLight.linear", point_light.linear);
    shader.set_uniform_float1("u_PointLight.quadratic", point_light.quadratic);

    shader.set_uniform_mat4("u_V", view);
    shader.set_uniform_mat4("u_M", model);
    shader.set_uniform_mat4("u_P", proj);

    material->bind(shader);
    meshes[0]->draw(render, shader);
    material->unbind();

    // third cube - the light
    {
      shader_light.bind();
      shader_light.set_uniform_mat4("u_V", view);

      shader_light.set_uniform_vec3("u_Light.ambient", point_light.ambient);
      shader_light.set_uniform_vec3("u_Light.diffuse", point_light.diffuse);

      point_light.position.x = 4.0f * sin(render.get_time());
      point_light.position.z = 5.0f * cos(render.get_time());

      model = glm::translate(glm::mat4(1.0f), point_light.position);
      model = glm::scale(model, glm::vec3(0.5));

      shader_light.set_uniform_mat4("u_M", model);
      shader_light.set_uniform_mat4("u_P", proj);

      light.draw(render, shader_light);  // todo: movo to a batch render and draw once;
    }

    imgui.draw();
    render.end_frame();
  }

  return 0;
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (first_mouse_call) {
    mouse_last_x = xpos;
    mouse_last_y = ypos;
    first_mouse_call = false;
  }

  float xoffset = xpos - mouse_last_x;
  float yoffset = mouse_last_y - ypos;  // reversed since y-coordinates go from bottom to top

  mouse_last_x = xpos;
  mouse_last_y = ypos;

  int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
  if (state) {
    camera.process_mouse_moviment(xoffset, yoffset);
  }
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { camera.process_mouse_scroll(yoffset); }
void viewport_size_callback(GLFWwindow *window, int width, int height) { glViewport(0, 0, width, height); }
