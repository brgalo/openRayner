#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include "orayobject.hpp"
#include "raytracing.hpp"
#include "vector"

namespace oray {

class Raytracer;

#ifndef STATE_H
#define STATE_H
struct State {

  static bool itemGetter(void *data, int idx, const char **out_str) {
    *out_str =
        reinterpret_cast<std::string*>(data)[idx].c_str();
    return true;
  };

  void setTriangleNames(OrayObject *obj) {
        std::vector<std::string> names;
    for (int i = 0; i < obj->geom->getIndexCount() / 3; ++i) {
      names.push_back("Triangle" + std::to_string(i));
    }
    triNames = names;
  }
  State() {};
  State(OrayObject *obj) {
    setTriangleNames(obj);
  };


  float lineWidth = 1.0f;
  bool doTrace = true;
  bool HAS_CHANGED = true;
  int currTri = 0;
  int nRays = 50;
  int nBufferElements = nRays;
  std::vector<std::string> triNames{};
};
}

#endif