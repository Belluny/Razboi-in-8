#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath> 
#include <cstdlib> // Pentru rand()
#include <ctime>   // Pentru time()

using namespace std;

// =========================================================
// 1. VARIABILELE SI FUNCTIILE JOCULUI (LOGICA)
// =========================================================

int matrice[8][8];
int randulJucatorului = 1; // 1 = Alb (Om), 2 = Negru (Om sau PC)
int adversar = 2;
int sursaRand = -1;
int sursaCol = -1;

// TIPUL DE JOC: 0 = PvP, 1 = PC Easy, 2 = PC Hard
int tipJoc = 0;

unsigned int mutariMinime = 0;
unsigned int contorMutari = 0;
unsigned int contorpieseAlbe = 0;
unsigned int contorpieseNegre = 0;

float offsetX = (1920.f - 800.f) / 2.f;
float offsetY = (1080.f - 800.f) / 2.f;

// --- FUNCTII AJUTATOARE ---

void construireMatrice(int rand, int col)
{
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            matrice[i][j] = 0;

    // Negru (2) Sus
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 != 0) matrice[i][j] = 2;
        }
    }
    // Alb (1) Jos
    for (int i = 6; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 != 0) matrice[i][j] = 1;
        }
    }
}

int verificaLoc(int rand, int col) {
    if (rand < 0 || rand > 7 || col < 0 || col > 7) return -1;
    return matrice[rand][col];
}

bool esteBlocata(int r, int c)
{
    if (r - 1 >= 0 && c - 1 >= 0 && matrice[r - 1][c - 1] == 0) return false;
    if (r - 1 >= 0 && c + 1 <= 7 && matrice[r - 1][c + 1] == 0) return false;
    if (r + 1 <= 7 && c - 1 >= 0 && matrice[r + 1][c - 1] == 0) return false;
    if (r + 1 <= 7 && c + 1 <= 7 && matrice[r + 1][c + 1] == 0) return false;
    return true;
}

int verificaMutare(int rSursa, int cSursa, int rDest, int cDest)
{
    if (rDest < 0 || rDest > 7 || cDest < 0 || cDest > 7) return 0;
    if (matrice[rDest][cDest] != 0) return 0;// trebuie sa fie go
    int difR = abs(rDest - rSursa);
    int difC = abs(cDest - cSursa);
    if (difR == 1 && difC == 1) return 1;
    return 0;
}

void mutaPiesa(int rVechi, int cVechi, int rNou, int cNou)
{
    matrice[rNou][cNou] = matrice[rVechi][cVechi];
    matrice[rVechi][cVechi] = 0;
}

void resetareJoc() {
    construireMatrice(0, 0);
    randulJucatorului = 1; // Albul incepe mereu
    adversar = 2;
    contorMutari = 0;
    contorpieseAlbe = 0;
    contorpieseNegre = 0;
    sursaRand = -1;
    sursaCol = -1;
}

// =========================================================
// 2. AI (INTELIGENTA ARTIFICIALA)
// =========================================================

struct Mutare {
    int rS, cS, rD, cD, scor;
};

// 1. Functia care genereaza mutarile posibile
std::vector<Mutare> genereazaToateMutarile(int jucator)
{
    std::vector<Mutare> mutariPosibile;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (matrice[i][j] == jucator) {
                int di[] = { -1, -1, 1, 1 };
                int dj[] = { -1, 1, -1, 1 };
                for (int k = 0; k < 4; k++) {
                    int ni = i + di[k];
                    int nj = j + dj[k];
                    if (verificaMutare(i, j, ni, nj)) {
                        Mutare m = { i, j, ni, nj, 0 };
                        mutariPosibile.push_back(m);
                    }
                }
            }
        }
    }
    return mutariPosibile;
}

// 2. Functia de eliminare (TREBUIE PUSA AICI, INAINTE DE MUTARE!)
void incearcaEliminareDupaMutare(int r, int c)
{
    if (contorMutari <= mutariMinime) return;

    int di[] = { -1, -1, 1, 1 };
    int dj[] = { -1, 1, -1, 1 };

    for (int k = 0; k < 4; k++) {
        int ni = r + di[k];
        int nj = c + dj[k];

        if (ni >= 0 && ni < 8 && nj >= 0 && nj < 8)
        {
            if (matrice[ni][nj] == 1) // Daca e piesa adversarului (Alb)
            {
                if (esteBlocata(ni, nj))
                {
                    matrice[ni][nj] = 0; // Eliminare
                    contorpieseAlbe++;
                    cout << "PC-ul a eliminat piesa de la " << ni << "," << nj << endl;
                }
            }
        }
    }
}

// 3. Mutare Easy (Acum stie cine e "incearcaEliminare...")
void mutareCalculatorEasy()
{
    std::vector<Mutare> mutari = genereazaToateMutarile(2);
    if (mutari.empty()) return;

    int index = rand() % mutari.size();
    Mutare m = mutari[index];
    mutaPiesa(m.rS, m.cS, m.rD, m.cD);
    cout << "PC (Easy) a mutat." << endl;

    // Apelam functia definita mai sus
    incearcaEliminareDupaMutare(m.rD, m.cD);
}

// 4. Mutare Hard (Acum stie cine e "incearcaEliminare...")
void mutareCalculatorHard()
{
    std::vector<Mutare> mutari = genereazaToateMutarile(2);
    if (mutari.empty()) return;

    for (auto& m : mutari) {
        int piesaDest = matrice[m.rD][m.cD];
        matrice[m.rD][m.cD] = 2;
        matrice[m.rS][m.cS] = 0;

        m.scor = 0;

        // Strategie Agresiva
        int di[] = { -1, -1, 1, 1 };
        int dj[] = { -1, 1, -1, 1 };
        for (int k = 0; k < 4; k++) {
            int ni = m.rD + di[k];
            int nj = m.cD + dj[k];
            if (ni >= 0 && ni < 8 && nj >= 0 && nj < 8 && matrice[ni][nj] == 1) {
                if (esteBlocata(ni, nj)) {
                    if (contorMutari > mutariMinime) m.scor += 1000;
                    else m.scor += 50;
                }
            }
        }

        // Strategie Defensiva
        if (esteBlocata(m.rD, m.cD)) m.scor -= 50;
        if (m.rD > 1 && m.rD < 6 && m.cD > 1 && m.cD < 6) m.scor += 10;
        m.scor += rand() % 5;

        matrice[m.rS][m.cS] = 2;
        matrice[m.rD][m.cD] = piesaDest;
    }

    Mutare best = mutari[0];
    for (auto& m : mutari) {
        if (m.scor > best.scor) {
            best = m;
        }
    }
    mutaPiesa(best.rS, best.cS, best.rD, best.cD);
    cout << "PC (Hard) a mutat strategic." << endl;

    // Apelam functia definita mai sus
    incearcaEliminareDupaMutare(best.rD, best.cD);
}
// Functie noua: PC-ul verifica daca a blocat pe cineva si il elimina


// =========================================================
// 3. UI (BUTOANE & MENU)
// =========================================================

enum class GameState { MENU, MODE_SELECT, GAME, OPTIONS, EXIT };

struct Button
{
    sf::RectangleShape shape;
    sf::Text text;

    Button(float x, float y, float width, float height, std::string label, const sf::Font& font)
        : text(font, label, 40)
    {
        shape.setPosition({ x, y });
        shape.setSize({ width, height });
        shape.setFillColor(sf::Color(50, 50, 50));
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::White);

        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f,
                         bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition({ x + width / 2.0f, y + height / 2.0f });
    }

    bool isHovered(sf::Vector2f mousePos) {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};

struct Slider
{
    sf::RectangleShape bar;
    sf::RectangleShape fill;
    sf::CircleShape cerc;
    float val;

    Slider(float x, float y, float width, float iniVal)
    {
        val = iniVal;
        bar.setPosition({ x, y });
        bar.setSize({ width, 10.f });
        bar.setFillColor(sf::Color(200, 200, 200));

        fill.setPosition({ x, y });
        fill.setSize({ width * (val / 100.f), 10.f });
        fill.setFillColor(sf::Color::White);

        cerc.setRadius(10.f);
        cerc.setFillColor(sf::Color::White);
        cerc.setOrigin({ 10.f, 10.f });
        cerc.setPosition({ x + width * (val / 100.f), y + 5.f });
    }

    void update(sf::Vector2f mousePos, bool isMouseDown)
    {
        sf::FloatRect bounds = bar.getGlobalBounds();
        bounds.size.x += 10; bounds.position.x -= 5;
        bounds.size.y += 20; bounds.position.y -= 10;

        if (isMouseDown && bounds.contains(mousePos))
        {
            float mouseX = std::clamp(mousePos.x, bar.getPosition().x, bar.getPosition().x + bar.getSize().x);
            float barX = bar.getPosition().x;
            float barWidth = bar.getSize().x;
            val = ((mouseX - barX) / barWidth) * 100.f;

            cerc.setPosition({ mouseX, bar.getPosition().y + 5.f });
            fill.setSize({ mouseX - barX, 10.f });
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(bar);
        window.draw(fill);
        window.draw(cerc);
    }
};

sf::Text creareTitlu(float x, float y, std::string str, const sf::Font& font)
{
    sf::Text text(font, str, 40);
    text.setFillColor(sf::Color(200, 200, 200));
    text.setPosition({ x, y });
    return text;
}

// =========================================================
// 4. MAIN
// =========================================================

int main()
{
    srand(time(NULL)); // Initializare Random

    // Configurare Consola
    cout << "Mutarile Minime inainte de eliminare: ";
    if (!(cin >> mutariMinime)) {
        mutariMinime = 0;
        cin.clear();
    }

    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Razboi in 8", sf::Style::Default, sf::State::Fullscreen);
    window.setFramerateLimit(60);

    // Resurse
    sf::Font font;
    if (!font.openFromFile("Wargate-Normal.ttf"))
    {
        if (!font.openFromFile("arial.ttf"))
        {
            std::cerr << "EROARE: Nu am gasit fontul!" << std::endl;
            return -1;
        }
    }

    sf::Music music;
    if (music.openFromFile("muzica.mp3")) {
        music.setLooping(true);
        music.setVolume(50);
        music.play();
    }

    construireMatrice(0, 0);
    sf::CircleShape piesa(50.f);
    piesa.setOrigin({ 0.f, 0.f });

    GameState currentState = GameState::MENU;

    // --- BUTOANE MENU PRINCIPAL ---
    float btnWidth = 400.f;
    float btnHeight = 120.f;
    float centerX = (1920.f / 2.f) - (btnWidth / 2.f);
    float startY = 1080.f / 2.f;

    Button btnStart(centerX, startY - 200.f, btnWidth, btnHeight, "START", font);
    Button btnSettings(centerX, startY, btnWidth, btnHeight, "SETTINGS", font);
    Button btnQuit(centerX, startY + 200.f, btnWidth, btnHeight, "QUIT", font);
    Button btnBack(50.f, 50.f, 200.f, 80.f, "BACK", font);

    // --- BUTOANE SELECTIE MOD (NOU) ---
    Button btnPvP(centerX, startY - 200.f, btnWidth, btnHeight, "PvP (Local)", font);
    Button btnEasy(centerX, startY, btnWidth, btnHeight, "PvC (Easy)", font);
    Button btnHard(centerX, startY + 200.f, btnWidth, btnHeight, "PvC (Hard)", font);

    // --- SETTINGS ---
    float colStanga = 600.f;
    float colDreapta = 1100.f;
    sf::Text titluRez = creareTitlu(colStanga, startY - 200.f, "RESOLUTION:", font);
    Button btnRez1(colDreapta - 125.f, startY - 200.f, 200.f, 50.f, "1920x1080", font);
    Button btnRez2(colDreapta + 125.f, startY - 200.f, 200.f, 50.f, "1280x720", font);
    sf::Text titluMusic = creareTitlu(colStanga, startY - 100.f, "MUSIC:", font);
    Slider sldMusic(colDreapta - 150.f, startY - 75.f, 500.f, 50.f);


    while (window.isOpen())
    {
        sf::Vector2f mousePosFloat = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2i mousePosPixel = sf::Mouse::getPosition(window);
        bool isMouseLeftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        // Update Muzica
        if (currentState == GameState::OPTIONS) {
            sldMusic.update(mousePosFloat, isMouseLeftDown);
            music.setVolume(sldMusic.val);
        }

        // --- LOGICA AI (Mutare Calculator) ---
        // Doar daca suntem in joc, e randul Negrului, si nu e PvP
        if (currentState == GameState::GAME && randulJucatorului == 2 && tipJoc != 0)
        {
            sf::sleep(sf::milliseconds(300)); // Mica pauza "de gandire"

            if (tipJoc == 1) mutareCalculatorEasy();
            else if (tipJoc == 2) mutareCalculatorHard();

            // Dupa ce a mutat PC-ul:
            contorMutari++;
            randulJucatorului = 1; // Randul Omului
            adversar = 2;
        }

        // --- EVENIMENTE ---
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePress->button == sf::Mouse::Button::Left)
                {
                    // 1. MENIU PRINCIPAL
                    if (currentState == GameState::MENU)
                    {
                        if (btnStart.isHovered(mousePosFloat)) currentState = GameState::MODE_SELECT; // Mergem la selectie
                        else if (btnSettings.isHovered(mousePosFloat)) currentState = GameState::OPTIONS;
                        else if (btnQuit.isHovered(mousePosFloat)) window.close();
                    }

                    // 2. SELECTIE MOD (NOU)
                    else if (currentState == GameState::MODE_SELECT)
                    {
                        if (btnPvP.isHovered(mousePosFloat)) {
                            tipJoc = 0; resetareJoc(); currentState = GameState::GAME;
                        }
                        else if (btnEasy.isHovered(mousePosFloat)) {
                            tipJoc = 1; resetareJoc(); currentState = GameState::GAME;
                        }
                        else if (btnHard.isHovered(mousePosFloat)) {
                            tipJoc = 2; resetareJoc(); currentState = GameState::GAME;
                        }
                        else if (btnBack.isHovered(mousePosFloat)) {
                            currentState = GameState::MENU;
                        }
                    }

                    // 3. SETARI
                    else if (currentState == GameState::OPTIONS)
                    {
                        if (btnRez1.isHovered(mousePosFloat)) {
                            window.create(sf::VideoMode({ 1920, 1080 }), "Razboi in 8", sf::Style::Default, sf::State::Fullscreen);
                            offsetX = (1920.f - 800.f) / 2.f; offsetY = (1080.f - 800.f) / 2.f;
                        }
                        else if (btnRez2.isHovered(mousePosFloat)) {
                            window.create(sf::VideoMode({ 1280, 720 }), "Razboi in 8", sf::Style::Default, sf::State::Windowed);
                            offsetX = (1280.f - 800.f) / 2.f; offsetY = (720.f - 800.f) / 2.f;
                        }
                        if (btnBack.isHovered(mousePosFloat)) currentState = GameState::MENU;
                    }

                    // 4. JOC (Input Om)
                    else if (currentState == GameState::GAME)
                    {
                        if (btnBack.isHovered(mousePosFloat)) currentState = GameState::MENU;

                        // Jucatorul poate muta DOAR daca e randul lui (1) SAU daca e PvP
                        // Daca e PvC, jucatorul e mereu 1 (Alb). Daca e PvP, poate fi 1 sau 2.
                        bool eRandulMeu = (tipJoc == 0) || (randulJucatorului == 1);

                        if (eRandulMeu && !btnBack.isHovered(mousePosFloat))
                        {
                            int c = (mousePosPixel.x - (int)offsetX) / 100;
                            int r = (mousePosPixel.y - (int)offsetY) / 100;

                            if (r >= 0 && r < 8 && c >= 0 && c < 8)
                            {
                                // LOGICA DE CLICK (SELECTIE / ELIMINARE / MUTARE)
                                // Copiata din versiunea anterioara
                                int val = verificaLoc(r, c);
                                int advCurent = (randulJucatorului == 1) ? 2 : 1;

                                if (sursaRand == -1) {
                                    if (val == randulJucatorului && !esteBlocata(r, c)) {
                                        sursaRand = r; sursaCol = c;
                                    }
                                    else if (val == advCurent && esteBlocata(r, c)) {
                                        if (contorMutari > mutariMinime) matrice[r][c] = 0; // Eliminare
                                    }
                                }
                                else {
                                    if (r == sursaRand && c == sursaCol) { sursaRand = -1; sursaCol = -1; }
                                    else if (val == randulJucatorului) { sursaRand = r; sursaCol = c; }
                                    else if (verificaMutare(sursaRand, sursaCol, r, c)) {
                                        mutaPiesa(sursaRand, sursaCol, r, c);
                                        // Schimbam tura
                                        if (randulJucatorului == 1) randulJucatorului = 2;
                                        else randulJucatorului = 1;

                                        adversar = randulJucatorului;
                                        contorMutari++;
                                        sursaRand = -1; sursaCol = -1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyEvent->code == sf::Keyboard::Key::Escape && currentState != GameState::MENU) currentState = GameState::MENU;
            }
        }

        // --- HOVER EFFECTS ---
        if (currentState == GameState::MENU) {
            btnStart.shape.setFillColor(btnStart.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnSettings.shape.setFillColor(btnSettings.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnQuit.shape.setFillColor(btnQuit.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }
        else if (currentState == GameState::MODE_SELECT) {
            btnPvP.shape.setFillColor(btnPvP.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnEasy.shape.setFillColor(btnEasy.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnHard.shape.setFillColor(btnHard.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnBack.shape.setFillColor(btnBack.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }
        else if (currentState == GameState::OPTIONS || currentState == GameState::GAME) {
            btnBack.shape.setFillColor(btnBack.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }

        // --- DESENARE ---
        window.clear(sf::Color(30, 30, 30));

        if (currentState == GameState::MENU)
        {
            sf::Text title(font, "RAZBOI IN 8", 100);
            title.setFillColor(sf::Color::Red);
            sf::FloatRect bounds = title.getLocalBounds();
            title.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            title.setPosition({ 960.f, startY - 400.f });
            window.draw(title);
            btnStart.draw(window);
            btnSettings.draw(window);
            btnQuit.draw(window);
        }
        else if (currentState == GameState::MODE_SELECT)
        {
            sf::Text title(font, "SELECT MODE", 80);
            title.setFillColor(sf::Color::White);
            sf::FloatRect bounds = title.getLocalBounds();
            title.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            title.setPosition({ 960.f, startY - 400.f });
            window.draw(title);

            btnPvP.draw(window);
            btnEasy.draw(window);
            btnHard.draw(window);
            btnBack.draw(window);
        }
        else if (currentState == GameState::GAME)
        {
            // TABLA
            sf::RectangleShape rectangle({ 100.f, 100.f });
            float rectX = offsetX;
            float rectY = offsetY;
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    rectangle.setPosition({ rectX, rectY });
                    if ((i + j) % 2 == 0) rectangle.setFillColor(sf::Color::White);
                    else rectangle.setFillColor(sf::Color(100, 100, 100));
                    window.draw(rectangle);
                    rectX += 100.f;
                }
                rectX = offsetX; rectY += 100.f;
            }
            // PIESE
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int valoare = verificaLoc(i, j);
                    if (valoare != 0) {
                        piesa.setPosition({ offsetX + j * 100.f, offsetY + i * 100.f });
                        if (valoare == 1) piesa.setFillColor(sf::Color::White);
                        else piesa.setFillColor(sf::Color::Black);
                        if (i == sursaRand && j == sursaCol) piesa.setFillColor(sf::Color::Green);
                        window.draw(piesa);
                    }
                }
            }
            btnBack.draw(window);
        }
        else if (currentState == GameState::OPTIONS)
        {
            sf::Text txt(font, "SETTINGS", 70);
            sf::FloatRect bounds = txt.getLocalBounds();
            txt.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            txt.setPosition({ 960.f, startY - 400.f });
            window.draw(txt);
            window.draw(titluRez); btnRez1.draw(window); btnRez2.draw(window);
            window.draw(titluMusic); sldMusic.draw(window);
            btnBack.draw(window);
        }

        window.display();
    }
    return 0;
}