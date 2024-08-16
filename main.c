#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <SDL.h>

// Number of program instructions an agent can have
#define evo_agent_program_size 16
// Height and width of grid
#define evo_grid_dimensions 128
#define evo_state_max_population 1000

const int evo_state_max_iterations = 5000;

const int window_width = 512;
const int window_height = 512;

typedef struct evo_agent;
typedef struct evo_state;

typedef enum {
	in_go_left,
	in_go_right,
	in_go_up,
	in_go_down,
	in_do_nothing,
	// End marker
	in_last
} instruction;

typedef struct {
	instruction program[evo_agent_program_size];
	// Where the agent is in its program
	int step;
	int x, y;
	// A dead agent will act like an empty cell
	bool dead;
	// evo_state pointer
	void* state;
} evo_agent;

typedef struct {
	evo_agent* agent;
	// If cell was updated in current frame
	bool updated;
} evo_cell;

typedef struct {
	evo_cell grid[evo_grid_dimensions][evo_grid_dimensions];
	evo_agent population[evo_state_max_population];
	int iterations;
	int generation;
} evo_state;

SDL_Window* window;
SDL_Renderer* renderer;

void evo_agent_initialize(evo_agent* agent, evo_state* state, int x, int y) {
	// Give the agent random program sequence
	for (int i = 0; i < evo_agent_program_size; i++) {
		agent->program[i] = rand() % in_last;
	}
	agent->dead = false;
	agent->step = 0;
	agent->x = x;
	agent->y = y;
	agent->state = state;
}

void evo_agent_move_to(evo_agent* agent, int new_x, int new_y) {
	if (new_x < 0 || new_x >= evo_grid_dimensions || new_y < 0 || new_y >= evo_grid_dimensions) {
		return;
	}
	evo_cell* old_cell = &((evo_state*)agent->state)->grid[agent->x][agent->y];
	evo_cell* new_cell = &((evo_state*)agent->state)->grid[new_x][new_y];
	if (new_cell->agent != NULL) {
		return;
	}
	old_cell->agent = NULL;
	old_cell->updated = true;
	new_cell->agent = agent;
	new_cell->updated = true;
	agent->x = new_x;
	agent->y = new_y;
}

void evo_agent_execute(evo_agent* agent) {
	instruction inst = agent->program[agent->step];

	switch (inst) {
		case in_go_left:
			evo_agent_move_to(agent, agent->x - 1, agent->y);
			break;
		case in_go_right:
			evo_agent_move_to(agent, agent->x + 1, agent->y);
			break;
		case in_go_up:
			evo_agent_move_to(agent, agent->x, agent->y - 1);
			break;
		case in_go_down:
			evo_agent_move_to(agent, agent->x, agent->y + 1);
			break;
	}
	agent->step++;

	if (agent->step >= evo_agent_program_size) {
		agent->step = 0;
	}
}

void evo_state_initialize(evo_state* state) {
	evo_cell* cell;
	for (int y = 0; y < evo_grid_dimensions; y++) {
		cell = &state->grid[0][y];
		cell->agent = malloc(sizeof(evo_agent));
		cell->updated = false;

		evo_agent_initialize(cell->agent, state, 0, y);

		for (int x = 1; x < evo_grid_dimensions; x++) {
			cell = &state->grid[x][y];
			cell->agent = NULL;
			cell->updated = false;
		}
	}
	state->iterations = 0;
	state->generation = 0;
}

void evo_state_evolve(evo_state* state) {
	
}

void evo_state_update(evo_state* state) {
	evo_cell* cell;
	for (int y = 0; y < evo_grid_dimensions; y++) {
		for (int x = 0; x < evo_grid_dimensions; x++) {
			cell = &state->grid[x][y];
			if (cell->agent == NULL || cell->updated == true) {
				continue;
			}
			evo_agent_execute(cell->agent);
		}
	}
	for (int y = 0; y < evo_grid_dimensions; y++) {
		for (int x = 0; x < evo_grid_dimensions; x++) {
			cell = &state->grid[x][y];
			cell->updated = false;
		}
	}
	state->iterations++;

	if (state->iterations >= evo_state_max_iterations) {
		state->iterations = 0;
		state->generation++;
		evo_state_evolve(state);
	}
}

void evo_state_draw(evo_state* state) {
	int cell_width = window_width / evo_grid_dimensions;
	int cell_height = window_height / evo_grid_dimensions;

	evo_cell* cell;

	for (int x = 0; x < evo_grid_dimensions; x++) {
		for (int y = 0; y < evo_grid_dimensions; y++) {
			cell = &state->grid[x][y];

			if (cell->agent == NULL) {
				continue;
			}

			int start_x = x * cell_width;
			int start_y = y * cell_height;

			SDL_Rect rect;
			rect.x = start_x;
			rect.y = start_y;
			rect.w = cell_width;
			rect.h = cell_height;

			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
	}
}

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	evo_state evolution;

	evo_state_initialize(&evolution);

	SDL_Event event;
	bool user_quit = false;

	while (user_quit == false) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				user_quit = true;
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		evo_state_update(&evolution);
		evo_state_draw(&evolution);
		SDL_RenderPresent(renderer);
		printf("%d\r", evolution.iterations);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}