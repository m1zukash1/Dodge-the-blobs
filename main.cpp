#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include <sstream>
#include <iomanip>
#include <list>
#include <time.h>
#include <stdlib.h>

using namespace sf;

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 900
#define PI 3.14159265


/// GAME DESIGN VARIABLES ///



/// PLAYER \\\


/// Greitis
float playerSpeed = 2.5;			// Žaidejo greitis
float friction = 0.05;				// 1 = max friction || 0 = so slippery you cant even move


/// Dash
float dashStrenght = 110;			// Žaidėjo šuolio stipris
float dashCooldown = 1.75;				// Laiko trukmė po, kurio žaidėjas vėl galės šoliuoti.

/// Shield
float shieldTime = 3;				// Laikas, kurio metu žaidėjas yra nemirtingas susinaudojus jo skydui.
int shieldPrice = 5000;			// Kiek scoro reikia surinkti, kad atgautu žaidėjas skydą.

///Trail animation
int animAlpha = 255;                // Trail animationo fade pradžia (255 = pilnai matosi ; 0 = visiškai nesimato)
float animScale = 0.95;             // Trail animationo scale pradžia (1 = pilno didumo ; 0 = visiškai nesimato)

int animAlphaFadeSpeed = 5;         // Trail animationo fade greitis (kaip greitai pranyksta trailas)
float animScaleDownSpeed = 0.0025;  // Trail animationo scale down greitis (kaip greitai mažėja trailas)

float _animSpawnCD = 0.05;          // Trail animationo atsiradimo dažnis sekundėmis



/// ENEMY \\\
 

///Priešų algorithmas
float enemySpawnCDStart = 1.0;      // Pradinis priešo spawn timerio cooldownas
float enemyCDSpeed = 0.005;        // Priešų atsiradimo timerio mažėjimo dažnis
float enemyCDCap = 0.2;            // Priešų atsiradimo timerio mažiausia reikšmė (greičiau priešai negali atsirasti neigu nurodyta šioj funkcjoj)


//Random Enemy Speed intervalai
float enemyBigSmallestSpeed = 1.5;  // Didelio priešo mažiausias galimas greitis
float enemyBigBiggestSpeed = 2.5;   // Didelio priešo didžiausias galimas greitis

float enemySmallestSpeed = 2.0;		// Paprasto priešo mažiausias galimas greitis
float enemyBiggestSpeed = 3.5;		// Paprasto priešo didžiausias galimas greitis


//

float pickupSpeed = 1.5;
float _animChangeFreq = 0.125;

int scorePerSecond = 100;
int scorePerPickUp = 500;

int popUpFadeStart = 255; //gali but tik int
float popUpScaleStart = 0.25;

float popUpScaleSpeedUp = 0.01;
int popUpFadeSpeedUp = 2; //gali but tik int
float popUpFadeMaxSize = 1;








/// FUNKCIJOS ///

int randIntRange(int from, int to) { //returnina random int intervale (from, to)
	return from + rand() % ((to + 1) - from);
}

float randFloatRange(float from, float to) { //returnina random float intervale (from, to)
	float r = (float)rand() / (float)RAND_MAX;
	return from + r * (to - from);
}

float lerp(float a, float b, float t)
{
	return (a + t * (b - a));
}

Vector2f normalizeVector2f(Vector2f vec) // vektoriaus normalizavimas
{
	float lenght = sqrt((vec.x * vec.x) + (vec.y * vec.y));
	if (lenght != 0)
	{
		return Vector2f(vec.x / lenght, vec.y / lenght);
	}
	else
	{
		return vec;
	}
}

//


struct scorePopUp {
	Text popUpLabel;
	Vector2f position;

	float s = popUpScaleStart;
	int a = popUpFadeStart;
	float scaleSpeed = popUpScaleSpeedUp;
	int fadeSpeed = popUpFadeSpeedUp;

	bool queueForDeletion = false;

	void init(Font &font, Vector2f _position){
		popUpLabel.setFont(font);
		popUpLabel.setCharacterSize(64);
		popUpLabel.setOutlineThickness(2);
		popUpLabel.setString(std::to_string(scorePerPickUp));
		position = _position;
		popUpLabel.setPosition(position);
		popUpLabel.setScale(s, s);

		popUpLabel.setOrigin(popUpLabel.getLocalBounds().width / 2, popUpLabel.getLocalBounds().height / 2);
	}

	void update(RenderWindow& window) {
		window.draw(popUpLabel);
		if(s <= popUpFadeMaxSize)
			s += scaleSpeed;
		if (s >= popUpFadeMaxSize) {
			if (!(a - fadeSpeed <= 0)) {
				a -= fadeSpeed;
			}
			else {
				a = 0;
				queueForDeletion = true;
			}
		}

		popUpLabel.setScale(s, s);
		popUpLabel.setFillColor(Color(255, 255, 255, a));
		popUpLabel.setOutlineColor(Color(0, 0, 0, a));
	}

};


struct GhostAnimation {
	Sprite sprite;

	int a = animAlpha; 
	float s = animScale;

	bool queueForDeletion = false;

	void init(Texture& texture, Vector2f _position) {
		texture.setSmooth(true);
		sprite.setTexture(texture);
		sprite.setOrigin(texture.getSize().x / 2, texture.getSize().y / 2);
		sprite.setPosition(_position);
	}

	void update(RenderWindow& window) {
		a -= animAlphaFadeSpeed;
		s -= animScaleDownSpeed;
		sprite.setColor(Color(255, 255, 255, a));
		sprite.setScale(Vector2f(s, s));
		window.draw(sprite);

		if (a <= 0) queueForDeletion = true;

	}

};

struct Entity {
	Vector2f position;
	Vector2f targetPosition;
	Vector2f direction;
	Sprite sprite;
	float speed;
	float size;

	void init(Texture& texture, Vector2f _position, float _speed){
		texture.setSmooth(true);
		sprite.setTexture(texture);
		position = _position;
		targetPosition = position;
		speed = _speed;

		size = texture.getSize().x / 2.f;

		sprite.setOrigin(texture.getSize().x / 2, texture.getSize().y / 2);
	}

	void update(RenderWindow& window) {
		sprite.setPosition(position);
		window.draw(sprite);
	}
};

struct Player : Entity {

	bool hasShield = true;
	bool invincible = false;

	int a = animAlpha;

	void updateMovement(bool dashReady = false) {
		direction = Vector2f(0, 0);
		if (Keyboard::isKeyPressed(Keyboard::W) or Keyboard::isKeyPressed(Keyboard::Up)) {
			direction.y -= 1;
		}
		if (Keyboard::isKeyPressed(Keyboard::S) or Keyboard::isKeyPressed(Keyboard::Down)) {
			direction.y += 1;
		}
		if (Keyboard::isKeyPressed(Keyboard::D) or Keyboard::isKeyPressed(Keyboard::Right)) {
			direction.x += 1;
		}
		if (Keyboard::isKeyPressed(Keyboard::A) or Keyboard::isKeyPressed(Keyboard::Left)) {
			direction.x -= 1;
		}

		direction = normalizeVector2f(direction);

		targetPosition += direction * speed;

		position.x = lerp(position.x, targetPosition.x, friction);
		position.y = lerp(position.y, targetPosition.y, friction);

	}


	void dash()
	{
		targetPosition += direction * dashStrenght;
	}

	void keepInBounds() {
		if (position.x <= 0) {
			position.x = 0;
		}
		if (position.x >= SCREEN_WIDTH) {
			position.x = SCREEN_WIDTH;
		}
		if (position.y <= 0) {
			position.y = 0;
		}
		if (position.y >= SCREEN_HEIGHT) {
			position.y = SCREEN_HEIGHT;
		}
	}

	void updateSprite(Texture& texture, int a) {
		sprite.setTexture(texture);
		sprite.setColor(Color(255, 255, 255, a));
	}

};

struct Enemy : Entity { 

	bool queueForDeletion = false;

	Clock animClock;
	float animChangeFreq = _animChangeFreq;

	Texture _texture;
	Texture _texture1;

	int lifeSpan = 2400; // 240 = 1s 

	void spawnEnemy(Sprite& sprite) // funkcija kuri randomizina kur atsiranda priesas ir taisiklingai nurodo, kur jam judeti
	{

		Texture _texture = *sprite.getTexture();
		int size = _texture.getSize().x;

		int siena = randIntRange(0, 3);

		switch (siena) 
		{

		case 0:
			position = Vector2f(randIntRange(0, SCREEN_WIDTH), -(size / 2));
			direction = Vector2f(randFloatRange(-1.f, 1.f), 1);
			sprite.setRotation(180 - direction.x * 180.0 / PI);
			break;
		case 1:
			position = Vector2f(SCREEN_WIDTH + (size / 2), randIntRange(0, SCREEN_HEIGHT));
			direction = Vector2f(-1, randFloatRange(-1.f, 1.f));
			sprite.setRotation(270 - direction.y * 180.0 / PI);
			break;
		case 2:
			position = Vector2f(randIntRange(0, SCREEN_WIDTH), SCREEN_HEIGHT + (size / 2));
			direction = Vector2f(randFloatRange(-1.f, 1.f), -1);
			sprite.setRotation(0 + direction.x * 180.0 / PI);
			break;
		case 3:
			position = Vector2f( -(size / 2), randIntRange(0, SCREEN_HEIGHT));
			direction = Vector2f(1, randFloatRange(-1.f, 1.f));
			sprite.setRotation(90 + direction.y * 180.0 / PI);
			break;
		}



	}

	void init(Texture& texture, Texture& texture1, float _speed) {
		texture.setSmooth(true);

		sprite.setTexture(texture);

		speed = _speed;

		_texture = texture;
		_texture1 = texture1;

		size = texture.getSize().x / 2.f;

		sprite.setOrigin(texture.getSize().x / 2, texture.getSize().y / 2);

		spawnEnemy(sprite);
	}

	void updateMovement()
	{
		direction = normalizeVector2f(direction);
		position += direction * speed;
	}

	bool animFrame = false;

	void update(RenderWindow& window) {
		sprite.setPosition(position);
		window.draw(sprite);
		lifeSpan -= 1;

		//printf("%f \n", animClock.getElapsedTime().asSeconds());
		if (animClock.getElapsedTime().asSeconds() >= animChangeFreq) {
			if (animFrame) {
				sprite.setTexture(_texture1);
			}
			else {
				sprite.setTexture(_texture);
			}
			animFrame = !animFrame;
			animClock.restart();
		}

		updateMovement();

		if (lifeSpan <= 0) queueForDeletion = true;

	}

};

struct Pickup : Enemy {

	float size;

	void init(Texture& texture, float _speed) {
		texture.setSmooth(true);

		sprite.setTexture(texture);

		speed = _speed;

		_texture = texture;

		sprite.setOrigin(texture.getSize().x / 2, texture.getSize().y / 2);

		size = texture.getSize().x / 2;

		spawnEnemy(sprite);
	}

	void update(RenderWindow& window) {
		sprite.setPosition(position);
		window.draw(sprite);
		lifeSpan -= 1;

		updateMovement();

		if (lifeSpan <= 0) queueForDeletion = true;

	}

};

bool isColliding(Vector2f player_position, float player_size, Pickup* _blob)
{
	return (_blob->position.x - player_position.x) * (_blob->position.x - player_position.x) +
		(_blob->position.y - player_position.y) * (_blob->position.y - player_position.y) <
		(player_size + _blob->size) * (player_size + _blob->size);
}

bool _isColliding(Vector2f player_position, float player_size, Enemy* _blob)
{
	return (_blob->position.x - player_position.x) * (_blob->position.x - player_position.x) +
		(_blob->position.y - player_position.y) * (_blob->position.y - player_position.y) <
		(player_size + _blob->size) * (player_size + _blob->size);
}

struct Button {

	Sprite sprite;
	Vector2f position;


	/*void init(Font& _font, String _text, Vector2f _position, int _size, int _jut) {
		text.setFont(_font);
		text.setString(_text);
		text.setOutlineThickness(2);
		text.setCharacterSize(_size);
		text.setOrigin(text.getLocalBounds().width / 2, text.getLocalBounds().height / 2);
		text.setPosition(_position);

		rect.setFillColor(Color(78, 66, 65, 255));
		rect.setPosition(Vector2f(_position.x, _position.y + text.getLocalBounds().height / 3));
		rect.setSize(Vector2f(text.getLocalBounds().width + _jut, text.getLocalBounds().height + _jut));
		rect.setOrigin(Vector2f(text.getOrigin().x + _jut / 2, text.getOrigin().y + _jut / 2));

		position = rect.getPosition();

	}*/

	void init(Texture& _texture, Vector2f _position) {
		sprite.setTexture(_texture);
		position = _position;
		sprite.setOrigin(_texture.getSize().x / 2, _texture.getSize().y / 2);
		sprite.setPosition(position);
	}

	void update(RenderWindow& window) {
		window.draw(sprite);
	}

	bool isPressed(RenderWindow& window) {
		return Mouse::isButtonPressed(Mouse::Left) and sprite.getGlobalBounds().contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
	}

};

struct TweenFloat {
	float from;
	float to;
	float duration;

	void init(float _from, float _to, float _duration) {
		from = _from;
		to = _to;
		duration = _duration;
	}

	float update(float time) {

	}

};

int main()
{


	RenderWindow window(VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Dodge The Blobs", Style::Close);
	Image icon; icon.loadFromFile("images/player.png");
	window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

gameStart:

	srand(time(0)); //seed randomizing

	int shieldScore = shieldPrice;

	int score = 0;
	Clock scoreClock;

	window.setFramerateLimit(240);

	/// TEXTURES ///

	Texture enemyTexture; enemyTexture.loadFromFile("images/enemy.png");
	Texture enemyTexture1; enemyTexture1.loadFromFile("images/enemy1.png");

	Texture enemyBigTexture; enemyBigTexture.loadFromFile("images/enemyBig.png");
	Texture enemyBigTexture1; enemyBigTexture1.loadFromFile("images/enemyBig1.png");

	Texture playerTexture; playerTexture.loadFromFile("images/player.png");
	Texture currentPlayerTexture; currentPlayerTexture = playerTexture;
	Texture player1hit; player1hit.loadFromFile("images/player1hit.png");

	Texture pickupTexture; pickupTexture.loadFromFile("images/pickup.png");

	Texture scoreUiTexture; scoreUiTexture.loadFromFile("images/scoreUI.png");
	Texture playButtonTexture; playButtonTexture.loadFromFile("images/playButton.png");
	Texture restartButtonTexture; restartButtonTexture.loadFromFile("images/restartButton.png");
	Texture gameNameTexture; gameNameTexture.loadFromFile("images/gameName.png");

	Sprite scoreUiSprite; scoreUiSprite.setTexture(scoreUiTexture);
	Sprite gameNameSprite; gameNameSprite.setTexture(gameNameTexture);

	gameNameSprite.setOrigin(Vector2f(gameNameTexture.getSize().x / 2, gameNameTexture.getSize().y / 2));
	gameNameSprite.setPosition(Vector2f(SCREEN_WIDTH / 2, 366));
	//



	Clock globalClock;
	Font defaultFont; defaultFont.loadFromFile("resources/Baloo.ttf");
	Text globalClockLabel;
	globalClockLabel.setOutlineThickness(2);
	globalClockLabel.setString("0");
	globalClockLabel.setFont(defaultFont);
	globalClockLabel.setCharacterSize(64);

	Text scoreLabel;
	scoreLabel.setFont(defaultFont);
	scoreLabel.setPosition(8, 8);
	scoreLabel.setOutlineThickness(2);
	scoreLabel.setCharacterSize(46);


	std::list<GhostAnimation*> ghostAnimations;
	std::list<Enemy*> enemies; 
	std::list<Pickup*> pickups;
	std::list<scorePopUp*> scorePopUps;

	Player player;
	player.init(playerTexture, Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2), playerSpeed);

	float waitTimeBeforeGameStart = 2.5f;

	float enemySpawnCD = enemySpawnCDStart; // enemy spawn cooldown
	Clock enemySpawnClock;

	float animSpawnCD = _animSpawnCD; //animation spawn cooldown
	Clock animSpawnClock;

	float invincibilityDuration = shieldTime; // invincibility timer after your shield activates
	Clock invincibilityTime;

	Clock dashCooldownClock;

	/*Text menu;
	menu.setFont(defaultFont);
	menu.setCharacterSize(128);
	menu.setOutlineThickness(2);
	menu.setString("DODGE \n    THE \n BLOBS");
	menu.setOrigin(menu.getLocalBounds().width / 2, 0);
	menu.setPosition(SCREEN_WIDTH / 2, 50);*/

	
	Text endscreen;
	endscreen.setFont(defaultFont);
	endscreen.setCharacterSize(128);
	endscreen.setOutlineThickness(4);
	endscreen.setFillColor(Color(255, 0, 0));
	endscreen.setString("GAME OVER");
	endscreen.setOrigin(endscreen.getLocalBounds().width / 2, 0);
	endscreen.setPosition(SCREEN_WIDTH / 2, 100);

	Button restartButton; 
	restartButton.init(restartButtonTexture, Vector2f(SCREEN_WIDTH / 2, 700));

	Button playButton;
	playButton.init(playButtonTexture, Vector2f(SCREEN_WIDTH / 2, 725));

	bool isInMenu = true;
	bool isGameOver = false;



	while (window.isOpen())
	{
		Event event;

		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
			{
				window.close();
				return EXIT_SUCCESS;
			}


			/// DASH ///


			if (event.type == Event::KeyPressed)
			{
				if (dashCooldownClock.getElapsedTime().asSeconds() >= dashCooldown and (event.key.code == Keyboard::LShift or event.key.code == Keyboard::Space)) {
					player.dash();
					dashCooldownClock.restart();
				}
			}
		}

		printf("Enemy spawn cooldown: %f \n", enemySpawnCD);
		//printf("Shield time: %f \n", shieldTime);
		printf("Has Shield: %s \n", player.hasShield ? "true" : "false");
		//printf("invincible: %s \n", player.invincible ? "true" : "false");
		printf("Dash cooldown Timer: %f , Dash cooldown %ff \n", dashCooldownClock.getElapsedTime().asSeconds(), dashCooldown);


		while (isInMenu) {
			while (window.pollEvent(event))
			{
				if (event.type == Event::Closed)
				{
					window.close();
					return EXIT_SUCCESS;
				}
			}
			window.clear(Color(105, 89, 88, 255));


			//printf("%s \n", playButton.isPressed(window) ? "true" : "false");

			playButton.update(window);
			window.draw(gameNameSprite);

			if (playButton.isPressed(window)) {
				window.setFramerateLimit(240);
				isInMenu = false;
				globalClock.restart();
				/*
				globalClock.restart();
				enemySpawnClock.restart();
				dashCooldownClock.restart();
				pickups.clear();
				score = 0;
				scoreLabel.setPosition(8, 8);
				scoreLabel.setOutlineThickness(2);
				scoreLabel.setCharacterSize(46);
				scoreLabel.setOrigin(0, 0);
				enemies.clear();
				ghostAnimations.clear();
				player.hasShield = true;
				player.position = Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
				player.direction = Vector2f(0, 0);
				player.targetPosition = Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
				*/
			}
			//window.draw(menu);
			window.display();
		}

		window.clear(Color(105, 89, 88, 255));




		/// TRAIL ANIMATION ///

		if (animSpawnClock.getElapsedTime().asSeconds() >= animSpawnCD) {
			GhostAnimation* anim = new GhostAnimation;
			anim->init(currentPlayerTexture, player.position);
			ghostAnimations.push_back(anim);
			animSpawnClock.restart();
		}


		for (auto animation : ghostAnimations) {
			animation->update(window);
		}

		for (auto i = ghostAnimations.begin(); i != ghostAnimations.end();) {
			GhostAnimation* temp = *i;
			if (temp->queueForDeletion) {
				i = ghostAnimations.erase(i);
				delete temp;
			}
			else i++;
		}

		//



		/// PICKUP  ///

		for (auto _pickup : pickups) {
			_pickup->update(window);
			if (isColliding(player.position, 24, _pickup)) {
				_pickup->queueForDeletion = true;
				score += scorePerPickUp;
				shieldScore -= scorePerPickUp;

				scorePopUp* popUp = new scorePopUp;
				popUp->init(defaultFont, _pickup->position);
				scorePopUps.push_back(popUp);
			}
		}

		for (auto i = pickups.begin(); i != pickups.end();) {
			Pickup* temp = *i;
			if (temp->queueForDeletion) {
				i = pickups.erase(i);
				delete temp;
			}
			else i++;
		}

		//



		/// ENEMIES ///

		if (enemySpawnClock.getElapsedTime().asSeconds() >= waitTimeBeforeGameStart) {

			//printf("Enemy Spawn Cooldown: %f \n", enemySpawnCD);

			int enemyType = randIntRange(0, 2);
			Enemy* enemy = new Enemy;
			Pickup* pickup = new Pickup;

			switch (enemyType)
			{
			case 0: //small
				enemy->init(enemyTexture, enemyTexture1, randFloatRange(enemySmallestSpeed, enemyBiggestSpeed));
				enemies.push_back(enemy);
				break;
			case 1: //big
				enemy->init(enemyBigTexture, enemyBigTexture1, randFloatRange(enemyBigSmallestSpeed, enemyBigBiggestSpeed));
				enemies.push_back(enemy);
				break;
			case 2:
				pickup->init(pickupTexture, pickupSpeed);
				pickups.push_back(pickup);
			}
			enemySpawnClock.restart();
			if (enemySpawnCD > enemyCDCap) {
				enemySpawnCD -= enemyCDSpeed;
				waitTimeBeforeGameStart = enemySpawnCD;
			}
		}

		for (auto _enemy : enemies) {
			_enemy->update(window);

			if (_isColliding(player.position, 32, _enemy)) {
				if (player.hasShield == true) {
					player.invincible = true;
					player.hasShield = false;
					player.updateSprite(player1hit, 255);
					currentPlayerTexture = player1hit;
					invincibilityTime.restart();
				}
				else if (player.invincible == false) isGameOver = true;
			}
		}
		

		for (auto i = enemies.begin(); i != enemies.end();) {
			Enemy* temp = *i;
			if (temp->queueForDeletion) {
				i = enemies.erase(i);
				delete temp;
			}
			else i++;
		}
		
		//



		/// PLAYER ///

		player.update(window);
		player.updateMovement(Event::KeyPressed);
		player.keepInBounds();

		//



		/// SHIELD ///

		if (player.hasShield == true) {
			player.updateSprite(playerTexture, 255);
			currentPlayerTexture = playerTexture;
		}
		

		if (invincibilityTime.getElapsedTime().asSeconds() >= invincibilityDuration && player.invincible == true) {
			player.invincible = false;
			player.hasShield = false;
			invincibilityTime.restart();
		}


		if (shieldScore <= 0)
		{
			shieldScore = shieldPrice + shieldScore;
			player.hasShield = true;
		}



		/// UI ///


		/// POP UPS ///

		for (auto popup : scorePopUps) {
			popup->update(window);
		}

		for (auto i = scorePopUps.begin(); i != scorePopUps.end();) {
			scorePopUp* temp = *i;
			if (temp->queueForDeletion) {
				i = scorePopUps.erase(i);
				delete temp;
			}
			else i++;
		}

		//TIMER
		globalClockLabel.setOrigin(globalClockLabel.getLocalBounds().width / 2, 0);
		globalClockLabel.setPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 96);

		window.draw(globalClockLabel);


		//SCORE
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(6) << std::to_string(score);
		scoreLabel.setString("SCORE: " + ss.str());
		window.draw(scoreUiSprite);
		window.draw(scoreLabel);

		if (scoreClock.getElapsedTime().asSeconds() >= 1.f) {
			score += scorePerSecond;
			shieldScore -= scorePerSecond;
			scoreClock.restart();

			globalClockLabel.setString(std::to_string((int)globalClock.getElapsedTime().asSeconds() + 1)); // <-- timer incrementing

		}



		/// GAME OVER ///
		
		while (isGameOver) {
			while (window.pollEvent(event))
			{
				if (event.type == Event::Closed)
				{
					window.close();
					return EXIT_SUCCESS;
				}
			}
			window.clear(Color(105, 89, 88, 255));

			restartButton.update(window);
			scoreLabel.setCharacterSize(64);
			scoreLabel.setOutlineThickness(4);
			scoreLabel.setOrigin(scoreLabel.getLocalBounds().width / 2, scoreLabel.getLocalBounds().height / 2);
			scoreLabel.setPosition(SCREEN_WIDTH / 2, endscreen.getPosition().y + 300);

			//printf("%s \n", playButton.isPressed(window) ? "true" : "false");

			if (restartButton.isPressed(window)) {
				//window.close();
				//main();
				goto gameStart;
			}
			window.draw(endscreen);
			window.draw(scoreLabel);
			window.display();
		}




		window.display();
	}
}
