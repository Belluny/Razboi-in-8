#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath> // Pentru abs()

using namespace std;

// 1. VARIABILELE SI FUNCTIILE JOCULUI (LOGICA)
int matrice[8][8];
int randulJucatorului = 1; // 1 = Alb, 2 = Negru
int adversar = 2;
int sursaRand = -1;        // Piesa selectata (-1 = nimic)
int sursaCol = -1;

// Variabile specifice 
unsigned int mutariMinime = 0;
unsigned int contorMutari = 0;
unsigned int contorpieseAlbe = 0;
unsigned int contorpieseNegre = 0;

// Offsets pentru centrarea tablei. Initializam pentru 1920x1080
// Tabla are 800x800.
float offsetX = (1920.f - 800.f) / 2.f; // 560.f
float offsetY = (1080.f - 800.f) / 2.f; // 140.f

void construireMatrice(int rand, int col)
{
    // Curatam tabla
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            matrice[i][j] = 0;

    // Negru (2) Sus - Liniile 0 si 1
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            if ((i + j) % 2 != 0) matrice[i][j] = 2;
        }
    }

    // Alb (1) Jos - Liniile 6 si 7
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
    // Verificăm cele 4 colțuri. Dacă un colț e valid si e gol, piesa nu e blocată.
    if (r - 1 >= 0 && c - 1 >= 0 && matrice[r - 1][c - 1] == 0) return false; // Stanga-Sus
    if (r - 1 >= 0 && c + 1 <= 7 && matrice[r - 1][c + 1] == 0) return false; // Dreapta-Sus
    if (r + 1 <= 7 && c - 1 >= 0 && matrice[r + 1][c - 1] == 0) return false; // Stanga-Jos
    if (r + 1 <= 7 && c + 1 <= 7 && matrice[r + 1][c + 1] == 0) return false; // Dreapta-Jos
    return true; // Blocată complet
}

int verificaMutare(int rSursa, int cSursa, int rDest, int cDest)
{
    if (rDest < 0 || rDest > 7 || cDest < 0 || cDest > 7) return 0;
    if (matrice[rDest][cDest] != 0) return 0; // Trebuie sa fie gol

    int difR = abs(rDest - rSursa);
    int difC = abs(cDest - cSursa);

    if (difR == 1 && difC == 1) return 1; // E diagonală validă
    return 0;
}

void mutaPiesa(int rVechi, int cVechi, int rNou, int cNou)
{
    matrice[rNou][cNou] = matrice[rVechi][cVechi];
    matrice[rVechi][cVechi] = 0;
}

void resetareJoc() {
    construireMatrice(0, 0);
    randulJucatorului = 1;
    adversar = 2;
    contorMutari = 0;
    contorpieseAlbe = 0;
    contorpieseNegre = 0;
    sursaRand = -1;
    sursaCol = -1;
}


// 2. STRUCTURILE MENIULUI (UI)


enum class GameState { MENU, GAME, OPTIONS, EXIT };

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
// 3. MAIN (Integrare)
// =========================================================

int main()
{
    // --- INPUT CONSOLA ---
    // Atentie: Asta va bloca deschiderea ferestrei pana scrii in consola!
    cout << "Mutarile Minime inainte de eliminare: ";
    if (!(cin >> mutariMinime)) {
        mutariMinime = 0; // Fallback daca userul nu introduce numar
        cin.clear();
    }

    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Razboi in 8", sf::Style::Default, sf::State::Fullscreen);
    window.setFramerateLimit(60);

    // ---------------- INCARCARE RESURSE ----------------
    sf::Font font;
    // ASIGURA-TE CA AI UN FONT .TTF LANGA EXECUTABIL

    if (!font.openFromFile("Wargate-Normal.ttf")) {
        if (!font.openFromFile("C:\Users\vrabi\Desktop\Razboi in 8\Razboi in 8")) {
            std::cerr << "EROARE: Nu am gasit niciun font (arial.ttf sau Wargate-Normal.ttf)!" << std::endl;
            return -1;
        }
    }


    sf::Music music;
    if (music.openFromFile("muzica.mp3")) {
        music.setLooping(true);
        music.setVolume(50);
        music.play();
    }

    // ---------------- INITIALIZARE JOC ----------------
    construireMatrice(0, 0); // Construim tabla la start
    sf::CircleShape piesa(50.f);
    piesa.setOrigin({ 0.f, 0.f });

    // ---------------- INITIALIZARE MENIU ----------------
    GameState currentState = GameState::MENU;

    float btnWidth = 400.f;
    float btnHeight = 120.f;
    float centerX = (1920.f / 2.f) - (btnWidth / 2.f);
    float startY = 1080.f / 2.f;

    // Butoane Meniu
    Button btnStart(centerX, startY - 200.f, btnWidth, btnHeight, "START", font);
    Button btnSettings(centerX, startY, btnWidth, btnHeight, "SETTINGS", font);
    Button btnQuit(centerX, startY + 200.f, btnWidth, btnHeight, "QUIT", font);
    Button btnBack(50.f, 50.f, 200.f, 80.f, "BACK", font);

    // Elemente Settings
    float colStanga = 600.f;
    float colDreapta = 1100.f;
    sf::Text titluRez = creareTitlu(colStanga, startY - 200.f, "RESOLUTION:", font);
    Button btnRez1(colDreapta - 125.f, startY - 200.f, 200.f, 50.f, "1920x1080", font);
    Button btnRez2(colDreapta + 125.f, startY - 200.f, 200.f, 50.f, "1280x720", font);

    sf::Text titluMusic = creareTitlu(colStanga, startY - 100.f, "MUSIC:", font);
    Slider sldMusic(colDreapta - 150.f, startY - 75.f, 500.f, 50.f);


    // ================= LOOP PRINCIPAL =================
    while (window.isOpen())
    {
        sf::Vector2f mousePosFloat = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2i mousePosPixel = sf::Mouse::getPosition(window);
        bool isMouseLeftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

        // Update Slider Volum
        if (currentState == GameState::OPTIONS) {
            sldMusic.update(mousePosFloat, isMouseLeftDown);
            music.setVolume(sldMusic.val);
        }

        // ---------------- EVENIMENTE ----------------
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) window.close();

            // A. LOGICA PENTRU MENIU
            if (currentState == GameState::MENU)
            {
                if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mousePress->button == sf::Mouse::Button::Left)
                    {
                        if (btnStart.isHovered(mousePosFloat)) {
                            resetareJoc(); // Resetam tabla cand incepem joc nou
                            currentState = GameState::GAME;
                        }
                        else if (btnSettings.isHovered(mousePosFloat)) currentState = GameState::OPTIONS;
                        else if (btnQuit.isHovered(mousePosFloat)) window.close();
                    }
                }
            }

            // B. LOGICA COMUNA (BACK BUTTON + ESCAPE)
            if (currentState == GameState::OPTIONS || currentState == GameState::GAME)
            {
                if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mousePress->button == sf::Mouse::Button::Left)
                    {
                        if (btnBack.isHovered(mousePosFloat)) currentState = GameState::MENU;
                    }
                }
                if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>())
                {
                    if (keyEvent->code == sf::Keyboard::Key::Escape) currentState = GameState::MENU;
                }
            }

            // C. LOGICA SETTINGS
            if (currentState == GameState::OPTIONS)
            {
                if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mousePress->button == sf::Mouse::Button::Left)
                    {
                        if (btnRez1.isHovered(mousePosFloat)) {
                            window.create(sf::VideoMode({ 1920, 1080 }), "Razboi in 8", sf::Style::Default, sf::State::Fullscreen);
                            offsetX = (1920.f - 800.f) / 2.f; // Recalculam offset
                            offsetY = (1080.f - 800.f) / 2.f;
                        }
                        else if (btnRez2.isHovered(mousePosFloat)) {
                            window.create(sf::VideoMode({ 1280, 720 }), "Razboi in 8", sf::Style::Default, sf::State::Windowed);
                            offsetX = (1280.f - 800.f) / 2.f; // Recalculam offset
                            offsetY = (720.f - 800.f) / 2.f;
                        }
                    }
                }
            }

            // D. LOGICA PENTRU JOC (MUTARI PIESE) !!!
            if (currentState == GameState::GAME)
            {
                if (const auto* mousePress = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    if (mousePress->button == sf::Mouse::Button::Left && !btnBack.isHovered(mousePosFloat))
                    {
                        // 1. Convertim Mouse la Coordonate Matrice (Scadem Offset!)
                        int c = (mousePosPixel.x - (int)offsetX) / 100;
                        int r = (mousePosPixel.y - (int)offsetY) / 100;

                        // Verificam daca click-ul a fost pe tabla
                        if (r >= 0 && r < 8 && c >= 0 && c < 8)
                        {
                            // CAZUL 1: Selectare Piesa
                            if (sursaRand == -1)
                            {
                                // Click pe piesa mea
                                if (verificaLoc(r, c) == randulJucatorului) {
                                    if (!esteBlocata(r, c)) {
                                        sursaRand = r;
                                        sursaCol = c;
                                        cout << "Selectat: " << r << " " << c << endl;
                                    }
                                }
                                // Click pe piesa adversar (Eliminare)
                                else if (verificaLoc(r, c) == adversar)
                                {
                                    if (esteBlocata(r, c))
                                    {
                                        if (contorMutari > mutariMinime)
                                        {
                                            if (matrice[r][c] == 1) contorpieseAlbe++;
                                            else if (matrice[r][c] == 2) contorpieseNegre++;

                                            matrice[r][c] = 0; // Eliminare
                                            cout << "Piesa eliminata!" << endl;
                                        }
                                    }
                                    else {
                                        cout << "Nu poti elimina piesa, are mutari valide!" << endl;
                                    }
                                }
                            }
                            // CAZUL 2: Am piesa selectata
                            else
                            {
                                // Click pe aceeasi piesa (Deselectare)
                                if (r == sursaRand && c == sursaCol) {
                                    sursaRand = -1; sursaCol = -1;
                                    cout << "Deselectat." << endl;
                                }
                                // Click pe alta piesa de-a mea (Schimbare)
                                else if (verificaLoc(r, c) == randulJucatorului) {
                                    sursaRand = r; sursaCol = c;
                                    cout << "Schimbat selectia." << endl;
                                }
                                // Click pe loc gol (Mutare)
                                else
                                {
                                    if (verificaMutare(sursaRand, sursaCol, r, c))
                                    {
                                        mutaPiesa(sursaRand, sursaCol, r, c);

                                        // Update logica de tura
                                        adversar = randulJucatorului;
                                        contorMutari++;

                                        if (randulJucatorului == 1) randulJucatorului = 2;
                                        else randulJucatorului = 1;

                                        sursaRand = -1; sursaCol = -1;
                                    }
                                    else {
                                        sursaRand = -1; sursaCol = -1;
                                        cout << "Mutare invalida!" << endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } // End PollEvent

        // ---------------- UPDATE VIZUAL BUTOANE (HOVER) ----------------
        if (currentState == GameState::MENU) {
            btnStart.shape.setFillColor(btnStart.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnSettings.shape.setFillColor(btnSettings.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
            btnQuit.shape.setFillColor(btnQuit.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }
        if (currentState == GameState::OPTIONS || currentState == GameState::GAME) {
            btnBack.shape.setFillColor(btnBack.isHovered(mousePosFloat) ? sf::Color(100, 100, 100) : sf::Color(50, 50, 50));
        }


        // ---------------- DESENARE ----------------
        window.clear(sf::Color(30, 30, 30)); // Fundal Gri Inchis

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
        else if (currentState == GameState::GAME)
        {
            // --- DESENARE TABLA DE JOC ---
            sf::RectangleShape rectangle({ 100.f, 100.f });

            // Folosim variabilele de pozitie, incepand de la Offset
            float rectX = offsetX;
            float rectY = offsetY;

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {

                    // Pozitionam patratul
                    rectangle.setPosition({ rectX, rectY });

                    // Culoare Tabla (Alb / Gri)
                    if ((i + j) % 2 == 0) rectangle.setFillColor(sf::Color::White);
                    else rectangle.setFillColor(sf::Color(100, 100, 100));

                    window.draw(rectangle);
                    rectX += 100.f; // Trecem la urmatoarea coloana
                }
                // Resetam coloana la OffsetX si trecem la randul urmator
                rectX = offsetX;
                rectY += 100.f;
            }

            // --- DESENARE PIESE ---
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int valoare = verificaLoc(i, j);
                    if (valoare != 0) {
                        // Formula corecta pentru pozitie: Offset + Indice * 100
                        piesa.setPosition({ offsetX + j * 100.f, offsetY + i * 100.f });

                        if (valoare == 1) piesa.setFillColor(sf::Color::White);
                        else piesa.setFillColor(sf::Color::Black);

                        if (i == sursaRand && j == sursaCol) piesa.setFillColor(sf::Color::Green);
                        window.draw(piesa);
                    }
                }
            }

            // Desenam si butonul de Back peste joc
            btnBack.draw(window);
        }
        else if (currentState == GameState::OPTIONS)
        {
            sf::Text txt(font, "SETTINGS", 70);
            sf::FloatRect bounds = txt.getLocalBounds();
            txt.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
            txt.setPosition({ 960.f, startY - 400.f });
            window.draw(txt);

            window.draw(titluRez);
            btnRez1.draw(window);
            btnRez2.draw(window);

            window.draw(titluMusic);
            sldMusic.draw(window);

            btnBack.draw(window);
        }

        window.display();
    }

    return 0;
}