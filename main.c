#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define BLOCK_SIZE 20
#define START_SPEED 0.2

#define COLS (WIN_WIDTH / BLOCK_SIZE)
#define ROWS (WIN_HEIGHT / BLOCK_SIZE)
#define NUM_BLOCKS (COLS * ROWS)

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

SDL_Color BG_COLOR = {0x00, 0x00, 0x00, 0xFF};
SDL_Color FG_COLOR = {0xFF, 0xFF, 0xFF, 0xFF};
SDL_Color HEAD_COLOR = {0xC7, 0xC7, 0xC7, 0xFF};
SDL_Color FOOD_COLOR = {0x9C, 0x17, 0x00, 0xFF};

SDL_Texture *NUMBERS_TEXTURE = NULL;

typedef struct {
  SDL_FPoint pos;
  int score;
  bool is_alive;
} Food;

typedef struct {
  int score;
  Uint64 last_ticks;

  SDL_FPoint snake[NUM_BLOCKS];
  Food food[10];
  int snake_size;

  int x_dir;
  int y_dir;
  float speed;
  SDL_FPoint finger_down_pos;

  bool visible_grid;
  bool is_paused;
  bool next_step;
} GameState;

void grow_snake(GameState *game) {
  SDL_FPoint *tail = &game->snake[game->snake_size - 1];
  SDL_FPoint p = {0, 0};

  if (game->snake_size > 1) {
    SDL_FPoint *prev = &game->snake[game->snake_size - 2];
    float dx = (tail->x / BLOCK_SIZE) - (prev->x / BLOCK_SIZE);
    float dy = (tail->y / BLOCK_SIZE) - (prev->y / BLOCK_SIZE);

    p.x = tail->x + (BLOCK_SIZE * dx);
    p.y = tail->y + (BLOCK_SIZE * dy);
  } else {
    p.x = tail->x + (BLOCK_SIZE * (game->x_dir * -1));
    p.y = tail->y + (BLOCK_SIZE * (game->y_dir * -1));
  }

  game->snake[game->snake_size] = p;
  game->snake_size++;
}

void add_food(GameState *game) {
  SDL_FPoint p = {};

  bool overlaps = true;
  do {
    p.x = SDL_rand(COLS) * BLOCK_SIZE;
    p.y = SDL_rand(ROWS) * BLOCK_SIZE;

    // Check if overlaps with other live food
    bool overlaps_food = false;
    for (int i = 0; i < sizeof(game->food) / sizeof(Food); i++) {
      if (game->food[i].is_alive && game->food[i].pos.x == p.x &&
          game->food[i].pos.y == p.y) {
        overlaps_food = true;
        break;
      }
    }

    // Check if overlaps with snake body
    bool overlaps_body = false;
    for (int i = 0; i < game->snake_size; i++) {
      if (game->snake[i].x == p.x && game->snake[i].y == p.y) {
        overlaps_body = true;
        break;
      }
    }

    overlaps = overlaps_food || overlaps_body;
  } while (overlaps);

  for (int i = 0; i < sizeof(game->food) / sizeof(Food); i++) {
    if (!game->food[i].is_alive) {
      game->food[i].pos = p;
      game->food[i].is_alive = true;
      game->food[i].score = (SDL_rand(2) + 1) * 10;
      SDL_Log("Added food %f,%f", p.x, p.y);
      break;
    }
  }
}

void initialize_textures() {
  SDL_Surface *surface = SDL_LoadBMP("numbers.bmp");
  SDL_SetSurfaceColorKey(surface, true, 0x000000);
  NUMBERS_TEXTURE = SDL_CreateTextureFromSurface(renderer, surface);
}

void initialize_game(GameState *game) {
  game->x_dir = 1;
  game->y_dir = 0;
  game->score = 0;
  game->is_paused = false;
  game->next_step = false;
  game->speed = START_SPEED;
  game->visible_grid = false;

  game->snake[0].x = WIN_WIDTH / 2.0f;
  game->snake[0].y = WIN_HEIGHT / 2.0f;
  game->snake_size = 1;

  grow_snake(game);
  grow_snake(game);

  for (int i = 0; i < sizeof(game->food) / sizeof(Food); i++) {
    game->food[i].is_alive = false;
  }

  add_food(game);

  game->last_ticks = SDL_GetTicks();
}

void render_grid() {
  SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b, 100);
  for (int i = 0; i <= COLS; i++) {
    SDL_RenderLine(renderer, BLOCK_SIZE * i, 0, BLOCK_SIZE * i, WIN_HEIGHT);
  }

  for (int i = 0; i <= ROWS; i++) {
    SDL_RenderLine(renderer, 0, BLOCK_SIZE * i, WIN_WIDTH, BLOCK_SIZE * i);
  }
  SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                         FG_COLOR.a);
}

void render_score(int score, SDL_FRect *dst) {
  char str_score[12];
  sprintf(str_score, "%d", score);
  int score_len = (int)strlen(str_score);

  for (int i = score_len - 1; i >= 0; i--) {
    int digit = str_score[i] - 48;

    SDL_FRect src = {32 * digit, 0, 32, 64};
    dst->x -= dst->w + 4;

    if (!SDL_RenderTexture(renderer, NUMBERS_TEXTURE, &src, dst)) {
      SDL_Log("Failed to render texture: %s", SDL_GetError());
    }
  }
}

void update_snake(GameState *game) {
  SDL_FPoint *head = &game->snake[0];
  float prev_x = head->x;
  float prev_y = head->y;

  if (head->x == WIN_WIDTH - BLOCK_SIZE && game->x_dir == 1) {
    head->x = 0;
  } else if (head->x < BLOCK_SIZE && game->x_dir == -1) {
    head->x = WIN_WIDTH - BLOCK_SIZE;
  } else {
    head->x += BLOCK_SIZE * game->x_dir;
  }

  if (head->y == WIN_HEIGHT - BLOCK_SIZE && game->y_dir == 1) {
    head->y = 0;
  } else if (head->y < BLOCK_SIZE && game->y_dir == -1) {
    head->y = WIN_HEIGHT - BLOCK_SIZE;
  } else {
    head->y += BLOCK_SIZE * game->y_dir;
  }

  for (int i = 1; i < game->snake_size; i++) {
    float x = game->snake[i].x;
    float y = game->snake[i].y;

    game->snake[i].x = prev_x;
    game->snake[i].y = prev_y;

    prev_x = x;
    prev_y = y;
  }

  game->next_step = false;
}

void feed_snake(GameState *game) {
  // TODO: levels logic based on snake size
  SDL_FPoint *head = &game->snake[0];

  for (int i = 0; i < sizeof(game->food) / sizeof(game->food[0]); i++) {
    if (game->food[i].is_alive && head->x == game->food[i].pos.x &&
        head->y == game->food[i].pos.y) {
      SDL_Log("Food eaten - score: %d\n", game->score);
      game->food[i].is_alive = false;
      game->score += game->food[i].score;
      grow_snake(game);
      add_food(game);

      if (game->snake_size == 10) {
        game->speed = 0.15;
      }
    }
  }
}

bool check_snake_collision(GameState *game) {
  SDL_FPoint *head = &game->snake[0];
  for (int i = 1; i < game->snake_size; i++) {
    if (head->x == game->snake[i].x && head->y == game->snake[i].y) {
      return true;
    }
  }
  return false;
}

void move_up(GameState *game) {
  if (game->y_dir != -1 && game->x_dir != 0) {
    game->x_dir = 0;
    game->y_dir = -1;
  }
}

void move_down(GameState *game) {
  if (game->y_dir != 1 && game->x_dir != 0) {
    game->x_dir = 0;
    game->y_dir = 1;
  }
}

void move_left(GameState *game) {
  if (game->y_dir != 0 && game->x_dir != -1) {
    game->x_dir = -1;
    game->y_dir = 0;
  }
}

void move_right(GameState *game) {
  if (game->y_dir != 0 && game->x_dir != 1) {
    game->x_dir = 1;
    game->y_dir = 0;
  }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Launching Snake");

  GameState *game = SDL_calloc(1, sizeof(GameState));
  initialize_game(game);

#ifdef __EMSCRIPTEN__
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
#endif

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDL_CreateWindowAndRenderer("Snake", WIN_WIDTH, WIN_HEIGHT,
                                   SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Failed to initialize SDL window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderLogicalPresentation(renderer, WIN_WIDTH, WIN_HEIGHT,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  initialize_textures();

  *appstate = game;

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  GameState *game = (GameState *)appstate;
  Uint64 now = SDL_GetTicks();
  float delta = ((float)(now - game->last_ticks)) / 1000.0f;

  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                         BG_COLOR.a);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                         FG_COLOR.a);
  SDL_RenderRect(renderer, NULL);

  if (!game->is_paused && delta >= game->speed || game->next_step) {
    game->last_ticks = now;
    update_snake(game);
    feed_snake(game);

    if (check_snake_collision(game)) {
      initialize_game(game);
      SDL_Log("Snake crashed, final Score: %d\n", game->score);
      // TODO: "You died" screen with final score
      return SDL_APP_CONTINUE;
    }
  }

  // Render grid
  if (game->visible_grid) {
    render_grid();
  }

  // Render Food
  SDL_SetRenderDrawColor(renderer, FOOD_COLOR.r, FOOD_COLOR.g, FOOD_COLOR.b,
                         FOOD_COLOR.a);
  for (int i = 0; i < sizeof(game->food) / sizeof(game->food[0]); i++) {
    if (!game->food[i].is_alive) {
      continue;
    }

    SDL_FRect r = {
        game->food[i].pos.x,
        game->food[i].pos.y,
        BLOCK_SIZE,
        BLOCK_SIZE,
    };

    SDL_RenderRect(renderer, &r);
    SDL_RenderFillRect(renderer, &r);
  }

  // Render snake body
  for (int i = 1; i < game->snake_size; i++) {
    SDL_FRect r = {
        game->snake[i].x,
        game->snake[i].y,
        BLOCK_SIZE,
        BLOCK_SIZE,
    };
    SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                           FG_COLOR.a);

    SDL_RenderRect(renderer, &r);
    SDL_RenderFillRect(renderer, &r);
  }

  // Render snake head
  SDL_FRect r = {
      game->snake[0].x,
      game->snake[0].y,
      BLOCK_SIZE,
      BLOCK_SIZE,
  };
  SDL_SetRenderDrawColor(renderer, HEAD_COLOR.r, HEAD_COLOR.g, HEAD_COLOR.b,
                         HEAD_COLOR.a);
  SDL_RenderRect(renderer, &r);
  SDL_RenderFillRect(renderer, &r);

  // Render score
  SDL_FRect dst = {WIN_WIDTH - 8, 10, 16, 32};
  render_score(game->score, &dst);

  SDL_RenderPresent(renderer);
  SDL_Delay(1);
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  GameState *game = (GameState *)appstate;

  if (event->type == SDL_EVENT_KEY_DOWN) {
    if (event->key.scancode == SDL_SCANCODE_Q) {
      return SDL_APP_SUCCESS;
    } else if (event->key.scancode == SDL_SCANCODE_SPACE) {
      game->is_paused = !game->is_paused;
    } else if (event->key.scancode == SDL_SCANCODE_PERIOD) {
      game->next_step = !game->next_step;
    } else if (event->key.scancode == SDL_SCANCODE_G) {
      game->visible_grid = !game->visible_grid;
    } else if (event->key.scancode == SDL_SCANCODE_R) {
      SDL_Log("Restaring game...");
      initialize_game(game);
    } else if (event->key.scancode == SDL_SCANCODE_UP) {
      move_up(game);
    } else if (event->key.scancode == SDL_SCANCODE_DOWN) {
      move_down(game);
    } else if (event->key.scancode == SDL_SCANCODE_LEFT) {
      move_left(game);
    } else if (event->key.scancode == SDL_SCANCODE_RIGHT) {
      move_right(game);
    }
  } else if (event->type == SDL_EVENT_FINGER_DOWN) {
    SDL_TouchFingerEvent *e = (SDL_TouchFingerEvent *)event;
    game->finger_down_pos.x = e->x;
    game->finger_down_pos.y = e->y;
  } else if (event->type == SDL_EVENT_FINGER_UP) {
    SDL_TouchFingerEvent *e = (SDL_TouchFingerEvent *)event;
    float dx = e->x - game->finger_down_pos.x;
    float dy = e->y - game->finger_down_pos.y;

    if (SDL_fabsf(dx) > SDL_fabsf(dy)) {
      if (dx > 0) {
        move_right(game);
      } else if (dx < 0) {
        move_left(game);
      }
    } else {
      if (dy > 0) {
        move_down(game);
      } else if (dy < 0) {
        move_up(game);
      }
    }
  } else if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  SDL_free(appstate);
  SDL_Log("Bye!");
}
