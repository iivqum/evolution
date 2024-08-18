#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL.h>

// Number of program instructions an agent can have
#define evo_agent_program_size 128
// Height and width of grid
#define evo_grid_dimensions 256
#define evo_state_population_size 10000

const int evo_state_max_iterations = 300;

const float evo_agent_mutation_chance = 0.1;
const float evo_agent_selection_bias = 0.2;

const int window_width = 512;
const int window_height = 512;

typedef enum {
	in_go_left,
	in_go_right,
	in_go_up,
	in_go_down,
	in_top_left,
	in_top_right,
	in_bottom_left,
	in_bottom_right,
	in_do_nothing,
	in_go_to_start,
	in_next,
	in_previous,
	in_stop,
	in_goto,
	// End marker
	in_last
} instruction;

typedef struct {
	instruction program[evo_agent_program_size];
	// Where the agent is in its program
	int step;
	int x, y;
	bool dead, updated;
	// evo_state pointer
	void* state;
} evo_agent;

typedef struct {
	evo_agent* agent;
	// If cell was updated in current frame
	bool updated;
} evo_cell;

typedef struct {
	//evo_cell grid[evo_grid_dimensions][evo_grid_dimensions];
	evo_agent* current_generation;
	evo_agent* next_generation;
	int* best_agents;
	float* fitnesses;
	// Total fitness of the best agents
	float total_fitness;
	int iterations;
	int generation;
} evo_state;

SDL_Window* window;
SDL_Renderer* renderer;

void evo_agent_initialize(evo_agent* agent, evo_state* state, int x, int y) {
	agent->dead = false;
	agent->updated = false;
	agent->step = 0;
	agent->x = x;
	agent->y = y;
	agent->state = state;
}

void evo_agent_move_to(evo_agent* agent, int new_x, int new_y) {
	if (new_x > 0 && new_x < evo_grid_dimensions) {
		agent->x = new_x;
	}
	if (new_y > 0 && new_y < evo_grid_dimensions) {
		agent->y = new_y;
	}
	/*
	evo_cell* old_cell = &((evo_state*)agent->state)->grid[agent->x][agent->y];
	evo_cell* new_cell = &((evo_state*)agent->state)->grid[new_x][new_y];
	if (new_cell->agent != NULL) {
		return;
	}
	old_cell->agent = NULL;
	old_cell->updated = true;
	new_cell->agent = agent;
	new_cell->updated = true;
	*/
}

instruction evo_agent_read_instruction(evo_agent* agent) {
	instruction inst = agent->program[agent->step];

	agent->step++;

	if (agent->step >= evo_agent_program_size) {
		agent->step = 0;
	}

	return inst;
}

void evo_agent_execute(evo_agent* agent) {
	instruction inst = evo_agent_read_instruction(agent);

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
		case in_top_left:
			evo_agent_move_to(agent, agent->x - 1, agent->y - 1);
			break;
		case in_top_right:
			evo_agent_move_to(agent, agent->x + 1, agent->y - 1);
			break;
		case in_bottom_left:
			evo_agent_move_to(agent, agent->x - 1, agent->y + 1);
			break;
		case in_bottom_right:
			evo_agent_move_to(agent, agent->x + 1, agent->y + 1);
			break;
		case in_go_to_start:
			agent->step = 0;
			break;
		case in_next:
			agent->step++;
			break;
		case in_previous:
			agent->step--;
			break;
		case in_stop:
			agent->dead = true;
			break;
		case in_goto:
			break; 
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

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		/*
		if (y >= evo_grid_dimensions) {
			//x++;
			y = 0;
		}
		if (x >= evo_grid_dimensions) {
			printf("Too many agents!\n");
			break;
		}
		*/
		evo_agent_initialize(agent, state, 0, 0);
		//y++;
	}
}

void evo_state_cleanup(evo_state* state) {
	free(state->current_generation);
	free(state->next_generation);
	free(state->fitnesses);
	free(state->best_agents);
}

void evo_state_initialize(evo_state* state) {
	state->current_generation = malloc(sizeof(evo_agent) * evo_state_population_size);
	state->next_generation = malloc(sizeof(evo_agent) * evo_state_population_size);
	state->fitnesses = malloc(sizeof(float) * evo_state_population_size);
	state->best_agents = malloc(sizeof(int) * evo_state_population_size * evo_agent_selection_bias);

	for (int i = 0; i < evo_state_population_size; i++) {
		evo_agent_randomize_program(&state->current_generation[i]);
	}

	state->iterations = 0;
	state->generation = 0;

	evo_state_setup(state);
}

float evo_agent_get_fitness(evo_agent* agent) {
	float center = (float)evo_grid_dimensions * 0.5;
	float dx = ((float)agent->x + 0.5) - center;
	float dy = ((float)agent->y + 0.5) - center;
	//float distance = sqrtf(dx * dx);
	float fitness = (fabs(dx) + fabs(dy)) / (float)evo_grid_dimensions;

	float dist = sqrt(dx * dx + dy * dy);

	return 1 - (dist / 190);
}

evo_agent* evo_state_select_agent(evo_state* state) {
	float random_choice = (float)(((double)rand() / (double)RAND_MAX)) * state->total_fitness;
	int amount = evo_state_population_size * evo_agent_selection_bias;
	float total = 0;

	for (int i = 0; i < amount; i++) {
		int index = state->best_agents[i];
		total += state->fitnesses[index];
		if (total >= random_choice) {
			return &state->current_generation[index];
		}
	}
	printf("Invalid agent!\nTotal fitness = %f", state->total_fitness);
	return NULL;
}

void shift_array_int(int* arr, int start, int len) {
	int last_index = len - 1;
	for (int i = last_index - 1; i >= start; i--) {
		arr[last_index] = arr[i];
		last_index = i;
	}
}

void evo_state_compute_fitness(evo_state *state) {
	// Calculate fitness for each agent
	for (int i = 0; i < evo_state_population_size; i++) {
		state->fitnesses[i] = evo_agent_get_fitness(&state->current_generation[i]);
	}
	// Select top agents 
	int amount = evo_state_population_size * evo_agent_selection_bias;

	memset(state->best_agents, -1, sizeof(int) * amount);

	for (int i = 0; i < evo_state_population_size; i++) {
		for (int j = 0; j < amount; j++) {
			int index = state->best_agents[j];
			if (index == -1 || state->fitnesses[i] > state->fitnesses[index]) {
				shift_array_int(state->best_agents, j, amount);
				state->best_agents[j] = i;
				break;
			}
		}
	}
	// Calculate total fitness of the best agents
	float total_fitness = 0;
	for (int i = 0; i < amount; i++) {
		int index = state->best_agents[i];
		total_fitness += state->fitnesses[index];
	}
	state->total_fitness = total_fitness;
}

void evo_agent_breed(evo_agent* child, evo_agent* parent_1, evo_agent* parent_2) {
	int split = rand() % evo_agent_program_size;

	memcpy(child->program, parent_1->program, sizeof(instruction) * (split + 1));
	memcpy(child->program + split + 1, parent_2->program, sizeof(instruction) * (evo_agent_program_size - split + 1));

	int mutation_chance = rand() % (int)(1 / evo_agent_mutation_chance);

	if (mutation_chance == 0) {
		int mutate_index = rand() % evo_agent_program_size;
		child->program[mutate_index] = rand() % in_last;
	}
}

void evo_state_evolve(evo_state* state) {
	evo_state_compute_fitness(state);

	for (int i = 0; i < evo_state_population_size; i++) {
		// It's possible for a parent to breed with itself
		evo_agent* parent_a = evo_state_select_agent(state);
		evo_agent* parent_b = evo_state_select_agent(state);

		evo_agent_breed(&state->next_generation[i], parent_a, parent_b);
	}
	memcpy(state->current_generation, state->next_generation, sizeof(evo_agent) * evo_state_population_size);
	evo_state_setup(state);

	float total_fitness = 0;
	for (int i = 0; i < evo_state_population_size; i++) {
		total_fitness += state->fitnesses[i];
	}

	printf("Total fitness = %f, generation = %d\r", total_fitness, state->generation);
}

void evo_state_update(evo_state* state) {
	evo_agent* agent;

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		if (agent->updated == true || agent->dead == true) {
			continue;
		}
		evo_agent_execute(agent);
		agent->updated = true;
	}

	for (int i = 0; i < evo_state_population_size; i++) {
		agent = &state->current_generation[i];
		agent->updated = false;
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

	srand(time(NULL));

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
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	evo_state_cleanup(&evolution);

	return 0;
}