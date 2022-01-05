#include"tree.h"

#include"raylib.h"
#include"rlgl.h"
#include"raymath.h"

#include"utility.h"
#include"vec.h"
#include"cvector.h"

#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

#define MAX_RECURSION 4

#define DECAY_FACTOR .1f

Vector2 rotate_point(float cx, float cy, float angle, Vector2 p){
    return (Vector2){cos(angle) * (p.x - cx) - sin(angle) * (p.y - cy) + cx,
        sin(angle) * (p.x - cx) + cos(angle) * (p.y - cy) + cy};
}

char* tree_status_to_string(s_tree* tree){
    if(tree->status == HEALTHY)
        return "HEALTHY";
    else if(tree->status == THIRSTY)
        return "THIRSTY";
    else if(tree->status == DYING)
        return "DYING";
    return "UNKOWN STATUS";
}

s_branch SpawnBranch(s_branch* from, int direction){
    Vector2 start = from->end;
    float x = 0.9f;
    float length = from->length*x;
    Vector2 end = (Vector2){
        from->end.x, 
        from->end.y-length
    };

    // Rotate end around start
    float angle = (30 + GetRandomValue(0, 40)) * DEG2RAD;
    end = rotate_point(start.x, start.y, angle*direction, end);

    // Generate some dynamic data for the branch
    s_dynamic_branch dynamic_branch;
    dynamic_branch.wind_amplitude = 10.0f;
    dynamic_branch.wind_frequency = GetRandomFloatValue01()*3.0f;
    dynamic_branch.wind_offset = GetRandomValue(0, 100);

    // Create the object
    s_branch new_branch;
    new_branch.start = start;
    new_branch.end = end;
    new_branch.length = length;
    new_branch.done = false;
    new_branch.dynamics = dynamic_branch;
    new_branch.child_a = NULL;
    new_branch.child_b = NULL;

    return new_branch;
}

void CreateTree(s_tree* tree, Vector2 origin) {
    tree->base_length = 90.0f;
    tree->base_thickness = 15.0f;
    tree->water_counter = 100;
    tree->status = HEALTHY;
    const int iteration_levels = 2;

    // Create "dynamic" arrays
    tree->dropped_branches = vector_create();
    tree->branches = vector_create();

    // Setup first branch
    s_branch* b = vector_add_asg(&tree->branches);
    b->done = false;
    b->start = 
        Vector2Add(origin, (Vector2){GetScreenWidth()/2, GetScreenHeight()});
    b->length = tree->base_length;
    b->end = 
        Vector2Add(origin, (Vector2){GetScreenWidth()/2, GetScreenHeight()-tree->base_length});
    b = NULL;

    for(int i = 0; i < iteration_levels; i++){
        int frozen_branch_count = vector_size(tree->branches);
        for(int j = 0; j < frozen_branch_count; j++){
            if(!tree->branches[j].done){
                tree->branches[j].done = true; // Mark this branch as done
                // Spawn 2 branches and insert to the end of branch list
                s_branch* b1 = vector_add_asg(&tree->branches);
                *b1 = SpawnBranch(&tree->branches[j], 1);
                tree->branches[j].child_a = b1;
                b1 = NULL;
                s_branch* b2 = vector_add_asg(&tree->branches);
                *b2 = SpawnBranch(&tree->branches[j], -1);
                tree->branches[j].child_b = b2;
                b2 = NULL;
            }
        }
    }

    // Load sound
    tree->hurt_sound = LoadSound(ASSETS_PATH"tree_hurt.wav");

    // Do pass and find leaves
    // for(int i = 0; i < tree->branch_count; i++){
    //     if(!tree->branches[i].done){
    //         // Found leaf!
    //         tree->leaves[tree->leaf_count].position = tree->branches[i].end;
    //         tree->leaves[tree->leaf_count].hp = 3; 
    //         tree->leaves[tree->leaf_count].attached_branch = &tree->branches[i];
    //         tree->leaf_count++;
    //     }
    // }
}

void UpdateTree(s_tree* tree){
    float dt = GetFrameTime();

    int delete_count = 0;
    float time = GetTime();
    for(int i = 0; i < vector_size(tree->dropped_branches); i++){
        // Increase branch offset
        tree->dropped_branches[i].offset = Vector2Add(
            tree->dropped_branches[i].offset, (Vector2){0.0, 50.0f*dt});
        s_dropped_branch dropped = tree->dropped_branches[i];
        // Fade it out
        float t = tree->dropped_branches[i].spawn_time - time;
        //float a = LinearInterpolate(0.0f, 1.0f, t);
        //tree->dropped_branches[i].color = ColorAlpha(BROWN, a);
        tree->dropped_branches[i].color = BROWN;
        // If it has reached a clear color, remove it from the dropped branch list
        if(t <= -3.0f){
            tree->indices_to_delete[delete_count] = i;
            delete_count++;
        }
    }
    // Go through and delete dropped branches
    for(int i = 0; i < delete_count; i++){
        vector_remove(&tree->dropped_branches, tree->indices_to_delete[i]);
    }

    // Update status
    tree->water_counter -= 6.0f * dt;
    if(tree->water_counter <= 0){
        tree->water_counter = 0;
    }
    if(tree->water_counter <= 25) tree->status = DYING;
    else if(tree->water_counter <= 70) tree->status = THIRSTY;
    else tree->status = HEALTHY;

    if(tree->status == HEALTHY){
        tree->grow_counter += 1.0f * dt;
        if(tree->grow_counter >= 5){
            GrowTree(tree);
            tree->grow_counter = 0;
        }
    }
    if(tree->status == DYING){
        tree->grow_counter += 1.0f * dt;
        if(tree->grow_counter >= 4){
            DropBranch(tree);
            tree->grow_counter = 0;
        }
    }
}

void RenderTree(s_tree* tree){
    // Branches
    for(int i = 0; i < vector_size(tree->branches); i++){
        // Branch thickness based on length of the branch, see tree->base_thickness & length
        s_branch* b = &tree->branches[i];
        float ratio = b->length / tree->base_length;
        DrawLineEx(b->start, b->end, tree->base_thickness*ratio, BROWN);
        if(IsBranchLeaf(b)){
              DrawCircle(b->end.x, b->end.y, 20, ColorAlpha(GREEN, 0.8f));
        }
    }

    // Render text
    char status_lit[20] = "Tree Status: ";
    strcat(status_lit, tree_status_to_string(tree));
    DrawText(status_lit, 0, 50, 32, WHITE);

    //char str[10000];
    //sprintf(str, "%d", vector_size(tree->branches));
    //DrawText(&str, 0, 0, 32, WHITE);
    // Dropped branches
    // for(int i = 0; i < vector_size(tree->dropped_branches); i++){
    //     s_dropped_branch dropped_branch = tree->dropped_branches[i];
    //     float ratio = dropped_branch.branch_copy.length / tree->base_length;
    //     Vector2 start = Vector2Add(dropped_branch.branch_copy.start, dropped_branch.offset);
    //     Vector2 end = Vector2Add(dropped_branch.branch_copy.end, dropped_branch.offset);
    //     //DrawLineEx(start, end, tree->base_thickness*ratio, dropped_branch.color);
    //     DrawCircleV(end, 30, dropped_branch.color);
    // }
}

void RecursiveTreeDraw(int length, int start_length, float angle){
    const float branch_decay = 0.67;
    const float min_length = 12;
    const float thickness_max = 30;

    // Calc thickness of line based on length
    float thickness = thickness_max * (length/(float)start_length);
    // Draw line
    DrawLineEx((Vector2){0.0, 0.0}, (Vector2){0.0, -length}, thickness, BROWN);
    rlTranslatef(0, -length, 0);
    if(length > min_length){
        
        rlPushMatrix();
        rlRotatef(angle, 0, 0, 1);
        RecursiveTreeDraw(length * branch_decay, start_length, angle);
        rlPopMatrix();

        rlPushMatrix();
        rlRotatef(-angle, 0, 0, 1);
        RecursiveTreeDraw(length * branch_decay, start_length, angle);
        rlPopMatrix();
    }               
}   

void DropBranch(s_tree* tree) {
    // Play sound
    PlaySound(tree->hurt_sound);
    s_branch* saved = (&tree->branches[vector_size(tree->branches)-1]);
    // vector_remove(&tree->branches, vector_size(tree->branches)-1);
    vector_erase(&tree->branches, vector_size(tree->branches)-1, 1);
    //free(saved);
    printf("%.2f\n", saved->start.x);
}

void GrowTreeR(s_branch* branch, int depth){
    if(branch == NULL) return; // Break condition
    // Calculate new branch length, depth is a number of how far we are in
    // grow proprtionally to depth^2 so branches further in grow more
    Vector2 line = Vector2Subtract(branch->end, branch->start);
    Vector2 direction = Vector2Scale(Vector2Normalize(line), depth*depth);
    line = Vector2Add(line, direction);
    branch->end = Vector2Add(branch->start, line); // new end
    // If we have children, tell them about their new start
    if(branch->child_a != NULL)
        branch->child_a->start = branch->end;
    if(branch->child_b != NULL)
        branch->child_b->start = branch->end;
    // Since tree is basically a binary-tree data structure, we use recursion iteration
    GrowTreeR(branch->child_a, depth+1);
    GrowTreeR(branch->child_b, depth+1);
}

void GrowTree(s_tree* tree) {
    // Grow the length  
    s_branch* current = &tree->branches[0];
    GrowTreeR(current, 0);

    // Add new branches where at leaf nodes
    int frozen_branch_count = vector_size(tree->branches);
    for(int i = 0; i < frozen_branch_count; i++){
        s_branch* branch = &tree->branches[i];
        if(branch->done) continue; // Is it done skip the rest of the checks
        // Check both children, spawn on random chance
        if(branch->child_a == NULL && GetRandomFloatValue01() > 0.5f){
            s_branch* b1 = vector_add_asg(&tree->branches);
            *b1 = SpawnBranch(&tree->branches[i], 1);
            tree->branches[i].child_a = b1;                 
            b1 = NULL;
        }
        if(branch->child_b == NULL && GetRandomFloatValue01() > 0.5f){
            s_branch* b1 = vector_add_asg(&tree->branches);
            *b1 = SpawnBranch(&tree->branches[i], -1);
            tree->branches[i].child_b = b1;
            b1 = NULL;
        }
        // If both children exist, this branch can't spawn any more children
        if(branch->child_a != NULL && branch->child_b != NULL)
            branch->done = true;
    }
}

// Free memory from tree
void DestructTree(s_tree* tree){
    vector_erase(tree->branches, 0, vector_size(tree->branches));
    vector_erase(tree->dropped_branches, 0, vector_size(tree->dropped_branches)); // Erase all elements
    free(tree->branches);
    free(tree->dropped_branches);
}

bool IsBranchLeaf(s_branch* branch){
    return (branch->child_a == NULL && branch->child_b == NULL);
}

void WaterTree(s_tree* tree){
    // Increase water counter, clamp to 3
    tree->water_counter+=25;
    if(tree->water_counter >= 100) tree->water_counter = 100;
}