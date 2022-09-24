#include <iostream>
#include <memory>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>

typedef struct Body
{
  SDL_FRect rect;
  SDL_FPoint velocity, acceleration, center;
  SDL_Rect texture_rect;
  int last_time, end_time;
  float rotate_velocity;
  SDL_Texture *texture;
  float angle;

  Body(SDL_FRect rect) noexcept
    : rect(rect), center({rect.w / 2.f, rect.h / 2.f}), rotate_velocity(0.f), angle(0.f)
  {
    texture_rect = {0, 0, 0, 0};
    last_time = SDL_GetTicks();
    end_time = std::rand() % 1000 + 250;
  }

  void loadTexture(SDL_Renderer *renderer, const char *image_path) noexcept
  {
    texture = IMG_LoadTexture(renderer, image_path);
  }
  bool textureRectValid()
  {
    return !(texture_rect.x == 0 && texture_rect.y == 0 && texture_rect.w == 0 && texture_rect.h == 0);
  }
  void fire_motor() noexcept
  {
    acceleration = { std::cos((angle - 90.f) * ((float) M_PI / 180.f)) / 100.f, std::sin((angle - 90.f) * ((float) M_PI / 180.f)) / 100.f};
    velocity.x += acceleration.x;
    velocity.y += acceleration.y;
  }
  void update() noexcept
  {
    rect.x += velocity.x;
    rect.y += velocity.y;
  }
  int mass() noexcept
  {
    return rect.w * rect.h;
  }
  void render(SDL_Renderer *renderer) noexcept
  {
    SDL_RenderCopyExF(renderer, texture, textureRectValid() ? &texture_rect : NULL, &rect, angle, &center, SDL_FLIP_NONE);
  }
  bool isDead() noexcept
  {
    return getLifeTime() > end_time;
  }
  int getLifeTime() noexcept
  {
    return SDL_GetTicks() - last_time;
  }
} Body;

class FireParticle
{
public:
  FireParticle(SDL_Renderer *renderer)
  {
    img = IMG_LoadTexture(renderer, "data/assets/fire.png");
  }
  void add(int countm, SDL_FPoint here, float angle) noexcept
  {
    (void) angle;
    for(int i = 0; i < countm; i++)
    {
      Body *b = new Body({here.x, here.y, 16.f, 16.f});
      b->texture = img;
      b->texture_rect = {0, 0, 8, 8};
      b->velocity.x = cos((angle + 90.f) * ((float) M_PI / 180.f)) * (std::rand() % 10 + 2);
      b->velocity.y = sin((angle + 90.f) * ((float) M_PI / 180.f)) * (std::rand() % 10 + 2);

      particles.push_back(b);
    }
  }

  void update() noexcept
  {
    for(size_t i = 0; i < particles.size(); i++)
    {
      Body &p = *particles[i];
      p.update();
      if(p.isDead())
        particles.erase(particles.begin() + i);
    }
  }
  
  void render(SDL_Renderer *renderer) noexcept
  {
    for(auto &it : particles)
      it->render(renderer);
  }
private:
  SDL_Texture *img;
  std::vector<Body*> particles;
};

class LawOfGravitationUniversal
{
public:
  static inline float G = 9.806f;

  static void UpdateBodys(Body &a, Body &b) noexcept
  {
    SDL_FPoint distance = {(b.rect.x + b.rect.w / 2) - (a.rect.x + a.rect.w / 2), (b.rect.y + b.rect.h / 2) - (a.rect.y + a.rect.h / 2)};
    float
      angle = std::atan2(distance.y, distance.x),
      mag = std::sqrt(std::pow(distance.x, 2) + std::pow(distance.y, 2)),
      force = LawOfGravitationUniversal::G * (a.mass() + b.mass()) / std::pow(mag, 2);
    a.acceleration = {std::cos(angle) * force / a.mass(), std::sin(angle) * force / a.mass()};

    a.rect.x += a.velocity.x += a.acceleration.x;
    a.rect.y += a.velocity.y += a.acceleration.y;

  }
};

int main()
{
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window (
    SDL_CreateWindow("LawOfGravitationUniversal", 0, 0, 600, 600, SDL_WINDOW_SHOWN),
    &SDL_DestroyWindow
  );
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer (
    SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_PRESENTVSYNC),
    &SDL_DestroyRenderer
  );

  Body rocket({200.f, 100.f, 16.f, 32.f}), earth({250.f, 250.f, 100.f, 100.f});
  rocket.velocity.x = 0.5f;
  rocket.loadTexture(renderer.get(), "data/assets/rocket.png");
  earth.loadTexture(renderer.get(), "data/assets/earth.png");
  bool rocket_rotate_left = false, rocket_rotate_right = false, rocket_motor_fire = false;
  std::vector<SDL_FPoint> rocket_progress;
  FireParticle particles(renderer.get());

  SDL_Event event;
  while(window.get() && renderer.get())
  {
    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_QUIT)
        window.~unique_ptr();
      if(event.type == SDL_KEYDOWN)
      {
        if(event.key.keysym.sym == SDLK_a)
          rocket_rotate_left = true;
        if(event.key.keysym.sym == SDLK_d)
          rocket_rotate_right = true;
        if(event.key.keysym.sym == SDLK_SPACE)
          rocket_motor_fire = true;
      }
      if(event.type == SDL_KEYUP)
      {
        if(event.key.keysym.sym == SDLK_a)
          rocket_rotate_left = false;
        if(event.key.keysym.sym == SDLK_d)
          rocket_rotate_right = false;
        if(event.key.keysym.sym == SDLK_SPACE)
          rocket_motor_fire = false;
      }
    }
    if(rocket_rotate_left)
      rocket.rotate_velocity -= 0.02f;
    else if(rocket_rotate_right)
      rocket.rotate_velocity += 0.02f;
    rocket.angle += rocket.rotate_velocity;
    if(rocket_motor_fire)
    {
      rocket.fire_motor();
      particles.add(10, {
        (rocket.rect.x + rocket.rect.w / 2.f) - 20.f * cos((rocket.angle - 90.f) * ((float) M_PI / 180.f)),
        (rocket.rect.y + rocket.rect.h / 2.f) - 20.f * sin((rocket.angle - 90.f) * ((float) M_PI / 180.f))
        //(rocket.rect.x + rocket.rect.w / 2.f) + (rocket.rect.h - 20) * cos((rocket.angle + 90.f) * ((float) M_PI / 180.f)),
        //(rocket.rect.y + rocket.rect.h / 2.f) + (rocket.rect.h - 20) * sin((rocket.angle + 90.f) * ((float) M_PI / 180.f))
      }, rocket.angle);
    }
    rocket.update();
    LawOfGravitationUniversal::UpdateBodys(rocket, earth);
    rocket_progress.push_back({rocket.rect.x + rocket.rect.w / 2, rocket.rect.y + rocket.rect.h / 2});

    SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer.get());

    rocket.render(renderer.get());
    earth.render(renderer.get());

    particles.update();
    particles.render(renderer.get());

    SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
    if(rocket_progress.size() > 0)
      for(int i = 0; i < (int) rocket_progress.size() - 1; i++)
      {
        SDL_FPoint b = rocket_progress[i], a = rocket_progress[i + 1];
        SDL_RenderDrawLineF(renderer.get(), a.x, a.y, b.x, b.y);
        if(rocket_progress.size() > 100)
          rocket_progress.erase(rocket_progress.begin());
      }
    SDL_RenderDrawLineF(renderer.get(), rocket.rect.x + rocket.rect.w / 2.f, rocket.rect.y + rocket.rect.h / 2.f, (rocket.rect.x + rocket.rect.w / 2.f) + 50.f * cos((rocket.angle - 90.f) * (M_PI / 180.f)), (rocket.rect.y + rocket.rect.h / 2.f) + 50.f * sin((rocket.angle - 90.f) * (M_PI / 180.f)));

    SDL_RenderPresent(renderer.get());
  }

  return EXIT_SUCCESS;
}
