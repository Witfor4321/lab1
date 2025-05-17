#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const {
		return 16.f * (float)render.size;
	}

	bool IsAlive() {
		return alive;
	}
	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}
	void TakeDamage(int dmg) {
		if (!alive){
				return;
		}
			hp -= dmg;
		if (hp <= 0){
				alive = 0;
		}
		
		return;
	}
	int GetSize() const {
		return static_cast<int>(render.size);
	}

protected:
	void init(int screenW, int screenH) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);
	}

	TransformA transform;
	Physics    physics;
	Renderable render;
	bool alive = true;
	float hp = 20 * (float)render.size;
	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};
class GeebleAsteroid : public Asteroid {
public:
	GeebleAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 15;
		if (!GeebleLoaded) {
		textureGeeble = LoadTexture("geeble.png");
		GenTextureMipmaps(&textureGeeble);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(textureGeeble, 2);
		GeebleLoaded = true;
		}
		scale = 0.2f;
		hp = 20 * (float)render.size;
	}
	void Draw() const override {
		Rectangle source = { 0, 0, static_cast<float>(textureGeeble.width), static_cast<float>(textureGeeble.height) };
		Rectangle dest = { 
			transform.position.x,
			transform.position.y,
			textureGeeble.width * scale * (float)render.size * 0.5f,
			textureGeeble.height * scale * (float)render.size * 0.5f

		};
		Vector2 origin = {
		dest.width *0.5f,
		dest.height *0.5f};
		DrawTexturePro(textureGeeble, source, dest , origin ,transform.rotation ,WHITE);
	}
	float GetRadius() const override {
		return (textureGeeble.width * scale * (float)render.size) * 0.25f;
	}
private:
	static Texture2D textureGeeble;
	static bool GeebleLoaded;
	float scale;

};
Texture2D GeebleAsteroid::textureGeeble = { 0 };
bool GeebleAsteroid::GeebleLoaded = false;

// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5,GEEBLE=6, RANDOM = 0 };
// Ship selector
enum class Character {PIBBLE, WASHINGTON,GMAIL, COUNT};
// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	case AsteroidShape::GEEBLE:
		return std::make_unique<GeebleAsteroid>(w, h);
	default: {
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)));
	}
	}
}

// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, MISSILE,GRENADES,SHRAPNEL, COUNT};

class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
		exploded = false;
		explodeTime = 2.0f;
		lifetime = 0.0f;
		shrapnel = false;
		if (type == WeaponType::SHRAPNEL) {
			shrapnel = true;
		}
		if (!TextureLoaded) {
			textureMissile = LoadTexture("spark_flame.png");
			TextureLoaded = true;
		}
	}

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		if (exploded)
		{
		explodeTime += 100*dt;
		lifetime += dt;
		}
		if (type == WeaponType::GRENADES || type == WeaponType::SHRAPNEL) {
			lifetime += dt;
			if (lifetime >= 30.0*dt) {
				explode();
			}
		}
		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}

	void explode() {
		if (!exploded && type == WeaponType::MISSILE) {
			explodeTime = 5.0f;
			lifetime = 0.0f;
			physics.velocity = { 0.0f, 0.0f };
			exploded = true;
		}else if (type == WeaponType::GRENADES || type == WeaponType::SHRAPNEL) {
			physics.velocity = { 0.0f, 0.0f };
			exploded = true;
			explodeTime = 45.0f;
		}
	}
	WeaponType split() const {
			return WeaponType::SHRAPNEL;
	}

	bool IsShard() const {
		return shrapnel;
	}

	bool checkexplosion() const {
		return exploded;
	}
	float getlifetime() const {
		return lifetime;
	}

	void Draw() const {
		if (type == WeaponType::BULLET) {
			DrawCircleV(transform.position, 5.f, WHITE);
		}
		else if (type == WeaponType::LASER) {
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
			DrawRectangleRec(lr, RED);
		}
		else if(type == WeaponType::GRENADES && !exploded) {
			DrawCircleV(transform.position, 5.f, GREEN);
		}
		else if (type == WeaponType::MISSILE && !exploded)
		{
				static constexpr float MISSILE_LENGTH = 20.f;
				DrawTriangle(
					{ transform.position.x, transform.position.y - MISSILE_LENGTH },
					{ transform.position.x - 5.f, transform.position.y },
					{ transform.position.x + 5.f, transform.position.y },
					BLUE
				);
		}
		else if (shrapnel && !exploded) {
			DrawCircleV(transform.position, 5.f, RED);
		}
		else if(exploded){
			Vector2 explosionradius = {
				transform.position.x -explodeTime * 2.5f,
				transform.position.y -explodeTime *2.5f 
			};
			DrawTextureEx(textureMissile, explosionradius, 0.0f, explodeTime*0.04f, WHITE);
		}

	}
	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		if (type == WeaponType::LASER) {
			return 5.0f;
		}
		else if (type == WeaponType::BULLET) {
			return 2.0f;
		}
		else if (type == WeaponType::MISSILE && exploded == false) {
			return 5.0f;
		}
		else if (type == WeaponType::MISSILE && exploded == true) {
			return explodeTime;
		}
		else return 5.0f;

	}

	int GetDamage() const {
		return baseDamage;
	}

	WeaponType GetWeaponType() const { 
		return type;
	}
private:
	bool shrapnel;
	float lifetime;
	bool exploded;
	float explodeTime;
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;

	static bool TextureLoaded;
	static Texture textureMissile;
};

bool Projectile::TextureLoaded = false;
Texture Projectile::textureMissile = { 0 };

inline static std::vector<Projectile> MakeProjectile(WeaponType wt, const Vector2 pos, float speed)
{	
	std::vector<Projectile> projectiles;
	Vector2 vel{ 0.0f, -speed };

	if (wt == WeaponType::LASER) {
		 projectiles.emplace_back(pos, vel, 20, wt);
	}
	else if(wt == WeaponType::BULLET){
		 projectiles.emplace_back(pos, vel, 10, wt);
	}
	else if (wt == WeaponType::MISSILE) {
		 projectiles.emplace_back(pos, vel, 45, wt);
	}
	else if (wt == WeaponType::GRENADES) {
		constexpr float angle = PI / 4.0f;
		Vector2 vel1{ -cosf(angle) * speed, sinf(-angle) * speed };
		Vector2 vel2{ cosf(angle)* speed, -sinf(+angle)* speed };
		projectiles.emplace_back(pos, vel1, 30, wt);
		projectiles.emplace_back(pos, vel2, 30, wt);
	}
	else if (wt == WeaponType::SHRAPNEL) {
		constexpr int shards = 6;
		constexpr float angleStep = 2.0f * PI / shards; 
		constexpr float baseAngle = -PI / 2.0f;          
		for (int i = 0; i < shards; ++i) {
			float angle = baseAngle + i * angleStep;
			Vector2 vel3{ cosf(angle) * speed, sinf(angle) * speed };
			projectiles.emplace_back(pos, vel3, 30, WeaponType::SHRAPNEL);
		}
	}
	return projectiles;

}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
			 screenW * 0.5f,
			 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		fireRateMissile = 3.0f;
		fireRateGrenades = 3.0f;
		fireRateShrapnel = 4.0f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
		spacingMissile = 100.f;
		spacingGrenades = 100.0f;
		spacingShrapnel = 80.0f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}
	void BuffHp(int hpBuff) {
		hp += hpBuff;
	}
	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		if(wt == WeaponType::LASER){
			return fireRateLaser;
		}
		else if (wt == WeaponType::BULLET) {
			return fireRateBullet;
		}
		else if (wt == WeaponType::MISSILE) {
			return fireRateMissile;
		}
		else if (wt == WeaponType::GRENADES) {
			return fireRateGrenades;
		}
		else if (wt == WeaponType::SHRAPNEL) {
			return fireRateShrapnel;
		}
		else return 5.0f;
	}

	float GetSpacing(WeaponType wt) const {
		if (wt == WeaponType::LASER) {
			return spacingLaser;
		}
		else if (wt == WeaponType::BULLET) {
			return spacingBullet;
		}
		else if (wt == WeaponType::MISSILE) {
			return spacingMissile;
		}
		else if (wt == WeaponType::GRENADES) {
			return spacingGrenades;
		}
		else if (wt == WeaponType::SHRAPNEL) {
			return spacingShrapnel;
		}
		else return 100.0f;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      fireRateMissile;
	float		fireRateGrenades;
	float		fireRateShrapnel;
	float      spacingLaser;
	float      spacingBullet;
	float	  spacingMissile;
	float		spacingGrenades;
	float		spacingShrapnel;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		currentCharacter = Character::PIBBLE;
		texture = LoadTexture("pibb.png");
		texture2 = LoadTexture("sleepy.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
		size = static_cast<float>(texture.width) * scale;

	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void SetCharacter(Character cc) {
			UnloadTexture(texture);
		if (cc == Character::PIBBLE) {
			hp = 100.0f;
			speed = 250.0f;
			texture = LoadTexture("pibb.png");
			scale = 0.25f;
		}
		else if (cc ==Character::WASHINGTON){
			hp = 50.0f;
			speed = 400.0f;
			texture = LoadTexture("washington.png");
			scale = size / texture.width;
		}
		else {
			hp = 75.0f;
			speed = 350.0f;
			texture = LoadTexture("gmail.png");
			scale = size / texture.width;
		}
			GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
			SetTextureFilter(texture, 2);
	};

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		if (alive) {
			DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
		}
		else {
			float scale2 = static_cast<float>(texture.width) / texture2.width * scale;
			DrawTextureEx(texture2, dstPos, 0.0f, scale2, WHITE);
		}
	}

	float GetRadius() const override {
		return (texture.width * scale ) * 0.5f;
	}

private:
	Texture2D texture, texture2;
	float     scale;
	float size;
	Character currentCharacter;
};

// Adds
class Adds {
public:
	Adds() {
		paused = false;
		timer = 0;
		image1 = LoadTexture("add1.png");
		image2 = LoadTexture("add2.png");

	}
	~Adds() {
		UnloadTexture(image1);
		UnloadTexture(image2);
	}

	int GetHpBuff() {
		return hpBuff;
	}
	void WatchAdd() {
		timer = 0;
		paused = true;
	}
	bool IsPaused() {
		return paused;
	}
	void Update(float dt) {
		if (paused) {
			timer += dt;
			if (timer >= maxTime) {
				paused = false;
			}
		}
	}
	void Draw(int w, int h) {
		if (!paused) return;
		float scale;
		if (timer < maxTime * 0.5f) {

			float scaleW = static_cast<float>(w) / image1.width;
			float scaleH = static_cast<float>(h) / image1.height;
			if (scaleW > scaleH) {
				scale = scaleH;
			}
			else  scale = scaleW;
			DrawTextureEx(image1, { 0,0 }, 0, scale, WHITE);
		}
		else {
			float scaleW = static_cast<float>(w) / image2.width;
			float scaleH = static_cast<float>(h) / image2.height;
			if (scaleW > scaleH) {
				scale = scaleH;
			}
			else  scale = scaleW;
			DrawTextureEx(image2, { 0, 0 }, 0, scale, WHITE);
		}
	}
private:
	const int hpBuff = 20;
	bool paused;
	float timer;
	Texture2D image1, image2;
	const float maxTime = 5.0f;

};

// --- APPLICATION ---
class Application {
public:

	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {

		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");
		auto player = std::make_unique <PlayerShip >(C_WIDTH, C_HEIGHT);

		Adds adds;

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;
		textureBackground = LoadTexture("background.png");

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

		if (IsKeyPressed(KEY_T) && !adds.IsPaused() && player->IsAlive()) {
			adds.WatchAdd();
			player->BuffHp(adds.GetHpBuff());
		}

		if (adds.IsPaused()) {
			adds.Update(dt);
			Renderer::Instance().Begin();
			adds.Draw(C_WIDTH, C_HEIGHT);
			Renderer::Instance().End();

			continue; 
		}

			// Update player
			player->Update(dt);
			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
				currentCharacter = Character::PIBBLE;
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::RANDOM;
			}
			if (IsKeyPressed(KEY_FIVE)) {
				currentShape = AsteroidShape::GEEBLE;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB) && currentCharacter != Character::GMAIL) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % (static_cast<int>(WeaponType::COUNT) - 2));
			}

			//Change charracter
			if (IsKeyPressed(KEY_F) && player->IsAlive())
			{
				Character previousCharacter = currentCharacter;
				currentCharacter = static_cast<Character>((static_cast<int>(currentCharacter) + 1) % static_cast<int>(Character::COUNT));
				player->SetCharacter(currentCharacter);
				if (currentCharacter == Character::GMAIL) {
					currentWeapon = WeaponType::GRENADES;
				}
				else if (previousCharacter == Character::GMAIL) {
					currentWeapon = WeaponType::LASER;
				}

			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						if (currentWeapon == WeaponType::GRENADES) {
							std::vector<Projectile> grenade1 = MakeProjectile(currentWeapon, p, projSpeed);
							projectiles.insert(projectiles.end(), grenade1.begin(), grenade1.end());
						}
						else {
							std::vector<Projectile> newbullet = MakeProjectile(currentWeapon, p, projSpeed);
							projectiles.insert(projectiles.end(), newbullet.begin(), newbullet.end());
						}
						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}

				if (IsKeyPressed(KEY_E))
				{
					for (int i = 0; i < projectiles.size(); i++)
					{
						if (projectiles[i].GetWeaponType() == WeaponType::MISSILE)
						{
							projectiles[i].explode();
						}
					}
				}
			}
			// Detonating
			if (IsKeyPressed(KEY_E))
			{
				for (int i = 0; i < projectiles.size(); i++)
				{
					if (projectiles[i].GetWeaponType() == WeaponType::MISSILE) {
						projectiles[i].explode();
					}
				}
			}
			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				if ((*pit).getlifetime() >= 50 * dt) {
					pit = projectiles.erase(pit);
					removed = true;
					break;
				}

				if ((*pit).GetWeaponType() == WeaponType::GRENADES && (*pit).checkexplosion()) {
					Vector2 p = (*pit).GetPosition();
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);
					removed = true;
					std::vector<Projectile> shrapnel1 = MakeProjectile((*pit).split(), p, projSpeed);
					projectiles.insert(projectiles.end(), shrapnel1.begin(), shrapnel1.end());
					pit = projectiles.erase(pit);
					break;
				}

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());


					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {

						(*ait)->TakeDamage((*pit).GetDamage());
						if (!(*ait)->IsAlive())
						{
							ait = asteroids.erase(ait);
						}

						if ((*pit).checkexplosion() == 0 && (*pit).GetWeaponType() == WeaponType::MISSILE) {
							(*pit).explode();
							break;
						}
						else if ((*pit).checkexplosion() == 0) {
							pit = projectiles.erase(pit);
							removed = true;
							break;
						}
						else {
							if ((*pit).checkexplosion() == 1)
							{
								pit = projectiles.erase(pit);
								removed = true;
							}
							break;
						}
					}
				}

				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision = [&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
			}

			// Render everything
			{
					Renderer::Instance().Begin();
					Rectangle source = { 0, 0, static_cast<float>(textureBackground.width), static_cast<float>(textureBackground.height) };
					Rectangle dest = { 0, 0, static_cast<float>(C_WIDTH), static_cast<float>(C_HEIGHT) };
					Vector2 origin = { 0, 0 };
					DrawTexturePro(textureBackground, source, dest, origin, 0.0f, Color{ 255, 255, 255, 130 });
					DrawText(TextFormat("HP: %d", player->GetHP()),
						10, 10, 20, GREEN);
					const char* weaponName = nullptr;
					if ((currentWeapon == WeaponType::LASER)) {
						weaponName = "Laser";
					}
					else if (currentWeapon == WeaponType::BULLET) {
						weaponName = "Bullet";
					}
					else if (currentWeapon == WeaponType::MISSILE) {
						weaponName = "Missile";
					}
					else if (currentWeapon == WeaponType::GRENADES) {
						weaponName = "Grenades";
					}

					DrawText(TextFormat("Weapon: %s", weaponName),
						10, 40, 20, BLUE);

					for (const auto& projPtr : projectiles) {
						projPtr.Draw();
					}
					for (const auto& astPtr : asteroids) {
						astPtr->Draw();
					}

					player->Draw();

					Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;
	Texture2D textureBackground = { 0 };

	AsteroidShape currentShape = AsteroidShape::GEEBLE;
	Character currentCharacter = Character::PIBBLE;

	static constexpr int C_WIDTH = 800;
	static constexpr int C_HEIGHT = 800;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;
	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};



int main() {
	Application::Instance().Run();
	return 0;
}
