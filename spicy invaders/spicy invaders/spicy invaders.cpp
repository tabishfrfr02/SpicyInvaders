#include <SFML/Graphics.hpp>   // SFML library for drawing windows, shapes, sprites, text
#include <SFML/Audio.hpp>      // SFML library for playing music and sound effects
#include <string>             
#include <cstdlib>             // rand() and srand() for random numbers
#include <ctime>               // time() so srand gets a different seed each run
#include <cmath>               


//  from here we start our code with GLOBAL VARIABLES
//  These are shared across the whole program.
//  Any function or class can read/write them.
// 
int Score = 0;   // Player's running score, increases when enemies are killed
int mult = 1;   // Score multiplier goes up on kill streaks, resets on hit
int WaveNum = 0;  // Tracks the current survival wave number



//  AudioManager CLASS
//  this is for our all audio sounds it owns every music track and sound effect.
// _____________________________________________
class AudioManager
{
private:
    // sf::Music streams audio from disk is picks it up from our assests folder
    sf::Music menuMusic;   // Plays on the main menu
    sf::Music gameMusic;   // Plays during normal wave gameplay
    sf::Music bossMusic1;  // Plays during level-1 boss fight
    sf::Music bossMusic2;  // Plays during level-2 boss fight
    sf::Music bossMusic3;  // Plays during level-3 boss fight

    // sf::SoundBuffer loads a short clip fully into memory
    sf::SoundBuffer shootBuf;     // Raw audio data for the shoot sound
    sf::SoundBuffer powerupBuf;   // Raw audio data for the power-up collect sound
    sf::Sound       shootSound;   // Player that plays shootBuf
    sf::Sound       powerupSound; // Player that plays powerupBuf

    sf::SoundBuffer explosionBuf; // Raw audio data for one explosion sound

    // We need multiple channels because several explosions can happen
    // at the same time (e.g. EMP kills all enemies at once).
    // If we only had one sf::Sound it would restart and cut off the previous one.
    static const int NUM_EXPLOSION_CHANNELS = 8;
    sf::Sound explosionChannels[NUM_EXPLOSION_CHANNELS]; // 8 independent explosion players
    int       nextExplosionChannel; // Index we'll use next (cycles 0-7)

    // Stops every music track so we can start a different one cleanly
    void stopAllMusic()
    {
        menuMusic.stop();
        gameMusic.stop();
        bossMusic1.stop();
        bossMusic2.stop();
        bossMusic3.stop();
    }

public:
    // Constructor – just initialise the channel counter to 0
    AudioManager() : nextExplosionChannel(0) {}

    // load() – opens all audio files from disk.
    // Returns false if any file is missing so the caller can handle errors.
    bool load()
    {
        if (!menuMusic.openFromFile("assets/menu.ogg"))    return false;
        if (!gameMusic.openFromFile("assets/game.ogg"))    return false;
        if (!bossMusic1.openFromFile("assets/boss1.ogg"))  return false;
        if (!bossMusic2.openFromFile("assets/boss2.ogg"))  return false;
        if (!bossMusic3.openFromFile("assets/boss3.ogg"))  return false;

        if (!shootBuf.loadFromFile("assets/shoot.wav"))       return false;
        if (!powerupBuf.loadFromFile("assets/powerup.wav"))   return false;
        if (!explosionBuf.loadFromFile("assets/explosion.wav")) return false;

        // Make all music loop so it never silently stops
        menuMusic.setLoop(true);
        gameMusic.setLoop(true);
        bossMusic1.setLoop(true);
        bossMusic2.setLoop(true);
        bossMusic3.setLoop(true);

        // Connect each sf::Sound to its buffer (the buffer holds the raw PCM data)
        shootSound.setBuffer(shootBuf);
        powerupSound.setBuffer(powerupBuf);
        for (int i = 0; i < NUM_EXPLOSION_CHANNELS; i++)
            explosionChannels[i].setBuffer(explosionBuf);

        return true;
    }

    // Music switchers 
    void playMenu() { stopAllMusic(); menuMusic.play(); }
    void playGame() { stopAllMusic(); gameMusic.play(); }
    void playBoss(int level)
    {
        stopAllMusic();
        if (level == 1) bossMusic1.play();
        if (level == 2) bossMusic2.play();
        if (level == 3) bossMusic3.play();
    }
    void resumeGame() { stopAllMusic(); gameMusic.play(); } // Called after boss dies

    // One-shot sound effects
    void playShoot() { shootSound.play(); }
    void playPowerup() { powerupSound.play(); }


    // explosions all play simultaneously instead of cutting each other off
    void playExplosion()
    {
        explosionChannels[nextExplosionChannel].play();
        nextExplosionChannel = (nextExplosionChannel + 1) % NUM_EXPLOSION_CHANNELS;
    }
};

// Global audio manager one instance shared by everything
AudioManager audio;

// Global font used for the pause screen and we declare here so both
// the Game class and main() can use it without passing it around
sf::Font pauseFont;



//  these are our CONSTANTS
//  Fixed numbers used across the whole game.
//  Putting them here means changing one value
//  updates every place that uses it.

const int MAX_BULLETS = 50;   // Normal player bullets alive at once it cannt be more than 50 we can up it but 50 is good
const int MAX_ASTEROIDS = 5;    // max Asteroids on screen
const int MAX_STARS = 300;  // max stars present in stars
const int MAX_HEARTS = 3;    // Player health hearts
const int MAX_DRONES = 5;    // max drone enemies
const int MAX_VIPERS = 3;    // max viper enemies
const int MAX_SEEKERS = 3;    // max seeker enemies
const int MAX_EXPLOSIONS = 20;   // Simultaneous explosion animations
const int MAX_ENEMY_BULLETS = 100;  // Enemy projectiles alive at once
const int MAX_EMP_DROPS = 10;   // EMP pickup items on screen
const int MAX_EMP_HOLD = 3;    // Max EMPs the player can carry
const int MAX_POWER_DROPS = 10;   // Power-up pickup items on screen
const int MAX_LASER_BEAMS = 5;    // Player laser beams
const int MAX_SPREAD_BULLETS = 150; // Spread-shot bullets alive at once

// Dash / invincibility timings
const float DASH_DISTANCE = 200.f; // How far one dash moves the ship in pixels
const float DASH_COOLDOWN = 3.0f;  // Seconds before dash is available again
const float DASH_INVINCIBLE = 0.5f;  // Seconds of invincibility granted by a dash

// Power-up durations (seconds)
const float SPREAD_DURATION = 15.f;  // How long spread-shot lasts
const float LASER_DURATION = 15.f;  // How long laser weapon lasts

// Laser weapon internal timings
const float LASER_WEAPON_ACTIVE_DURATION = 5.0f; // Fires for 5 s …
const float LASER_WEAPON_COOLDOWN_DURATION = 1.0f; // then recharges for 1 s

// Wave / boss flow timings
const float WAVE_DURATION = 15.f;  // Seconds of normal waves before boss intro
const float BOSS_INTRO_PAUSE = 6.0f;  // Warning screen duration before boss spawns
const float BOSS_ENTRY_SPEED = 80.f;  // Pixels per second the boss slides into view

// Damage feedback timings
const float DAMAGE_INVINCIBLE_DURATION = 1.0f; // Invincibility window after being hit
const float DAMAGE_FLASH_DURATION = 0.3f;  // How long the red flash lasts on ship


// Tells the ship which weapon is currently active
enum class WeaponMode { NORMAL, SPREAD, LASER };

// Describes what part of a level we're in
enum class GamePhase
{
    WAVE,        // Normal enemy spawning
    BOSS_INTRO,  // Warning banner, no boss yet
    BOSS_FIGHT,  // Boss is alive and fighting
    LEVEL_CLEAR  // Boss defeated, brief celebration
};


// Which game mode the player chose from the menu
enum class GameMode { NONE, ARCADE, SURVIVAL };



//  GameObject BASE CLASS
//  this is our Pure abstract base class and every object in the game
//  inherits from this so they all share the same 
//  update / draw / onCollision
//  This lets us call the same function on any object
class GameObject
{
public:
    virtual void update(float dt) = 0; // Move / animate each frame; dt means seconds that have passed
    virtual void draw(sf::RenderWindow& window) = 0; // Draws the object to the screen
    virtual void onCollision() = 0; // Called on collision
    virtual ~GameObject() {}
};

// Forward declarations – the compiler needs to know these classes exist
class Asteroid;
class Bullet;



//  Bullet CLASS
//  Represents a single player projectile.
//  Bullets are stored in a fixed-size (array of pointers). When the player fires,
//  we find an inactive one and activate it.
class Bullet : public GameObject
{
private:
    float x, y;    // Current position in pixels
    float vx, vy;  // Velocity
    bool  active;  // True = bullet is flying; false = slot is free
    sf::Texture* tex; // Pointer to shared texture

public:
    // Default constructor – start inactive in the top-left corner
    Bullet() : x(0), y(0), vx(0), vy(-15.f), active(false), tex(nullptr) {}
    ~Bullet() override {}

    void setTexture(sf::Texture* t) { tex = t; }

    // Activate this bullet at position which goes straight up
    void spawn(float sx, float sy)
    {
        x = sx; y = sy; vx = 0; vy = -15.f; active = true;
    }

    // Activate this bullet going at an angle for spread module  
    // angleRad is offset from straight-up in radians
    void spawnAngled(float sx, float sy, float angleRad)
    {
        x = sx; y = sy;
        vx = sinf(angleRad) * 15.f;   // Horizontal component
        vy = -cosf(angleRad) * 15.f;  // Vertical component (negative = upward)
        active = true;
    }

    // Called every frame – move bullet and deactivate if it leaves the screen
    void update(float) override
    {
        if (!active) return;
        x += vx; y += vy; // Move by velocity
        // Deactivate when off-screen on any edge
        if (y < -10 || y > 1210 || x < -10 || x > 1930) active = false;
    }

    // Draw a sprite rotated in the direction of travel and then we later load our texture on this sprite 
    void draw(sf::RenderWindow& window) override
    {
        if (!active || !tex) return;
        sf::Sprite sp(*tex);
        sp.setScale(0.2f, 0.2f);
        // Calculate visual rotation so bullet points the way it's travelling
        float ang = atan2f(vx, -vy) * 180.f / 3.14159f;
        sp.setRotation(-90.f + ang);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setPosition(x, y);
        window.draw(sp);
    }

    void onCollision() override { active = false; } // Hit something – deactivate
    void destroy() { active = false; }
    bool  isActive() const { return active; }
    float getX()     const { return x; }
    float getY()     const { return y; }
};



//  LaserBeam CLASS
//  The continuous beam fired during LASER mode
//  Unlike a bullet it isn't a moving projectile well it appears as so  
//  it's refreshed at the ship position every with (like no diff)frame
//  while the weapon is active, creating the
//  illusion of a persistent beam from ship to top
//  It can pierce through 2 enemies aswell

class LaserBeam : public GameObject
{
private:
    float x, y;       // Position – x is ship X, y is ship Y
    float lifetime;   // Seconds until beam auto-deactivates which give it a cooldown of some seconds if not refreshed
    bool  active;
    int   piercedCount;           // How many enemies have been hit so far
    static const int MAX_PIERCE = 2; // Beam deactivates after piercing this many
    sf::Texture* tex;

public:
    LaserBeam() : x(0), y(0), lifetime(0), active(false), piercedCount(0), tex(nullptr) {}
    ~LaserBeam() override {}

    void setTexture(sf::Texture* t) { tex = t; }

    // First activation same as bullet
    void spawn(float sx, float sy)
    {
        x = sx; y = sy; lifetime = 0.5f; active = true; piercedCount = 0;
    }

    // Called every frame the player holds fire while in laser mode.
    // Keeps the beam alive and pinned to the ship.
    void refresh(float sx, float sy)
    {
        x = sx;
        if (!active)
        {
            active = true; piercedCount = 0;
        }
        lifetime = 0.08f;
        y = sy;
    }

    // Tick down lifetime; deactivate when it runs out
    void update(float) override
    {
        if (!active) return;
        lifetime -= 0.016f;
        if (lifetime <= 0 || y < -600) active = false;
    }

    // Draw the beam as a scaled sprite stretching from ship position to screen top simple sfml drawing
    void draw(sf::RenderWindow& window) override
    {
        if (!active) return;
        if (tex)
        {
            float beamLength = y + 20.f; // Beam reaches from ship Y all the way to y=0 at the top
            sf::Sprite sp(*tex);
            sf::FloatRect b = sp.getLocalBounds();
            sp.setOrigin(b.width / 2.f, b.height);
            float scaleX = 90.f / b.width;
            float scaleY = beamLength / b.height;
            sp.setScale(scaleX, scaleY);
            sp.setPosition(x, y);
            sp.setColor(sf::Color(255, 255, 255, 230));
            window.draw(sp);
        }
    }

    void onCollision() override {} // Laser doesn't deactivate on first hit 

    // Called each time the beam hits an enemy.
    // Returns true (and deactivates) once MAX_PIERCE enemies have been hit.
    bool registerHit()
    {
        piercedCount++;
        if (piercedCount >= MAX_PIERCE) { active = false; return true; }
        return false;
    }

    // Check whether a point (ex, ey) is inside the beam's width and height
    bool collidesWithPoint(float ex, float ey) const
    {
        if (!active) return false;
        float dx = fabsf(ex - x);          // Horizontal distance from beam centre
        return dx < 30.f && ey >= 0.f && ey <= y; // Within width and above ship
    }

    bool  isActive() const { return active; }
    float getX()     const { return x; }
    float getY()     const { return y; }
};



//  EnemyBullet CLASS
//  Projectiles fired by enemies (drones, vipers)  boss turrets. They move downward toward the
//  player and it follows the same functionality of player bullet bascially same;

class EnemyBullet : public GameObject
{
private:
    float x, y;    // Position
    float speed;   // Downward speed in pixels per frame
    float vx;      // Horizontal velocity 
    bool  active;
    sf::Texture* tex;

public:
    EnemyBullet() : x(0), y(0), speed(6.f), active(false), tex(nullptr), vx(0.f) {}
    ~EnemyBullet() override {}

    void setTexture(sf::Texture* t) { tex = t; }

    // Spawn going straight down
    void spawn(float sx, float sy) { x = sx; y = sy; vx = 0.f; active = true; }

    // Spawn going at a horizontal angle used by turret spread shot
    void spawnAngled(float sx, float sy, float vxDir) { x = sx; y = sy; vx = vxDir; active = true; }

    void update(float) override
    {
        if (!active) return;
        x += vx; y += speed; // Move diagonally / straight down
        if (y > 1200 || x < -10 || x > 1930) active = false; // Off screen
    }

    void draw(sf::RenderWindow& window) override
    {
        if (!active || !tex) return;
        sf::Sprite sp(*tex);
        sp.setScale(0.2f, 0.2f); sp.setRotation(90.f); // Rotate so bullet points downward
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setPosition(x, y); window.draw(sp);
    }

    void onCollision() override { active = false; }
    void destroy() { active = false; }
    bool  isActive() const { return active; }
    float getX()     const { return x; }
    float getY()     const { return y; }
};



//  Asteroid CLASS
//  basically rocks that drift down the screen.
//  Three types (small / big / medium) with
//  different sizes, speeds, and spin rates.
//  They respawn from the top after going off-screen
//  unless noRespawn is true (during boss fights).

class Asteroid : public GameObject
{
private:
    float x, y;       // Position
    float speed;      // Downward drift speed
    float scale;      // Visual size scale
    float rotation;   // Current rotation angle in degrees
    float rotSpeed;   // Degrees rotated per frame
    float vx;         // Horizontal drift speed
    bool  noRespawn;  // If true, deactivate instead of respawning (during boss)
    bool  active;
    int   type;       // 0 = small, 1 = big, 2 = medium
    sf::Texture* texSmall; // Pointer to shared small texture
    sf::Texture* texBig;   // Pointer to shared big texture
    sf::Texture* texMed;   // Pointer to shared medium texture

    // Set speed / scale / rotation based on type
    void applyType()
    {
        if (type == 0) {       // Small – fast and spins quickly
            scale = 0.1f; speed = 2.f + (rand() % 15) / 10.f;
            rotSpeed = 0.4f + (rand() % 20) / 10.f;
            vx = -0.5f + (rand() % 11) / 10.f;
        }
        else if (type == 1) {  // Big – slow and heavy
            scale = 0.13f; speed = 0.5f + (rand() % 8) / 10.f;
            rotSpeed = 0.2f + (rand() % 10) / 10.f;
            vx = -0.3f + (rand() % 7) / 10.f;
        }
        else {                 // Medium – balanced
            scale = 0.2f; speed = 1.f + (rand() % 10) / 10.f;
            rotSpeed = 0.15f + (rand() % 8) / 10.f;
            vx = -0.4f + (rand() % 9) / 10.f;
        }
    }

public:
    Asteroid() : x(0), y(0), speed(1.f), scale(0.05f),
        rotation(0.f), rotSpeed(0.5f), vx(0.f),
        type(0), active(false), noRespawn(false),
        texSmall(nullptr), texBig(nullptr), texMed(nullptr) {
    }

    // Give this asteroid all three texture pointers 
    void setTextures(sf::Texture* s, sf::Texture* b, sf::Texture* m)
    {
        texSmall = s; texBig = b; texMed = m;
    }

    // Activate at a given position with a random type
    void spawn(float sx, float sy)
    {
        type = rand() % 3;
        x = sx; y = sy;
        active = true;
        rotation = (float)(rand() % 360); applyType();
    }

    // Move, spin, wrap horizontally respawn if below screen
    void update(float dt) override
    {
        if (!active) return;
        x += vx; y += speed;
        rotation += rotSpeed;
        if (rotation > 360.f)
            rotation -= 360.f;
        // Wrap around left/right edges
        if (x < -50.f)  x = 1970.f;
        if (x > 1970.f) x = -50.f;
        if (y > 1200)   respawn(); // Gone below screen – restart from top
    }

    // Move to a new random position at the top of the screen
    void respawn()
    {
        if (noRespawn) { active = false; return; } // During boss fights, just disappear
        type = rand() % 3; x = 100.f + rand() % 1720; y = -150.f;
        rotation = (float)(rand() % 360); applyType();
    }

    // Draw the correct texture for this asteroid's type, rotated and scaled
    void draw(sf::RenderWindow& window) override
    {
        if (!active) return;
        sf::Texture* t = (type == 0) ? texSmall : (type == 1) ? texBig : texMed;
        if (!t) return;
        sf::Sprite sp(*t);
        sp.setScale(scale, scale);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f); // Rotate around centre
        sp.setRotation(rotation); sp.setPosition(x, y);
        window.draw(sp);
    }

    // When hit by a bullet, just respawn (asteroids aren't destroyed)
    void onCollision() override { respawn(); }

    // Returns true if a bullet is close enough to count as a hit
    bool collidesWithBullet(Bullet* b) const
    {
        if (!b || !b->isActive()) return false;
        float dx = x - b->getX(), dy = y - b->getY();
        float r = getRadius() + 80.f; // Generous hit radius so it feels fair
        return (dx * dx + dy * dy) < r * r;
    }

    void  setNoRespawn(bool b) { noRespawn = b; }
    float getRadius() const { return scale * 400.f; } // Approximate collision radius
    bool  isActive()  const { return active; }
    float getX()      const { return x; }
    float getY()      const { return y; }
};


//  Star CLASS
//  Background stars which are basically white dots that some are dim some glow some are big some small that scroll
//  downward at varying speeds to create a scrolling and a depth to the space
class Star : public GameObject
{
private:
    float x, y, speed, size;
    int   brightness; // faster stars appear brighter between 0-255
public:
    Star() : x(0), y(0), speed(0), size(0), brightness(0) {}
    ~Star() override {}

    // Randomise position, speed, size, brightness
    void spawn()
    {
        x = (float)(rand() % 1920); y = (float)(rand() % 1080);
        speed = 0.5f + (rand() % 30) / 10.f;
        size = 0.5f + (rand() % 20) / 10.f;
        brightness = 100 + (int)(speed * 50); // Faster = brighter = looks closer
    }

    // Scroll downward; when off bottom, reappear at the top
    void update(float) override
    {
        y += speed;
        if (y > 1080) { x = (float)(rand() % 1920); y = 0; }
    }

    // Draw a tiny filled circle
    void draw(sf::RenderWindow& window) override
    {
        sf::CircleShape c(size); c.setPosition(x, y);
        c.setFillColor(sf::Color(brightness, brightness, brightness));
        window.draw(c);
    }

    void onCollision() override {} // Stars can't be hit
};


//  Enemy BASE CLASS
//  Common data and behaviour shared by all just some tweaks for viper and seeker and drone
//  Abstract class is inherited so basic methods and members are used additionally we have
class Enemy : public GameObject
{
protected:
    float x, y, speed; // Position and movement speed
    int   health;      // Current hit points
    bool  alive;       // False = this slot is free

public:
    Enemy(float x, float y, int health)
        : x(x), y(y), speed(0.f), health(health), alive(false) {
    }
    ~Enemy() override {}

    // Activate at position, reset health to max
    virtual void spawnAt(float sx, float sy)
    {
        x = sx; y = sy; alive = true; health = getMaxHealth();
    }

    //for declaring max health for each enemy
    virtual int getMaxHealth() = 0;

    // Reduce health; kill when it reaches zero
    void takeDamage(int dmg)
    {
        health -= dmg;
        if (health <= 0) alive = false;
    }

    // Returns true if a player bullet hit this enemy
    bool collidesWithBullet(Bullet* b) const
    {
        if (!b || !b->isActive() || !alive) return false;
        float dx = x - b->getX(), dy = y - b->getY();
        return (dx * dx + dy * dy) < (70.f * 70.f); //hitbox
    }

    bool  isAlive() const { return alive; }
    float getX()    const { return x; }
    float getY()    const { return y; }
    void  empKill() { alive = false; } // Instant death from EMP

    virtual void setSpeedScale(float s) { speed *= s; }
};


//  Drone CLASS
//  Simplest enemy flies straight down,
//  shoots at regular intervals
//  1 hit point
class Drone : public Enemy
{
private:
    float shootTimer;    // Seconds since last shot
    float shootInterval; // Seconds between shots
    float baseSpeed;     // Default speed before wave scaling
    float baseInterval;  // Default shoot interval before wave scaling
    sf::Texture* tex;

public:
    Drone() : Enemy(0, 0, 1), shootTimer(0), shootInterval(2.f + (rand() % 30) / 10.f),
        baseSpeed(1.5f), baseInterval(2.f + (rand() % 30) / 10.f), tex(nullptr)
    {
        speed = 1.5f;
    }
    ~Drone() override {}

    void setTexture(sf::Texture* t) { tex = t; }
    int  getMaxHealth() override { return 1; }

    void spawnAt(float sx, float sy) override
    {
        Enemy::spawnAt(sx, sy); // Reset position and health
        shootTimer = 0; shootInterval = baseInterval;
    }

    // Scale speed and fire rate based on current survival wave
    void applyWaveScale(float speedMult, float fireRateMult)
    {
        speed = baseSpeed * speedMult;
        shootInterval = baseInterval / fireRateMult; // Higher mult = shorter interval = faster fire
    }

    // Move straight down; count time for shooting; deactivate below screen
    void update(float) override
    {
        if (!alive) return;
        y += speed;
        shootTimer += 0.016f;
        if (y > 1200) alive = false;
    }
    //simple sfml code for drawing the drone sprite on which our texture will be loaded
    void draw(sf::RenderWindow& window) override
    {
        if (!alive || !tex) return;
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(0.1f, 0.1f); sp.setRotation(360.f);
        sp.setPosition(x, y); window.draw(sp);
    }

    // Colliding with player damages and kills drone
    void onCollision() override { takeDamage(1); alive = false; }

    // Returns true when enough time has passed to fire again
    bool readyToShoot()
    {
        if (shootTimer >= shootInterval)
        {
            shootTimer = 0; shootInterval = baseInterval; return true;
        }
        return false;
    }
};


//  Viper CLASS
//  Enemy that moves in a side-to-side sine wave
//  while drifting downward.
//  1 hit points.

class Viper : public Enemy
{
private:
    float angle;         // Current phase of the sine wave (radians, increases each frame)
    float shootTimer;
    float shootInterval;
    float startX;        // Horizontal centre of the sine wave
    float baseSpeed;
    float baseInterval;
    sf::Texture* tex;

public:
    Viper() : Enemy(0, 0, 2), angle(0), shootTimer(0),
        shootInterval(2.5f + (rand() % 20) / 10.f),
        baseSpeed(1.2f),
        baseInterval(2.5f + (rand() % 20) / 10.f),
        startX(0), tex(nullptr)
    {
        speed = 1.2f;
    }
    ~Viper() override {}

    void setTexture(sf::Texture* t) { tex = t; }
    int  getMaxHealth() override { return 1; }

    void spawnAt(float sx, float sy) override
    {
        Enemy::spawnAt(sx, sy);
        startX = sx; angle = 0; shootTimer = 0; shootInterval = baseInterval;
    }

    void applyWaveScale(float speedMult, float fireRateMult)
    {
        speed = baseSpeed * speedMult;
        shootInterval = baseInterval / fireRateMult;
    }

    // Move downward; oscillate left/right using a sine wave
    void update(float) override
    {
        if (!alive) return;
        y += speed;
        angle += 0.05f; // Advance the wave phase
        x = startX + sinf(angle) * 360.f; // Horizontal position = sine of angle * amplitude
        shootTimer += 0.016f;
        if (y > 1200) alive = false;
    }

    void draw(sf::RenderWindow& window) override
    {
        if (!alive || !tex) return;
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(0.1f, 0.1f); sp.setRotation(180.f);
        sp.setPosition(x, y); window.draw(sp);
    }

    //colliding does one damage but does not kill 
    void onCollision() override { takeDamage(1); }

    bool readyToShoot()
    {
        if (shootTimer >= shootInterval)
        {
            shootTimer = 0; shootInterval = baseInterval; return true;
        }
        return false;
    }
};


//  Seeker CLASS
//  Homing enemy that locks on to the player's
//  X position when it spawns and accelerates
//  downward. 1 hit point.
class Seeker : public Enemy
{
private:
    float targetX;      // Player X at the time of spawn seeker locks in and destroys the player ship
    float acceleration; // Speed increases by this much every frame
    float baseSpeed;
    sf::Texture* tex;

public:
    Seeker() : Enemy(0, 0, 1), targetX(960.f), acceleration(0.1f), baseSpeed(5.f), tex(nullptr)
    {
        speed = 5.f;
    }
    ~Seeker() override {}

    void setTexture(sf::Texture* t) { tex = t; }
    int  getMaxHealth() override { return 1; }

    // Spawn with a known target X (player's position)
    void spawnAt(float sx, float sy, float px)
    {
        Enemy::spawnAt(sx, sy); targetX = px; speed = baseSpeed;
    }
    void spawnAt(float sx, float sy) override { spawnAt(sx, sy, 960.f); }

    void applyWaveScale(float speedMult, float)
    {
        baseSpeed = 5.f * speedMult; speed = baseSpeed;
    }

    // Slide horizontally toward targetX, accelerate downward
    void update(float) override
    {
        if (!alive) return;
        if (x < targetX) x += 8.f; // Steer right
        else if (x > targetX) x -= 8.f; // Steer left
        speed += acceleration;           // Get faster over time
        y += speed;
        if (y > 1200) alive = false;
    }
    //sfml code for drawing sprite later we will put texture
    void draw(sf::RenderWindow& window) override
    {
        if (!alive || !tex) return;
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(0.1f, 0.1f); sp.setRotation(180.f);
        sp.setPosition(x, y); window.draw(sp);
    }

    void onCollision() override { alive = false; } // Dies on contact immediately
};


//  Explosion CLASS
//  Plays a sprite-sheet animation at a location.
//  The sheet is a 5-column grid of frames.
//  Sound is played on the very first frame.
class Explosion : public GameObject
{
private:
    float x, y;
    bool  active;
    bool  soundPlayed;   // Prevents the sound from triggering more than once per explosion
    sf::Texture* tex;
    int   frame;        // Which frame of the animation we're on
    int   totalFrames;  // Total frames in the sprite sheet (5 cols × 5 rows = 25)
    float timer;        // Time accumulator for frame stepping
    float frameTime;    // Seconds per frame
    int   cols;         // How many columns in the sprite sheet
    int   frameW, frameH; // Pixel size of each frame in the sheet

public:
    Explosion() : x(0), y(0), active(false), soundPlayed(false), tex(nullptr),
        frame(0), totalFrames(25), timer(0),
        frameTime(0.05f), cols(5), frameW(130), frameH(130) {
    }

    void setTexture(sf::Texture* t) { tex = t; }

    // Activate at a position and reset the animation
    void spawn(float sx, float sy)
    {
        x = sx; y = sy; active = true; frame = 0; timer = 0;
        soundPlayed = false; // Allow sound to fire once
    }

    // Step through animation frames; play sound on frame 0
    void update(float) override
    {
        if (!active) return;
        if (!soundPlayed) { audio.playExplosion(); soundPlayed = true; } // First frame only
        timer += 0.016f;
        if (timer >= frameTime)
        {
            timer = 0; frame++;
            if (frame >= totalFrames) active = false; // Animation finished
        }
    }

    // Cut out the correct frame from the sprite sheet and draw it
    void draw(sf::RenderWindow& window) override
    {
        if (!active || !tex) return;
        int col = frame % cols;         // Which column this frame is in
        int row = frame / cols;         // Which row this frame is in
        sf::IntRect rect(col * frameW, row * frameH, frameW, frameH); // Crop rect
        sf::Sprite sp(*tex); sp.setTextureRect(rect);
        sp.setOrigin(frameW / 2.f, frameH / 2.f); // Centre pivot for clean positioning
        sp.setPosition(x, y); sp.setScale(1.5f, 1.5f);
        window.draw(sp);
    }

    void onCollision() override {}
    bool isActive() const { return active; }
};


// _____________________________________________
//  ShieldBreakParticle CLASS
//  One tiny glowing dot that flies outward when
//  the energy shield is destroyed.
//  32 of these are spawned at once (see EnergyShield).
// _____________________________________________
class ShieldBreakParticle
{
private:
    float x, y;      // Current position
    float vx, vy;    // Velocity (set at spawn, slows down via drag)
    float life;      // Seconds remaining
    float maxLife;   // Total lifetime (used to calculate fade ratio)
    float size;      // Initial radius in pixels
    bool  active;

public:
    ShieldBreakParticle()
        : x(0), y(0), vx(0), vy(0), life(0), maxLife(0), size(0), active(false) {
    }

    // Fire outward at a given angle and speed
    void spawn(float ox, float oy, float angle, float speed)
    {
        x = ox; y = oy;
        vx = cosf(angle) * speed;
        vy = sinf(angle) * speed;
        maxLife = life = 0.6f + (rand() % 20) / 40.f; // Slightly randomise lifespan
        size = 4.f + (rand() % 8);
        active = true;
    }

    // Move and apply drag; count down lifetime
    void update(float dt)
    {
        if (!active) return;
        x += vx; y += vy;
        vx *= 0.96f; vy *= 0.96f; // Drag slows particles down each frame
        life -= dt;
        if (life <= 0.f) active = false;
    }

    // Draw a circle that shrinks and fades as lifetime runs out
    void draw(sf::RenderWindow& window)
    {
        if (!active) return;
        float ratio = life / maxLife;              // 1.0 = fresh, 0.0 = about to die
        sf::Uint8 alpha = (sf::Uint8)(220 * ratio); // Fade out
        sf::CircleShape c(size * ratio);            // Shrink over time
        c.setOrigin(size * ratio, size * ratio);
        c.setPosition(x, y);
        c.setFillColor(sf::Color(80, 200, 255, alpha)); // Blue-white glow colour
        window.draw(c);
    }

    bool isActive() const { return active; }
};

// How many particles burst out when the shield breaks
const int MAX_SHIELD_PARTICLES = 32;


//  EMPDrop CLASS
//  A collectable item that falls from the top.
//  When the player touches it they gain one EMP
//  charge (up to 3)
class EMPDrop : public GameObject
{
private:
    float x, y, speed;
    float rotAngle; // Spinning angle for visual effect
    float bobTimer; // Time counter driving vertical bobbing (sine wave)
    bool  active;
    sf::Texture* tex;

public:
    EMPDrop() : x(0), y(0), speed(1.8f), rotAngle(0), bobTimer(0), active(false), tex(nullptr) {}
    ~EMPDrop() override {}

    void setTexture(sf::Texture* t) { tex = t; }

    void spawn(float sx, float sy)
    {
        x = sx; y = sy; active = true; rotAngle = 0; bobTimer = 0;
    }

    // Drift down, spin, and bob; deactivate below screen
    void update(float) override
    {
        if (!active) return;
        y += speed; rotAngle += 1.5f; bobTimer += 0.05f;
        if (y > 1200) active = false;
    }

    // Draw with a slow bob (sinf offset) and purple tint
    void draw(sf::RenderWindow& window) override
    {
        if (!active || !tex) return;
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        float sc = 60.f / b.width;
        sp.setScale(sc, sc); sp.setRotation(rotAngle);
        sp.setColor(sf::Color(200, 150, 255, 220));
        sp.setPosition(x, y + sinf(bobTimer) * 5.f); // Bob ±5 pixels
        window.draw(sp);
    }

    void onCollision() override { active = false; } // Collected – disappear
    bool  isActive() const { return active; }
    float getX()     const { return x; }
    float getY()     const { return y; }
};


//  PowerDrop CLASS
//  A collectable item that grants one of three
//  weapon upgrades:
//    type 0 = Spread shot 
//    type 1 = Laser weapon 
//    type 2 = Energy shield
class PowerDrop : public GameObject
{
private:
    float x, y, speed, rotAngle, bobTimer;
    bool  active;
    int   type;
    sf::Texture* texSpread;
    sf::Texture* texLaser;
    sf::Texture* texShield;

    // Returns the correct colour tint based on type
    sf::Color tintColor() const
    {
        if (type == 0) return sf::Color(255, 220, 80, 230); // Yellow
        if (type == 1) return sf::Color(80, 210, 255, 230); // Blue
        return              sf::Color(80, 255, 140, 230); // Green
    }

public:
    PowerDrop() : x(0), y(0), speed(2.f), rotAngle(0), bobTimer(0),
        active(false), type(0),
        texSpread(nullptr), texLaser(nullptr), texShield(nullptr) {
    }
    ~PowerDrop() override {}

    void setTextures(sf::Texture* s, sf::Texture* l, sf::Texture* sh)
    {
        texSpread = s; texLaser = l; texShield = sh;
    }

    void spawn(float sx, float sy, int t)
    {
        x = sx; y = sy; type = t; active = true; rotAngle = 0; bobTimer = 0;
    }

    void update(float) override
    {
        if (!active) return;
        y += speed; rotAngle += 1.2f; bobTimer += 0.05f;
        if (y > 1200) active = false;
    }

    // Draw the icon with spin + bob, plus an outline ring for visibility
    void draw(sf::RenderWindow& window) override
    {
        if (!active) return;
        sf::Texture* t = (type == 0) ? texSpread : (type == 1) ? texLaser : texShield;
        if (!t) return;
        sf::Sprite sp(*t);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        float sc = 58.f / b.width;
        sp.setScale(sc, sc); sp.setRotation(rotAngle); sp.setColor(tintColor());
        sp.setPosition(x, y + sinf(bobTimer) * 6.f);
        window.draw(sp);

        // Draw an outline ring around the icon so it stands out on busy backgrounds
        sf::CircleShape ring(34.f);
        ring.setOrigin(34.f, 34.f); ring.setPosition(x, y + sinf(bobTimer) * 6.f);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.5f);
        sf::Color rc = tintColor(); rc.a = 160;
        ring.setOutlineColor(rc); window.draw(ring);
    }

    void onCollision() override { active = false; }
    bool  isActive() const { return active; }
    int   getType()  const { return type; }
    float getX()     const { return x; }
    float getY()     const { return y; }
};


//  EnergyShield CLASS
//  The player's defensive bubble.
//  Absorbs 2 hits before breaking.
//  On break, spawns 32 particles flying outward.
class EnergyShield
{
private:
    int   hitsLeft;      // Remaining hits before shield breaks (starts at 2)
    bool  active;
    float pulseTimer;    // Drives the pulsing glow animation

    // Array of particles that burst out when the shield breaks
    ShieldBreakParticle particles[MAX_SHIELD_PARTICLES];
    bool particlesActive; // True while any particle is still flying

public:
    EnergyShield() : hitsLeft(0), active(false), pulseTimer(0), particlesActive(false) {}

    void activate() { hitsLeft = 2; active = true; } // First-time activation
    void reset() { hitsLeft = 2; }                // Re-collected while already active

    // Call this when the ship takes a hit.
    // Returns true if the shield absorbed the hit (so the ship doesn't lose HP).
    // If the shield breaks, spawns the particle burst at the ship position.
    bool absorbHit(float shipX, float shipY)
    {
        if (!active) return false;
        hitsLeft--;
        if (hitsLeft <= 0)
        {
            active = false;
            particlesActive = true;
            // Spawn one particle per slot, evenly spread around a full circle
            for (int i = 0; i < MAX_SHIELD_PARTICLES; i++)
            {
                float angle = (float)i / (float)MAX_SHIELD_PARTICLES * 2.f * 3.14159f;
                float speed = 4.f + (rand() % 60) / 10.f; // Random speed per particle
                particles[i].spawn(shipX, shipY, angle, speed);
            }
        }
        return true; // Shield took the hit
    }

    // Update the pulse timer and all live particles
    void update(float dt)
    {
        if (active) pulseTimer += 0.04f;
        if (particlesActive)
        {
            bool any = false;
            for (int i = 0; i < MAX_SHIELD_PARTICLES; i++)
            {
                particles[i].update(dt);
                if (particles[i].isActive()) any = true;
            }
            if (!any) particlesActive = false; // All particles done
        }
    }

    // Draw the bubble (if active) and any live burst particles
    void draw(sf::RenderWindow& window, float shipX, float shipY)
    {
        // Draw burst particles even after the shield is gone (they keep flying)
        if (particlesActive)
            for (int i = 0; i < MAX_SHIELD_PARTICLES; i++)
                particles[i].draw(window);

        if (!active) return;

        // Full shield = brighter; damaged shield = dimmer
        sf::Uint8 baseAlpha = (hitsLeft == 2) ? 200 : 100;
        float pulse = 0.85f + 0.15f * sinf(pulseTimer); // Gentle brightness oscillation

        // Outer glowing ring
        float r = 55.f;
        sf::CircleShape outer(r);
        outer.setOrigin(r, r); outer.setPosition(shipX, shipY);
        outer.setFillColor(sf::Color::Transparent);
        outer.setOutlineThickness(6.f * pulse);
        outer.setOutlineColor(sf::Color(80, 200, 255, (sf::Uint8)(baseAlpha * pulse)));
        window.draw(outer);

        // Inner semi-transparent fill
        float r2 = 42.f;
        sf::CircleShape inner(r2);
        inner.setOrigin(r2, r2); inner.setPosition(shipX, shipY);
        inner.setFillColor(sf::Color(60, 180, 255, (sf::Uint8)(30 * pulse)));
        inner.setOutlineThickness(2.f);
        inner.setOutlineColor(sf::Color(140, 230, 255, (sf::Uint8)(baseAlpha * 0.6f * pulse)));
        window.draw(inner);
    }

    bool isActive() const { return active; }
    int  getHits()  const { return hitsLeft; }
};


//  EMPWave CLASS
//  The expanding ring fired when the player uses
//  an EMP. It kills all enemies on screen when
//  its radius passes 150 px.
class EMPWave
{
private:
    float x, y;        // Origin (player position when fired)
    float radius;      // Current radius in pixels
    float maxRadius;   // How wide it grows before disappearing
    float speed;       // Pixels of radius growth per frame
    bool  active;
    bool  hasKilled;   // Prevents the kill trigger from firing more than once
    sf::Texture* tex;

public:
    EMPWave() : x(960.f), y(540.f), radius(0.f), maxRadius(1400.f), speed(18.f),
        active(false), hasKilled(false), tex(nullptr) {
    }

    void setTexture(sf::Texture* t) { tex = t; }

    // Activate at the player's current position
    void activate(float px, float py)
    {
        x = px; y = py; radius = 0.f; active = true; hasKilled = false;
    }

    // Expand the ring each frame.
    // Returns true exactly once – the moment the ring is big enough to kill enemies.
    bool update()
    {
        if (!active) return false;
        radius += speed;
        if (radius >= maxRadius) { active = false; return false; } // Ring fully expanded – done
        if (!hasKilled && radius >= 150.f) { hasKilled = true; return true; } // Trigger kill!
        return false;
    }

    // Draw the expanding ring, fading out as it grows
    void draw(sf::RenderWindow& window)
    {
        if (!active || !tex) return;
        float progress = radius / maxRadius;
        sf::Uint8 alpha = (sf::Uint8)(255 * (1.f - progress)); // Fade to transparent
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        float sc = (radius * 2.f) / b.width; // Scale so sprite diameter matches ring diameter
        sp.setScale(sc, sc); sp.setColor(sf::Color(180, 100, 255, alpha));
        sp.setPosition(x, y); window.draw(sp);
    }

    bool isActive() const { return active; }
};


//  BOSS SYSTEM
//  Three unique boss cruiser and twin tong and mother classes all inherit from
//  the Boss base class, which handles the shared
//  logic: health, health bar, entry animation.

class Boss
{
protected:
    float x, y;           // Position (always centred horizontally to start)
    int   health, maxHealth;
    bool  alive;
    float damageCooldown; // Brief window after being hit where further hits are ignored
    bool  entering;       // True while boss is sliding into view from off-screen top
    float targetY;        // Y position the boss stops at after entry
    float entrySpeed;     // Pixels per second during entry slide

public:
    Boss(int maxHp) : x(960.f), y(-200.f), health(maxHp), maxHealth(maxHp),
        alive(false), damageCooldown(0.f),
        entering(false), targetY(150.f), entrySpeed(BOSS_ENTRY_SPEED) {
    }
    virtual ~Boss() {}

    // Reset stats and start the entry animation from above the screen
    virtual void spawn()
    {
        alive = true; health = maxHealth; damageCooldown = 0.f;
        y = -200.f; entering = true;
    }

    // Slide boss downward until it reaches targetY
    void updateEntry(float dt)
    {
        if (entering)
        {
            y += entrySpeed * dt;
            if (y >= targetY) { y = targetY; entering = false; }
        }
    }

    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    // Generic bullet collision – 80 px radius circle
    virtual bool collidesWithBullet(Bullet* b) const
    {
        if (!b || !b->isActive() || !alive) return false;
        float dx = x - b->getX(), dy = y - b->getY();
        return (dx * dx + dy * dy) < (80.f * 80.f);
    }

    // Radius used to check if the player ship has crashed into the boss body
    virtual float getCollisionRadius() const { return 90.f; }

    // Reduce health; apply brief invincibility so rapid hits don't double-count
    virtual bool takeDamage(int dmg)
    {
        if (!alive || damageCooldown > 0.f || entering) return false;
        health -= dmg; damageCooldown = 0.15f;
        if (health <= 0) { health = 0; alive = false; }
        return true;
    }

    // Draw the red/green HP bar at the top of the screen
    void drawHealthBar(sf::RenderWindow& window, const std::string& label = "BOSS")
    {
        if (!alive) return;
        float barW = 600.f, barH = 22.f;
        float barX = (1920.f - barW) / 2.f, barY = 18.f; // Centred at the top
        float ratio = (float)health / (float)maxHealth;

        // Dark background track
        sf::RectangleShape bg(sf::Vector2f(barW, barH));
        bg.setPosition(barX, barY);
        bg.setFillColor(sf::Color(40, 10, 10, 220));
        bg.setOutlineColor(sf::Color(180, 60, 60, 200));
        bg.setOutlineThickness(2.f); window.draw(bg);

        // Coloured fill that transitions red→green as health goes 0→full
        sf::Uint8 r = (sf::Uint8)(255 * (1.f - ratio));
        sf::Uint8 g = (sf::Uint8)(255 * ratio);
        sf::RectangleShape fill(sf::Vector2f(barW * ratio, barH));
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(r, g, 30, 230)); window.draw(fill);

        // End caps and quarter-segment markers for readability
        sf::RectangleShape cap(sf::Vector2f(4.f, barH + 8.f));
        cap.setOrigin(2.f, 4.f); cap.setPosition(barX, barY);
        cap.setFillColor(sf::Color(255, 80, 80, 200)); window.draw(cap);
        cap.setPosition(barX + barW, barY); window.draw(cap);
        for (int seg = 1; seg < 4; seg++)
        {
            sf::RectangleShape sl(sf::Vector2f(2.f, barH));
            sl.setPosition(barX + barW * 0.25f * seg, barY);
            sl.setFillColor(sf::Color(0, 0, 0, 120)); window.draw(sl);
        }
        (void)label; // Label parameter reserved for future use
    }

    bool  isAlive()    const { return alive; }
    bool  isEntering() const { return entering; }
    float getX()       const { return x; }
    float getY()       const { return y; }
    int   getHealth()  const { return health; }
    int   getMaxHp()   const { return maxHealth; }
};


//  Lightning drawing helpers
//  Used by the Cruiser boss to draw vertical
//  and horizontal laser stripe effects.

// Draw a vertical stripe of the lightning texture spanning full screen height
static void drawLightningStripe(sf::RenderWindow& window, sf::Texture* lightningTex,
    float xCenter, float alpha, float width = 36.f)
{
    if (!lightningTex) return;
    sf::Sprite sp(*lightningTex);
    sf::FloatRect b = sp.getLocalBounds();
    sp.setOrigin(b.width / 2.f, 0.f);
    float scaleX = width / b.width, scaleY = 1080.f / b.height;
    sp.setScale(scaleX, scaleY);
    sp.setPosition(xCenter, 0.f);
    sp.setColor(sf::Color(255, 255, 255, (sf::Uint8)alpha));
    window.draw(sp);
}

// Draw a horizontal stripe of the lightning texture
static void drawLightningHStripe(sf::RenderWindow& window, sf::Texture* lightningTex,
    float yCenterPos, float alpha, float height = 55.f)
{
    if (!lightningTex) return;
    sf::Sprite sp(*lightningTex);
    sf::FloatRect b = sp.getLocalBounds();
    sp.setOrigin(b.width / 2.f, b.height / 2.f);
    float scaleX = height / b.width, scaleY = 1920.f / b.height;
    sp.setScale(scaleX, scaleY); sp.setRotation(90.f);
    sp.setPosition(960.f, yCenterPos);
    sp.setColor(sf::Color(200, 160, 255, (sf::Uint8)alpha));
    window.draw(sp);
}


//  BossCruiser CLASS  (Level 1 boss)
//  Moves left and right at the top of the screen.
//  Periodically fires 7 vertical laser columns,
//  leaving exactly one safe gap the player must
//  dodge into. Warning phase flashes red before
//  the lasers become lethal.
class BossCruiser : public Boss
{
private:
    float moveDir, moveSpeed; // Horizontal drift direction (+1 or -1) and speed

    static const int NUM_LASERS = 8; // 8 evenly-spaced laser slots, 1 is always safe
    float* laserX;      // X position of each laser column
    bool* laserActive; // whether each laser slot is active (true = deadly)
    int    safeGap;     // Index of the one inactive (safe) laser slot

    float laserWarningTimer; // Counts down warning phase
    float laserActiveTimer;  // Counts down firing phase
    bool  laserWarning;      // True during the flashing warning phase
    bool  laserFiring;       // True while lasers are actually lethal
    float patternTimer;      // Time since last laser pattern started

    // Durations and interval as class-level constants
    static const float WARNING_DURATION;
    static const float FIRING_DURATION;
    static const float PATTERN_INTERVAL;

    sf::Texture* tex;
    sf::Texture* lightningTex;
    float bobTimer; // Drives gentle up/down bobbing while stationary

    // Choose a new random safe gap and activate all other laser slots
    void newPattern()
    {
        float spacing = 1920.f / NUM_LASERS;
        for (int i = 0; i < NUM_LASERS; i++)
        {
            laserX[i] = spacing * 0.5f + spacing * i; // Evenly spaced across screen
            laserActive[i] = true;
        }
        safeGap = rand() % NUM_LASERS; // Random safe lane
        laserActive[safeGap] = false;
        laserWarning = true; laserFiring = false;
        laserWarningTimer = WARNING_DURATION; laserActiveTimer = 0.f;
    }

public:
    BossCruiser() : Boss(50), moveDir(1.f), moveSpeed(2.5f),
        laserWarningTimer(0.f), laserActiveTimer(0.f),
        laserWarning(false), laserFiring(false), patternTimer(0.f), safeGap(0),
        tex(nullptr), lightningTex(nullptr), bobTimer(0.f)
    {
        targetY = 140.f;
        // Allocate heap arrays for laser positions and states
        laserX = new float[NUM_LASERS];
        laserActive = new bool[NUM_LASERS];
        for (int i = 0; i < NUM_LASERS; i++)
        {
            laserX[i] = 0.f;
            laserActive[i] = false;
        }
    }

    // Destructor frees heap arrays to prevent memory leak
    ~BossCruiser() override { delete[] laserX; delete[] laserActive; }

    void setTexture(sf::Texture* t) { tex = t; }
    void setLightningTexture(sf::Texture* t) { lightningTex = t; }

    void spawn() override
    {
        Boss::spawn(); x = 960.f; moveDir = 1.f;
        laserWarning = false; laserFiring = false;
        patternTimer = PATTERN_INTERVAL * 0.5f; // Start mid-interval so first laser isn't immediate
        bobTimer = 0.f;
    }

    // Move left/right, manage laser pattern phases
    void update(float dt) override
    {
        if (!alive) return;
        if (damageCooldown > 0.f) damageCooldown -= dt;
        updateEntry(dt); // Slide in from off-screen
        if (entering) return; // Don't do anything else while still entering

        bobTimer += dt;

        // Drift left and right, bounce off screen edges
        x += moveDir * moveSpeed;
        if (x > 1780.f) { x = 1780.f; moveDir = -1.f; }
        if (x < 140.f) { x = 140.f;  moveDir = 1.f; }

        // Count up to next laser pattern
        if (!laserWarning && !laserFiring)
        {
            patternTimer += dt;
            if (patternTimer >= PATTERN_INTERVAL)
            {
                patternTimer = 0.f; newPattern();
            }
        }

        // Warning → Firing transition
        if (laserWarning)
        {
            laserWarningTimer -= dt;
            if (laserWarningTimer <= 0.f) { laserWarning = false; laserFiring = true; laserActiveTimer = FIRING_DURATION; }
        }

        // Firing → Idle transition
        if (laserFiring)
        {
            laserActiveTimer -= dt;
            if (laserActiveTimer <= 0.f) laserFiring = false;
        }
    }

    // Returns true if the ship's X position overlaps any active (lethal) laser column
    bool playerInLaser(float shipX) const
    {
        if (!laserFiring) return false;
        float halfW = 18.f; // half width of each laser column
        for (int i = 0; i < NUM_LASERS; i++)
        {
            if (!laserActive[i]) continue;
            if (shipX >= laserX[i] - halfW && shipX <= laserX[i] + halfW) return true;
        }
        return false;
    }

    // Draw: warning stripes, firing lightning columns, then boss sprite
    void draw(sf::RenderWindow& window) override
    {
        if (!alive) return;

        // Flashing red warning strips before lasers fire
        if (laserWarning && !entering)
        {
            float alpha = 100.f + 155.f * (sinf(laserWarningTimer * 20.f) * 0.5f + 0.5f);
            for (int i = 0; i < NUM_LASERS; i++)
            {
                if (!laserActive[i]) continue;
                sf::RectangleShape warn(sf::Vector2f(36.f, 1080.f));
                warn.setOrigin(18.f, 0.f); warn.setPosition(laserX[i], 0.f);
                warn.setFillColor(sf::Color(255, 40, 40, (sf::Uint8)alpha));
                window.draw(warn);
            }
        }

        // Lethal lightning columns
        if (laserFiring && !entering)
        {
            for (int i = 0; i < NUM_LASERS; i++)
            {
                if (!laserActive[i]) continue;
                if (lightningTex) drawLightningStripe(window, lightningTex, laserX[i], 220.f, 220.f);
                else
                {
                    // Fallback plain rectangle
                    sf::RectangleShape core(sf::Vector2f(10.f, 1080.f));
                    core.setOrigin(5.f, 0.f); core.setPosition(laserX[i], 0.f);
                    core.setFillColor(sf::Color(80, 220, 255, 230)); window.draw(core);
                }
            }
        }

        // Boss sprite (red flash when recently hit, gentle bob when idle)
        if (tex)
        {
            sf::Sprite sp(*tex);
            sf::FloatRect b = sp.getLocalBounds();
            sp.setOrigin(b.width / 2.f, b.height / 2.f);
            float sc = 500.f / b.width; sp.setScale(sc, sc); sp.setRotation(180.f);
            if (damageCooldown > 0.f) sp.setColor(sf::Color(255, 200, 200, 255)); // Damage flash
            sp.setPosition(x, y + (entering ? 0.f : sinf(bobTimer * 1.5f) * 8.f));
            window.draw(sp);
        }
        else
        {
            // Fallback rectangle if texture missing
            sf::RectangleShape body(sf::Vector2f(220.f, 70.f));
            body.setOrigin(110.f, 35.f); body.setPosition(x, y);
            sf::Uint8 flash = (damageCooldown > 0.f) ? 255 : 180;
            body.setFillColor(sf::Color(flash, 30, 30, 230)); window.draw(body);
        }

        drawHealthBar(window); // Always draw HP bar while alive
    }

    bool isLaserFiring()  const { return laserFiring; }
    bool isLaserWarning() const { return laserWarning; }
};
// Static constant definitions (must live outside the class body in C++)
const float BossCruiser::WARNING_DURATION = 1.2f;
const float BossCruiser::FIRING_DURATION = 1.8f;
const float BossCruiser::PATTERN_INTERVAL = 3.5f;


//  Turret CLASS
//  A destructible sub-component of BossTwinCannons.
//  Two turrets are attached to the boss body.
//  While either turret is alive, the boss core
//  is immune to damage. Each turret shoots a
//  3-way spread.
class Turret
{
private:
    float relX;   // X relative to boss core e.g -240 for left turret
    float absX, absY; // Absolute position updated every frame from boss core position
    int   health, maxHealth;
    bool  alive;
    float shootTimer, shootInterval; // Time tracking for shooting
    float damageCooldown;
    sf::Texture* texBarrel; // Gun barrel sprite
    sf::Texture* texBody;   // Turret base sprite

public:
    Turret(float relativeX, int hp = 10)
        : relX(relativeX), absX(0), absY(0), health(hp), maxHealth(hp), alive(false),
        shootTimer(0.f), shootInterval(2.f + (rand() % 20) / 10.f), damageCooldown(0.f),
        texBarrel(nullptr), texBody(nullptr) {
    }

    void setTextures(sf::Texture* barrel, sf::Texture* body) { texBarrel = barrel; texBody = body; }

    // Activate from boss core position
    void spawn(float coreX, float coreY)
    {
        alive = true; health = maxHealth; damageCooldown = 0.f;
        absX = coreX + relX; absY = coreY;
    }

    // Follow the boss core position every frame
    void update(float dt, float coreX, float coreY)
    {
        if (!alive) return;
        absX = coreX + relX; absY = coreY; // Stay attached to boss
        if (damageCooldown > 0.f) damageCooldown -= dt;
        shootTimer += dt;
    }

    // Returns true when shoot interval has elapsed; resets timer
    bool readyToShoot()
    {
        if (!alive) return false;
        if (shootTimer >= shootInterval)
        {
            shootTimer = 0; shootInterval = 2.f + (rand() % 20) / 10.f; return true;
        }
        return false;
    }

    // Apply damage; brief invincibility prevents double-counting
    bool takeDamage(int dmg)
    {
        if (!alive || damageCooldown > 0.f) return false;
        health -= dmg; damageCooldown = 0.12f;
        if (health <= 0) { health = 0; alive = false; }
        return true;
    }

    // Returns true if a player bullet overlaps this turret
    bool collidesWithBullet(Bullet* b) const
    {
        if (!alive || !b || !b->isActive()) return false;
        float dx = absX - b->getX(), dy = absY - b->getY();
        return (dx * dx + dy * dy) < (55.f * 55.f);
    }

    // Draw barrel, body, and a small HP bar
    void draw(sf::RenderWindow& window)
    {
        if (!alive) return;
        sf::Uint8 flash = (damageCooldown > 0.f) ? 255 : 200; // White flash on hit

        if (texBarrel)
        {
            sf::Sprite bs(*texBarrel);
            sf::FloatRect bb = bs.getLocalBounds();
            bs.setOrigin(bb.width / 2.f, bb.height / 2.f);
            bs.setScale(100.f / bb.width, 50.f / bb.height);
            bs.setRotation(180.f); bs.setPosition(absX, absY + 40.f);
            bs.setColor(sf::Color(flash, flash, flash, 230)); window.draw(bs);
        }
        if (texBody)
        {
            sf::Sprite bs(*texBody);
            sf::FloatRect bb = bs.getLocalBounds();
            bs.setOrigin(bb.width / 2.f, bb.height / 2.f);
            float sc = 80.f / bb.width; bs.setScale(sc, sc);
            bs.setPosition(absX, absY);
            bs.setColor(sf::Color(flash, flash, flash, 220)); window.draw(bs);
        }

        // HP bar above turret
        float barW = 60.f, barH = 6.f;
        sf::RectangleShape bg(sf::Vector2f(barW, barH));
        bg.setOrigin(barW / 2.f, 0.f); bg.setPosition(absX, absY - 46.f);
        bg.setFillColor(sf::Color(40, 10, 10, 200)); window.draw(bg);

        float ratio = (float)health / (float)maxHealth;
        sf::RectangleShape fill(sf::Vector2f(barW * ratio, barH));
        fill.setOrigin(barW / 2.f, 0.f); fill.setPosition(absX, absY - 46.f);
        fill.setFillColor(sf::Color(255, (sf::Uint8)(200 * ratio), 30, 220)); window.draw(fill);
    }

    bool  isAlive()   const { return alive; }
    float getX()      const { return absX; }
    float getY()      const { return absY; }
    int   getHealth() const { return health; }
    int   getMaxHp()  const { return maxHealth; }
};


//  BossTwinCannons CLASS  (Level 2 boss)
//  Has two Turret sub-objects left and right.
//  The boss core is immune until both turrets
//  are destroyed. Bobs up and down, drifts
//  sideways, and fires 3-way spreads from turrets.
//
//  Object Composition: Turret objects are direct
//  data members of this class, not pointers.
//  This is true composition – the turrets live
//  inside BossTwinCannons and are destroyed with it.
class BossTwinCannons : public Boss
{
private:
    // ── COMPOSITION: Turret objects are direct data members ──
    // They are constructed in the initializer list, live inside
    // BossTwinCannons, and are automatically destroyed with it.
    Turret leftTurret;   // Left turret: 240 px to the left of core
    Turret rightTurret;  // Right turret: 240 px to the right of core

    float bobTimer, moveDir, moveSpeed;
    bool  coreImmune; // True while at least one turret is alive

    sf::Texture* texArm;        // Connector arm between core and turrets
    sf::Texture* tex;           // Core body sprite
    sf::Texture* lightningTex;

public:
    // Turret objects are initialised directly in the initializer list (composition)
    BossTwinCannons() : Boss(20) /*Boss HP*/,
        leftTurret(-240.f, 50),   // Constructed as a direct member
        rightTurret(240.f, 50),   // Constructed as a direct member
        bobTimer(0.f), moveDir(1.f), moveSpeed(0.8f),
        coreImmune(true), tex(nullptr), lightningTex(nullptr), texArm(nullptr)
    {
        targetY = 160.f;
    }

    // No manual delete needed – Turret members are destroyed automatically
    ~BossTwinCannons() override {}

    void setTexture(sf::Texture* t) { tex = t; }
    void setLightningTexture(sf::Texture* t) { lightningTex = t; }
    void setTurretTextures(sf::Texture* barrel, sf::Texture* body)
    {
        leftTurret.setTextures(barrel, body);
        rightTurret.setTextures(barrel, body);
    }

    void spawn() override
    {
        Boss::spawn(); x = 960.f; bobTimer = 0.f; coreImmune = true;
        leftTurret.spawn(x, targetY);   // Turrets start at boss's initial position
        rightTurret.spawn(x, targetY);
    }

    void update(float dt) override
    {
        if (!alive) return;
        if (damageCooldown > 0.f) damageCooldown -= dt;
        bobTimer += dt;
        updateEntry(dt);

        // Only move once entry is complete
        if (!entering)
        {
            x += moveDir * moveSpeed;
            if (x > 1700.f) { x = 1700.f; moveDir = -1.f; }
            if (x < 220.f) { x = 220.f;  moveDir = 1.f; }
        }

        // Bob slowly up and down using a sine wave
        y = targetY + sinf(bobTimer * 0.8f) * 20.f;

        // Update turrets so they follow the boss core
        leftTurret.update(dt, x, y);
        rightTurret.update(dt, x, y);

        // Core is only vulnerable when both turrets are down
        coreImmune = (leftTurret.isAlive() || rightTurret.isAlive());
    }

    // Override: core can't be hurt while coreImmune or still entering
    bool takeDamage(int dmg) override
    {
        if (coreImmune || entering) return false;
        return Boss::takeDamage(dmg);
    }

    // Check if a bullet hit either turret; apply damage and return which one (1=left, 2=right, 0=none)
    int hitTurret(Bullet* b)
    {
        if (leftTurret.collidesWithBullet(b)) { leftTurret.takeDamage(1);  return 1; }
        if (rightTurret.collidesWithBullet(b)) { rightTurret.takeDamage(1); return 2; }
        return 0;
    }

    // Expose per-turret shoot timers to Game so it can decide when to fire spread bullets
    bool leftTurretShoot() { return leftTurret.readyToShoot(); }
    bool rightTurretShoot() { return rightTurret.readyToShoot(); }

    void draw(sf::RenderWindow& window) override
    {
        if (!alive) return;

        // Draw turrets first (they're behind the core visually)
        leftTurret.draw(window);
        rightTurret.draw(window);

        // Draw connector arms between core and turrets
        for (int side = -1; side <= 1; side += 2)
        {
            if (texArm)
            {
                sf::Sprite as(*texArm);
                sf::FloatRect ab = as.getLocalBounds();
                as.setOrigin(ab.width / 2.f, ab.height / 2.f);
                as.setScale(190.f / ab.width, 20.f / ab.height);
                as.setPosition(x + (side > 0 ? 120.f : -120.f), y);
                as.setColor(sf::Color(200, 200, 200, 255)); window.draw(as);
            }
        }

        // Draw boss core (flashes red when hit; dimmer when immune)
        if (tex)
        {
            sf::Sprite sp(*tex);
            sf::FloatRect b = sp.getLocalBounds();
            sp.setOrigin(b.width / 2.f, b.height / 2.f);
            float sc = 500.f / b.width; sp.setScale(sc, sc); sp.setRotation(270.f);
            if (damageCooldown > 0.f && !coreImmune) sp.setColor(sf::Color(255, 200, 200, 255));
            else if (coreImmune)                      sp.setColor(sf::Color(255, 255, 255, 200));
            sp.setPosition(x, y); window.draw(sp);
        }
        else
        {
            // Fallback circle
            sf::Uint8 flash = (damageCooldown > 0.f) ? 255 : 180;
            sf::CircleShape core(60.f);
            core.setOrigin(60.f, 60.f); core.setPosition(x, y);
            core.setFillColor(sf::Color(flash, coreImmune ? 30 : 120, coreImmune ? 30 : 30, 230));
            core.setOutlineColor(coreImmune ? sf::Color(255, 60, 60, 200) : sf::Color(255, 200, 40, 220));
            core.setOutlineThickness(4.f); window.draw(core);
        }

        // Pulsing immune ring around core while turrets are alive
        if (coreImmune)
        {
            float pulse = 0.6f + 0.4f * sinf(bobTimer * 4.f);
            sf::CircleShape sr(70.f * pulse);
            sr.setOrigin(70.f * pulse, 70.f * pulse); sr.setPosition(x, y);
            sr.setFillColor(sf::Color::Transparent);
            sr.setOutlineThickness(3.f);
            sr.setOutlineColor(sf::Color(255, 80, 80, (sf::Uint8)(150 * pulse)));
            window.draw(sr);
        }

        drawHealthBar(window);
    }

    void    setArmTexture(sf::Texture* t) { texArm = t; }
    bool    isCoreImmune()    const { return coreImmune; }
    // Return address-of the composed member (not a heap pointer)
    Turret* getLeftTurret() { return &leftTurret; }
    Turret* getRightTurret() { return &rightTurret; }
};


//  BossMothership CLASS  (Level 3 boss)
//  Stationary at the top. Periodically fires a
//  wide downward cone laser. Also launches
//  Seeker enemies at the player periodically.
//  Most dangerous of the three bosses.
class BossMothership : public Boss
{
private:
    float seekerTimer, seekerInterval; // How often seekers are launched

    // Cone laser phase tracking
    float laserWarningTimer;  // Counts down the warning phase
    float laserFiringTimer;   // Counts down the firing phase
    float laserPatternTimer;  // Time since last laser cycle

    bool  laserWarning; // True during warning phase (triangle flashing)
    bool  laserFiring;  // True during firing phase (cone is lethal)

    // Durations as class constants
    static const float LASER_WARNING_DURATION;
    static const float LASER_FIRING_DURATION;
    static const float LASER_PATTERN_INTERVAL;
    static const float SEEKER_INTERVAL;

    // Half-angle of the cone in radians (0.80 rad ≈ 46°)
    static const float CONE_HALF_ANGLE;

    sf::Texture* tex;
    sf::Texture* lightningTex; // Kept for possible future use
    sf::Texture* coneTex;      // Texture used to draw the cone laser effect

public:
    BossMothership()
        : Boss(100),
        seekerTimer(0.f), seekerInterval(SEEKER_INTERVAL),
        laserWarningTimer(0.f), laserFiringTimer(0.f), laserPatternTimer(0.f),
        laserWarning(false), laserFiring(false),
        tex(nullptr), lightningTex(nullptr), coneTex(nullptr)
    {
        targetY = 140.f;
    }

    ~BossMothership() override {}

    void setTexture(sf::Texture* t) { tex = t; }
    void setLightningTexture(sf::Texture* t) { lightningTex = t; }
    void setConeTexture(sf::Texture* t) { coneTex = t; }

    void spawn() override
    {
        Boss::spawn(); x = 960.f;
        laserWarning = false; laserFiring = false;
        laserPatternTimer = LASER_PATTERN_INTERVAL * 0.6f; // First laser slightly delayed
        seekerTimer = 0.f;
    }

    // Returns true when it's time to launch a new seeker
    bool wantsSeeker(float dt)
    {
        if (!alive || entering) return false;
        seekerTimer += dt;
        if (seekerTimer >= seekerInterval) { seekerTimer = 0.f; return true; }
        return false;
    }

    // Manage laser pattern
    void update(float dt) override
    {
        if (!alive) return;
        if (damageCooldown > 0.f) damageCooldown -= dt;
        updateEntry(dt);
        if (entering) return; // Wait until fully entered

        // Count up to next laser cycle
        if (!laserWarning && !laserFiring)
        {
            laserPatternTimer += dt;
            if (laserPatternTimer >= LASER_PATTERN_INTERVAL)
            {
                laserPatternTimer = 0.f; startLaserPattern();
            }
        }

        // Warning → Firing transition
        if (laserWarning)
        {
            laserWarningTimer -= dt;
            if (laserWarningTimer <= 0.f)
            {
                laserWarning = false; laserFiring = true; laserFiringTimer = LASER_FIRING_DURATION;
            }
        }

        // Firing → Idle transition
        if (laserFiring)
        {
            laserFiringTimer -= dt;
            if (laserFiringTimer <= 0.f) laserFiring = false;
        }
    }

    // Start the warning phase of the laser cycle
    void startLaserPattern()
    {
        laserWarning = true; laserFiring = false;
        laserWarningTimer = LASER_WARNING_DURATION; laserFiringTimer = 0.f;
    }

    // Returns true if the player's position is inside the firing cone
    bool playerInConeLaser(float shipX, float shipY) const
    {
        if (!laserFiring || entering) return false;
        float dx = shipX - x, dy = shipY - y;
        if (dy < 0.f) return false; // Above the boss – safe
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1.f) return true; // Right at the tip
        float angle = fabsf(atan2f(dx, dy)); // Angle from straight down
        return angle < CONE_HALF_ANGLE; // Inside the cone's spread
    }

    void draw(sf::RenderWindow& window) override
    {
        if (!alive) return;

        // ── Warning triangle (semi-transparent, flashes) ──
        if (laserWarning && !entering)
        {
            float alpha = 60.f + 100.f * (sinf(laserWarningTimer * 18.f) * 0.5f + 0.5f);
            float bottomY = 1080.f;
            float height = bottomY - y;
            float halfWidth = height * tanf(CONE_HALF_ANGLE); // Width of cone at bottom

            sf::ConvexShape cone;
            cone.setPointCount(3);
            cone.setPoint(0, sf::Vector2f(x, y));                          // Tip at boss
            cone.setPoint(1, sf::Vector2f(x - halfWidth, bottomY));        // Bottom left
            cone.setPoint(2, sf::Vector2f(x + halfWidth, bottomY));        // Bottom right
            cone.setFillColor(sf::Color(255, 80, 20, (sf::Uint8)alpha));
            window.draw(cone);
        }

        // ── Firing cone (lethal, drawn with cone texture) ──
        if (laserFiring && !entering)
        {
            float bottomY = 1080.f;
            float height = bottomY - y;
            float halfWidth = height * tanf(CONE_HALF_ANGLE);

            if (coneTex)
            {
                sf::Sprite sp(*coneTex);
                sf::FloatRect b = sp.getLocalBounds();
                sp.setOrigin(b.width / 2.f, 0.f); // Anchor to top-centre (tip of cone)
                // Scale to exactly cover the cone triangle
                float scaleX = (2.f * halfWidth) / b.width;
                float scaleY = height / b.height;
                sp.setScale(scaleX, scaleY);
                sp.setPosition(x, y);
                sp.setColor(sf::Color(255, 255, 255, 210));
                window.draw(sp);
            }
        }

        // Boss sprite
        if (tex)
        {
            sf::Sprite sp(*tex);
            sf::FloatRect b = sp.getLocalBounds();
            sp.setOrigin(b.width / 2.f, b.height / 2.f);
            float sc = 1000.f / b.width; sp.setScale(sc, sc); sp.setRotation(360.f);
            if (damageCooldown > 0.f) sp.setColor(sf::Color(255, 200, 200, 255));
            sp.setPosition(x, y); window.draw(sp);
        }


        drawHealthBar(window);
    }

    bool  isLaserFiring()    const { return laserFiring; }
    bool  isLaserWarning()   const { return laserWarning; }
    float getConeHalfAngle() const { return CONE_HALF_ANGLE; }
};
// Static constant definitions
const float BossMothership::LASER_WARNING_DURATION = 3.0f;
const float BossMothership::LASER_FIRING_DURATION = 5.0f;
const float BossMothership::LASER_PATTERN_INTERVAL = 10.0f;
const float BossMothership::SEEKER_INTERVAL = 4.0f;
const float BossMothership::CONE_HALF_ANGLE = 0.80f;


//  Ship CLASS
//  The player-controlled spaceship.
//  Handles movement, dashing, all weapon modes,
//  the energy shield, and damage / invincibility.
class Ship
{
private:
    float x, y, speed;  // Position and movement speed per frame
    int   health;        // Current HP (max 3 = MAX_HEARTS)
    bool  alive;
    sf::Texture* tex;

    float dashCooldownTimer; // Counts up toward DASH_COOLDOWN
    float invincibleTimer;   // Counts down toward 0 when invincible
    bool  invincible;        // True = ignore all incoming damage
    int   empCount;          // EMPs currently held
    WeaponMode weaponMode;   // Current weapon (NORMAL / SPREAD / LASER)
    float weaponTimer;       // Seconds of power-up time remaining

    EnergyShield shield;     // The bubble shield subsystem

    // Laser weapon has its own active/cooldown sub-cycle within weaponTimer
    float laserWeaponActiveTimer;   // Seconds laser can still fire
    float laserWeaponCooldownTimer; // Seconds until laser can fire again
    bool  laserWeaponOnCooldown;
    float damageFlashTimer; // Counts down; ship is red while > 0

public:
    Ship() : x(960.f), y(800.f), speed(5.f), health(3), alive(true), tex(nullptr),
        dashCooldownTimer(DASH_COOLDOWN), invincibleTimer(0.f), invincible(false),
        empCount(1), weaponMode(WeaponMode::NORMAL), weaponTimer(0.f),
        laserWeaponActiveTimer(0.f), laserWeaponCooldownTimer(0.f),
        laserWeaponOnCooldown(false), damageFlashTimer(0.f) {
    }

    void setTexture(sf::Texture* t) { tex = t; }

    // Movement
    void moveLeft() { if (x > 0)    x -= speed; }
    void moveRight() { if (x < 1920) x += speed; }
    void moveUp() { if (y > 0)    y -= speed; }
    void moveDown() { if (y < 1080) y += speed; }

    // ── Per-frame timer updater 
    void updateTimers(float dt)
    {
        // Charge the dash cooldown back up
        if (dashCooldownTimer < DASH_COOLDOWN)
            dashCooldownTimer += dt;

        // Count down for invincibility 
        if (invincible)
        {
            invincibleTimer -= dt;
            if (invincibleTimer <= 0.f)
            {
                invincibleTimer = 0.f;
                invincible = false;
            }
        }

        // Count down red flash
        if (damageFlashTimer > 0.f)
            damageFlashTimer -= dt;

        // Count down active weapon timer and then when timer over then go back to normal when expired
        if (weaponMode != WeaponMode::NORMAL && weaponTimer > 0.f)
        {
            weaponTimer -= dt;
            if (weaponTimer <= 0.f)
            {
                weaponMode = WeaponMode::NORMAL; weaponTimer = 0.f;
                laserWeaponActiveTimer = 0.f; laserWeaponCooldownTimer = 0.f;
                laserWeaponOnCooldown = false;
            }
        }

        // Laser cooldown recharges active time
        if (weaponMode == WeaponMode::LASER && laserWeaponOnCooldown)
        {
            laserWeaponCooldownTimer -= dt;
            if (laserWeaponCooldownTimer <= 0.f)
            {
                laserWeaponOnCooldown = false;
                laserWeaponActiveTimer = LASER_WEAPON_ACTIVE_DURATION;
            }
        }

        shield.update(dt); // Update shield particles and pulse
    }
    //dash
    // Teleport a fixed distance in the held direction and gain brief invincibility
    bool tryDash(bool left, bool right, bool up, bool down)
    {
        if (!canDash() || (!left && !right && !up && !down)) return false;
        if (left)  x -= DASH_DISTANCE;
        if (right) x += DASH_DISTANCE;
        if (up)    y -= DASH_DISTANCE;
        if (down)  y += DASH_DISTANCE;
        //make sure it does not go out of bound
        if (x < 0) x = 0; if (x > 1920) x = 1920;
        if (y < 0) y = 0; if (y > 1080) y = 1080;
        dashCooldownTimer = 0.f;         // Start cooldown
        invincible = true; invincibleTimer = DASH_INVINCIBLE;
        return true;
    }

    // EMP 
    bool tryFireEMP() { if (empCount <= 0) return false; empCount--; return true; }
    void collectEMP() { if (empCount < MAX_EMP_HOLD) empCount++; }

    // Weapon activations 
    void activateSpread()
    {
        weaponMode = WeaponMode::SPREAD; weaponTimer = SPREAD_DURATION;
    }
    void activateLaser()
    {
        weaponMode = WeaponMode::LASER; weaponTimer = LASER_DURATION;
        laserWeaponActiveTimer = LASER_WEAPON_ACTIVE_DURATION;
        laserWeaponCooldownTimer = 0.f; laserWeaponOnCooldown = false;
    }
    void activateShield()
    {
        if (shield.isActive())
            shield.reset(); // Refresh hits if already active
        else shield.activate();
    }

    // Called every frame while space is held in laser mode.
    // Drains active time; switches to cooldown when exhausted.
    bool tickLaserFire(float dt)
    {
        if (weaponMode != WeaponMode::LASER)
            return false;
        if (laserWeaponOnCooldown)
            return false;
        laserWeaponActiveTimer -= dt;
        if (laserWeaponActiveTimer <= 0.f)
        {
            laserWeaponOnCooldown = true;
            laserWeaponCooldownTimer = LASER_WEAPON_COOLDOWN_DURATION;
            laserWeaponActiveTimer = 0.f;
            return false;
        }
        return true;
    }

    bool isLaserReady() const
    {
        return (weaponMode == WeaponMode::LASER && !laserWeaponOnCooldown && laserWeaponActiveTimer > 0.f);
    }

    // Damage 
    // Returns true if the shield absorbed the hit (ship HP unchanged).
    // Otherwise reduces HP and triggers red flash.
    bool takeDamage(float invincDuration = DASH_INVINCIBLE)
    {
        if (invincible) return false; // Already invincible ignore hit entirely

        // Shield absorbs the hit if active
        if (shield.absorbHit(x, y))
        {
            invincible = true; invincibleTimer = invincDuration;
            damageFlashTimer = DAMAGE_FLASH_DURATION;
            return true; // Absorbed
        }

        // No shield – take real damage
        damageFlashTimer = DAMAGE_FLASH_DURATION;
        health--;
        if (health <= 0) alive = false;
        return false;
    }

    // Grant invincibility externally (e.g. after boss contact)
    void startInvincibility(float duration)
    {
        invincible = true; invincibleTimer = duration;
    }

    // Delegate shield drawing to the EnergyShield object
    void drawShield(sf::RenderWindow& w) { shield.draw(w, x, y); }

    // Draw the ship sprite; red tint on damage, blinking while invincible
    void draw(sf::RenderWindow& window)
    {
        if (!tex) return;
        sf::Sprite sp(*tex);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(0.2f, 0.2f); sp.setRotation(90.f);

        if (damageFlashTimer > 0.f)
        {
            sp.setColor(sf::Color(255, 60, 60, 255)); // Solid red flash
        }
        else if (invincible)
        {
            // Blink by alternating opacity every 0.08 s
            int f = (int)(invincibleTimer / 0.08f);
            if (f % 2 == 0) sp.setColor(sf::Color(255, 255, 255, 180));
        }

        sp.setPosition(x, y); window.draw(sp);
    }

    // Collision helpers
    bool collidesWithAsteroid(Asteroid* a) const
    {
        if (!a || !a->isActive()) return false;
        float dx = x - a->getX(), dy = y - a->getY();
        float r = 30.f + a->getRadius();
        return (dx * dx + dy * dy) < r * r;
    }

    bool collidesWithEnemy(Enemy* e) const
    {
        if (!e || !e->isAlive()) return false;
        float dx = x - e->getX(), dy = y - e->getY();
        return (dx * dx + dy * dy) < (80.f * 80.f); // 80 px hit radius
    }

    bool collidesWithEnemyBullet(EnemyBullet* b) const
    {
        if (!b || !b->isActive()) return false;
        float dx = x - b->getX(), dy = y - b->getY();
        return (dx * dx + dy * dy) < (80.f * 80.f);
    }

    bool collidesWithEMPDrop(EMPDrop* d) const
    {
        if (!d || !d->isActive()) return false;
        float dx = x - d->getX(), dy = y - d->getY();
        return (dx * dx + dy * dy) < (60.f * 60.f);
    }

    // Check if ship has crashed into a boss body (uses boss's declared collision radius)
    bool collidesWithBoss(float bx, float by, float radius) const
    {
        float dx = x - bx, dy = y - by;
        return (dx * dx + dy * dy) < ((30.f + radius) * (30.f + radius));
    }

    // Getters 
    bool       canDash()             const { return dashCooldownTimer >= DASH_COOLDOWN; }
    bool       isInvincible()        const { return invincible; }
    bool       isAlive()             const { return alive; }
    int        getHealth()           const { return health; }
    int        getEMPCount()         const { return empCount; }
    WeaponMode getWeapon()           const { return weaponMode; }
    float      getWeaponTimer()      const { return weaponTimer; }
    bool       shieldActive()        const { return shield.isActive(); }
    int        shieldHits()          const { return shield.getHits(); }
    float      getX()                const { return x; }
    float      getY()                const { return y; }
    // 0.0 = dash on cooldown, 1.0 = dash ready
    float      getDashRatio()        const
    {
        return (dashCooldownTimer < DASH_COOLDOWN) ? (dashCooldownTimer / DASH_COOLDOWN) : 1.f;
    }
    float      getLaserActiveRatio() const
    {
        if (weaponMode != WeaponMode::LASER || laserWeaponOnCooldown) return 0.f;
        return laserWeaponActiveTimer / LASER_WEAPON_ACTIVE_DURATION;
    }
    bool       isLaserOnCooldown()   const { return laserWeaponOnCooldown; }
};



//  Game CLASS
//  The main game controller. Owns all objects,
//  runs update and draw every frame, handles
//  collisions and the WAVE BOSSINTRO BOSSFIGHT
//  LEVEL CLEAR phase cycle.

class Game
{
private:
    Ship ship; // The player's ship (directly owned, not a pointer)

    // ── Object pools 
    // Each pool is a heap-allocated array of pointers.
    // "Bullet**" = pointer to an array of Bullet pointers.
    // We pre-allocate all slots at startup (pool pattern) so
    // there are no allocations during gameplay that could cause stutter.
    Bullet** bullets;        // Player normal bullets
    Bullet** spreadBullets;  // Player spread-shot bullets (separate pool so each has its own fire rate)
    LaserBeam** laserBeams;     // Player laser beams
    EnemyBullet** enemyBullets;   // All enemy projectiles
    Asteroid** asteroids;      // Background/hazard asteroids
    Star** stars;          // Background parallax stars
    Drone** drones;         // Drone enemies
    Viper** vipers;         // Viper enemies
    Seeker** seekers;        // Seeker enemies
    Explosion** explosions;     // Explosion animations
    EMPDrop** empDrops;       // EMP collectables
    PowerDrop** powerDrops;     // Weapon power-up collectables

    EMPWave empWave; // The expanding EMP ring (one at a time)


    // ** Pointers to heap-allocated boss objects (one per level, owned here)
    BossCruiser* bossCruiser;
    BossTwinCannons* bossTwinCannons;
    BossMothership* bossMothership;

    // ----- State -----
    GameMode  mode;      // ARCADE or SURVIVAL
    bool      gameOver;

    // Spawn timers used in ARCADE wave phase
    float droneSpawnTimer, viperSpawnTimer, seekerSpawnTimer;

    int       currentLevel; // 1, 2, or 3 (ARCADE only)
    GamePhase phase;        // Current phase of the level
    float     phaseTimer;   // General-purpose countdown for BOSS_INTRO / LEVEL_CLEAR
    float     waveTimer;    // How long the current WAVE phase has been running (ARCADE)

    // --- Survival wave tracking ----------
    int   survivalWave;       // Which wave we're on (increases each time all enemies die)
    int   waveEnemyCount;     // Total enemies to spawn this wave
    int   waveEnemiesSpawned; // How many have been spawned so far
    int   waveEnemiesKilled;  // How many have been killed so far
    float waveSpeedMult;      // Enemy speed multiplier (grows each wave)
    float waveFireRateMult;   // Enemy fire-rate multiplier (grows each wave)

    float multiplier;          // (Unused accumulator – score uses global mult)
    float damageCooldownTimer; // Extra cooldown on top of invincibility (removed in tryDamageShip)

    bool laserBeamActive; // Tracks whether we're currently refreshing a laser beam

    sf::Font hudFont; // Font used for all in-game HUD text

    // ------------- Textures ------------------------
    // Each texture is loaded once and shared via raw pointer with all objects that need it.
    sf::Texture texShip, texBullet, texHeart;
    sf::Texture texAsteroidSmall, texAsteroidBig, texAsteroidMed;
    sf::Texture texDrone, texViper, texSeeker;
    sf::Texture texExplosion;
    sf::Texture texEMP, texSpreadIcon, texLaserIcon, texShieldIcon;
    sf::Texture texBoss16, texBoss17, texBoss18;
    sf::Texture texLaserBeam, texLightning;
    sf::Texture texTurretBarrel, texTurretBody;
    sf::Texture texConeLaser;

    // ----- Private helpers -------------------------------

    // Find the first inactive enemy bullet slot and spawn a straight-down shot
    void fireEnemyBullet(float ex, float ey)
    {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
            if (!enemyBullets[i]->isActive())
            {
                enemyBullets[i]->spawn(ex, ey); return;
            }
    }

    // Activate a drone from the pool at a random X position above the screen
    void spawnDrone()
    {
        for (int i = 0; i < MAX_DRONES; i++)
            if (!drones[i]->isAlive())
            {
                drones[i]->spawnAt(100.f + rand() % 1720, -60.f);
                drones[i]->applyWaveScale(waveSpeedMult, waveFireRateMult);
                return;
            }
    }

    // Activate a viper from the pool
    void spawnViper()
    {
        for (int i = 0; i < MAX_VIPERS; i++)
            if (!vipers[i]->isAlive())
            {
                vipers[i]->spawnAt(200.f + rand() % 1520, -60.f);
                vipers[i]->applyWaveScale(waveSpeedMult, waveFireRateMult);
                return;
            }
    }

    // Activate a seeker aimed at a given X (defaults to player X)
    void spawnSeeker(float targetX = -1.f)
    {
        float tx = (targetX < 0) ? ship.getX() : targetX;
        for (int i = 0; i < MAX_SEEKERS; i++)
            if (!seekers[i]->isAlive())
            {
                seekers[i]->spawnAt(100.f + rand() % 1720, -60.f, tx);
                seekers[i]->applyWaveScale(waveSpeedMult, 1.f);
                return;
            }
    }

    // Find an inactive explosion slot and start the animation at (ex, ey)
    void spawnExplosionAt(float ex, float ey)
    {
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
            if (!explosions[i]->isActive()) { explosions[i]->spawn(ex, ey); return; }
    }

    // 5% chance to drop an EMP pickup at the given position
    void trySpawnEMPDrop(float ex, float ey)
    {
        if ((rand() % 100) >= 5) return;
        for (int i = 0; i < MAX_EMP_DROPS; i++)
            if (!empDrops[i]->isActive()) { empDrops[i]->spawn(ex, ey); return; }
    }

    // 15% chance to drop a random power-up at the given position
    void trySpawnPowerDrop(float ex, float ey)
    {
        if ((rand() % 100) >= 15) return;
        int t = rand() % 3; // 0=spread, 1=laser, 2=shield
        for (int i = 0; i < MAX_POWER_DROPS; i++)
            if (!powerDrops[i]->isActive()) { powerDrops[i]->spawn(ex, ey, t); return; }
    }

    // Kill every live enemy instantly (EMP effect) and spawn explosions
    void empKillAll()
    {
        for (int i = 0; i < MAX_DRONES; i++) if (drones[i]->isAlive()) { spawnExplosionAt(drones[i]->getX(), drones[i]->getY());  drones[i]->empKill(); }
        for (int i = 0; i < MAX_VIPERS; i++) if (vipers[i]->isAlive()) { spawnExplosionAt(vipers[i]->getX(), vipers[i]->getY());  vipers[i]->empKill(); }
        for (int i = 0; i < MAX_SEEKERS; i++) if (seekers[i]->isAlive()) { spawnExplosionAt(seekers[i]->getX(), seekers[i]->getY()); seekers[i]->empKill(); }
        // Clear all enemy bullets too
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i]->destroy();
    }
    void empDamageBoss()
    {
        if (phase != GamePhase::BOSS_FIGHT) return;

        Boss* activeBoss = nullptr;
        if (currentLevel == 1) activeBoss = bossCruiser;
        else if (currentLevel == 2) activeBoss = bossTwinCannons;
        else if (currentLevel == 3) activeBoss = bossMothership;

        if (!activeBoss || !activeBoss->isAlive() || activeBoss->isEntering()) return;

        // For level 2, EMP also damages both turrets directly
        if (currentLevel == 2)
        {
            bossTwinCannons->getLeftTurret()->takeDamage(30);
            bossTwinCannons->getRightTurret()->takeDamage(30);
            spawnExplosionAt(bossTwinCannons->getLeftTurret()->getX(), bossTwinCannons->getLeftTurret()->getY());
            spawnExplosionAt(bossTwinCannons->getRightTurret()->getX(), bossTwinCannons->getRightTurret()->getY());
        }

        // Deal 20% of max HP as flat damage to the boss core
        int empDmg = activeBoss->getMaxHp() / 3;
        for (int i = 0; i < empDmg; i++) activeBoss->takeDamage(20);

        // Spawn several explosions across the boss to make it feel impactful
        spawnExplosionAt(activeBoss->getX(), activeBoss->getY());
        spawnExplosionAt(activeBoss->getX() - 80.f, activeBoss->getY() + 40.f);
        spawnExplosionAt(activeBoss->getX() + 80.f, activeBoss->getY() + 40.f);
        spawnExplosionAt(activeBoss->getX(), activeBoss->getY() - 60.f);
    }
    // Called when an enemy reaches 0 HP.
    // Spawns explosion, tries to drop pickups, increments combo multiplier and score.
    void killEnemy(Enemy* /*e*/, float ex, float ey)
    {
        spawnExplosionAt(ex, ey);
        trySpawnEMPDrop(ex, ey);
        trySpawnPowerDrop(ex, ey);
        waveEnemiesKilled++;

        // Multiplier chain: 1 -> 2 -> 4 (resets on taking damage)
        if (mult == 1) mult = 2; else if (mult == 2) mult = 4;
        Score += 100 * mult;
    }

    // Fire a 3-bullet spread from a turret position
    void fireTurretSpread(float ex, float ey)
    {
        float vxDirs[3] = { -3.5f, 0.f, 3.5f }; // Left-diagonal, straight, right-diagonal
        for (int a = 0; a < 3; a++)
            for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
                if (!enemyBullets[i]->isActive()) { enemyBullets[i]->spawnAngled(ex, ey, vxDirs[a]); break; }
    }

    // Keep the laser beam pinned to the ship for one more frame
    void maintainLaserBeam()
    {
        laserBeams[0]->refresh(ship.getX(), ship.getY()); laserBeamActive = true;
    }

    // Fire three angled bullets for spread shot
    void fireSpread()
    {
        const float angles[3] = { -0.28f, 0.f, 0.28f }; // Radians offset from straight-up
        for (int a = 0; a < 3; a++)
            for (int i = 0; i < MAX_SPREAD_BULLETS; i++)
                if (!spreadBullets[i]->isActive())
                {
                    spreadBullets[i]->spawnAngled(ship.getX(), ship.getY(), angles[a]); break;
                }
    }

    // Instantly deactivate all enemies and bullets (used before boss fight starts)
    void clearAllEnemies()
    {
        for (int i = 0; i < MAX_DRONES; i++) drones[i]->empKill();
        for (int i = 0; i < MAX_VIPERS; i++) vipers[i]->empKill();
        for (int i = 0; i < MAX_SEEKERS; i++) seekers[i]->empKill();
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i]->destroy();
    }

    // Returns true only when every enemy pool slot is inactive
    bool allWaveEnemiesDead() const
    {
        for (int i = 0; i < MAX_DRONES; i++) if (drones[i]->isAlive())  return false;
        for (int i = 0; i < MAX_VIPERS; i++) if (vipers[i]->isAlive())  return false;
        for (int i = 0; i < MAX_SEEKERS; i++) if (seekers[i]->isAlive()) return false;
        return true;
    }

    // Formula for how many enemies appear in each survival wave
    int computeWaveEnemyCount(int wave) const { return 4 + wave * 2; }

    // Advance to the next survival wave: bump counters, reset timers, increase difficulty
    void startNewSurvivalWave()
    {
        survivalWave++;
        WaveNum = survivalWave; // Update display counter
        waveEnemyCount = computeWaveEnemyCount(survivalWave);
        waveEnemiesSpawned = 0;
        waveEnemiesKilled = 0;
        waveSpeedMult = 1.f + (survivalWave - 1) * 0.05f;  // +5% speed per wave
        waveFireRateMult = 1.f + (survivalWave - 1) * 0.10f;  // +10% fire rate per wave
        droneSpawnTimer = viperSpawnTimer = seekerSpawnTimer = 0.f;
    }

    // Choose a random enemy type suitable for this wave and spawn one
    void spawnWaveEnemy()
    {
        if (waveEnemiesSpawned >= waveEnemyCount) return;
        bool canViper = (survivalWave >= 5); // Vipers unlock at wave 5
        bool canSeeker = (survivalWave >= 8); // Seekers unlock at wave 8

        int maxType = 0;
        if (canViper)  maxType = 1;
        if (canSeeker) maxType = 2;
        int t = rand() % (maxType + 1);

        if (t == 2 && canSeeker) spawnSeeker();
        else if (t == 1 && canViper)  spawnViper();
        else                          spawnDrone();
        waveEnemiesSpawned++;
    }

    // Generic template: check whether bullet b hits any enemy in pool[0..count-1]
    // If so: apply damage, deactivate bullet, spawn explosion, call killEnemy if just died.
    template<typename EnemyType>
    bool bulletHitsEnemy(Bullet* b, EnemyType** pool, int count, bool doubleDmg = false)
    {
        for (int j = 0; j < count; j++)
        {
            if (!pool[j]->isAlive()) continue;
            if (pool[j]->collidesWithBullet(b))
            {
                bool wasDead = !pool[j]->isAlive();
                pool[j]->takeDamage(doubleDmg ? 2 : 1);
                b->onCollision(); // Deactivate bullet
                if (!wasDead && !pool[j]->isAlive())
                    killEnemy(pool[j], pool[j]->getX(), pool[j]->getY()); // Just killed it
                else
                    spawnExplosionAt(b->getX(), b->getY()); // Hit but not dead
                return true;
            }
        }
        return false;
    }

    // Run one bullet against drones, vipers, and seekers
    void processBulletVsEnemies(Bullet* b, bool doubleDmg = false)
    {
        bulletHitsEnemy(b, drones, MAX_DRONES, doubleDmg);
        bulletHitsEnemy(b, vipers, MAX_VIPERS, doubleDmg);
        // Seekers use onCollision() which calls alive=false directly
        for (int j = 0; j < MAX_SEEKERS; j++)
        {
            if (!seekers[j]->isAlive()) continue;
            if (seekers[j]->collidesWithBullet(b))
            {
                bool wasDead = !seekers[j]->isAlive();
                seekers[j]->onCollision(); b->onCollision();
                if (!wasDead && !seekers[j]->isAlive())
                    killEnemy(seekers[j], seekers[j]->getX(), seekers[j]->getY());
                else
                    spawnExplosionAt(b->getX(), b->getY());
                return;
            }
        }
    }

    // Check all player bullets and laser beams against the active boss (and its turrets)
    void processBulletsVsBoss()
    {
        Boss* activeBoss = nullptr;
        if (phase == GamePhase::BOSS_FIGHT)
        {
            if (currentLevel == 1) activeBoss = bossCruiser;
            else if (currentLevel == 2) activeBoss = bossTwinCannons;
            else if (currentLevel == 3) activeBoss = bossMothership;
        }
        if (!activeBoss || !activeBoss->isAlive()) return;


        auto processBulletPool = [&](Bullet** pool, int count)
            {
                for (int i = 0; i < count; i++)
                {
                    if (!pool[i]->isActive()) continue;

                    // Level 2: check turrets first before core
                    if (currentLevel == 2)
                    {
                        BossTwinCannons* tc = bossTwinCannons;
                        int turretHit = tc->hitTurret(pool[i]);
                        if (turretHit > 0)
                        {
                            Turret* t = (turretHit == 1) ? tc->getLeftTurret() : tc->getRightTurret();
                            spawnExplosionAt(t->getX(), t->getY());
                            pool[i]->onCollision(); continue;
                        }
                    }

                    // Hit the boss core
                    if (activeBoss->collidesWithBullet(pool[i]))
                    {
                        activeBoss->takeDamage(1);
                        spawnExplosionAt(pool[i]->getX(), pool[i]->getY());
                        pool[i]->onCollision();
                    }
                }
            };

        processBulletPool(bullets, MAX_BULLETS);
        processBulletPool(spreadBullets, MAX_SPREAD_BULLETS);

        // Laser beams hit boss with 2 damage per tick and can pierce turrets
        for (int i = 0; i < MAX_LASER_BEAMS; i++)
        {
            if (!laserBeams[i]->isActive()) continue;

            if (currentLevel == 2)
            {
                BossTwinCannons* tc = bossTwinCannons;
                if (tc->getLeftTurret()->isAlive() &&
                    laserBeams[i]->collidesWithPoint(tc->getLeftTurret()->getX(), tc->getLeftTurret()->getY()))
                {
                    tc->getLeftTurret()->takeDamage(2);
                    spawnExplosionAt(tc->getLeftTurret()->getX(), tc->getLeftTurret()->getY());
                    laserBeams[i]->registerHit(); continue;
                }
                if (tc->getRightTurret()->isAlive() &&
                    laserBeams[i]->collidesWithPoint(tc->getRightTurret()->getX(), tc->getRightTurret()->getY()))
                {
                    tc->getRightTurret()->takeDamage(2);
                    spawnExplosionAt(tc->getRightTurret()->getX(), tc->getRightTurret()->getY());
                    laserBeams[i]->registerHit(); continue;
                }
            }

            if (laserBeams[i]->collidesWithPoint(activeBoss->getX(), activeBoss->getY()))
            {
                activeBoss->takeDamage(2);
                spawnExplosionAt(laserBeams[i]->getX(), activeBoss->getY());
                laserBeams[i]->registerHit();
            }
        }
    }

    // Phase state machine
    // Called every frame drives the WAVE->BOSS-INTRO->BOSS-FIGHT->LEVEL-CLEAR->cycle.
    void updatePhase(float dt)
    {
        switch (phase)
        {
        case GamePhase::WAVE:
            if (mode == GameMode::SURVIVAL)
            {
                // Survival: spawn enemies one at a time every 2 s until the wave quota is met;
                // when all spawned enemies are dead, start the next wave
                droneSpawnTimer += dt;
                float spawnGap = 2.0f;
                if (droneSpawnTimer >= spawnGap && waveEnemiesSpawned < waveEnemyCount)
                {
                    spawnWaveEnemy(); droneSpawnTimer = 0.f;
                }
                if (waveEnemiesSpawned >= waveEnemyCount && allWaveEnemiesDead())
                    startNewSurvivalWave();
            }
            else
            {
                // ARCADE: spawn enemies on fixed timers; after WAVE_DURATION seconds
                // and all enemies dead, switch to BOSS_INTRO
                waveTimer += dt;
                droneSpawnTimer += dt; viperSpawnTimer += dt; seekerSpawnTimer += dt;

                if (droneSpawnTimer > 3.f) { spawnDrone();  droneSpawnTimer = 0; }
                if (viperSpawnTimer > 6.f) { spawnViper();  viperSpawnTimer = 0; }
                if (seekerSpawnTimer > 8.f) { spawnSeeker(); seekerSpawnTimer = 0; }

                if ((waveTimer >= WAVE_DURATION && allWaveEnemiesDead()) || waveTimer >= WAVE_DURATION + 5.f)
                {
                    phase = GamePhase::BOSS_INTRO;
                    phaseTimer = BOSS_INTRO_PAUSE;
                    clearAllEnemies(); waveTimer = 0.f;
                }
            }
            break;

        case GamePhase::BOSS_INTRO:
            // Count down the warning pause; then spawn the boss and start fight music
            phaseTimer -= dt;
            if (phaseTimer <= 0.f)
            {
                phase = GamePhase::BOSS_FIGHT;
                audio.playBoss(currentLevel);
                if (currentLevel == 1) bossCruiser->spawn();
                else if (currentLevel == 2) bossTwinCannons->spawn();
                else if (currentLevel == 3) bossMothership->spawn();
            }
            break;

        case GamePhase::BOSS_FIGHT:
        {
            // Check if the active boss has died; if so go to LEVEL_CLEAR
            Boss* activeBoss = nullptr;
            if (currentLevel == 1) activeBoss = bossCruiser;
            else if (currentLevel == 2) activeBoss = bossTwinCannons;
            else if (currentLevel == 3) activeBoss = bossMothership;

            if (activeBoss && !activeBoss->isAlive())
            {
                audio.resumeGame();
                Score += currentLevel * 200; // Bonus for defeating boss
                phase = GamePhase::LEVEL_CLEAR; phaseTimer = 3.0f;
            }
            break;
        }

        case GamePhase::LEVEL_CLEAR:
            // Brief celebration then advance to next level (or end game at level 3)
            phaseTimer -= dt;
            if (phaseTimer <= 0.f)
            {
                if (currentLevel < 3)
                {
                    currentLevel++;
                    waveTimer = 0.f; phase = GamePhase::WAVE;
                    droneSpawnTimer = viperSpawnTimer = seekerSpawnTimer = 0.f;
                }
                else { gameOver = true; } // Beat all 3 levels – victory
            }
            break;
        }
    }

public:

    // Allocates all object pools, loads all textures, wires textures to objects.
    Game(GameMode m = GameMode::ARCADE)
        : mode(m), gameOver(false),
        droneSpawnTimer(0), viperSpawnTimer(0), seekerSpawnTimer(0),
        damageCooldownTimer(0.5f),
        currentLevel(1), phase(GamePhase::WAVE), phaseTimer(0.f),
        waveTimer(0.f), laserBeamActive(false),
        survivalWave(0), waveEnemyCount(0), waveEnemiesSpawned(0), waveEnemiesKilled(0),
        waveSpeedMult(1.f), waveFireRateMult(1.f), multiplier(1.f)
    {
        srand(time(nullptr)); // Seed random number generator with current time
        hudFont.loadFromFile("assets/font.ttf");

        // Load all textures from disk
        texShip.loadFromFile("assets/4.png");
        texBullet.loadFromFile("assets/5.png");
        texHeart.loadFromFile("assets/6.png");
        texAsteroidSmall.loadFromFile("assets/7.png");
        texAsteroidBig.loadFromFile("assets/8.png");
        texAsteroidMed.loadFromFile("assets/9.png");
        texDrone.loadFromFile("assets/1.png");
        
        texViper.loadFromFile("assets/2.png");
        texSeeker.loadFromFile("assets/3.png");
        texExplosion.loadFromFile("assets/10.png");
        texEMP.loadFromFile("assets/12.png");
        texSpreadIcon.loadFromFile("assets/13.png");
        texLaserIcon.loadFromFile("assets/14.png");
        texShieldIcon.loadFromFile("assets/15.png");
        texBoss16.loadFromFile("assets/16.png");
        texBoss17.loadFromFile("assets/17.png");
        texBoss18.loadFromFile("assets/18.png");
        texLaserBeam.loadFromFile("assets/14.png");
        texLightning.loadFromFile("assets/19.png");
        texTurretBarrel.loadFromFile("assets/20.png");
        texTurretBody.loadFromFile("assets/21.png");
        texConeLaser.loadFromFile("assets/23.png");

        ship.setTexture(&texShip);
        empWave.setTexture(&texEMP);

        // Allocate the double-pointer arrays (each holds MAX_X pointers)
        bullets = new Bullet * [MAX_BULLETS];
        spreadBullets = new Bullet * [MAX_SPREAD_BULLETS];
        laserBeams = new LaserBeam * [MAX_LASER_BEAMS];
        enemyBullets = new EnemyBullet * [MAX_ENEMY_BULLETS];
        asteroids = new Asteroid * [MAX_ASTEROIDS];
        stars = new Star * [MAX_STARS];
        drones = new Drone * [MAX_DRONES];
        vipers = new Viper * [MAX_VIPERS];
        seekers = new Seeker * [MAX_SEEKERS];
        explosions = new Explosion * [MAX_EXPLOSIONS];
        empDrops = new EMPDrop * [MAX_EMP_DROPS];
        powerDrops = new PowerDrop * [MAX_POWER_DROPS];

        // Allocate the three boss objects
        bossCruiser = new BossCruiser();
        bossTwinCannons = new BossTwinCannons();
        bossMothership = new BossMothership();

        // Wire textures to bosses
        bossCruiser->setTexture(&texBoss16);
        bossCruiser->setLightningTexture(&texLightning);
        bossTwinCannons->setTexture(&texBoss17);
        bossTwinCannons->setLightningTexture(&texLightning);
        bossTwinCannons->setTurretTextures(&texTurretBarrel, &texTurretBody);
        bossTwinCannons->setArmTexture(&texTurretBarrel);
        bossMothership->setTexture(&texBoss18);
        bossMothership->setLightningTexture(&texLightning);
        bossMothership->setConeTexture(&texConeLaser);

        // Allocate each individual object inside each pool and wire its texture
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            bullets[i] = new Bullet();
            bullets[i]->setTexture(&texBullet);
        }
        for (int i = 0; i < MAX_SPREAD_BULLETS; i++)
        {
            spreadBullets[i] = new Bullet();
            spreadBullets[i]->setTexture(&texBullet);
        }
        for (int i = 0; i < MAX_LASER_BEAMS; i++)
        {
            laserBeams[i] = new LaserBeam();
            laserBeams[i]->setTexture(&texLaserBeam);
        }
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
        {
            enemyBullets[i] = new EnemyBullet();
            enemyBullets[i]->setTexture(&texBullet);
        }
        for (int i = 0; i < MAX_ASTEROIDS; i++)
        {
            asteroids[i] = new Asteroid();
            asteroids[i]->setTextures(&texAsteroidSmall, &texAsteroidBig, &texAsteroidMed);
            asteroids[i]->spawn(100.f + rand() % 1720, (float)(rand() % 600));
        }
        for (int i = 0; i < MAX_STARS; i++)
        {
            stars[i] = new Star();
            stars[i]->spawn();
        }
        for (int i = 0; i < MAX_DRONES; i++)
        {
            drones[i] = new Drone();
            drones[i]->setTexture(&texDrone);
        }
        for (int i = 0; i < MAX_VIPERS; i++)
        {
            vipers[i] = new Viper();
            vipers[i]->setTexture(&texViper);
        }
        for (int i = 0; i < MAX_SEEKERS; i++)
        {
            seekers[i] = new Seeker();
            seekers[i]->setTexture(&texSeeker);
        }
        for (int i = 0; i < MAX_EXPLOSIONS; i++)
        {
            explosions[i] = new Explosion();
            explosions[i]->setTexture(&texExplosion);
        }
        for (int i = 0; i < MAX_EMP_DROPS; i++)
        {
            empDrops[i] = new EMPDrop();
            empDrops[i]->setTexture(&texEMP);
        }
        for (int i = 0; i < MAX_POWER_DROPS; i++)
        {
            powerDrops[i] = new PowerDrop();
            powerDrops[i]->setTextures(&texSpreadIcon, &texLaserIcon, &texShieldIcon);
        }


        if (mode == GameMode::SURVIVAL) startNewSurvivalWave();
    }

    //  Destructor
    // Delete every individual object, then the array itself.
    // Two-step required: first delete each pointer inside the array,
    // then delete[] the array of pointers.
    ~Game()
    {
        for (int i = 0; i < MAX_BULLETS; i++) delete bullets[i];
        for (int i = 0; i < MAX_SPREAD_BULLETS; i++) delete spreadBullets[i];
        for (int i = 0; i < MAX_LASER_BEAMS; i++) delete laserBeams[i];
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) delete enemyBullets[i];
        for (int i = 0; i < MAX_ASTEROIDS; i++) delete asteroids[i];
        for (int i = 0; i < MAX_STARS; i++) delete stars[i];
        for (int i = 0; i < MAX_DRONES; i++) delete drones[i];
        for (int i = 0; i < MAX_VIPERS; i++) delete vipers[i];
        for (int i = 0; i < MAX_SEEKERS; i++) delete seekers[i];
        for (int i = 0; i < MAX_EXPLOSIONS; i++) delete explosions[i];
        for (int i = 0; i < MAX_EMP_DROPS; i++) delete empDrops[i];
        for (int i = 0; i < MAX_POWER_DROPS; i++) delete powerDrops[i];

        // Delete the array containers
        delete[] bullets;      delete[] spreadBullets; delete[] laserBeams;
        delete[] enemyBullets; delete[] asteroids;     delete[] stars;
        delete[] drones;       delete[] vipers;        delete[] seekers;
        delete[] explosions;   delete[] empDrops;      delete[] powerDrops;

        // Delete boss objects
        delete bossCruiser; delete bossTwinCannons; delete bossMothership;
    }

    // shoot()
    // Called from main() whenever the player presses Space.
    // Routes to the correct weapon system.
    void shoot(bool spaceHeld)
    {
        WeaponMode w = ship.getWeapon();
        if (w == WeaponMode::SPREAD)
        {
            fireSpread(); // Three angled bullets
        }
        else if (w == WeaponMode::LASER)
        {
            // Only maintain beam while space is held and laser isn't on cooldown
            if (spaceHeld && ship.isLaserReady()) maintainLaserBeam();
        }
        else
        {
            // Normal: find the first inactive bullet slot and fire it
            for (int i = 0; i < MAX_BULLETS; i++)
                if (!bullets[i]->isActive())
                {
                    bullets[i]->spawn(ship.getX(), ship.getY());
                    return;
                }
        }
    }

    // update()
    // Main game logic tick, called every frame from main().
    // Moves everything, checks collisions, manages pickups.
    void update(bool left, bool right, bool up, bool down, bool eKey, bool nKeyPressed, bool spaceHeld)
    {
        if (gameOver) return; // game is over
        const float dt = 0.016f; // Fixed timestep 

        // Player movement
        if (eKey) ship.tryDash(left, right, up, down); // E = dash in held direction
        else
        {
            if (left)  ship.moveLeft();
            if (right) ship.moveRight();
            if (up)    ship.moveUp();
            if (down)  ship.moveDown();
        }

        ship.updateTimers(dt); // Count down all ship timers each frame

        // Drain laser active time while space is held
        if (spaceHeld && ship.getWeapon() == WeaponMode::LASER) ship.tickLaserFire(dt);

        // N key fires EMP if not already active and player has charges
        if (nKeyPressed && !empWave.isActive())
            if (ship.tryFireEMP()) empWave.activate(ship.getX(), ship.getY());

        // Expand EMP ring; the moment it's big enough, kill all enemies
        if (empWave.isActive()) if (empWave.update()) { empKillAll(); empDamageBoss(); }

        updatePhase(dt); // Advance state

        //Update all object pools
        for (int i = 0; i < MAX_STARS; i++) stars[i]->update(dt);
        for (int i = 0; i < MAX_BULLETS; i++) bullets[i]->update(dt);
        for (int i = 0; i < MAX_SPREAD_BULLETS; i++) spreadBullets[i]->update(dt);
        for (int i = 0; i < MAX_LASER_BEAMS; i++) laserBeams[i]->update(dt);

        // Asteroids stop respawning during boss fights (they drift off and disappear)
        bool bossActive = (phase == GamePhase::BOSS_FIGHT);
        for (int i = 0; i < MAX_ASTEROIDS; i++) { asteroids[i]->setNoRespawn(bossActive); asteroids[i]->update(dt); }

        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i]->update(dt);
        for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i]->update(dt);
        for (int i = 0; i < MAX_EMP_DROPS; i++) empDrops[i]->update(dt);
        for (int i = 0; i < MAX_POWER_DROPS; i++) powerDrops[i]->update(dt);

        // Update enemies (move + fire) for every live slot
        for (int i = 0; i < MAX_DRONES; i++) { if (!drones[i]->isAlive())  continue; drones[i]->update(dt);  if (drones[i]->readyToShoot())  fireEnemyBullet(drones[i]->getX(), drones[i]->getY()); }
        for (int i = 0; i < MAX_VIPERS; i++) { if (!vipers[i]->isAlive())  continue; vipers[i]->update(dt);  if (vipers[i]->readyToShoot())  fireEnemyBullet(vipers[i]->getX(), vipers[i]->getY()); }
        for (int i = 0; i < MAX_SEEKERS; i++) seekers[i]->update(dt); // Seekers don't shoot

        // Update the active boss and its special attacks
        if (phase == GamePhase::BOSS_FIGHT)
        {
            if (currentLevel == 1)
            {
                bossCruiser->update(dt); // Moves and manages laser patterns
            }
            else if (currentLevel == 2)
            {
                bossTwinCannons->update(dt);
                if (!bossTwinCannons->isEntering())
                {
                    // Fire turret spreads when each turret's timer is ready
                    if (bossTwinCannons->leftTurretShoot())
                        fireTurretSpread(bossTwinCannons->getLeftTurret()->getX(), bossTwinCannons->getLeftTurret()->getY());
                    if (bossTwinCannons->rightTurretShoot())
                        fireTurretSpread(bossTwinCannons->getRightTurret()->getX(), bossTwinCannons->getRightTurret()->getY());
                }
            }
            else if (currentLevel == 3)
            {
                bossMothership->update(dt);
                if (bossMothership->wantsSeeker(dt)) spawnSeeker(ship.getX()); // Spawn seeker aimed at player
            }
        }

        // Bullet vs enemy collision
        for (int i = 0; i < MAX_BULLETS; i++) if (bullets[i]->isActive())      processBulletVsEnemies(bullets[i]);
        for (int i = 0; i < MAX_SPREAD_BULLETS; i++) if (spreadBullets[i]->isActive()) processBulletVsEnemies(spreadBullets[i]);

        // Laser beam vs enemies drone and viper only, seekers die on contact
        for (int i = 0; i < MAX_LASER_BEAMS; i++)
        {
            if (!laserBeams[i]->isActive()) continue;
            for (int j = 0; j < MAX_DRONES; j++)
                if (drones[j]->isAlive() && laserBeams[i]->collidesWithPoint(drones[j]->getX(), drones[j]->getY()))
                {
                    bool was = !drones[j]->isAlive(); drones[j]->takeDamage(2); laserBeams[i]->registerHit();
                    if (!was && !drones[j]->isAlive()) killEnemy(drones[j], drones[j]->getX(), drones[j]->getY());
                    else spawnExplosionAt(drones[j]->getX(), drones[j]->getY());
                    if (!laserBeams[i]->isActive()) break;
                }
            if (!laserBeams[i]->isActive()) continue;
            for (int j = 0; j < MAX_VIPERS; j++)
                if (vipers[j]->isAlive() && laserBeams[i]->collidesWithPoint(vipers[j]->getX(), vipers[j]->getY()))
                {
                    bool was = !vipers[j]->isAlive(); vipers[j]->takeDamage(2); laserBeams[i]->registerHit();
                    if (!was && !vipers[j]->isAlive()) killEnemy(vipers[j], vipers[j]->getX(), vipers[j]->getY());
                    else spawnExplosionAt(vipers[j]->getX(), vipers[j]->getY());
                    if (!laserBeams[i]->isActive()) break;
                }
        }

        processBulletsVsBoss(); // Check all bullets against active boss

        // Bullets hitting asteroids 
        auto hitAsteroid = [&](Bullet* b)
            {
                for (int i = 0; i < MAX_ASTEROIDS; i++)
                    if (asteroids[i]->collidesWithBullet(b)) { spawnExplosionAt(b->getX(), b->getY()); b->onCollision(); }
            };
        for (int j = 0; j < MAX_BULLETS; j++) hitAsteroid(bullets[j]);
        for (int j = 0; j < MAX_SPREAD_BULLETS; j++) hitAsteroid(spreadBullets[j]);

        // Player collecting EMP drops
        for (int i = 0; i < MAX_EMP_DROPS; i++)
            if (ship.collidesWithEMPDrop(empDrops[i]))
            {
                ship.collectEMP(); empDrops[i]->onCollision();
            }

        // Player collecting power-up drops
        for (int i = 0; i < MAX_POWER_DROPS; i++)
        {
            if (!powerDrops[i]->isActive()) continue;
            float dx = ship.getX() - powerDrops[i]->getX();
            float dy = ship.getY() - powerDrops[i]->getY();
            if ((dx * dx + dy * dy) < (60.f * 60.f))
            {
                int t = powerDrops[i]->getType(); powerDrops[i]->onCollision();
                audio.playPowerup();
                if (t == 0) ship.activateSpread();
                else if (t == 1) ship.activateLaser();
                else             ship.activateShield();
            }
        }

        if (!ship.isAlive()) gameOver = true; // Ship died – flag game over
    }


    //  tryDamageShip()
    //  Called every frame from main().
    //  Checks every possible hazard the ship
    //  could be touching and applies one hit
    //  if the ship is not currently invincible.
    //  Returns true if the ship just died.

    bool tryDamageShip()
    {
        // If the ship is already in its invincibility window, skip all checks.
        // ship.isInvincible() is set for DAMAGE_INVINCIBLE_DURATION after every hit,
        // so this naturally prevents being hit again immediately.
        if (ship.isInvincible()) return false;

        // ── Boss body contact ────────────────
        if (phase == GamePhase::BOSS_FIGHT)
        {
            Boss* activeBoss = nullptr;
            if (currentLevel == 1) activeBoss = bossCruiser;
            else if (currentLevel == 2) activeBoss = bossTwinCannons;
            else if (currentLevel == 3) activeBoss = bossMothership;

            // Ran into the boss itself
            if (activeBoss && activeBoss->isAlive() && !activeBoss->isEntering())
            {
                if (ship.collidesWithBoss(activeBoss->getX(), activeBoss->getY(), activeBoss->getCollisionRadius()))
                {
                    mult = 1; // Reset score multiplier on taking damage
                    ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                    ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                    return !ship.isAlive();
                }
            }

            // Cruiser's vertical laser columns
            if (currentLevel == 1 && bossCruiser->isAlive() && bossCruiser->playerInLaser(ship.getX()))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                return !ship.isAlive();
            }

            // Mothership's cone laser
            if (currentLevel == 3 && bossMothership->isAlive() &&
                bossMothership->playerInConeLaser(ship.getX(), ship.getY()))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                return !ship.isAlive();
            }
        }

        // ── Asteroid contact ─────────────────
        for (int i = 0; i < MAX_ASTEROIDS; i++)
            if (ship.collidesWithAsteroid(asteroids[i]))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                return !ship.isAlive();
            }

        // ── Enemy body contact ───────────────
        // Each block: if ship overlaps enemy  deal damage, grant invincibility,
        // spawn explosion at enemy, kill that enemy, return whether ship just died.
        for (int i = 0; i < MAX_DRONES; i++)
            if (ship.collidesWithEnemy(drones[i]))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                spawnExplosionAt(drones[i]->getX(), drones[i]->getY());
                drones[i]->onCollision(); // Kill drone on contact
                return !ship.isAlive();
            }

        for (int i = 0; i < MAX_VIPERS; i++)
            if (ship.collidesWithEnemy(vipers[i]))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                spawnExplosionAt(vipers[i]->getX(), vipers[i]->getY());
                vipers[i]->onCollision(); // Vipers take 1 damage on contact (don't always die)
                return !ship.isAlive();
            }

        for (int i = 0; i < MAX_SEEKERS; i++)
            if (ship.collidesWithEnemy(seekers[i]))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                spawnExplosionAt(seekers[i]->getX(), seekers[i]->getY());
                seekers[i]->onCollision(); // Kill seeker on contact
                return !ship.isAlive();
            }

        // ── Enemy bullet contact ─────────────
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
            if (ship.collidesWithEnemyBullet(enemyBullets[i]))
            {
                mult = 1;
                ship.takeDamage(DAMAGE_INVINCIBLE_DURATION);
                ship.startInvincibility(DAMAGE_INVINCIBLE_DURATION);
                enemyBullets[i]->onCollision(); // Deactivate the bullet
                return !ship.isAlive();
            }

        return false; // Nothing hit the ship this frame
    }

    // ── HUD draw functions ───────────────────

    // Draw EMP charge icons at the left side of the screen.
    // Lit = player has that charge; dim = empty slot.
    void drawEMPHUD(sf::RenderWindow& window)
    {
        int held = ship.getEMPCount();
        float sx = 30.f, sy = 140.f, sz = 55.f, gap = 65.f;
        for (int i = 0; i < MAX_EMP_HOLD; i++)
        {
            sf::Sprite sp(texEMP);
            sf::FloatRect b = sp.getLocalBounds();
            sp.setOrigin(b.width / 2.f, b.height / 2.f);
            sp.setScale(sz / b.width, sz / b.width);
            sp.setPosition(sx + i * gap + sz / 2.f, sy + sz / 2.f);
            // Bright purple = held; dark = empty
            sp.setColor(i < held ? sf::Color(200, 120, 255, 255) : sf::Color(80, 40, 100, 100));
            window.draw(sp);
        }
    }

    // Draw the active weapon icon and a duration/cooldown bar.
    // Nothing drawn in NORMAL mode.
    void drawWeaponHUD(sf::RenderWindow& window)
    {
        WeaponMode w = ship.getWeapon();
        if (w == WeaponMode::NORMAL) return;

        sf::Texture* t = (w == WeaponMode::SPREAD) ? &texSpreadIcon : &texLaserIcon;
        sf::Color    c = (w == WeaponMode::SPREAD) ? sf::Color(255, 220, 80, 230) : sf::Color(80, 210, 255, 230);

        sf::Sprite sp(*t);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(60.f / b.width, 60.f / b.width);
        sp.setColor(c); sp.setPosition(1920.f - 110.f, 190.f); window.draw(sp);

        if (w == WeaponMode::LASER)
        {
            // Show active-time bar or red cooldown bar
            bool  onCD = ship.isLaserOnCooldown();
            float ratio = onCD ? 0.f : ship.getLaserActiveRatio();
            sf::Color barColor = onCD ? sf::Color(180, 60, 60, 200) : sf::Color(80, 210, 255, 220);

            sf::RectangleShape bg(sf::Vector2f(80.f, 6.f));
            bg.setPosition(1920.f - 150.f, 230.f); bg.setFillColor(sf::Color(60, 60, 60, 200)); window.draw(bg);

            sf::RectangleShape fill(sf::Vector2f(80.f * ratio, 6.f));
            fill.setPosition(1920.f - 150.f, 230.f); fill.setFillColor(barColor); window.draw(fill);

            if (onCD)
            {
                sf::RectangleShape cdBar(sf::Vector2f(80.f, 6.f));
                cdBar.setPosition(1920.f - 150.f, 230.f);
                cdBar.setFillColor(sf::Color(180, 60, 60, 120)); window.draw(cdBar);
            }
        }
        else
        {
            // Spread: show simple time-remaining bar
            float ratio = ship.getWeaponTimer() / SPREAD_DURATION;
            if (ratio < 0) ratio = 0; if (ratio > 1) ratio = 1;

            sf::RectangleShape bg(sf::Vector2f(80.f, 6.f));
            bg.setPosition(1920.f - 150.f, 230.f); bg.setFillColor(sf::Color(60, 60, 60, 200)); window.draw(bg);

            sf::RectangleShape fill(sf::Vector2f(80.f * ratio, 6.f));
            fill.setPosition(1920.f - 150.f, 230.f);
            sf::Color fc = c; fc.a = 220; fill.setFillColor(fc); window.draw(fill);
        }
    }

    // Draw a shield icon and two pip circles showing hits remaining (2 or 1).
    void drawShieldHUD(sf::RenderWindow& window)
    {
        if (!ship.shieldActive()) return;
        int hits = ship.shieldHits();
        sf::Sprite sp(texShieldIcon);
        sf::FloatRect b = sp.getLocalBounds();
        sp.setOrigin(b.width / 2.f, b.height / 2.f);
        sp.setScale(55.f / b.width, 55.f / b.width); sp.setPosition(57.f, 257.f);
        sp.setColor(hits == 2 ? sf::Color(80, 255, 140, 240) : sf::Color(80, 255, 140, 120));
        window.draw(sp);
        for (int i = 0; i < 2; i++)
        {
            sf::CircleShape pip(5.f);
            pip.setOrigin(5.f, 5.f); pip.setPosition(65.f + i * 16.f, 240.f);
            pip.setFillColor(i < hits ? sf::Color(80, 255, 140, 255) : sf::Color(50, 80, 50, 120));
            window.draw(pip);
        }
    }

    // Draw wave/level counter, survival kill-progress bar, or arcade wave-timer bar.
    void drawWaveHUD(sf::RenderWindow& window)
    {
        // Always-visible wave / level label
        sf::Text waveTxt;
        waveTxt.setFont(hudFont);
        waveTxt.setString(mode == GameMode::SURVIVAL
            ? "WAVE " + std::to_string(survivalWave)
            : "LVL " + std::to_string(currentLevel));
        waveTxt.setCharacterSize(28);
        waveTxt.setFillColor(sf::Color(180, 220, 255, 210));
        waveTxt.setPosition(30.f, 200.f);
        window.draw(waveTxt);

        if (mode == GameMode::SURVIVAL && phase == GamePhase::WAVE)
        {
            // Progress bar: killed / total enemies this wave
            int total = waveEnemyCount;
            float ratio = (total > 0) ? (float)waveEnemiesKilled / (float)total : 0.f;
            if (ratio > 1.f) ratio = 1.f;
            float barW = 200.f, barH = 8.f, barX = 30.f, barY = 115.f;

            sf::RectangleShape bg(sf::Vector2f(barW, barH));
            bg.setPosition(barX, barY); bg.setFillColor(sf::Color(40, 40, 60, 180)); window.draw(bg);

            sf::RectangleShape fill(sf::Vector2f(barW * ratio, barH));
            fill.setPosition(barX, barY); fill.setFillColor(sf::Color(80, 220, 120, 220)); window.draw(fill);

            sf::RectangleShape border(sf::Vector2f(barW + 2.f, barH + 2.f));
            border.setPosition(barX - 1.f, barY - 1.f);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(1.5f);
            border.setOutlineColor(sf::Color(120, 120, 180, 160)); window.draw(border);

            sf::Text killTxt; killTxt.setFont(hudFont);
            killTxt.setString(std::to_string(waveEnemiesKilled) + "/" + std::to_string(total));
            killTxt.setCharacterSize(20); killTxt.setFillColor(sf::Color(160, 255, 160, 200));
            killTxt.setPosition(barX + barW + 8.f, barY - 4.f); window.draw(killTxt);
        }
        else if (mode == GameMode::ARCADE && phase == GamePhase::WAVE)
        {
            // Countdown bar at the top-centre showing how long until boss arrives
            float ratio = 1.f - (waveTimer / WAVE_DURATION);
            if (ratio < 0.f) ratio = 0.f;
            float barW = 400.f, barH = 10.f, barX = (1920.f - barW) / 2.f, barY = 55.f;
            sf::Uint8 r = (sf::Uint8)(255 * (1.f - ratio));
            sf::Uint8 g = (sf::Uint8)(255 * ratio);

            sf::RectangleShape bg(sf::Vector2f(barW, barH));
            bg.setPosition(barX, barY); bg.setFillColor(sf::Color(40, 40, 60, 180)); window.draw(bg);

            sf::RectangleShape fill(sf::Vector2f(barW * ratio, barH));
            fill.setPosition(barX, barY); fill.setFillColor(sf::Color(r, g, 60, 220)); window.draw(fill);

            sf::RectangleShape border(sf::Vector2f(barW + 2.f, barH + 2.f));
            border.setPosition(barX - 1.f, barY - 1.f);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(1.5f);
            border.setOutlineColor(sf::Color(120, 120, 180, 160)); window.draw(border);
        }
    }

    // Draw the flashing "BOSS BATTLE INCOMING" or "BOSS DEFEATED" banners
    void drawPhaseBanner(sf::RenderWindow& window)
    {
        if (phase == GamePhase::BOSS_INTRO)
        {
            float flash = 0.5f + 0.5f * sinf(phaseTimer * 8.f);
            sf::Uint8 alpha = (sf::Uint8)(180 + 75 * flash);
            sf::Text warningText; warningText.setFont(hudFont);
            warningText.setString("!! BOSS BATTLE INCOMING !!");
            warningText.setCharacterSize(58); warningText.setStyle(sf::Text::Bold);
            warningText.setFillColor(sf::Color(255, 60, 60, alpha));
            sf::FloatRect wb = warningText.getLocalBounds();
            warningText.setOrigin(wb.width / 2.f, wb.height / 2.f);
            warningText.setPosition(960.f, 530.f); window.draw(warningText);
        }

        if (phase == GamePhase::LEVEL_CLEAR)
        {
            float pulse = 0.5f + 0.5f * sinf(phaseTimer * 6.f);
            sf::Uint8 alpha = (sf::Uint8)(160 + 95 * pulse);
            sf::Text clearText; clearText.setFont(hudFont);
            clearText.setString(currentLevel < 3
                ? "BOSS DEFEATED  - YOU ARE WORTHY FOR NEXT LEVEL"
                : "BOSS DEFEATED  -  YOU DA GOAT!");
            clearText.setCharacterSize(52); clearText.setStyle(sf::Text::Bold);
            clearText.setFillColor(sf::Color(80, 255, 120, alpha));
            sf::FloatRect cb = clearText.getLocalBounds();
            clearText.setOrigin(cb.width / 2.f, cb.height / 2.f);
            clearText.setPosition(960.f, 530.f); window.draw(clearText);
        }
    }

    // ── drawAll() ───────────────────────────
    // Draw every game object in the correct back-to-front order,
    // then all HUD elements on top.
    // heartSprites[] and dashIcon are owned by main() and passed in.
    void drawAll(sf::RenderWindow& window, sf::Sprite heartSprites[], int heartCount, sf::Sprite& dashIcon)
    {
        // ── Background layer ─────────────────
        for (int i = 0; i < MAX_STARS; i++) stars[i]->draw(window);
        for (int i = 0; i < MAX_ASTEROIDS; i++) asteroids[i]->draw(window);

        // ── Enemies ──────────────────────────
        for (int i = 0; i < MAX_DRONES; i++) drones[i]->draw(window);
        for (int i = 0; i < MAX_VIPERS; i++) vipers[i]->draw(window);
        for (int i = 0; i < MAX_SEEKERS; i++) seekers[i]->draw(window);

        // ── VFX ──────────────────────────────
        for (int i = 0; i < MAX_EXPLOSIONS; i++) explosions[i]->draw(window);
        for (int i = 0; i < MAX_EMP_DROPS; i++) empDrops[i]->draw(window);
        for (int i = 0; i < MAX_POWER_DROPS; i++) powerDrops[i]->draw(window);

        // ── Boss (only during BOSS_FIGHT) ────
        if (phase == GamePhase::BOSS_FIGHT)
        {
            if (currentLevel == 1) bossCruiser->draw(window);
            else if (currentLevel == 2) bossTwinCannons->draw(window);
            else if (currentLevel == 3) bossMothership->draw(window);
        }

        // ── Player layer ─────────────────────
        empWave.draw(window);                            // EMP ring (if active)
        for (int i = 0; i < MAX_LASER_BEAMS; i++) laserBeams[i]->draw(window);
        ship.drawShield(window);                         // Shield bubble / particles
        ship.draw(window);                               // Ship sprite

        // ── Projectiles ──────────────────────
        for (int i = 0; i < MAX_BULLETS; i++) bullets[i]->draw(window);
        for (int i = 0; i < MAX_SPREAD_BULLETS; i++) spreadBullets[i]->draw(window);
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemyBullets[i]->draw(window);

        // ── HUD ──────────────────────────────
        for (int i = 0; i < heartCount; i++) window.draw(heartSprites[i]); // Health hearts
        drawEMPHUD(window);
        drawShieldHUD(window);
        drawWeaponHUD(window);
        drawWaveHUD(window);
        drawPhaseBanner(window);

        // Dash icon: dim when on cooldown, bright when ready
        float ratio = ship.getDashRatio();
        dashIcon.setColor(sf::Color(255, 255, 255, (sf::Uint8)(60 + 195 * ratio)));
        window.draw(dashIcon);

        // Dash cooldown bar next to the icon
        float barX = 1920.f - 110.f, barY = 160.f;
        sf::RectangleShape bg(sf::Vector2f(80.f, 6.f));
        bg.setPosition(barX, barY); bg.setFillColor(sf::Color(60, 60, 60, 200)); window.draw(bg);
        sf::RectangleShape fill(sf::Vector2f(80.f * ratio, 6.f));
        fill.setPosition(barX, barY);
        fill.setFillColor(sf::Color(255, (sf::Uint8)(255 * ratio), 80, 220)); window.draw(fill);

        // Score text (top-centre-left)
        sf::Text scoreTxt; scoreTxt.setFont(hudFont);
        scoreTxt.setString("SCORE: " + std::to_string(Score));
        scoreTxt.setCharacterSize(34); scoreTxt.setFillColor(sf::Color(220, 220, 255, 230));
        scoreTxt.setPosition(1000.f / 2.f - 120.f, 18.f); window.draw(scoreTxt);

        // Multiplier text (changes colour at x2 and x4)
        sf::Text multTxt; multTxt.setFont(hudFont);
        multTxt.setString("x" + std::to_string(mult));
        multTxt.setCharacterSize(34);
        sf::Color multColor = (mult == 4) ? sf::Color(255, 80, 80, 240)
            : (mult == 2) ? sf::Color(255, 220, 60, 240)
            : sf::Color(160, 160, 160, 180);
        multTxt.setFillColor(multColor); multTxt.setPosition(2820.f / 2.f + 110.f, 18.f);
        window.draw(multTxt);

        window.display(); // Swap back-buffer to screen
    }

    // Public accessors used by main()
    Ship& getShip() { return ship; }
    bool       isGameOver() const { return gameOver; }
    sf::Texture& getHeartTex() { return texHeart; }
    int        getLevel()   const { return currentLevel; }
    GamePhase  getPhase()   const { return phase; }
};


// _____________________________________________
//  MainMenu CLASS
//  Displayed before the game starts.
//  Player chooses ARCADE or SURVIVAL with
//  Up/Down arrow keys and Enter/Space.
// _____________________________________________
class MainMenu
{
private:
    sf::Font   font;
    sf::Texture bgTex;      // Optional background image
    sf::Sprite  bgSprite;
    Star* stars[MAX_STARS]; // Parallax stars on the menu screen (same class as in-game)

    int      selected; // 0 = ARCADE, 1 = SURVIVAL
    GameMode choice;   // Set when player confirms selection
    bool     confirmed;

    sf::Text titleText;
    sf::Text optionTexts[2];    // Labels for each mode
    sf::Text subtitleTexts[2];  // Short description under each label
    sf::Text controlsText;      // "UP/DOWN to select, ENTER to confirm"
    sf::RectangleShape selector; // Animated box that highlights the selected option

public:
    MainMenu() : selected(0), choice(GameMode::NONE), confirmed(false)
    {
        // Allocate and randomise background stars
        for (int i = 0; i < MAX_STARS; i++) { stars[i] = new Star(); stars[i]->spawn(); }

        font.loadFromFile("assets/font.ttf");

        // Load background image; scale to fill 1920×1080
        if (bgTex.loadFromFile("assets/22.png"))
        {
            bgTex.setSmooth(true); bgSprite.setTexture(bgTex);
            sf::Vector2u ts = bgTex.getSize();
            bgSprite.setScale(1920.f / ts.x, 1080.f / ts.y);
            bgSprite.setColor(sf::Color(180, 180, 180, 255)); // Slightly dimmed
        }

        // Title text centred at y=200
        titleText.setFont(font); titleText.setString("SPICY INVADERS");
        titleText.setCharacterSize(90); titleText.setFillColor(sf::Color(152, 255, 152));
        titleText.setStyle(sf::Text::Bold);
        sf::FloatRect tb = titleText.getLocalBounds();
        titleText.setOrigin(tb.width / 2.f, tb.height / 2.f); titleText.setPosition(960.f, 200.f);

        // Two menu options
        const char* labels[2] = { "ARCADE MODE", "SURVIVAL MODE" };
        float yPositions[2] = { 480.f, 650.f };
        for (int i = 0; i < 2; i++)
        {
            optionTexts[i].setFont(font); optionTexts[i].setString(labels[i]);
            optionTexts[i].setCharacterSize(52); optionTexts[i].setFillColor(sf::Color::White);
            sf::FloatRect ob = optionTexts[i].getLocalBounds();
            optionTexts[i].setOrigin(ob.width / 2.f, ob.height / 2.f);
            optionTexts[i].setPosition(960.f, yPositions[i]);

            subtitleTexts[i].setFont(font); subtitleTexts[i].setCharacterSize(26);
            subtitleTexts[i].setFillColor(sf::Color(152, 255, 152));
            sf::FloatRect sb = subtitleTexts[i].getLocalBounds();
            subtitleTexts[i].setOrigin(sb.width / 2.f, sb.height / 2.f);
            subtitleTexts[i].setPosition(960.f, yPositions[i] + 50.f);
        }

        // Animated selection box
        selector.setSize(sf::Vector2f(700.f, 110.f)); selector.setOrigin(350.f, 55.f);
        selector.setFillColor(sf::Color(0, 0, 0, 0));
        selector.setOutlineThickness(2.5f); selector.setOutlineColor(sf::Color(152, 255, 152, 200));
    }

    // Destructor: delete heap-allocated stars
    ~MainMenu()
    {
        for (int i = 0; i < MAX_STARS; i++) { delete stars[i]; stars[i] = nullptr; }
    }

    // Handle keyboard input; return false when a choice is confirmed (exits menu loop)
    bool handleEvent(const sf::Event& event)
    {
        if (event.type == sf::Event::KeyPressed)
        {
            if (event.key.code == sf::Keyboard::Up)    selected = (selected == 0) ? 1 : selected - 1;
            else if (event.key.code == sf::Keyboard::Down)  selected = (selected + 1) % 2;
            else if (event.key.code == sf::Keyboard::Return || event.key.code == sf::Keyboard::Space)
            {
                choice = (selected == 0) ? GameMode::ARCADE : GameMode::SURVIVAL;
                confirmed = true;
            }
        }
        return !confirmed; // Return true while menu should keep running
    }

    // Scroll all background stars
    void update(float dt) { for (int i = 0; i < MAX_STARS; i++) stars[i]->update(dt); }

    // Draw the full menu screen
    void draw(sf::RenderWindow& window, float elapsedTime)
    {
        window.clear(sf::Color(5, 5, 15));
        window.draw(bgSprite);
        for (int i = 0; i < MAX_STARS; i++) stars[i]->draw(window);
        window.draw(titleText);

        float yPositions[2] = { 480.f, 650.f };
        float pulse = 0.5f + 0.5f * sinf(elapsedTime * 2.f); // Pulsing outline brightness

        for (int i = 0; i < 2; i++)
        {
            if (i == selected)
            {
                // Highlight selected option in green with a pulsing box
                optionTexts[i].setFillColor(sf::Color(152, 255, 152));
                selector.setPosition(960.f, yPositions[i] + 13.f);
                selector.setOutlineColor(sf::Color(152, 255, 152, (sf::Uint8)(180 + 70 * pulse)));
                window.draw(selector);
            }
            else { optionTexts[i].setFillColor(sf::Color(180, 180, 180)); }

            window.draw(optionTexts[i]);
            window.draw(subtitleTexts[i]);
        }
        window.draw(controlsText);
        window.display();
    }

    GameMode getChoice() const { return choice; }
};


// _____________________________________________
//  main()
//  Program entry point.
//  1. Show main menu → get game mode
//  2. Run main game loop
//  3. On game over: show Game Over screen, wait for ESC
// _____________________________________________
int main()
{
    // Create the window (1920×1080, windowed)
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Space Invaders");

    // Load all audio; play menu music immediately
    audio.load();
    audio.playMenu();

    // ── Main Menu ────────────────────────────
    GameMode selectedMode = GameMode::NONE;
    {
        MainMenu menu;
        sf::Clock menuClock, totalTime;
        bool menuRunning = true;

        while (window.isOpen() && menuRunning)
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) { window.close(); return 0; }
                menuRunning = menu.handleEvent(event); // false when player confirms
            }
            float dt = menuClock.restart().asSeconds();
            menu.update(dt);
            menu.draw(window, totalTime.getElapsedTime().asSeconds());
        }
        selectedMode = menu.getChoice();
    } // MainMenu destroyed here (frees its star pool)

    audio.playGame(); // Switch to game music
    if (!window.isOpen()) return 0;

    // ── Game setup ───────────────────────────
    Game game(selectedMode); // Construct with chosen mode

    // Create heart sprites – one per HP slot, positioned in the top-left
    // heartSprites is a heap array so the size could vary; here it's always MAX_HEARTS
    sf::Sprite* heartSprites = new sf::Sprite[MAX_HEARTS];
    for (int i = 0; i < MAX_HEARTS; i++)
    {
        heartSprites[i].setTexture(game.getHeartTex());
        heartSprites[i].setScale(0.2f, 0.2f);
        heartSprites[i].setPosition(30.f + i * 70.f, 30.f); // Spaced 70 px apart
    }

    // Dash icon in the top-right corner
    sf::Texture dashTex; dashTex.loadFromFile("assets/11.png");
    sf::Sprite dashIcon(dashTex);
    sf::FloatRect db = dashIcon.getLocalBounds();
    dashIcon.setScale(150.f / db.width, 150.f / db.width);
    dashIcon.setPosition(1920.f - 150.f, 10.f);

    // Clocks and state for the game loop
    sf::Clock shootCooldown;        // Limits how fast the player can fire
    bool prevN = false;             // Previous frame's N key state (for edge detection)
    bool paused = false;
    bool prevEsc = false;           // Previous frame's Escape state (for edge detection)
    int  pauseSelected = 0;         // Reserved for future pause menu options

    pauseFont.loadFromFile("assets/font.ttf"); // Global pause font (used in paused block)

    // ── Main game loop ───────────────────────
    while (window.isOpen())
    {
        // Process window events (close button etc.)
        sf::Event event;
        while (window.pollEvent(event))
            if (event.type == sf::Event::Closed) window.close();

        // Toggle pause on Escape (edge-detected so one press = one toggle)
        bool escNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Escape);
        if (escNow && !prevEsc) { paused = !paused; pauseSelected = 0; }
        prevEsc = escNow;

        // ── Pause screen ─────────────────────
        if (paused)
        {
            // Draw frozen game state underneath the "PAUSED" text
            game.drawAll(window, heartSprites, MAX_HEARTS, dashIcon);
            sf::Text pauseText; pauseText.setFont(pauseFont); pauseText.setString("PAUSED");
            pauseText.setCharacterSize(90); pauseText.setFillColor(sf::Color::White);
            sf::FloatRect pb = pauseText.getLocalBounds();
            pauseText.setOrigin(pb.width / 2.f, pb.height / 2.f); pauseText.setPosition(960.f, 540.f);
            window.draw(pauseText); window.display();
            continue; // Skip all update and input processing while paused
        }

        // ── Read input ───────────────────────
        bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        bool space = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
        bool eKey = sf::Keyboard::isKeyPressed(sf::Keyboard::E);
        bool nNow = sf::Keyboard::isKeyPressed(sf::Keyboard::N);
        bool nPressed = nNow && !prevN; // True only on the frame N is first pressed
        prevN = nNow;

        // ── Shooting ─────────────────────────
        // Fire rate is limited by shootCooldown; laser is handled differently (held beam)
        WeaponMode wm = game.getShip().getWeapon();
        if (wm == WeaponMode::LASER)
        {
            if (space) { game.shoot(true); audio.playShoot(); } // Beam refreshed every frame
        }
        else if (wm == WeaponMode::SPREAD)
        {
            float fireRate = 0.12f; // Slightly slower than normal for balance
            if (space && shootCooldown.getElapsedTime().asSeconds() > fireRate)
            {
                game.shoot(true); audio.playShoot(); shootCooldown.restart();
            }
        }
        else
        {
            float fireRate = 0.1f; // Normal fire rate
            if (space && shootCooldown.getElapsedTime().asSeconds() > fireRate)
            {
                game.shoot(true); audio.playShoot(); shootCooldown.restart();
            }
        }

        // ── Update ───────────────────────────
        game.update(left, right, up, down, eKey, nPressed, space);

        // ── Damage check ─────────────────────
        // Called every frame (no clock gate) so contact damage is detected immediately.
        // ship.startInvincibility() inside tryDamageShip prevents rapid repeated hits.
        game.tryDamageShip();

        // Sync heart HUD colours to current ship HP every frame.
        // Hearts below current HP become grey; hearts at or below HP stay white.
        {
            int h = game.getShip().getHealth();
            for (int i = 0; i < MAX_HEARTS; i++)
            {
                if (i < h)
                    heartSprites[i].setColor(sf::Color(255, 255, 255, 255)); // Full white = alive
                else
                    heartSprites[i].setColor(sf::Color(80, 80, 80, 150));   // Dim grey = lost
            }
        }

        // ── Game Over screen ─────────────────
        if (game.isGameOver())
        {
            // Draw the last frame of gameplay underneath the overlay
            window.clear(sf::Color(5, 5, 15));
            game.drawAll(window, heartSprites, MAX_HEARTS, dashIcon);

            // Semi-transparent black overlay to darken the background
            sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 160));
            window.draw(overlay);

            // "GAME OVER" in large red text
            sf::Text gameOverText;
            gameOverText.setFont(pauseFont);
            gameOverText.setString("GAME OVER");
            gameOverText.setCharacterSize(100); gameOverText.setStyle(sf::Text::Bold);
            gameOverText.setFillColor(sf::Color(255, 40, 40, 255));
            sf::FloatRect gb = gameOverText.getLocalBounds();
            gameOverText.setOrigin(gb.width / 2.f, gb.height / 2.f);
            gameOverText.setPosition(960.f, 460.f);
            window.draw(gameOverText);

            // Final score
            sf::Text finalScore;
            finalScore.setFont(pauseFont);
            finalScore.setString("FINAL SCORE: " + std::to_string(Score));
            finalScore.setCharacterSize(48); finalScore.setFillColor(sf::Color(220, 220, 255, 230));
            sf::FloatRect sb = finalScore.getLocalBounds();
            finalScore.setOrigin(sb.width / 2.f, sb.height / 2.f);
            finalScore.setPosition(960.f, 590.f);
            window.draw(finalScore);

            // Quit prompt
            sf::Text prompt;
            prompt.setFont(pauseFont);
            prompt.setString("Press ESC to quit");
            prompt.setCharacterSize(30); prompt.setFillColor(sf::Color(150, 150, 150, 200));
            sf::FloatRect pb2 = prompt.getLocalBounds();
            prompt.setOrigin(pb2.width / 2.f, pb2.height / 2.f);
            prompt.setPosition(960.f, 700.f);
            window.draw(prompt);

            window.display();

            // Wait loop: keep window alive until player presses ESC or closes window
            while (window.isOpen())
            {
                sf::Event e;
                while (window.pollEvent(e))
                    if (e.type == sf::Event::Closed) window.close();
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) window.close();
            }
            break; // Exit the main game loop
        }

        // ── Normal frame draw ─────────────────
        window.clear(sf::Color(5, 5, 15));
        game.drawAll(window, heartSprites, MAX_HEARTS, dashIcon);
    }

    // Free the heart sprite array allocated in main()
    delete[] heartSprites;
    return 0;
}