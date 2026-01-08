#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath> 
#include <cstdlib> // Pentru rand()
#include <ctime>   // Pentru time()
#include <string>
#include <fstream>
using namespace std;

std::vector<sf::Color> culoriDisponibile = {
    sf::Color::White, sf::Color::Black, sf::Color(128, 0, 32), sf::Color::Blue,
    sf::Color::Green, sf::Color::Yellow, sf::Color::Cyan, sf::Color::Magenta
};

int idxP1 = 0; // Default P1 = Alb
int idxP2 = 1; // Default P2 = Negru

// =========================================================
// 1. VARIABILELE SI FUNCTIILE JOCULUI (LOGICA)
// =========================================================

int matrice[8][8];
int randulJucatorului = 1; // 1 = Alb (Om), 2 = Negru (Om sau PC)
int adversar = 2;
int sursaRand = -1;
int sursaCol = -1;
int timpAcumulat;

// Variabile pentru Undo  (Coordonate)
int undo_rVechi = -1, undo_cVechi = -1;
int undo_rNou = -1, undo_cNou = -1;
int undo_valoarePiesa = 0;
bool amMutareDeAnulat = false;

// TIPUL DE JOC: 0 = PvP, 1 = PC Easy, 2 = PC Hard
int tipJoc = 0;

unsigned int mutariMinime = 0;
unsigned int contorMutari = 0;
unsigned int contorpieseAlbe = 0;
unsigned int contorpieseNegre = 0;

float offsetX = (1920.f - 800.f) / 2.f;
float offsetY = (1080.f - 800.f) / 2.f;
float cellSize = 100.f;

char bufferTimp[] = "Timp: 00:00";

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
char* obtineTimpFormatat(int secundeTotale) {
    int minute = secundeTotale / 60;
    int secunde = secundeTotale % 60;

    bufferTimp[6] = (minute / 10) + '0';
    bufferTimp[7] = (minute % 10) + '0';
    bufferTimp[9] = (secunde / 10) + '0';
    bufferTimp[10] = (secunde % 10) + '0';

    return bufferTimp;
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

// 2. Functia de eliminare
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

struct Button {
    sf::RectangleShape shape;
    sf::Text text;
    float width, height;

    Button(float x, float y, float w, float h, std::string label, const sf::Font& font)
        : text(font, label, 30), width(w), height(h)
    {
        shape.setPosition({ x, y });
        shape.setSize({ width, height });
        shape.setFillColor(sf::Color(50, 50, 50));
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::White);
        text.setFillColor(sf::Color::White);
        centerText(x, y);
    }

    void centerText(float x, float y) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({ bounds.position.x + bounds.size.x / 2.0f,
                         bounds.position.y + bounds.size.y / 2.0f });
        text.setPosition({ x + width / 2.0f, y + height / 2.0f });
    }

    void setPosition(float x, float y) {
        shape.setPosition({ x, y });
        centerText(x, y);
    }

    bool isHovered(sf::Vector2f mousePos) {
        return shape.getGlobalBounds().contains(mousePos);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};

struct ColorButton {
    sf::CircleShape shape;
    int colorIndex;

    ColorButton(float x, float y, float radius, int index, sf::Color color) {
        shape.setRadius(radius);
        shape.setPosition({ x, y });
        shape.setFillColor(color);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color(80, 80, 80));
        colorIndex = index;
    }

    void setPosition(float x, float y) {
        shape.setPosition({ x, y });
    }

    bool isHovered(sf::Vector2f mousePos) {
        return shape.getGlobalBounds().contains(mousePos);
    }

    // Evidentierea cercului selectat
    void setSelected(bool selected) {
        if (selected) {
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(4);
        }
        else {
            shape.setOutlineColor(sf::Color(80, 80, 80));
            shape.setOutlineThickness(2);
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
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

    void setPosition(float x, float y) {
        bar.setPosition({ x, y });
        fill.setPosition({ x, y });
        fill.setSize({ bar.getSize().x * (val / 100.f), 10.f });
        cerc.setPosition({ x + bar.getSize().x * (val / 100.f), y + 5.f });
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
    sf::Clock aiTimer;

    GameState currentState = GameState::MENU;

    // --- BUTOANE MENU PRINCIPAL ---
    float btnW = 400.f; float btnH = 80.f;

    // Menu Buttons
    Button btnStart(0, 0, btnW, btnH, "START", font);
    Button btnSettings(0, 0, btnW, btnH, "SETTINGS", font);
    Button btnQuit(0, 0, btnW, btnH, "QUIT", font);

    // Mode Buttons
    Button btnPvP(0, 0, btnW, btnH, "PvP (Local)", font);
    Button btnEasy(0, 0, btnW, btnH, "PvC (Easy)", font);
    Button btnHard(0, 0, btnW, btnH, "PvC (Hard)", font);
    Button btnBack(50, 50, 200, 60, "BACK", font);

    // In-Game UI
    Button btnUndo(50, 150, 200, 60, "UNDO", font);
    sf::Text txtTimp(font, "Timp: 00:00", 35);
    txtTimp.setPosition({ 50.f, 300.f });
    sf::Text txtEliminare(font, "", 30);
    txtEliminare.setPosition({ 50.f, 400.f });
    txtEliminare.setFillColor(sf::Color::Yellow);

    // Settings UI
    sf::Text titluSettings = creareTitlu(0, 0, "SETTINGS", font);
    titluSettings.setCharacterSize(70);
    titluSettings.setFillColor(sf::Color::White);
    sf::FloatRect boundsSet = titluSettings.getLocalBounds();
    titluSettings.setOrigin({ boundsSet.size.x / 2.f, boundsSet.size.y / 2.f });

    sf::Text titluRez = creareTitlu(0, 0, "RESOLUTION:", font);
    Button btnRez1(0, 0, 200, 50, "1920x1080", font);
    Button btnRez2(0, 0, 200, 50, "1280x720", font);

    sf::Text titluP1 = creareTitlu(0, 0, "PLAYER 1:", font);
    std::vector<ColorButton> paletaP1;

    sf::Text titluP2 = creareTitlu(0, 0, "PLAYER 2:", font);
    std::vector<ColorButton> paletaP2;

    float cercRadius = 20.f;
    for (int i = 0; i < culoriDisponibile.size(); i++) {
        paletaP1.push_back(ColorButton(0, 0, cercRadius, i, culoriDisponibile[i]));
        paletaP2.push_back(ColorButton(0, 0, cercRadius, i, culoriDisponibile[i]));
    }
    sf::Text titluMusic = creareTitlu(0, 0, "MUSIC:", font);
    Slider sldMusic(0, 0, 400.f, 50.f);

    sf::Clock ceasDelta;      // Masoara timpul dintre cadre
    float timpAcumulat = 0;   // Retine timpul total de joc

    auto updateLayout = [&](float w, float h) {
        float cx = w / 2.f; float cy = h / 2.f;

        btnStart.setPosition(cx - btnW / 2, cy - 150);
        btnSettings.setPosition(cx - btnW / 2, cy);
        btnQuit.setPosition(cx - btnW / 2, cy + 150);

        btnPvP.setPosition(cx - btnW / 2, cy - 150);
        btnEasy.setPosition(cx - btnW / 2, cy);
        btnHard.setPosition(cx - btnW / 2, cy + 150);

        float colL = w * 0.25f;
        float colR = w * 0.60f;

        titluSettings.setPosition({ cx, h * 0.15f });

        float yRez = cy - 200;
        titluRez.setPosition({ colL, yRez });
        btnRez1.setPosition(colR - 210.f, yRez);
        btnRez2.setPosition(colR + 10.f, yRez);

        float startX_Color = colR - 195.f;

        float yP1 = cy - 100;
        titluP1.setPosition({ colL, yP1 });
        for (int i = 0; i < paletaP1.size(); i++) {
            paletaP1[i].setPosition(startX_Color + (i * 50.f), yP1);
        }

        float yP2 = cy;
        titluP2.setPosition({ colL, yP2 });
        for (int i = 0; i < paletaP2.size(); i++) {
            paletaP2[i].setPosition(startX_Color + (i * 50.f), yP2);
        }

        float yMus = cy + 100;
        titluMusic.setPosition({ colL, yMus });
        sldMusic.setPosition(colR - 200.f, yMus + 10.f);

        if (h < 900.f) cellSize = 80.f;
        else cellSize = 100.f;
        float boardSize = 8 * cellSize;
        offsetX = (w - boardSize) / 2.f;
        offsetY = (h - boardSize) / 2.f;

        btnUndo.setPosition(50.f, 150.f);

        btnBack.setPosition(50, 50);
        };

    updateLayout((float)window.getSize().x, (float)window.getSize().y);

    while (window.isOpen())
    {
        sf::Vector2f mousePosFloat = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2i mousePosPixel = sf::Mouse::getPosition(window);
        bool isMouseLeftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        float dt = ceasDelta.restart().asSeconds();
        if (currentState == GameState::GAME) {
            timpAcumulat += dt;
        }

        // Update Muzica
        if (currentState == GameState::OPTIONS) {
            sldMusic.update(mousePosFloat, isMouseLeftDown);
            music.setVolume(sldMusic.val);
        }

        // --- LOGICA AI (Mutare Calculator) ---
        // Doar daca suntem in joc, e randul Negrului, si nu e PvP
        if (currentState == GameState::GAME && randulJucatorului == 2 && tipJoc != 0)
        {
            if (aiTimer.getElapsedTime().asSeconds() < 2.0f)
            {
                // Nu face nimic
            }
            else 
            { 
                if (tipJoc == 1) mutareCalculatorEasy();
                else if (tipJoc == 2) mutareCalculatorHard();

                // Dupa ce a mutat PC-ul:
                contorMutari++;
                randulJucatorului = 1; // Randul Omului
                adversar = 2;
                amMutareDeAnulat = false;
            }
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
                            updateLayout(1920.f, 1080.f);
                        }
                        else if (btnRez2.isHovered(mousePosFloat)) {
                            window.create(sf::VideoMode({ 1280, 720 }), "Razboi in 8", sf::Style::Default, sf::State::Windowed);
                            offsetX = (1280.f - 800.f) / 2.f; offsetY = (720.f - 800.f) / 2.f;
                            updateLayout(1280.f, 720.f);
                        }

                        for (auto& btn : paletaP1) {
                            if (btn.isHovered(mousePosFloat)) {
                                int vechiulIndexP1 = idxP1;
                                idxP1 = btn.colorIndex;
                                if (idxP1 == idxP2) idxP2 = vechiulIndexP1;
                            }
                        }
                        for (auto& btn : paletaP2) {
                            if (btn.isHovered(mousePosFloat)) {
                                int vechiulIndexP2 = idxP2;
                                idxP2 = btn.colorIndex;
                                if (idxP2 == idxP1) idxP1 = vechiulIndexP2;
                            }
                        }

                        if (btnBack.isHovered(mousePosFloat)) currentState = GameState::MENU;
                    }
                    // --- BUTONUL UNDO (NOU) ---
                    if (btnUndo.isHovered(mousePosFloat))
                    {
                        if (amMutareDeAnulat)
                        {
                            // Executăm logica de Undo (bazată pe coordonate)
                            matrice[undo_rVechi][undo_cVechi] = undo_valoarePiesa;
                            matrice[undo_rNou][undo_cNou] = 0;

                            // Schimbăm tura înapoi
                            if (randulJucatorului == 1) randulJucatorului = 2;
                            else randulJucatorului = 1;

                            adversar = (randulJucatorului == 1) ? 2 : 1;
                            contorMutari--;

                            // Resetam selecția și starea de undo
                            sursaRand = -1; sursaCol = -1;
                            amMutareDeAnulat = false;

                            std::cout << "Ai apasat butonul UNDO!" << std::endl;
                        }
                        else {
                            std::cout << "Nu ai nicio mutare de anulat!" << std::endl;
                        }
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
                            int c = (int)((mousePosPixel.x - offsetX) / cellSize);
                            int r = (int)((mousePosPixel.y - offsetY) / cellSize);

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
                                else
                                {
                                    if (r == sursaRand && c == sursaCol)
                                    {
                                        sursaRand = -1;
                                        sursaCol = -1;
                                    }
                                    else if (val == randulJucatorului)
                                    {
                                        sursaRand = r; sursaCol = c;
                                    }
                                    else if (verificaMutare(sursaRand, sursaCol, r, c))
                                    {
                                        // --- MEMORĂM POZIȚIA ---
                                        undo_rVechi = sursaRand;
                                        undo_cVechi = sursaCol;
                                        undo_rNou = r;
                                        undo_cNou = c;
                                        undo_valoarePiesa = matrice[sursaRand][sursaCol];
                                        amMutareDeAnulat = true;
                                        mutaPiesa(sursaRand, sursaCol, r, c);
                                        // Schimbam tura
                                        if (randulJucatorului == 1) randulJucatorului = 2;
                                        else randulJucatorului = 1;

                                        adversar = randulJucatorului;
                                        contorMutari++;
                                        sursaRand = -1; sursaCol = -1;
                                        aiTimer.restart();
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

        if (currentState == GameState::GAME)
        {
            // Trimitem (int)timpAcumulat catre functie
            txtTimp.setString(obtineTimpFormatat((int)timpAcumulat));
            int ramase = mutariMinime - contorMutari;
            if (ramase > 0) {
                txtEliminare.setString("Eliminare in: " + std::to_string(ramase));
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
        else if (currentState == GameState::OPTIONS) {
            btnBack.shape.setFillColor(btnBack.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));

            btnRez1.shape.setFillColor(btnRez1.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));

            btnRez2.shape.setFillColor(btnRez2.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }
        else if (currentState == GameState::GAME) {
            btnBack.shape.setFillColor(btnBack.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));

            btnUndo.shape.setFillColor(btnUndo.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }


        // --- DESENARE ---
        window.clear(sf::Color(30, 30, 30));

        if (currentState == GameState::MENU)
        {
            sf::Text title(font, "RAZBOI IN 8", 100);
            title.setFillColor(sf::Color::Red);
            sf::FloatRect bounds = title.getLocalBounds();
            title.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            title.setPosition({ (float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 300.f });
            window.draw(title);
            btnStart.draw(window);
            btnSettings.draw(window);
            btnQuit.draw(window);
        }
        else if (currentState == GameState::MODE_SELECT)
        {
            sf::Text title(font, "SELECTEAZA MOD", 80);
            title.setFillColor(sf::Color::White);
            sf::FloatRect bounds = title.getLocalBounds();
            title.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            title.setPosition({ (float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 300.f });
            window.draw(title);

            btnPvP.draw(window);
            btnEasy.draw(window);
            btnHard.draw(window);
            btnBack.draw(window);
        }
        else if (currentState == GameState::GAME)
        {
            // TABLA
            sf::RectangleShape rectangle({ cellSize, cellSize });
            float rectX = offsetX;
            float rectY = offsetY;
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    rectangle.setPosition({ rectX, rectY });
                    if ((i + j) % 2 == 0) rectangle.setFillColor(sf::Color::White);
                    else rectangle.setFillColor(sf::Color(100, 100, 100));
                    window.draw(rectangle);
                    rectX += cellSize;
                }
                rectX = offsetX; rectY += cellSize;
            }
            // PIESE
            piesa.setRadius(cellSize / 2.f - 5.f);
            piesa.setOrigin({ cellSize / 2.f - 5.f, cellSize / 2.f - 5.f });
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int valoare = verificaLoc(i, j);
                    if (valoare != 0) {
                        float posX = offsetX + j * cellSize + cellSize / 2.f;
                        float posY = offsetY + i * cellSize + cellSize / 2.f;
                        piesa.setPosition({ posX, posY });
                        if (valoare == 1) piesa.setFillColor(culoriDisponibile[idxP1]);
                        else piesa.setFillColor(culoriDisponibile[idxP2]);

                        if (i == sursaRand && j == sursaCol)
                        {
                            piesa.setOutlineThickness(-5.f);
                            piesa.setOutlineColor(sf::Color::Red);
                        }
                        else
                            piesa.setOutlineThickness(0.f);
                        window.draw(piesa);
                    }
                }
            }
            window.draw(txtTimp);
            btnBack.draw(window);
            btnUndo.draw(window);

            if (contorMutari < mutariMinime)
            {
                window.draw(txtEliminare);
            }
        }
        else if (currentState == GameState::OPTIONS)
        {
            window.draw(titluSettings);
            window.draw(titluRez); btnRez1.draw(window); btnRez2.draw(window);

            //Culori
            window.draw(titluP1);
            for (auto& btn : paletaP1) {
                btn.setSelected(btn.colorIndex == idxP1);
                // Facem gri daca e ocupat de celalalt
                if (btn.colorIndex == idxP2) btn.shape.setFillColor(sf::Color(60, 60, 60));
                else btn.shape.setFillColor(culoriDisponibile[btn.colorIndex]);
                btn.draw(window);
            }
            window.draw(titluP2);
            for (auto& btn : paletaP2) {
                btn.setSelected(btn.colorIndex == idxP2);
                if (btn.colorIndex == idxP1) btn.shape.setFillColor(sf::Color(60, 60, 60));
                else btn.shape.setFillColor(culoriDisponibile[btn.colorIndex]);
                btn.draw(window);
            }

            window.draw(titluMusic); sldMusic.draw(window);
            btnBack.draw(window);
        }

        window.display();
    }
    return 0;
}