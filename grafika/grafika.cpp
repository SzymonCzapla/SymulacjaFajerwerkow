#include <GL/glut.h>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


struct Particle {
    float x, y;
    float vx, vy;
    float r, g, b, a;
    float life;
};

struct Trail {
    float x, y;
    float r, g, b, a;
    float life;
};

struct Rocket {
    float x, y;
    float vx, vy;
    float ay;
    float targetY;
    bool  exploded;
};

static std::vector<Particle> particles;
static std::vector<Trail>    trails;
static std::vector<Rocket>   rockets;


static const int   MAX_PARTICLES = 20000;
static       int   BURST_COUNT = 300;
static const float INITIAL_LIFE = 4.0f;
static const float GRAVITY = -20.0f;
static const float DRAG_COEF = 0.1f;
static const float ROCKET_ACCEL = 800.0f;
static const float WIND_X = 10.0f;
static const float TRAIL_LIFE = 0.4f;
static const float PARTICLE_SPEED_MEAN = 50.0f;
static const float PARTICLE_SPEED_VAR = 40.0f;
static const float BURST_OFFSET_RADIUS = 1.0f;

using uint = unsigned int;
static uint lastTime = 0;

static float randFloat(float a, float b) {
    return a + static_cast<float>(rand()) / RAND_MAX * (b - a);
}

static void launchRocket(float wx, float wy) {
    rockets.push_back({ wx, -glutGet(GLUT_WINDOW_HEIGHT) / 2.0f, 0,0, ROCKET_ACCEL, wy, false });
}

static void spawnBurst(float cx, float cy) {
    float cr = randFloat(0.4f, 1.0f), cg = randFloat(0.4f, 1.0f), cb = randFloat(0.4f, 1.0f);
    for (int i = 0; i < BURST_COUNT && (int)particles.size() < MAX_PARTICLES; ++i) {
        float ang = randFloat(0, 2.0f * M_PI);
        float spd = randFloat(PARTICLE_SPEED_MEAN - PARTICLE_SPEED_VAR,
            PARTICLE_SPEED_MEAN + PARTICLE_SPEED_VAR);
        Particle p;
        p.x = cx + cosf(ang) * BURST_OFFSET_RADIUS;
        p.y = cy + sinf(ang) * BURST_OFFSET_RADIUS;
        p.vx = cosf(ang) * spd;
        p.vy = sinf(ang) * spd;
        p.r = cr; p.g = cg; p.b = cb;
        p.life = INITIAL_LIFE;
        p.a = 1.0f;
        particles.push_back(p);
    }
}

static void updateRockets(float dt) {
    for (auto& r : rockets) {
        if (!r.exploded) {
            trails.push_back({ r.x, r.y, 1,1,1, 1, TRAIL_LIFE });
            r.vy += (r.ay + GRAVITY) * dt;
            r.x += r.vx * dt;
            r.y += r.vy * dt;
            if (r.y >= r.targetY) {
                spawnBurst(r.x, r.y);
                r.exploded = true;
            }
        }
    }
    rockets.erase(
        std::remove_if(rockets.begin(), rockets.end(), [](auto& r) { return r.exploded; }),
        rockets.end());
}

static void updateParticles(float dt) {
    for (auto& p : particles) {
        trails.push_back({ p.x, p.y, p.r, p.g, p.b, p.a, TRAIL_LIFE });
        p.vx += WIND_X * dt;
        p.vx -= DRAG_COEF * p.vx * dt;
        p.vy -= DRAG_COEF * p.vy * dt;
        p.vy += GRAVITY * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
        p.a = std::max(0.0f, p.life / INITIAL_LIFE);
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(), [](auto& p) { return p.life <= 0.0f; }),
        particles.end());
}

static void updateTrails(float dt) {
    for (auto& t : trails) t.life -= dt;
    trails.erase(
        std::remove_if(trails.begin(), trails.end(), [](auto& t) { return t.life <= 0.0f; }),
        trails.end());
}

static void display() {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (const auto& t : trails) {
        float alpha = t.life / TRAIL_LIFE;
        float size = 1.0f + 4.0f * alpha;
        glPointSize(size);
        glColor4f(t.r, t.g, t.b, alpha * t.a);
        glBegin(GL_POINTS);
        glVertex2f(t.x, t.y);
        glEnd();
    }

    for (const auto& r : rockets) {
        float ang = atan2f(r.vy, r.vx) * 180.0f / M_PI - 90.0f;
        glPushMatrix();
        glTranslatef(r.x, r.y, 0);
        glRotatef(ang, 0, 0, 1);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, 12);
        glVertex2f(-6, -6);
        glVertex2f(6, -6);
        glEnd();
        glPopMatrix();
    }

    const float R = 4.0f; const int SEG = 12;
    for (const auto& p : particles) {
        glColor4f(p.r, p.g, p.b, p.a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(p.x, p.y);
        for (int i = 0; i <= SEG; i++) {
            float th = 2 * M_PI * i / SEG;
            glVertex2f(p.x + cosf(th) * R, p.y + sinf(th) * R);
        }
        glEnd();
    }

    glutSwapBuffers();
}

static void idle() {
    uint now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;
    updateRockets(dt);
    updateParticles(dt);
    updateTrails(dt);
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
        launchRocket(x - w * 0.5f, (h - y) - h * 0.5f);
    }
}

static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-w * 0.5f, w * 0.5f, -h * 0.5f, h * 0.5f, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

enum MenuOptions {
    INCREASE_BURST = 1,
    DECREASE_BURST
};

static void menu(int option) {
    switch (option) {
    case INCREASE_BURST:
        BURST_COUNT += 100;
        break;
    case DECREASE_BURST:
        BURST_COUNT = std::max(10, BURST_COUNT - 100);
        break;
    }
}

static void createMenu() {
    glutCreateMenu(menu);
    glutAddMenuEntry("Zwieksz ilosc czastek o 100", INCREASE_BURST);
    glutAddMenuEntry("Zmniejsz ilosc czastek o 100", DECREASE_BURST);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Colored Rocket Fireworks");
    glClearColor(0, 0, 0.05f, 1);
    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    createMenu();
    glutMainLoop();
    return 0;
}
