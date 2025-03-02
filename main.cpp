/**
* Author: Kristie Lee
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <random>

enum AppStatus { RUNNING, TERMINATED };

// --------------------- Constants --------------------- //

constexpr float WINDOW_SIZE_MULT = 2.0f;

constexpr int WINDOW_WIDTH = 640 * WINDOW_SIZE_MULT,
WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED = 0.0f,
BG_GREEN = 0.0f,
BG_BLUE = 0.0f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char PADDLE1_SPRITE_FILEPATH[] = "assets/fork8bit.png";
constexpr char PADDLE2_SPRITE_FILEPATH[] = "assets/knife8bit.png";
constexpr char BALL_SPRITE_FILEPATH[] = "assets/donut8bit.png";
constexpr char WINNER1_SPRITE_FILEPATH[] = "assets/player1wins.png";
constexpr char WINNER2_SPRITE_FILEPATH[] = "assets/player2wins.png";

constexpr float PADDLE_SPEED = 3.0f; // speed of paddle
constexpr float BALL_SPEED = 1.8f; // speed of ball

constexpr float MINIMUM_COLLISION_DISTANCE = 1.0f;
constexpr glm::vec3
INIT_SCALE_BALL = glm::vec3(0.8f, 0.8f, 0.0f),
INIT_POS_BALL = glm::vec3(-2.0f, 0.0f, 0.0f),
INIT_SCALE_PADDLE1 = glm::vec3(0.4f, 1.8f, 0.0f),
INIT_POS_PADDLE1 = glm::vec3(-4.75f, 0.0f, 0.0f),
INIT_SCALE_PADDLE2 = glm::vec3(0.35f, 1.8f, 0.0f),
INIT_POS_PADDLE2 = glm::vec3(4.75f, 0.0f, 0.0f);

// Walls (bounce limits)
constexpr float CEILING_Y = 3.4f;
constexpr float FLOOR_Y = -3.4f;

// ------------- Random Direction Vector Generator ------------ //
static std::random_device rd; // Call once globally for performance
static std::mt19937 gen(rd()); // Call once globally for performance

glm::vec3 generate_random_direction() {
        std::uniform_real_distribution<float> x_dist(0.6f, 1.0f);
    std::uniform_int_distribution<int> y_dist(0, 1);

    float random_x = x_dist(gen);
    float random_y = (y_dist(gen) == 0) ? 1.0f : -1.0f;

    return glm::normalize(glm::vec3(random_x, random_y, 0.0f));
}

// ------------- Ball Struct ------------ //
struct Ball {
    glm::mat4 matrix = glm::mat4(1.0f);
    glm::vec3 position = INIT_POS_BALL;
    glm::vec3 movement = generate_random_direction();
    glm::vec3 size = glm::vec3(1.0f, 1.0f, 0.0f);
};

// --------------------- Global Variables --------------------- //
SDL_Window* g_display_window;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix, g_projection_matrix,
g_PADDLE1_matrix, g_PADDLE2_matrix,
g_WINNER1_matrix, g_WINNER2_matrix;

float g_previous_ticks = 0.0f;

GLuint g_PADDLE1_texture_id;
GLuint g_PADDLE2_texture_id;

GLuint g_BALL_texture_id;

GLuint g_WINNER1_texture_id;
GLuint g_WINNER2_texture_id;

bool g_player1_wins = false;
bool g_player2_wins = false;
bool g_single_player_mode = false;
int g_active_balls = 1;

glm::vec3 g_PADDLE1_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_PADDLE1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_PADDLE2_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_PADDLE2_movement = glm::vec3(0.0f, 0.0f, 0.0f);

Ball BALL1;
Ball BALL2;
Ball BALL3;

void initialise();
void process_input();
void update();
void render();
void shutdown();

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
        SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("User-Input and Collisions Exercise",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);


    if (g_display_window == nullptr) shutdown();

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_PADDLE1_matrix = glm::mat4(1.0f);
    g_PADDLE2_matrix = glm::mat4(1.0f);

    BALL1.matrix = glm::mat4(1.0f);
    BALL2.matrix = glm::mat4(1.0f);
    BALL3.matrix = glm::mat4(1.0f);

    g_WINNER1_matrix = glm::mat4(1.0f);
    g_WINNER2_matrix = glm::mat4(1.0f);

    g_WINNER1_matrix = glm::scale(g_WINNER1_matrix, glm::vec3(5.5f));
    g_WINNER2_matrix = glm::scale(g_WINNER2_matrix, glm::vec3(5.5f));

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_PADDLE1_texture_id = load_texture(PADDLE1_SPRITE_FILEPATH);
    g_PADDLE2_texture_id = load_texture(PADDLE2_SPRITE_FILEPATH);

    g_BALL_texture_id = load_texture(BALL_SPRITE_FILEPATH);

    g_WINNER1_texture_id = load_texture(WINNER1_SPRITE_FILEPATH);
    g_WINNER2_texture_id = load_texture(WINNER2_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    }

void process_input()
{
    g_PADDLE1_movement.y = 0.0f;
    g_PADDLE2_movement.y = 0.0f;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_q:
                g_app_status = TERMINATED;
                break;

            case SDLK_t:
                g_single_player_mode = !g_single_player_mode;
                break;

            case SDLK_1:
                if (!g_player1_wins && !g_player2_wins) g_active_balls = 1;
                break;

            case SDLK_2:
                if (!g_player1_wins && !g_player2_wins) g_active_balls = 2;
                break;

            case SDLK_3:
                if (!g_player1_wins && !g_player2_wins) g_active_balls = 3;
                break;

            default: break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // Right paddle (Player controlled always)
    if (key_state[SDL_SCANCODE_UP] && g_PADDLE2_position.y < CEILING_Y) {
        g_PADDLE2_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN] && g_PADDLE2_position.y > FLOOR_Y) {
        g_PADDLE2_movement.y = -1.0f;
    }

    // Left paddle (AI controlled by default)
    if (!g_single_player_mode) {
        if (key_state[SDL_SCANCODE_W] && g_PADDLE1_position.y < CEILING_Y) {
            g_PADDLE1_movement.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_S] && g_PADDLE1_position.y > FLOOR_Y) {
            g_PADDLE1_movement.y = -1.0f;
        }
    }

    if (glm::length(BALL1.movement) > 1.0f)
    {
        BALL1.movement = glm::normalize(BALL1.movement);
    }
    if (glm::length(BALL2.movement) > 1.0f)
    {
        BALL2.movement = glm::normalize(BALL2.movement);
    }
    if (glm::length(BALL3.movement) > 1.0f)
    {
        BALL3.movement = glm::normalize(BALL3.movement);
    }
}

bool collide_paddle(const Ball& ball) {
    float paddle1_x_distance = fabs(ball.position.x - INIT_POS_PADDLE1.x) - ((INIT_SCALE_BALL.x + INIT_SCALE_PADDLE1.x) / 2.0f);
    float paddle1_y_distance = fabs(ball.position.y - g_PADDLE1_position.y) - ((INIT_SCALE_BALL.y + INIT_SCALE_PADDLE1.y) / 2.0f);

    float paddle2_x_distance = fabs(ball.position.x - INIT_POS_PADDLE2.x) - ((INIT_SCALE_BALL.x + INIT_SCALE_PADDLE2.x) / 2.0f);
    float paddle2_y_distance = fabs(ball.position.y - g_PADDLE2_position.y) - ((INIT_SCALE_BALL.y + INIT_SCALE_PADDLE2.y) / 2.0f);

    // --- BALL COLLISION WITH PADDLES --- //
    return (paddle1_x_distance < 0 && paddle1_y_distance < 0
        || paddle2_x_distance < 0 && paddle2_y_distance < 0);
}

void BALL_update(Ball& ball, float delta_time) {
    // --- BALL TRANSLATION --- //
    ball.position += ball.movement * BALL_SPEED * delta_time;
    ball.matrix = glm::mat4(1.0f);
    ball.matrix = glm::translate(ball.matrix, ball.position);

    // --- BALL SCALING --- //
    ball.matrix = glm::scale(ball.matrix, INIT_SCALE_BALL);
    ball.matrix = glm::scale(ball.matrix, ball.size);

    // --- BALL COLLISION WITH TOP/BOTTOM WALLS --- //
    if (ball.position.y >= CEILING_Y ||
        ball.position.y < FLOOR_Y) {
        ball.movement.y *= -1.0f;
    }
    // --- BALL COLLISION WITH PADDLES --- //
    if (collide_paddle(ball)) {
        ball.movement.x *= -1.0f;
    }
    // --- TERMINATE GAME IF BALL1 HITS WALL --- //
    if (ball.position.x < -4.7f) {
        g_player2_wins = true;
    }
    else if (ball.position.x > 4.7f) {
        g_player1_wins = true;
    }
}

void update()
{
    // --- DELTA TIME CALCULATIONS --- //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    if (!g_player1_wins && !g_player2_wins) {
        // --- SINGLE PLAYER MODE: AUTO PADDLE TRANSLATION --- //
        if (g_single_player_mode) {
            float ball_y = BALL1.position.y;
            float paddle1_y = g_PADDLE1_position.y;

            if (ball_y > paddle1_y + 0.2f) {
                g_PADDLE1_movement.y = 1.0f;
            }
            else if (ball_y < paddle1_y - 0.2f) {
                g_PADDLE1_movement.y = -1.0f;
            }
            else {
                g_PADDLE1_movement.y = 0.0f;
            }
        }
        // --- UPDATE BALL(S) ---
        BALL_update(BALL1, delta_time);
        if (g_active_balls >= 2) BALL_update(BALL2, delta_time);
        if (g_active_balls >= 3) BALL_update(BALL3, delta_time);

        // --- PADDLE TRANSLATION --- //
        g_PADDLE1_position += g_PADDLE1_movement * PADDLE_SPEED * delta_time;
        g_PADDLE2_position += g_PADDLE2_movement * PADDLE_SPEED * delta_time;

        g_PADDLE1_matrix = glm::mat4(1.0f);
        g_PADDLE1_matrix = glm::translate(g_PADDLE1_matrix, INIT_POS_PADDLE1);
        g_PADDLE1_matrix = glm::translate(g_PADDLE1_matrix, g_PADDLE1_position);

        g_PADDLE2_matrix = glm::mat4(1.0f);
        g_PADDLE2_matrix = glm::translate(g_PADDLE2_matrix, INIT_POS_PADDLE2);
        g_PADDLE2_matrix = glm::translate(g_PADDLE2_matrix, g_PADDLE2_position);

        // --- PADDLE SCALING --- //
        g_PADDLE1_matrix = glm::scale(g_PADDLE1_matrix, INIT_SCALE_PADDLE1);
        g_PADDLE2_matrix = glm::scale(g_PADDLE2_matrix, INIT_SCALE_PADDLE2);
    }
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(BALL1.matrix, g_BALL_texture_id);
    if (g_active_balls >= 2) {
        draw_object(BALL2.matrix, g_BALL_texture_id);
    }
    if (g_active_balls >= 3) {
        draw_object(BALL3.matrix, g_BALL_texture_id);
    }
    draw_object(g_PADDLE1_matrix, g_PADDLE1_texture_id);
    draw_object(g_PADDLE2_matrix, g_PADDLE2_texture_id);

    if (g_player1_wins) {
        draw_object(g_WINNER1_matrix, g_WINNER1_texture_id);
    }
    else if (g_player2_wins) {
        draw_object(g_WINNER2_matrix, g_WINNER2_texture_id);

    }

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

