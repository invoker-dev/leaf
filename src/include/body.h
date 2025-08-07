#include <cmath>
#include <entity.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/trigonometric.hpp>
#include <types.h>
#pragma once

struct Body {

  EntityData entityData;
  f64        baseScale;
  bool       isPlanet;
  u32        textureIndex;

  f64 a;     // semi-major axis
  f64 e;     // eccentricity
  f64 T;     // orbital period in sec
  f64 M0;    // mean anomaly at t=0
  f64 t;     // time since periapsis
  f64 i;     // inclination (rad)
  f64 o;     // argument of periapsis
  f64 Omega; // longitude of ascending node

  void update(f64 dt) { t += dt; }

  f64 eccentricAnomaly(f64 M) const {
    f64 E = M;
    for (int i = 0; i < 8; ++i) {
      E -= (E - e * sin(E) - M) / (1 - e * cos(E));
    }
    return E;
  }

  glm::vec3 getPosition(f32 distanceScale) const {
    if (isPlanet) {

      f64 pi = glm::pi<f64>();
      f64 M  = glm::radians(M0) + 2 * pi * (t / T);
      M      = fmod(M, 2 * pi);

      f64 E = eccentricAnomaly(M);

      f64 theta = 2 * atan2(sqrt(1 + e) * sin(E / 2), sqrt(1 - e) * cos(E / 2));

      f64 r = a * (1 - e * cos(E)) * distanceScale;

      f64 cosThetaOmega = cos(theta + glm::radians(o));
      f64 sinThetaOmega = sin(theta + glm::radians(o));
      f64 cosOmega      = cos(glm::radians(Omega));
      f64 sinOmega      = sin(glm::radians(Omega));
      f64 cosi          = cos(glm::radians(i));
      f64 sini          = sin(glm::radians(i));

      f64 x = r * (cosOmega * cosThetaOmega - sinOmega * sinThetaOmega * cosi);
      f64 y = r * (sinOmega * cosThetaOmega + cosOmega * sinThetaOmega * cosi);
      f64 z = r * (sinThetaOmega * sini);

      return glm::vec3(x, z, y);
    } else {
      return glm::vec3(0);
    }
  }
};
