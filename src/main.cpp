#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <vector>

struct Snowflake {
  sf::Vector2f position;
  sf::Vector2f speed;
  float radius;
  float sway;
  float phase;
};

float randomFloat(std::mt19937 &random, float minimum, float maximum) {
  std::uniform_real_distribution<float> range(minimum, maximum);
  return range(random);
}

void addSnow(std::vector<float> &snowDepth, float x, float amount, float spread,
             float maxDepth) {
  const int first = static_cast<int>(std::floor(x - spread));
  const int last = static_cast<int>(std::ceil(x + spread));

  float totalWeight = 0.0f;
  for (int i = first; i <= last; ++i) {
    const float distance = std::abs(static_cast<float>(i) - x);
    totalWeight += std::max(0.0f, 1.0f - distance / spread);
  }

  if (totalWeight == 0.0f)
    return;

  for (int i = first; i <= last; ++i) {
    if (i < 0 || i >= static_cast<int>(snowDepth.size()))
      continue;

    const float distance = std::abs(static_cast<float>(i) - x);
    const float weight = std::max(0.0f, 1.0f - distance / spread);
    snowDepth[static_cast<std::size_t>(i)] =
        std::min(maxDepth, snowDepth[static_cast<std::size_t>(i)] +
                               amount * weight / totalWeight);
  }
}

void smoothSnow(std::vector<float> &snowDepth, float deltaTime) {
  constexpr float MaxSlope = 0.85f;

  for (std::size_t i = 0; i + 1 < snowDepth.size(); ++i) {
    const float difference = snowDepth[i] - snowDepth[i + 1];
    const float slope = std::abs(difference);
    if (slope <= MaxSlope)
      continue;

    const float moveAmount = (slope - MaxSlope) * deltaTime * 0.7f;
    if (difference > 0.0f) {
      snowDepth[i] -= moveAmount;
      snowDepth[i + 1] += moveAmount;
    } else {
      snowDepth[i] += moveAmount;
      snowDepth[i + 1] -= moveAmount;
    }
  }
}

int main() {
  constexpr unsigned int WindowWidth = 800;
  constexpr unsigned int WindowHeight = 600;
  constexpr float FixedTimeStep = 1.0f / 60.0f;
  constexpr float Gravity = 28.0f;
  constexpr float MaxFallSpeed = 95.0f;
  constexpr float BaseSnowfallRate = 165.0f;
  constexpr float TwoPi = 2 * 3.1415926f;

  const float spawnMargin =
      std::max(120.0f, static_cast<float>(WindowWidth) * 0.2f);
  const float spawnWidth = static_cast<float>(WindowWidth) + spawnMargin * 2.0f;
  const float snowfallRate =
      BaseSnowfallRate * spawnWidth / static_cast<float>(WindowWidth);
  const float maxSnowDepth = static_cast<float>(WindowHeight) * 0.72f;

  sf::RenderWindow window(sf::VideoMode({WindowWidth, WindowHeight}),
                          "snow_simul");
  window.setVerticalSyncEnabled(true);

  std::mt19937 random(std::random_device{}());
  std::vector<Snowflake> snowflakes;
  std::vector<float> snowDepth(WindowWidth + 1, 0.0f);

  sf::VertexArray snowSurface(sf::PrimitiveType::TriangleStrip);
  sf::VertexArray snowLine(sf::PrimitiveType::LineStrip);
  sf::VertexArray snowflakeVertices(sf::PrimitiveType::Triangles);

  sf::Clock frameClock;
  float timeAccumulator = 0.0f;
  float spawnAccumulator = 0.0f;
  float elapsedTime = 0.0f;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        window.close();
    }

    if (!window.isOpen())
      break;

    timeAccumulator += std::min(frameClock.restart().asSeconds(), 0.25f);

    while (timeAccumulator >= FixedTimeStep) {
      elapsedTime += FixedTimeStep;

      spawnAccumulator += FixedTimeStep * snowfallRate;
      while (spawnAccumulator >= 1.0f) {
        Snowflake flake;
        flake.position = {
            randomFloat(random, -spawnMargin,
                        static_cast<float>(WindowWidth) + spawnMargin),
            randomFloat(random, -12.0f, 0.0f)};
        flake.speed = {randomFloat(random, -7.0f, 7.0f),
                       randomFloat(random, 28.0f, 52.0f)};
        flake.radius = randomFloat(random, 1.1f, 2.6f);
        flake.sway = randomFloat(random, 4.0f, 14.0f);
        flake.phase = randomFloat(random, 0.0f, TwoPi);
        snowflakes.push_back(flake);

        spawnAccumulator -= 1.0f;
      }

      const float wind = std::sin(elapsedTime * 0.31f) * 13.0f +
                         std::sin(elapsedTime * 0.83f) * 5.0f;

      for (std::size_t i = 0; i < snowflakes.size();) {
        Snowflake &flake = snowflakes[i];
        flake.speed.y =
            std::min(flake.speed.y + Gravity * FixedTimeStep, MaxFallSpeed);
        flake.position.x +=
            (flake.speed.x + wind +
             std::sin(elapsedTime * 1.8f + flake.phase) * flake.sway) *
            FixedTimeStep;
        flake.position.y += flake.speed.y * FixedTimeStep;

        const bool outsideSpawnArea =
            flake.position.x < -spawnMargin - flake.radius ||
            flake.position.x > WindowWidth + spawnMargin + flake.radius;
        const bool belowWindow = flake.position.y > WindowHeight + flake.radius;

        if (outsideSpawnArea || belowWindow) {
          snowflakes[i] = snowflakes.back();
          snowflakes.pop_back();
          continue;
        }

        const bool insideWindow =
            flake.position.x >= 0.0f && flake.position.x <= WindowWidth;
        if (insideWindow) {
          const int x = static_cast<int>(flake.position.x);
          const float groundY =
              WindowHeight - snowDepth[static_cast<std::size_t>(x)];
          if (flake.position.y + flake.radius >= groundY) {
            addSnow(snowDepth, flake.position.x, flake.radius * 2.2f,
                    flake.radius * 3.0f, maxSnowDepth);
            snowflakes[i] = snowflakes.back();
            snowflakes.pop_back();
            continue;
          }
        }

        ++i;
      }

      smoothSnow(snowDepth, FixedTimeStep);
      timeAccumulator -= FixedTimeStep;
    }

    snowSurface.clear();
    snowLine.clear();
    snowflakeVertices.clear();

    const sf::Color snowTopColor(228, 241, 252);
    const sf::Color snowBottomColor(190, 215, 240);
    const sf::Color snowWhite(250, 253, 255);

    for (unsigned int x = 0; x <= WindowWidth; ++x) {
      const float screenX = static_cast<float>(x);
      const float surfaceY = static_cast<float>(WindowHeight) - snowDepth[x];
      snowSurface.append(sf::Vertex{{screenX, surfaceY}, snowTopColor});
      snowSurface.append(sf::Vertex{{screenX, static_cast<float>(WindowHeight)},
                                    snowBottomColor});
      snowLine.append(sf::Vertex{{screenX, surfaceY}, snowWhite});
    }

    for (const Snowflake &flake : snowflakes) {
      const float x = flake.position.x;
      const float y = flake.position.y;
      const float r = flake.radius;
      const sf::Color color(235, 245, 255);

      snowflakeVertices.append(sf::Vertex{{x, y - r}, color});
      snowflakeVertices.append(sf::Vertex{{x - r, y}, color});
      snowflakeVertices.append(sf::Vertex{{x, y + r}, color});
      snowflakeVertices.append(sf::Vertex{{x, y - r}, color});
      snowflakeVertices.append(sf::Vertex{{x + r, y}, color});
      snowflakeVertices.append(sf::Vertex{{x, y + r}, color});
    }

    window.clear(sf::Color(12, 22, 42));
    window.draw(snowSurface);
    window.draw(snowflakeVertices);
    window.draw(snowLine);
    window.display();
  }
}
