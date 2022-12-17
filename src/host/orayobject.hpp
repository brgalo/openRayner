#pragma once

#include "geometry.hpp"

#include <glm/fwd.hpp>
#include <memory>

namespace oray {

struct Transform2dComponent {
  glm::vec2 translation{};
  glm::vec2 scale{1.0f, 1.0f};
  float rotation = 0;
  glm::mat2 mat2() {
    const float s = glm::sin(rotation);
    const float c = glm::cos(rotation);
    glm::mat2 rotMat{{c,s},{-s,c}};
    glm::mat2 scaleMat{{scale.x, 0.f}, {0.f, scale.y}};
    return rotMat*scaleMat; }
};

class OrayObject {
public:
  using id_t = unsigned int;
  static OrayObject createOrayObject() {
    static id_t currentId = 0;
    return OrayObject{currentId++};
  }

  OrayObject(const OrayObject &) = delete;
  OrayObject &operator=(const OrayObject &) = delete;
  OrayObject(OrayObject &&) = default;
  OrayObject &operator=(OrayObject &&) = default;

  id_t getIf() { return id; }

  std::shared_ptr<Geometry> geom{};
  glm::vec3 color{};
  Transform2dComponent transform2d;
  
private:
  OrayObject(id_t objId) : id{objId} {};
  id_t id;
};
}