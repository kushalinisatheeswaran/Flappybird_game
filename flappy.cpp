#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

using namespace sf;

// -------------------- BIRD CLASS --------------------
class Bird {
public:
    Sprite sprite;
    const float flapVY = -5.f;
    const float stepDY = 5.f;
    const float speedX = 3.f;
    float lockedY;
    bool horizontalLock;

    Sound *flapSound;

    Bird(Texture &texture, float startX, float startY, Sound *fs) {
        sprite.setTexture(texture);
        sprite.setScale(0.15f, 0.15f);
        sprite.setPosition(startX, startY);
        lockedY = startY;
        horizontalLock = false;
        flapSound = fs;
    }

    void update() {
        bool upPress = Keyboard::isKeyPressed(Keyboard::Space) || Keyboard::isKeyPressed(Keyboard::PageUp);
        bool downPress = Keyboard::isKeyPressed(Keyboard::PageDown);

        if(upPress) {
            sprite.move(0, flapVY);
            if(flapSound) flapSound->play();
        } else if(downPress) {
            sprite.move(0, stepDY);
        } else {
            sprite.move(0, 2.f); // gravity
        }

        // Keep bird inside window vertically
        FloatRect bounds = sprite.getGlobalBounds();
        if(sprite.getPosition().y < 0) sprite.setPosition(sprite.getPosition().x, 0);
        if(sprite.getPosition().y + bounds.height > 600) sprite.setPosition(sprite.getPosition().x, 600 - bounds.height);
    }

    FloatRect getBounds() { return sprite.getGlobalBounds(); }
};

// -------------------- PIPE CLASS --------------------
class Pipe {
public:
    Sprite topPipe, bottomPipe;
    float x, gapY;
    bool passed;

    Pipe(Texture &topTex, Texture &bottomTex, float startX) {
        x = startX;
        gapY = rand() % 300 + 150;
        passed = false;

        topPipe.setTexture(topTex);
        bottomPipe.setTexture(bottomTex);

        topPipe.setScale(0.4f, 0.4f);
        bottomPipe.setScale(0.2f, 0.2f);

        topPipe.setPosition(x, gapY - 150 - topPipe.getGlobalBounds().height);
        bottomPipe.setPosition(x, gapY + 150);
    }

    void move(float speed) {
        x -= speed;
        topPipe.setPosition(x, topPipe.getPosition().y);
        bottomPipe.setPosition(x, bottomPipe.getPosition().y);
    }

    bool checkCollision(Bird &bird) {
        return bird.getBounds().intersects(topPipe.getGlobalBounds()) ||
               bird.getBounds().intersects(bottomPipe.getGlobalBounds());
    }

    bool offscreen() { return x + topPipe.getGlobalBounds().width < 0; }

    void draw(RenderWindow &window) {
        window.draw(topPipe);
        window.draw(bottomPipe);
    }
};

// -------------------- GAME CLASS --------------------
class Game {
private:
    RenderWindow window;
    Texture bgTex, birdTex, pipeTopTex, pipeBottomTex;
    Sprite background;
    Bird *bird;

    std::vector<Pipe> pipes;
    float pipeSpeed;
    int score, level;
    float levelTime;
    Clock levelClock;

    Font font;
    Text infoText;

    SoundBuffer flapBuffer, pointBuffer, gameOverBuffer;
    Sound flapSound, pointSound, gameOverSound;

    enum GameState { MENU, PLAYING, LOSE, WIN } state;

    int triesLeft;
    const int maxTries = 3;

public:
    Game() : window(VideoMode(800,600), "Flappy Bird"), pipeSpeed(3.f), score(0), level(1), levelTime(30.f), state(MENU) {
        window.setFramerateLimit(60);
        srand(time(0));

        if(!bgTex.loadFromFile("assets/background.png") ||
           !birdTex.loadFromFile("assets/bird1.png") ||
           !pipeTopTex.loadFromFile("assets/pipe_top.png") ||
           !pipeBottomTex.loadFromFile("assets/pipe_bottom.png")) exit(-1);

        background.setTexture(bgTex);
        background.setScale(
            float(window.getSize().x)/bgTex.getSize().x,
            float(window.getSize().y)/bgTex.getSize().y
        );

        // Load sounds
        if(!flapBuffer.loadFromFile("assets/flap.wav")) std::cout << "Failed to load flap.wav\n";
        if(!pointBuffer.loadFromFile("assets/point.wav")) std::cout << "Failed to load point.wav\n";
        if(!gameOverBuffer.loadFromFile("assets/gameover.wav")) std::cout << "Failed to load gameover.wav\n";

        flapSound.setBuffer(flapBuffer);
        pointSound.setBuffer(pointBuffer);
        gameOverSound.setBuffer(gameOverBuffer);

        bird = new Bird(birdTex, window.getSize().x / 2.f, window.getSize().y / 2.f, &flapSound);

        if(!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) exit(-1);

        infoText.setFont(font);
        infoText.setCharacterSize(32);
        infoText.setFillColor(Color::White);
        infoText.setStyle(Text::Bold);
    }

    void run() {
        while(window.isOpen()) {
            handleEvents();
            if(state == PLAYING) update();
            draw();
        }
    }

private:
    void handleEvents() {
        Event event;
        while(window.pollEvent(event)) {
            if(event.type == Event::Closed) window.close();
        }

        if(state == MENU) {
            if(Keyboard::isKeyPressed(Keyboard::Enter)) {
                triesLeft = maxTries; // reset tries
                startLevel();
            }
            if(Keyboard::isKeyPressed(Keyboard::Escape)) window.close();
        } else if((state == LOSE || state == WIN) && Keyboard::isKeyPressed(Keyboard::Enter)) {
            triesLeft = maxTries; // reset tries for new level
            startLevel();
        }
    }

    void startLevel() {
        state = PLAYING;
        pipes.clear();
        score = 0;
        bird->sprite.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.f);
        pipeSpeed = 3.f + (level - 1) * 0.5f;
        levelClock.restart();
    }

    void update() {
        bird->update();

        // Spawn pipes
        if(pipes.empty() || pipes.back().x < 600) {
            pipes.push_back(Pipe(pipeTopTex, pipeBottomTex, 800));
        }

        // Move pipes and collision
        for(auto &pipe : pipes) {
            pipe.move(pipeSpeed);

            if(pipe.checkCollision(*bird)) {
                triesLeft--;
                gameOverSound.play();

                if(triesLeft > 0) {
                    // retry: reset bird and pipes
                    bird->sprite.setPosition(window.getSize().x / 2.f,
    window.getSize().y / 2.f);
                    pipes.clear();
                    score = 0;
                    levelClock.restart();
                    return;
                } else {
                    state = LOSE;
                    return;
                }
            }

            if(!pipe.passed && pipe.x + pipe.topPipe.getGlobalBounds().width < bird->sprite.getPosition().x) {
                score++;
                pipe.passed = true;
                pointSound.play();
            }
        }

        if(!pipes.empty() && pipes.front().offscreen()) pipes.erase(pipes.begin());

        // Check level timer
        if(levelClock.getElapsedTime().asSeconds() >= levelTime) {
            level++;
            state = WIN;
        }
    }

    void draw() {
        window.clear();
        window.draw(background);

        if(state == MENU) {
            infoText.setString(
                "FLAPPY BIRD\n\n"
                "Press Enter to Start\n"
                "Press Escape to Exit\n"
                "Each Level: 30s, 3 Tries"
            );
            infoText.setPosition(100, 200);
            window.draw(infoText);
        } else if(state == PLAYING) {
            for(auto &pipe : pipes) pipe.draw(window);
            window.draw(bird->sprite);

            float timeLeft = levelTime - levelClock.getElapsedTime().asSeconds();
            if(timeLeft < 0) timeLeft = 0;

            infoText.setString(
                "Score: " + std::to_string(score) +
                "  Level: " + std::to_string(level) +
                "  Time: " + std::to_string((int)timeLeft) +
                "  Tries Left: " + std::to_string(triesLeft)
            );
            infoText.setPosition(20, 20);
            window.draw(infoText);
        } else if(state == LOSE) {
            infoText.setString("Game Over! Press Enter to Retry");
            infoText.setPosition(100, 250);
            window.draw(infoText);
        } else if(state == WIN) {
            infoText.setString("Level Complete! Press Enter for Next Level");
            infoText.setPosition(80, 250);
            window.draw(infoText);
        }

        window.display();
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}