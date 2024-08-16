#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL.h>

// Number of program instructions an agent can have
#define evo_agent_program_size 16
// Height and width of grid
#define evo_grid_dimensions 128
#define evo_state_population_size 100

const int evo_state_max_iterations = 5000;

const int window_width = 512;
const int window_height = 512;

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
	evo_agent current_generation[evo_state_population_size];
	evo_agent next_generation[evo_state_population_size];
	float fitnesses[evo_state_population_size];
	int iterations;
	int generation;
} evo_state;

SDL_Window* window;
SDL_Renderer* renderer;

void evo_agent_initialize(evo_agent* agent, evo_state* state, int x, int y) {
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

void evo_agent_randomize_program(evo_agent* agent) {
	// Give the agent random program sequence
	for (int i = 0; i < evo_agent_program_size; i++) {
		agent->program[i] = rand() % in_last;
	}
}

void evo_state_setup(evo_state* state) {
	evo_cell* cell;
	evo_agent* agent;

	int x = 0;
	int y = 0;

	for (int i = 0; i < evo_grid_dimensions * evo_grid_dimensions; i++) {
		cell = &state->grid;
		cell += i;
		cell->agent = NULL;
		cell->updated = false;
	}

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		if (y >= evo_grid_dimensions) {
			x++;
			y = 0;
		}
		if (x >= evo_grid_dimensions) {
			printf("Too many agents!\n");
			break;
		}
		evo_agent_initialize(agent, state, x, y);
		y++;
	}
}

void evo_state_initialize(evo_state* state) {
	evo_cell* cell;
	evo_agent* agent;

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		evo_agent_randomize_program(agent);
	}

	state->iterations = 0;
	state->generation = 0;

	evo_state_setup(state);
}

float evo_agent_get_fitness(evo_agent* agent) {
	float center = (float)evo_grid_dimensions * 0.5;
	float dx = agent->x - center;
	//float dy = ((float)agent->y + 0.5) - center;
	//float distance = sqrtf(dx * dx);
	//printf("%f\n", dx);
	return 1 - fabs(dx / center);
}

evo_agent* evo_state_select_agent(evo_state* state) {
	float total_fitness = 0;
	for (int i = 0; i < evo_state_population_size; i++) {
		state->fitnesses[i] = evo_agent_get_fitness(&state->current_generation[i]);
		total_fitness += state->fitnesses[i];
	}
	float random_choice = ((float)rand() / (float)RAND_MAX) * total_fitness;
	float total = 0;
	for (int i = 0; i < evo_state_population_size; i++) {
		total += state->fitnesses[i];
		if (total >= random_choice) {
			return &state->current_generation[i];
		}
	}
	return NULL;
}

void evo_agent_breed(evo_agent* child, evo_agent* parent_1, evo_agent* parent_2) {
	memcpy(child->program, parent_1->program, sizeof(instruction) * evo_agent_program_size);
}

void evo_state_evolve(evo_state* state) {
	for (int i = 0; i < evo_state_population_size; i++) {
		// It's possible for a parent to breed with itself
		evo_agent* parent_a = evo_state_select_agent(state);
		evo_agent* parent_b = evo_state_select_agent(state);

		evo_agent_breed(&state->next_generation[i], parent_a, parent_b);
	}
	memcpy(state->current_generation, state->next_generation, sizeof(evo_agent) * evo_state_population_size);
	evo_state_setup(state);
}

void evo_state_update(evo_state* state) {
	evo_cell* cell;
	evo_agent* agent;

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		evo_agent_execute(agent);
	}

	for (int i = 0; i < evo_grid_dimensions * evo_grid_dimensions; i++) {
		cell = &state->grid;
		cell += i;
		cell->updated = false;
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

	evo_agent* agent;

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];

		int start_x = agent->x * cell_width;
		int start_y = agent->y * cell_height;

		SDL_Rect rect;
		rect.x = start_x;
		rect.y = start_y;
		rect.w = cell_width;
		rect.h = cell_height;

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderFillRect(renderer, &rect);
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