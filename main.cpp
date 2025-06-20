#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
const int screenW = 1280;
const int screenH = 720;
const int numDrops = 3000;
const float rainAreaSize = 50.0f;
const float baseRainSpeed = 0.8f;
const float dropLength = 0.4f;
const float groundLevel = 0.0f;
const Vector3 windVelocity = {0.1f, 0.0f, 0.04f};
const int numSplashParticles = 8;
const float splashLifetime = 0.3f;
const float splashVelocity = 2.0f;
const float splashGravity = -5.0f;
const float waterWidth = rainAreaSize * 1.5f;
const float waterDepth = rainAreaSize * 1.5f;
const int waterDivisionsX = 60;
const int waterDivisionsZ = 60;
Mesh waterMesh = {0};
Model waterModel = {0};
typedef struct {
  Vector2 center;
  float startTime;
  float strength;
  float maxRadius;
  float duration;
  float wavelength;
  bool active;
} WaterRipple;
std::vector<WaterRipple> waterRipples;
const int MAX_RIPPLES = 30;
Model iCubeModel = {0};
Vector3 iCubePos = {0.0f, 0.0f, 0.0f};
float iCubeSize = 5.0f;
Color iCubeColor = RED;
float iCubeBobbleTime = 0.0f;
typedef struct {
  Vector3 pos;
  Vector3 velocity;
  Color color;
  float speedVariation;
} RainDrop;
typedef struct {
  Vector3 pos;
  Vector3 velocity;
  Color color;
  float lifeLeft;
} SplashParticle;
float getRandomFloat(float min, float max) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}
void CreateSplash(std::vector<SplashParticle> &splashes, Vector3 impactPos,
                  Color baseColor) {
  for (int i = 0; i < numSplashParticles; ++i) {
    SplashParticle p = {};
    p.pos = impactPos;
    p.velocity.x = getRandomFloat(-splashVelocity, splashVelocity) * 0.5f;
    p.velocity.y = getRandomFloat(0.5f, 1.0f) * splashVelocity;
    p.velocity.z = getRandomFloat(-splashVelocity, splashVelocity) * 0.5f;
    p.color = Fade(baseColor, 0.7f);
    p.lifeLeft = splashLifetime;
    splashes.push_back(p);
  }
}
int main(void) {
  InitWindow(screenW, screenH, "Stormy Sim with Interactive Cube");
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  Camera camera = {0};
  camera.position = Vector3{25.0f, 15.0f, 25.0f};
  camera.target = Vector3{0.0f, 3.0f, 0.0f};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;
  Image skyImg = GenImageGradientLinear(
      screenW, screenH, 0, Color{15, 20, 25, 255}, Color{40, 50, 60, 255});
  TextureCubemap skyboxTexture =
      LoadTextureCubemap(skyImg, CUBEMAP_LAYOUT_AUTO_DETECT);
  UnloadImage(skyImg);
  Mesh unitCubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
  Model skyboxModel = LoadModelFromMesh(unitCubeMesh);
  skyboxModel.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = skyboxTexture;
  iCubeModel = LoadModelFromMesh(unitCubeMesh);
  iCubeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = iCubeColor;
  iCubePos = Vector3{0.0f, groundLevel + iCubeSize / 2.0f + 1.0f, 0.0f};
  waterMesh =
      GenMeshPlane(waterWidth, waterDepth, waterDivisionsX, waterDivisionsZ);
  waterModel = LoadModelFromMesh(waterMesh);
  waterModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color =
      Color{60, 100, 150, 200};
  waterRipples.resize(MAX_RIPPLES);
  for (int i = 0; i < MAX_RIPPLES; ++i) waterRipples[i].active = false;
  std::vector<RainDrop> raindrops(numDrops);
  std::vector<SplashParticle> splashes;
  splashes.reserve(numDrops * numSplashParticles / 10);
  for (int i = 0; i < numDrops; i++) {
    raindrops[i].pos.x =
        getRandomFloat(-rainAreaSize / 2.0f, rainAreaSize / 2.0f);
    raindrops[i].pos.y =
        getRandomFloat(groundLevel + 5.0f, rainAreaSize * 1.5f);
    raindrops[i].pos.z =
        getRandomFloat(-rainAreaSize / 2.0f, rainAreaSize / 2.0f);
    raindrops[i].speedVariation = getRandomFloat(0.8f, 1.2f);
    raindrops[i].velocity.x = windVelocity.x;
    raindrops[i].velocity.y =
        (-baseRainSpeed * raindrops[i].speedVariation) + windVelocity.y;
    raindrops[i].velocity.z = windVelocity.z;
    unsigned char base_val = (unsigned char)getRandomFloat(100.0f, 180.0f);
    raindrops[i].color =
        Color{base_val, base_val, (unsigned char)(base_val + 10), 200};
  }
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    float currentTime = (float)GetTime();
    UpdateCamera(&camera, CAMERA_ORBITAL);
    iCubeBobbleTime += dt;
    iCubePos.x = cosf(iCubeBobbleTime * 0.4f) * (rainAreaSize / 3.5f);
    iCubePos.z = sinf(iCubeBobbleTime * 0.4f) * (rainAreaSize / 3.5f);
    iCubePos.y = groundLevel + iCubeSize / 2.0f + 0.5f +
                 sinf(iCubeBobbleTime * 0.6f) * 2.0f;
    for (int i = 0; i < numDrops; i++) {
      raindrops[i].pos = Vector3Add(
          raindrops[i].pos, Vector3Scale(raindrops[i].velocity, dt * 60.0f));
      if (raindrops[i].pos.y < groundLevel) {
        Vector3 impactPos = {raindrops[i].pos.x, groundLevel,
                             raindrops[i].pos.z};
        CreateSplash(splashes, impactPos, raindrops[i].color);
        for (int r = 0; r < MAX_RIPPLES; ++r) {
          if (!waterRipples[r].active) {
            waterRipples[r].active = true;
            waterRipples[r].center = Vector2{impactPos.x, impactPos.z};
            waterRipples[r].startTime = currentTime;
            waterRipples[r].strength = getRandomFloat(0.15f, 0.3f);
            waterRipples[r].maxRadius = getRandomFloat(3.0f, 5.0f);
            waterRipples[r].duration = getRandomFloat(2.0f, 3.5f);
            waterRipples[r].wavelength = getRandomFloat(0.5f, 1.0f);
            break;
          }
        }
        raindrops[i].pos.x =
            getRandomFloat(-rainAreaSize / 2.0f, rainAreaSize / 2.0f);
        raindrops[i].pos.y =
            rainAreaSize + getRandomFloat(0.0f, rainAreaSize * 0.5f);
        raindrops[i].pos.z =
            getRandomFloat(-rainAreaSize / 2.0f, rainAreaSize / 2.0f);
      }
    }
    for (int i = splashes.size() - 1; i >= 0; --i) {
      splashes[i].lifeLeft -= dt;
      if (splashes[i].lifeLeft <= 0) {
        splashes.erase(splashes.begin() + i);
      } else {
        splashes[i].velocity.y += splashGravity * dt;
        splashes[i].pos =
            Vector3Add(splashes[i].pos, Vector3Scale(splashes[i].velocity, dt));
        float lifeRatio = splashes[i].lifeLeft / splashLifetime;
        splashes[i].color.a = (unsigned char)(255.0f * lifeRatio);
      }
    }
    float *vertices = waterMesh.vertices;
    for (int i = 0; i < waterMesh.vertexCount; i++) {
      float vx = vertices[i * 3 + 0];
      float vz = vertices[i * 3 + 2];
      float dynamicWaterHeight = groundLevel;
      dynamicWaterHeight += 0.3f * sinf(vx * 0.3f + currentTime * 1.5f);
      dynamicWaterHeight += 0.25f * cosf(vz * 0.25f + currentTime * 1.1f);
      for (int r = 0; r < MAX_RIPPLES; ++r) {
        if (waterRipples[r].active) {
          WaterRipple *ripple = &waterRipples[r];
          float age = currentTime - ripple->startTime;
          if (age > ripple->duration) {
            ripple->active = false;
            continue;
          }
          float distToCenter = Vector2Distance(Vector2{vx, vz}, ripple->center);
          float strengthFactor =
              ripple->strength * (1.0f - age / ripple->duration);
          float propagationSpeed = ripple->maxRadius / ripple->duration;
          float currentWaveFrontRadius = propagationSpeed * age;
          float wavePacketOffset = distToCenter - currentWaveFrontRadius;
          float envelopeWidthFactor = 1.5f;
          if (fabsf(wavePacketOffset) <
              ripple->wavelength * envelopeWidthFactor) {
            float phase = wavePacketOffset / ripple->wavelength * 2.0f * PI;
            float rippleVal = sinf(phase);
            float envelope = expf(
                -powf(wavePacketOffset / (ripple->wavelength * 0.8f), 2.0f));
            dynamicWaterHeight += strengthFactor * rippleVal * envelope;
          }
        }
      }
      vertices[i * 3 + 1] = dynamicWaterHeight;
      float distToICubeXZ =
          Vector2Distance(Vector2{vx, vz}, Vector2{iCubePos.x, iCubePos.z});
      float cWakeRad = iCubeSize * 1.5f;
      float cWakeStr = 0.1f;
      float cWakeWaveLen = 2.5f;
      float cWakeSpd = 2.0f;
      if (distToICubeXZ < cWakeRad && distToICubeXZ > iCubeSize * 0.3f) {
        float normalizedDist = distToICubeXZ / cWakeRad;
        float falloff = (1.0f - normalizedDist) * (1.0f - normalizedDist);
        float phase = (distToICubeXZ / cWakeWaveLen) -
                      (currentTime * cWakeSpd / cWakeWaveLen);
        vertices[i * 3 + 1] += sinf(phase * 2.0f * PI) * cWakeStr * falloff;
      }
      float iCubeBottomY = iCubePos.y - iCubeSize / 2.0f;
      bool isUnderICubeXZ = (vx >= iCubePos.x - iCubeSize / 2.0f &&
                             vx <= iCubePos.x + iCubeSize / 2.0f &&
                             vz >= iCubePos.z - iCubeSize / 2.0f &&
                             vz <= iCubePos.z + iCubeSize / 2.0f);
      if (isUnderICubeXZ) {
        if (iCubeBottomY < vertices[i * 3 + 1]) {
          vertices[i * 3 + 1] = iCubeBottomY;
        }
      }
    }
    UpdateMeshBuffer(
        waterMesh, 0, waterMesh.vertices,
        waterMesh.vertexCount * 3 * static_cast<int>(sizeof(float)), 0);
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModel(skyboxModel, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    DrawModel(waterModel, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    DrawModelEx(iCubeModel, iCubePos, Vector3{0.0f, 1.0f, 0.0f}, 0.0f,
                Vector3{iCubeSize, iCubeSize, iCubeSize}, WHITE);
    for (int i = 0; i < numDrops; i++) {
      Vector3 velDir = Vector3Normalize(raindrops[i].velocity);
      Vector3 dropEndPos =
          Vector3Add(raindrops[i].pos, Vector3Scale(velDir, -dropLength));
      float dist = Vector3Distance(raindrops[i].pos, camera.position);
      float fadeStartDist = rainAreaSize * 0.5f;
      float fadeEndDist = rainAreaSize * 1.5f;
      float alpha = 1.0f;
      if (dist > fadeStartDist) {
        alpha = 1.0f - (dist - fadeStartDist) / (fadeEndDist - fadeStartDist);
        alpha = Clamp(alpha, 0.0f, 1.0f);
      }
      Color dropColor = raindrops[i].color;
      dropColor.a = (unsigned char)((float)raindrops[i].color.a * alpha);
      DrawLine3D(raindrops[i].pos, dropEndPos, dropColor);
    }
    for (const auto &p : splashes) {
      DrawPoint3D(p.pos, p.color);
    }
    EndMode3D();
    DrawFPS(10, 10);
    DrawText("Drag mouse to orbit camera", 10, 40, 20, LIME);
    DrawText(TextFormat("Splashes: %zu", splashes.size()), 10, 70, 20, LIME);
    DrawText(TextFormat("Active Ripples: %d/%d",
                        (int)std::count_if(
                            waterRipples.begin(), waterRipples.end(),
                            [](const WaterRipple &r) { return r.active; }),
                        MAX_RIPPLES),
             10, 100, 20, LIME);
    EndDrawing();
  }
  UnloadTexture(skyboxTexture);
  UnloadModel(skyboxModel);
  UnloadModel(iCubeModel);
  UnloadModel(waterModel);
  CloseWindow();
  return 0;
}
