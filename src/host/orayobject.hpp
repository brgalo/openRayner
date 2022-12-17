#pragma once

#include "geometry.hpp"

#include <glm/fwd.hpp>
#include <memory>

namespace oray {

struct Transform2dComponent {
  glm::vec2 translation{};

  glm::mat2 mat2() { return glm::mat2{1.0f}; }
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