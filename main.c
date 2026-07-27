#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

  bool is_paused;
  bool is_debug;
  bool next_step;
} GameState;

void grow_snake(GameState *game) {
  SDL_FPoint tail = game->snake[game->snake_size - 1];
  SDL_FPoint extra;

  if (game->snake_size > 1) {
    SDL_FPoint prev = game->snake[game->snake_size - 2];
    float dx = (prev.x / BLOCK_SIZE) - (tail.x / BLOCK_SIZE);
    float dy = (prev.y / BLOCK_SIZE) - (tail.y / BLOCK_SIZE);

    extra.x = tail.x + BLOCK_SIZE * dx;
    extra.y = tail.y + BLOCK_SIZE * dy;
  } else {
    extra.x = tail.x - BLOCK_SIZE;
    extra.y = tail.y;
  }

  game->snake[game->snake_size] = extra;
  game->snake_size = game->snake_size + 1;
}

void add_food(GameState *game) {
  for (int i = 0; i < sizeof(game->food) / sizeof(game->food[0]); i++) {
    if (!game->food[i].is_alive) {
      bool overlaps = false;

      SDL_FPoint p = {
          SDL_rand(COLS) * BLOCK_SIZE,
          SDL_rand(ROWS) * BLOCK_SIZE,
      };
      for (int j = 0; j < game->snake_size; j++) {
        if (game->snake[j].x == p.x && game->snake[j].y == p.y) {
          overlaps = true;
          continue;
        }
      }

      game->food[i].pos = p;
      game->food[i].is_alive = true;
      game->food[i].score = SDL_rand(40);
      SDL_Log("added food %f,%f", p.x, p.y);
      break;
    }
  }

  SDL_Log("add food");
}

void initialize_game(GameState *game) {
  SDL_FPoint head = {WIN_WIDTH / 2.0f, WIN_HEIGHT / 2.0f};
  game->snake[0] = head;
  game->snake_size = 1;
  game->score = 0;

  grow_snake(game);
  grow_snake(game);
  grow_snake(game);

  for (int i = 0; i < sizeof(game->food) / sizeof(game->food[0]); i++) {
    game->food[i].is_alive = false;
  }

  add_food(game);

  game->x_dir = 1;
  game->y_dir = 0;
  game->is_paused = false;
  game->next_step = false;
  game->is_debug = true;
  game->speed = START_SPEED;
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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_Log("Launching Cnake");

  GameState *game = SDL_calloc(1, sizeof(GameState));
  initialize_game(game);

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

  // render_grid();

  // Update snake state
  if (!game->is_paused && delta >= game->speed ||
      (game->is_debug && game->next_step)) {
    SDL_FPoint *head = &game->snake[0];
    // SDL_Log("head %f,%f (%d)", head->x, head->y, game->snake_size);

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

    // Check for eaten food
    for (int i = 0; i < sizeof(game->food) / sizeof(game->food[0]); i++) {
      if (game->food[i].is_alive && head->x == game->food[i].pos.x &&
          head->y == game->food[i].pos.y) {
        game->food[i].is_alive = false;
        game->score += game->food[i].score;
        SDL_Log("food eaten - score: %d\n", game->score);
        grow_snake(game);
        add_food(game);

        if (game->snake_size == 10) {
          game->speed = 0.15;
        }
      }
    }

    game->last_ticks = now;
    game->next_step = false;
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

  // Render Snake
  for (int i = 0; i < game->snake_size; i++) {
    SDL_FRect r = {
        game->snake[i].x,
        game->snake[i].y,
        BLOCK_SIZE,
        BLOCK_SIZE,
    };

    if (i == 0) {
      SDL_SetRenderDrawColor(renderer, HEAD_COLOR.r, HEAD_COLOR.g, HEAD_COLOR.b,
                             HEAD_COLOR.a);
    } else {
      SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                             FG_COLOR.a);
    }

    SDL_RenderRect(renderer, &r);
    SDL_RenderFillRect(renderer, &r);
  }

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
    } else if (event->key.scancode == SDL_SCANCODE_R) {
      initialize_game(game);
    } else if (event->key.scancode == SDL_SCANCODE_UP ||
               event->key.scancode == SDL_SCANCODE_K) {
      game->x_dir = 0;
      game->y_dir = -1;
    } else if (event->key.scancode == SDL_SCANCODE_DOWN ||
               event->key.scancode == SDL_SCANCODE_J) {
      game->x_dir = 0;
      game->y_dir = 1;
    } else if (event->key.scancode == SDL_SCANCODE_LEFT ||
               event->key.scancode == SDL_SCANCODE_H) {
      game->x_dir = -1;
      game->y_dir = 0;
    } else if (event->key.scancode == SDL_SCANCODE_RIGHT ||
               event->key.scancode == SDL_SCANCODE_L) {
      game->x_dir = 1;
      game->y_dir = 0;
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
