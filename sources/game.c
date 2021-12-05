#include"game.h"

#include<stdlib.h>

#include"raylib.h"
#include"raymath.h"
#include"rlgl.h"

#include"tree.h"
#include"attacker.h"
#include"lsystem.h"

void StartGame(s_game* game){
    game->game_state = PLAYING;

    InitWindow(game->window_size.x, game->window_size.y, game->title);

    SetTargetFPS(144);
    RunGame(game); // Go further into it by starting the game loop
}

void RunGame(s_game* game) {
    // Setup post-processing stuff
    game->pp_shader = LoadShader(0, TextFormat(ASSETS_PATH"postprocessing.glsl", 330)); // Load bloom shader
    game->framebuffer_texture = LoadRenderTexture(game->window_size.x, game->window_size.y);

    int loc = GetShaderLocation(game->pp_shader, "pixel_w");
    //SetShaderValue(game->pp_shader, loc, 1, SHADER_UNIFORM_FLOAT);
    game->pp_shader.locs[0] = loc;
    // TODO(dave): Fix this crashes the game?
    // SetShaderValue(game->pp_shader, 0, (const void*)1, SHADER_UNIFORM_FLOAT);

    // Create the ground object
    game->ground.width = 3000;
    game->ground.height = game->window_size.y * 0.1f;
    game->ground.x = -game->ground.width/2;
    game->ground.y = game->window_size.y-game->ground.height;

    // Setup camera
    game->camera.zoom = 1.0f;

    // Do object-based tree generation
    CreateTree(&game->tree, (Vector2){0.0, -game->ground.height});

    // Create player
    float player_height = 80;
    float player_width = 40;
    game->player.rect = (Rectangle){100, game->window_size.y - game->ground.height - player_height, player_width, player_height};
    game->player.color = (Color){200, 100, 200, 255}; 
    game->player.horizontal_speed = 10000;
    game->player.mass = 10;
    game->player.air_resistance = 50;

    // Setup other
    game->second_counter = 0;

    while(!WindowShouldClose()){

        // Update input related stuff
        //InputGame(game);
        
        switch(game->game_state){
            case MENU:
                RunMenu(game);
                break;
            case PLAYING:
                RunPlaying(game);
                break;
            case PAUSED:
                RunPaused(game);
                break;
            default:
                printf("Invalid game state!!!!");
                CloseWindow();
                break;
        }
    }

    //TODO(david): Make sure all memeory is deallocated please and thank you!
    CloseWindow();
}

void EndGame(s_game* game) {

}

void DrawCoordinateAxis() {
    Vector2 origin = (Vector2){ 0.0f, 0.0f };
    float axisThickness = 8.0f;
    float axisScale = 500.0f;
    Color axisColor = RED;

    DrawLineEx(origin, Vector2Add(origin, (Vector2) { axisScale, 0.0f }), axisThickness, axisColor); // X-axis
    DrawLineEx(origin, Vector2Add(origin, (Vector2) { 0.0f, axisScale }), axisThickness, axisColor); // Y-axis
}

void InputGame(s_game* game) {
    Vector2 moveVector = { 0 };

    bool boosted = IsKeyDown(KEY_LEFT_SHIFT);
    
    if (IsKeyDown(KEY_D)) {
        moveVector.x--;
    }
    else if (IsKeyDown(KEY_A)) {
        moveVector.x++;
    }

    if (IsKeyDown(KEY_S)) {
        moveVector.y--;
    }
    else if (IsKeyDown(KEY_W)) {
        moveVector.y++;
    }

    float cameraSpeed = 140.f;
    if(boosted) cameraSpeed *= 3.f;

    // Scale with delta time
    moveVector = Vector2Scale(moveVector, cameraSpeed*GetFrameTime());

    // Move camera
    game->camera.offset = Vector2Add(game->camera.offset, moveVector);

    // Angle input thing
    if(IsKeyDown(KEY_UP)){
        game->fractal_tree_angle += 10.0f * GetFrameTime();
    }else if(IsKeyDown(KEY_DOWN)){
        game->fractal_tree_angle += -10.0f * GetFrameTime();
    }   

    if(IsKeyDown(KEY_Z)){
        game->camera.zoom += 1.f * GetFrameTime();
    } else if(IsKeyDown(KEY_X)){
        game->camera.zoom -= 1.f * GetFrameTime();
    }
}

/* RUNNERS */

void RunMenu(s_game* game){

}

void RunPaused(s_game* game){

}

void RunPlaying(s_game* game){
        // for(int i = 0; i < MAX_ATTACKERS; i++){
        //     UpdateAttacker(&game->tree, &game->attackers[i]);
        // }

        // Spawn attackers
        // if(GetTime() - game->second_counter >= 3){
        //     game->second_counter = GetTime();
        //     SpawnAttackers(game, 2);
        // }

        UpdatePlayer(&game->player, game);

        BeginTextureMode(game->framebuffer_texture); // Enable so we draw to the framebuffer texture!
        BeginMode2D(game->camera);

            ClearBackground(SKYBLUE);

            /* Draws the basis vectors from 0, 0 on x,y axis */
            //DrawCoordinateAxis();

            // Render ground
            //DrawRectanglePro(game->ground, (Vector2){0.0, 0.0}, 0, BROWN);
            DrawRectangleGradientV(
                game->ground.x, game->ground.y, game->ground.width, game->ground.height,
                GREEN, BROWN
            );

            // Renders the tree object
            RenderTree(&game->tree);

            // Render player
            RenderPlayer(&game->player);

            // Render attackers
            for(int i = 0; i < MAX_ATTACKERS; i++){
                RenderAttacker(&game->attackers[i]);
            }
            
        EndMode2D();
        EndTextureMode(); // End framebuffer texture

        BeginDrawing(); // Draw the actual post processed framebuffer

            ClearBackground(WHITE);
            BeginShaderMode(game->pp_shader);
                // Flip the y due to how the default opengl coordinates work (left-bottom)
                DrawTextureRec(game->framebuffer_texture.texture, (Rectangle){0, 0, (float)+game->framebuffer_texture.texture.width, (float)-game->framebuffer_texture.texture.height}, (Vector2){0.0, 0.0}, WHITE);
            EndShaderMode();

        EndDrawing();
}