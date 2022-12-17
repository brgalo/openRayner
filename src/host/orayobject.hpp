#pragma once

#include "geometry.hpp"

#include <glm/fwd.hpp>
#include <memory>

#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace oray {

struct TransformComponent {
  glm::vec3 translation{};
  glm::vec3 scale{1.0f, 1.0f, 1.f};
  glm::vec3 rotation{};

  glm::mat4 mat4();
  glm::mat3 normalMatrix();
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
  TransformComponent transform;

private:
  OrayObject(id_t objId) : id{objId} {};
  id_t id;
};
} // namespace oray