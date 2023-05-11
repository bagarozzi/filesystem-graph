#include <raylib.h>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <string>
#include <math.h>
#include <list>

#define WINDOW_SIZE 1080

#define MAX_CIRCLE_SIZE 500
#define MIN_CIRCLE_SIZE 150

#define SPREAD_MULT 100
#define GRAVITY_CONST 1.1
#define FORCE_CONST 4000

class Node {
    public:
        Node(unsigned long size, const std::string& name) : size(size), name(name), circleSize(1), pushed(false) {
            pos = { (float)(rand() % (2 * SPREAD_MULT * WINDOW_SIZE) - (SPREAD_MULT * WINDOW_SIZE)), (float)(rand() % (2 * SPREAD_MULT * WINDOW_SIZE) - (SPREAD_MULT * WINDOW_SIZE)) };
            force = { 0, 0 };
        }

        float mass() {
            return (2 * PI * circleSize) / 1.5;
        }

        void update() {
            Vector2 vel;
            vel.x = force.x / mass();
            vel.y = force.y / mass();
            pos.x += vel.x;
            pos.y += vel.y;
        }
    public:
        unsigned long size;
        const std::string name;
        std::list<Node> children;
        float circleSize;
        Vector2 pos;
        Vector2 force;
        bool pushed;
};

void addNodes(Node& current, const std::string& path) {
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
        if (is_directory(entry.path())) {
            Node node(0, entry.path().filename());
            current.children.push_back(node);
            addNodes(current.children.back(), entry.path());
        } else {
            try {
                Node node(entry.file_size(), entry.path().filename());
                current.children.push_front(node);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}

void getMaxSizes(Node& current, unsigned long& maxSize, unsigned long& minSize) {
    for (Node& node : current.children) {
        getMaxSizes(node, maxSize, minSize);
    }
    if (current.size < minSize) {
        minSize = current.size;
    }
    if (current.size > maxSize) {
        maxSize = current.size;
    }
}

unsigned long calculateFolderSizes(Node& current) {
    for (Node& node : current.children) {
        current.size += calculateFolderSizes(node);
    }
    return current.size;
}

void calculateCircleSizes(Node& current, unsigned long& maxSize, unsigned long& minSize) {
    for (Node& node : current.children) {
        calculateCircleSizes(node, maxSize, minSize);
    }
    current.circleSize = ((float)current.size - (float)minSize) * ((float)MAX_CIRCLE_SIZE - (float)MIN_CIRCLE_SIZE) / ((float)maxSize - (float)minSize) + (float)(MIN_CIRCLE_SIZE);
}

void drawNodes(Node& current, Vector2 parentPos, unsigned short depth) {
    current.pushed = false;
    DrawCircleV(current.pos, current.circleSize, WHITE);
    DrawText(current.name.c_str(), current.pos.x, current.pos.y, 3 * current.circleSize, WHITE);
    if (depth != 0) {
        DrawLineV(current.pos, parentPos, { 255, 255, 255, 75 });
    }
    current.update();
    for (Node& node : current.children) {
        drawNodes(node, current.pos, depth + 1);
    }
}

void repulseFromOthers(Node& main, Node& current) {
    if (!current.pushed) {
        Vector2 dir;
        dir.x = current.pos.x - main.pos.x;
        dir.y = current.pos.y - main.pos.y;
        float magSq = sqrt(dir.x * dir.x + dir.y * dir.y) * 2;
        Vector2 force;
        force.x = dir.x / magSq * FORCE_CONST;
        force.y = dir.y / magSq * FORCE_CONST;
        main.force.x += -force.x;
        main.force.y += -force.y;
        current.force.x += force.x;
        current.force.y += force.y;
    }
    for (Node& node : current.children) {
        repulseFromOthers(main, node);
    }
}

void applyForces(Node& current, Node& root) {
    for (Node& node : current.children) {
        applyForces(node, root);
    }
    // Apply gravity force on node
    Vector2 gravity;
    gravity.x = -current.pos.x * GRAVITY_CONST;
    gravity.y = -current.pos.y * GRAVITY_CONST;
    current.force = gravity;
    // Repulse from every other node
    current.pushed = true;
    repulseFromOthers(current, root);
    // Attract from connected nodes
    for (Node& node : current.children) {
        Vector2 dis;
        dis.x = current.pos.x - node.pos.x;
        dis.y = current.pos.y - node.pos.y;
        current.force.x -= dis.x;
        current.force.y -= dis.y;
        node.force.x += dis.x;
        node.force.y += dis.y;
    }
}

void debugPrint(Node& current, unsigned short depth) {
    for (unsigned short i = 0; i < depth; ++i) {
        std::cout << "\t";
    }
    std::cout << current.name << ", " << current.size << std::endl;
    for (Node& node : current.children) {
        debugPrint(node, depth + 1);
    }
}

int main(int argc, char *argv[]) {
    // Initial setup
    srand(0);
    std::string pathMain = (std::string)getenv("HOME") + "/Documents/Progetti";
    if (argc > 1) {
        srand(strtol(argv[1], NULL, 10));
        if (argc > 2) {
            pathMain = argv[2];
            if (argc > 3) {
                std::cout << "Syntax error, correct usage:" << std::endl;
                std::cout << "graph" << std::endl;
                std::cout << "graph <seed>" << std::endl;
                std::cout << "graph <seed> <rootPath>" << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    // Gather data
    Node root(0, "Root");
    addNodes(root, pathMain);
    calculateFolderSizes(root);
    unsigned long maxSize = 0;
    unsigned long minSize = -1;
    getMaxSizes(root, maxSize, minSize);
    calculateCircleSizes(root, maxSize, minSize);
    // Generating window and graph
    InitWindow(WINDOW_SIZE, WINDOW_SIZE, "Graph View");
    SetTargetFPS(60);
    Camera2D camera = { 0 };
    camera.target = (Vector2){ (float)-WINDOW_SIZE / 2, (float)-WINDOW_SIZE / 2 };
    camera.offset = (Vector2){ (float)WINDOW_SIZE / 2, (float)WINDOW_SIZE / 2 };
    camera.rotation = 0.0f;
    camera.zoom = 0.1f;
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_P)) {
            camera.zoom += 0.001;
        }
        if (IsKeyDown(KEY_O)) {
            camera.zoom -= 0.001;
        }
        if (IsKeyDown(KEY_UP)) {
            camera.target.y -= 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            camera.target.y += 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            camera.target.x += 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_LEFT)) {
            camera.target.x -= 1 / camera.zoom * 3.0f;
        }
        applyForces(root, root);
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
        drawNodes(root, root.pos, 0);
        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
