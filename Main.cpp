

#include <iostream>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <set>


enum class GAMETAG {PLAYER, BULLET, ENEMY};
enum class COLLISION_LAYER {PLAYER, ENEMY};
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

sf::RenderWindow* window;
const sf::Color regionNormalColor = sf::Color(219, 235, 52);
const sf::Color regionActiveColor = sf::Color(235, 155, 52);


static struct Keybindings
{
    const static sf::Keyboard::Key STRAFE = sf::Keyboard::Key::LShift;
    const static sf::Keyboard::Key FIRE = sf::Keyboard::Key::C;
};

class GameObject
{
    public:
        
        GAMETAG getTag() { return this->tag; }
        virtual std::string debugInfo() = 0; // Force override

        sf::Vector2f getPosition()
        {
            return position;
        }
        void setPosition(sf::Vector2f pos)
        {
            position = pos;
            // Update partition
        }
        // Adds vector to position. (You need to multiply by deltatime yourself if you want framerate independence)
        void translate(sf::Vector2f vector)
        {
            setPosition(getPosition() + vector);
        }
        virtual int Update(float dt)
        {
            return 0;
        }
        //virtual void Draw();
        virtual std::vector<sf::Vector2f> GetBounds()
        {
            return std::vector<sf::Vector2f>({ getPosition() });
        }
    protected:
        GAMETAG tag;
    private:
        sf::Vector2f position;
};

struct GridCell
{
    sf::Vector2f topLeft;
    sf::Vector2f bottomRight;
    std::vector<GameObject*> contents;
    bool isActive = false;
    //bool isActive = false;
    sf::Vector2f getSize() { return sf::Vector2f({ bottomRight.x - topLeft.x, bottomRight.y - topLeft.y }); }
    bool contains(float x, float y) {
        return (x >= topLeft.x && x < bottomRight.x &&
            y >= topLeft.y && y < bottomRight.y);
    }

    bool containsObject(GameObject* g)
    {
        std::vector<sf::Vector2f> bounds = g->GetBounds();
        for (sf::Vector2f v : bounds)
        {
            if (contains(v.x, v.y)) return true;
        }
        return false;
    }
}
;
class Grid
{
public:
    std::vector<GridCell> _grid;
    int _COLS;
    int _ROWS;
    int MAPWIDTH;
    int MAPHEIGHT;

    // Returns a reference (so mutable) to the Gridcell at x,y
    GridCell& get(int x, int y)
    {
        // Optional bounds check (debug mode)
        if (x < 0 || x >= _COLS || y < 0 || y >= _ROWS)
            throw std::out_of_range("Grid::get coordinates out of range");

        return _grid[y * _COLS + x];
    }
    GridCell& getByIndex(int index)
    {
        return _grid[index];
    }

    void printDebug()
    {
        std::cout << "[GRID]: " << _ROWS << "x" << _COLS << "\n";
        for (int i = 0; i < _grid.size(); i++)
        {
            sf::Vector2i coords = index2Coords(i);
            std::cout << "CELL " << coords.y << ", " << coords.x << ": " << _grid[i].isActive;
            for (GameObject* g : _grid[i].contents)
            {
               
                std::cout << g->debugInfo() << ", ";
            }
            std::cout << "\n";
        }
    }

    void Generate(float mapWidth, float mapHeight, int cols, int rows)
    {
        _COLS = cols;
        _ROWS = rows;
        MAPWIDTH = mapWidth;
        MAPHEIGHT = mapHeight;

        _grid = std::vector<GridCell>(_ROWS * _COLS, GridCell());
        for (int i = 0; i < _ROWS; i++)
        {
            for (int j = 0; j < _COLS; j++)
            {
                int index = coord2Index(j, i);
                _grid[index].topLeft = sf::Vector2f(j * (MAPWIDTH / _COLS), i * (MAPHEIGHT / _ROWS));
                _grid[index].bottomRight = sf::Vector2f( (j+1) * (MAPWIDTH / _COLS), (i+1) * (MAPHEIGHT / _ROWS));
            }
        }
    }

    // Gets the (x,y) coords associated with an index
    sf::Vector2i index2Coords(int index)
    {
        int _y = index / _COLS;
        int _x = index % _COLS;
        return sf::Vector2i(_x, _y);
    }

    // Gets the index associated with grid[y,x]
    int coord2Index(int x, int y)
    {
        return  y * _COLS + x;
    }

    int _getPos(float _pos, int _mapsize, int _cols)
    {
        int sizeX = _mapsize / _cols;
        int x = 0;
        float quotient = _pos / (float)sizeX;
        if (quotient == 0) x = 0;
        // If "whole", go to "previous partition) unless zero. Otherwise, round down
        // Check if whole
        else if (quotient == std::floor(quotient)) x = (int)(std::floor(quotient) - 1);
        else x = int(std::floor(quotient));
        return x;
    }


    // Returns index of gridcell associated with this world positon.
    int Position2CellIndex(sf::Vector2f pos)
    {
        int x = _getPos(pos.x, MAPWIDTH, _COLS);
        int y = _getPos(pos.y, MAPHEIGHT, _ROWS);
        
        return coord2Index(x, y);

    }
    // Places the provided gameobject in the appropriate partition(s) based on its position. 
    // Also returns the indices of all partitions it is in
    std::set<int> PlaceInPartitions(GameObject* g)
    {

        std::set<int> result;
        std::vector<sf::Vector2f> bounds = g->GetBounds();
        for (sf::Vector2f b : bounds)
        {
            int ind = Position2CellIndex(b);
            // Only add if target is valid position within game world
            if (ind < _grid.size())
            {
                _grid[ind].contents.push_back(g);
                if (g->getTag() == GAMETAG::PLAYER) _grid[ind].isActive = true;
                result.insert(ind);
            }
           
        }
        return result;
    }
        
    // Removes gameobject from every partition provided by a list of indices.
    void RemoveFromPartitions(GameObject* g, std::set<int> index_list)
    {
        for (int i : index_list)
        {
            if (g->getTag() == GAMETAG::PLAYER) _grid[i].isActive = false;
            auto& contents = _grid[i].contents;  // get vector of objects in this cell
            // erase-remove idiom to remove the pointer
            //contents.clear();
            contents.erase(std::remove(contents.begin(), contents.end(), g), contents.end());
            //contents.clear();
        }
    }

    void RenderGrid()
    {
        for (int i = 0; i < _ROWS; i++)
        {
            for (int j = 0; j < _COLS; j++)
            {
                GridCell cell = get(j, i);
                sf::RectangleShape r = sf::RectangleShape(sf::Vector2f({ cell.getSize() }));
                r.setPosition(cell.topLeft);
                
                if (cell.isActive) r.setFillColor(regionActiveColor);
                else r.setFillColor(regionNormalColor);
                r.setOutlineColor(sf::Color::Black);
                r.setOutlineThickness(1.0f);
                window->draw(r);
            }
        }
    }
};

class GridGameObject : public GameObject
{
    public:
        Grid* grid;
        std::set<int> gridPartitions = std::set<int>({}); // Stores which current partitions this is in

        virtual void Init(Grid* g)
        {
            grid = g;
        }

        void setPosition(sf::Vector2f pos)
        {
            GameObject::setPosition(pos);

            // Check if still in partitions
            /*for (int i : gridPartitions)
            {
                //if (!grid.getByIndex(i).containsObject(this)
            }*/
            // Repartition this
            if (gridPartitions.size() != 0) grid->RemoveFromPartitions(this, gridPartitions);
            gridPartitions = grid->PlaceInPartitions(this);
            
        }
};

class Bullet : public GridGameObject
{

public:
    //static int count;
    int id;
    sf::Vector2f v;
    GameObject* owner;
    std::vector<COLLISION_LAYER> canDamage;

    // Settings

    // COLLISION LAYER is which layer can be hurt by this type of bullet
    Bullet(sf::Vector2f velocity, sf::Vector2f pos, int count, Grid* g, std::vector<COLLISION_LAYER> _canDamage, GameObject* _owner = nullptr)
    {
        this->canDamage = _canDamage;
        this->owner = _owner;
        this->tag = GAMETAG::BULLET;
        this->grid = g;
        id = count;
        setPosition(pos);
        v = velocity;
        count += 1;
        std::cout << "Bullet[" << id << "]" << " with velocity " << v.x << "," << v.y << "\n";
        
    }

    std::string debugInfo() override
    {
        return "Bullet";
    }

    int Update(float dt)
    {
        //this->position.x += spd;
        // 
        setPosition(getPosition() + v * dt); //cant use this version because it will try to move it to a partition off the screen

        sf::Vector2f position = getPosition();
        if (position.y < 0 || position.x < 0 || position.x > SCREEN_WIDTH || position.y > SCREEN_HEIGHT)
        {
            // Remove from partitions
            //grid->RemoveFromPartitions(this, this->gridPartitions);
            return -1; // Signal to BulletManager to delete this
        }
        //std::cout << "Update bullet position " << position.x << "," << position.y << "\n";
        return 0;
    }


    void Draw()
    {
        sf::RectangleShape r = sf::RectangleShape(sf::Vector2f({ 5,5 }));
        r.setFillColor(sf::Color::Magenta);
        //r.setOrigin(sf::Vector2f({ 2.5f, 2.5f }));
        r.setPosition(getPosition());
        window->draw(r);
    }
};

// One instance of this in game
class BulletManager
{
    private: 
        std::vector<Bullet*> playerBullets;
        std::vector<Bullet*> enemyBullets;

    public:
        Grid* grid;


        void createBullet(int bulletType, sf::Vector2f pos, sf::Vector2f dir, GameObject* owner)
        {
            Bullet* b;
            if (bulletType == 0) b = new Bullet(dir * 600.0f, pos, getBulletCount(0), grid, std::vector<COLLISION_LAYER>{COLLISION_LAYER::ENEMY}, owner);
            else if (bulletType == 1) b = new Bullet(dir * 800.0f , pos, getBulletCount(1), grid, std::vector<COLLISION_LAYER>{COLLISION_LAYER::PLAYER}, owner);
            else throw std::invalid_argument("Invalid bullet type");

            addBullet(bulletType, b);
        }

        void DrawBullets()
        {
            for (auto& b : playerBullets)
            {
                b->Draw();
            }

            for (auto& b : enemyBullets)
            {
                b->Draw();
            }
        }

        // 0 = player, 1 = enemy
        int getBulletCount(int type)
        {
            if (type == 0) return playerBullets.size();
            else if (type == 1) return enemyBullets.size();
            else return -1;
        }
        // collectionId: 0=player, 1= enemy
        int addBullet(int collectionId, Bullet* b)
        {
            if (collectionId == 0) playerBullets.push_back(b);
            else if (collectionId == 1) enemyBullets.push_back(b);
            else return -1;
            return 0;
        }

        void UpdateBullets(std::vector<Bullet*>& collection, float dt)
        {
            for (auto it = collection.begin(); it != collection.end(); /* no increment here */) {
                int result = (*it)->Update(dt); // Update now returns int
                if (result == -1) { // Delete bullets that return -1 (delete flag)
                    (*it)->grid->RemoveFromPartitions((*it), (*it)->gridPartitions);

                    it = collection.erase(it); // erase returns the next valid iterator
                }
                else {
                    ++it; // only increment if not erased
                }
            }
        }

        int Update(float dt)
        {
            UpdateBullets(playerBullets, dt);
            UpdateBullets(enemyBullets, dt);
            return 0;
        }

        void Init(Grid* g)
        {
            this->grid = g;
            playerBullets.clear();
            //std::vector<Bullet*> enemyBullets;
        }
};

class Player : public GridGameObject
{
private:
    sf::Vector2f lastDir = sf::Vector2f(0, -1);

public:
    float r_Size = 48; // pixels
    sf::Angle gunRot;
    const float movSpd = 300.0f;
    const float strafeSpd = 300.0f;
    BulletManager* bulletManager;
    //std::vector<Bullet*> bullets;

    std::string debugInfo() override
    {
        return "Player";
    }

    void Init(Grid* g, BulletManager* b)
    {
        this->bulletManager = b;
        this->tag = GAMETAG::PLAYER;
        GridGameObject::Init(g);
        
        setPosition(sf::Vector2f({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }));
        gunRot = sf::degrees(360);
        std::cout << "Bulletmanager is null? " << (bulletManager == nullptr) << "\n";
    }

    int Update(float dt) override
    {
        float inputX = 0;
        float inputY = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            inputX += 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            inputX -= 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            inputY += 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            inputY -= 1;
        }

        // Normalize it
        sf::Vector2f input = sf::Vector2f(inputX, inputY);
        if (input.length() > 0)
        {
            input = input.normalized();
            float _spd = 0;

            // Not Strafing
            if (sf::Keyboard::isKeyPressed(Keybindings::STRAFE)) _spd = strafeSpd;
            else { _spd = movSpd, lastDir = input; }
            //std::cout << "Input: " << input.x << ", " << input.y << "\n";
            setPosition(getPosition() + (input * (_spd * dt)));
        }

        // Run update on any bullets
        
        
        return 0;
    }
    
    void OnFireButtonPress()
    {
        // spawn bullet
        //Bullet* b = new Bullet(lastDir, , bulletManager->getBulletCount(0), grid, std::vector<COLLISION_LAYER>{COLLISION_LAYER::ENEMY}, this);
        bulletManager->createBullet(0, getPosition(), lastDir, this);
        //bulletManager->addBullet(0, b); 
    }

    /*
    void DrawBullets()
    {
        for (auto& b : bullets)
        {
            b->Draw();
        }
    }*/

    void Draw()
    {
        // Draw player
        sf::RectangleShape r = sf::RectangleShape(sf::Vector2f({ r_Size, r_Size }));
        r.setFillColor(sf::Color::Red);
        //r.setOutlineThickness(2.0f);
        r.setOrigin(sf::Vector2f({ r_Size / 2, r_Size / 2 }));
        r.setPosition(getPosition());
        window->draw(r);

        // Draw gun
        sf::RectangleShape gun = sf::RectangleShape(sf::Vector2f({ 10, 48 }));
        gun.setFillColor(sf::Color::Black);
        gun.setOrigin(sf::Vector2f({ 5,0 }));
        gun.setPosition(getPosition());
        gun.setRotation(sf::radians(-3.14159265f / 2 + std::atan2(lastDir.y, lastDir.x)));
        window->draw(gun);



    }
};
class Enemy : public GridGameObject
{
public:
    float r_Size = 64;
    GAMETAG tag = GAMETAG::ENEMY;
    float y_center;
    Player* player;
    BulletManager* bulletManager;


    float y;
    float _timer;
    float fireTimer; // counter


    // Settings
    float amplitude = 200;
    float timeScale = 1;
    float fireWaitTime = 0.1f; // Seconds
    float distanceToFire = 64;


    void Init(Grid* g, sf::Vector2f startPos, Player* _player, BulletManager* b)
    {
        bulletManager = b;
        player = _player;
        GridGameObject::Init(g);
        setPosition(startPos);
        y_center = getPosition().y;
        y = y_center;
        _timer = 0;
        fireTimer = fireWaitTime; // Allowed to fire immediately
    }

    std::string debugInfo() { return "Enemy"; }


    void Draw()
    {
        sf::RectangleShape r = sf::RectangleShape(sf::Vector2f({ r_Size, r_Size }));
        r.setFillColor(sf::Color::Yellow);
        //r.setOutlineThickness(2.0f);
        r.setOrigin(sf::Vector2f({ r_Size / 2, r_Size / 2 }));
        r.setPosition(getPosition());
        window->draw(r);

    }

    int Update(float dt) override
    {
        //std::cout << "In enemy update\n";
        y = y_center + amplitude*std::sin(_timer*timeScale);
        this->setPosition(sf::Vector2f{ getPosition().x, y });
        
        if (std::abs(y - player->getPosition().y) <= distanceToFire && fireTimer >= fireWaitTime)
        {
            Fire();
        }
        else fireTimer += dt;
        

        _timer += dt;
        return 0;

    }

    void Fire()
    {
        fireTimer = 0;
        std::cout << "Enemy fire bullet!";
        sf::Vector2f _dir{ 0,0 };
        if (player->getPosition().x <= getPosition().x) _dir.x = -1;
        else _dir.x = 1;

        
        bulletManager->createBullet(1, getPosition(), _dir, this);
        //EnemyBulletManager.SpawnBullet
    }

    bool collidesWithPt(sf::Vector2f pt)
    {
        sf::RectangleShape r = sf::RectangleShape(sf::Vector2f({ r_Size, r_Size }));
        //r.setFillColor(sf::Color::Yellow);
        //r.setOutlineThickness(2.0f);
        r.setOrigin(sf::Vector2f({ r_Size / 2, r_Size / 2 }));
        r.setPosition(getPosition());
        return r.getGlobalBounds().contains(pt);
    }
};




// References
sf::Clock game_clock;
Player player;
Enemy enemy;
Grid grid;
BulletManager* bulletManager;
//Enemy enem1;
//Enemy enem2;

//int Bullet::count = 0;



void Init()
{

    window = new sf::RenderWindow(sf::VideoMode({ SCREEN_WIDTH, SCREEN_HEIGHT }), "TOP DOWN SHOOTER");
    grid.Generate(SCREEN_WIDTH, SCREEN_HEIGHT, 2, 1);

    bulletManager = new BulletManager();
    bulletManager->Init(&grid);
    player.Init(&grid, bulletManager);
    enemy.Init(&grid, player.getPosition() + sf::Vector2f{ 200, 0 }, &player, bulletManager);
    
}

void Update(float dt)
{
    player.Update(dt);
    /*
    std::cout << "Player grid partitions ";
    for (int i : player.gridPartitions)
    {
        std::cout << i << ",";
    }
    std::cout << "\n";*/
    int _ind = *player.gridPartitions.begin();
    //std::cout << "Isactive: " << player.grid->getByIndex(_ind).isActive << "\n";
    //grid.printDebug();
    //GameObject* p = &player;
    //std::cout << p->debugInfo() << "\n";
    enemy.Update(dt);
    bulletManager->Update(dt);
}

void RenderGrid()
{
    // Top left
    sf::RectangleShape r1(sf::Vector2f({ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 }));
    r1.setFillColor(regionActiveColor);
    r1.setPosition(sf::Vector2f(0,0));
    r1.setOutlineThickness(1.0f); 
    r1.setOutlineColor(sf::Color::Black);
    window->draw(r1);
    
    // Top right
    r1.setFillColor(regionNormalColor);
    r1.setPosition(sf::Vector2f(SCREEN_WIDTH / 2, 0));
    window->draw(r1);

    // bottom right
    r1.setFillColor(regionNormalColor);
    r1.setPosition(sf::Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT/2));
    window->draw(r1);

    // bottom LEFT
    r1.setFillColor(regionNormalColor);
    r1.setPosition(sf::Vector2f(0, SCREEN_HEIGHT / 2));
    window->draw(r1);
}

void Draw()
{
    grid.RenderGrid();
    //RenderGrid();
    player.Draw();
    enemy.Draw();
    bulletManager->DrawBullets();
    //player.DrawBullets();

}


int main()
{
    
    Init();

    while (window->isOpen())
    {
        float dt = game_clock.restart().asSeconds();
        while (const std::optional event = window->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window->close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window->close();
                if (keyPressed->code == Keybindings::FIRE)
                {
                    //std::cout << "spacey\n";
                    player.OnFireButtonPress();
                }
            }
        }
        Update(dt);
        window->clear(sf::Color::Green);
        Draw();
        window->display();
    }

    return 0;
}
