#include "mainwindow.h"

//Funkcja do testowania, normalnie nieużywana
void writeINIDefaults()
{
    std::ofstream gameINI("game.ini");
    gameINI << "[GAME]"                                                                             << std::endl
            << "uTickRate = "                       << GameSettings::TICK_RATE                      << std::endl
            << "uBoardSize = "                      << GameSettings::BOARD_SIZE                     << std::endl
            << "uPlayerLineSize = "                 << GameSettings::LINE_SIZE                      << std::endl
            << "fMovementSpeed = "                  << GameSettings::SPEED                          << std::endl
            << "fRotationSpeed = "                  << GameSettings::ANGLE_SPEED                    << std::endl
            << "dGapChance = "                      << GameSettings::NO_DRAW_CHANCE                 << std::endl
            << "uMovementDataPackets = "            << GameSettings::DATA_SEND_TIMEOUT              << std::endl
            << "uWallThickness = "                  << GameSettings::WALL_THICKNESS                 << std::endl
            << "uMinGapSizeInTicks = "              << GameSettings::NO_DRAW_MIN                    << std::endl
            << "uMaxGapSizeInTicks = "              << GameSettings::NO_DRAW_MAX                    << std::endl
            << "uCollisionIgnoreAfterDrawTicks = "  << GameSettings::COLLISION_DRAW_FIRST_IGNORE    << std::endl
            << "uRoundStartWaitTicks = "            << GameSettings::FREEZE_TIME                    << std::endl
            << "uNoCollisionStartTicks = "          << GameSettings::GRACE_TIME                     << std::endl
            << "uAfterWinWaitTicks = "              << GameSettings::WIN_TIME                       << std::endl
                                                                                                    << std::endl
            << "[GUI]"                                                                              << std::endl
            << "uPlayerSize = "                     << GameSettings::PLAYER_SIZE                    << std::endl
            << "uFreezeFontSize = "                 << GameSettings::FREEZE_FONT_SIZE               << std::endl
            << "uWinFontSize = "                    << GameSettings::WIN_FONT_SIZE                  << std::endl
            << "uNameFontSize = "                   << GameSettings::NAME_FONT_SIZE;
    gameINI.close();

    std::ofstream networkINI("network.ini");
    networkINI
            << "[NET]"                                                                              << std::endl
            << "uTimeout = "                        << NetworkSettings::TIMEOUT                     << std::endl
            << "uPingInterval = "                   << NetworkSettings::PING_INTERVAL               << std::endl
            << "uPortStart = "                   << NetworkSettings::UDP_PORT_START                 << std::endl
            << "uPoolTime = "                   << NetworkSettings::POOL_TIME;
    networkINI.close();
}
void loadGameINI(MainWindow *window)
{
    //Tworzy plik .ini, jeżeli wcześniej go nie było
    std::ofstream gameINI("game.ini", std::ofstream::app);
    gameINI.close();

    //Wczytywanie opcji przy pomocy INIH
    //https://github.com/benhoyt/inih
    INIReader gameReader("game.ini");
    double def = GameSettings::SPEED;
    GameSettings::SPEED = gameReader.GetReal("GAME", "fSpeed", def);

    if (GameSettings::SPEED <= 0.f)
    {
        GameSettings::SPEED = def;
        QMessageBox::critical(window, "Error",
                              "Movement speed has invalid value: it cannot be less or equal 0! Defaulting...");
    }

    def = GameSettings::ANGLE_SPEED;
    GameSettings::ANGLE_SPEED = gameReader.GetReal("GAME", "fRotationSpeed", def);
    if (GameSettings::ANGLE_SPEED <= 0.f)
    {
        GameSettings::ANGLE_SPEED = def;
        QMessageBox::critical(window, "Error",
                              "Rotation speed has invalid value: it cannot be less or equal 0! Defaulting...");
    }

    def = GameSettings::NO_DRAW_CHANCE;
    GameSettings::NO_DRAW_CHANCE = gameReader.GetReal("GAME", "dGapChance", def);
    if (GameSettings::NO_DRAW_CHANCE < 0.0)
    {
        GameSettings::NO_DRAW_CHANCE = def;
        QMessageBox::critical(window, "Error",
                              ": it cannot be less than 0! Defaulting...");
    }

    auto getUnsigned = [&gameReader, window](const char *section, const char *name, unsigned &read)
    {
        long val = gameReader.GetInteger(section, name, read);
        if (val < 0)
        {
            QMessageBox::critical(window, "Error",
                                 (name + (": it cannot be less than 0! Returning to the default value " + std::to_string(read))).c_str());
        }
        else read = static_cast<unsigned>(val);
    };

    getUnsigned("GAME", "uBoardSize",                           GameSettings::BOARD_SIZE);
    getUnsigned("GAME", "uWallThickness",                       GameSettings::WALL_THICKNESS);
    getUnsigned("GAME", "uPlayerLineSize",                      GameSettings::LINE_SIZE);
    getUnsigned("GAME", "uMinGapSizeInTicks",                   GameSettings::NO_DRAW_MIN);
    getUnsigned("GAME", "uMaxGapSizeInTicks",                   GameSettings::NO_DRAW_MAX);
    getUnsigned("GAME", "uCollisionIgnoreAfterDrawTicks",       GameSettings::COLLISION_DRAW_FIRST_IGNORE);
    getUnsigned("GAME", "uMovementDataPackets",                 GameSettings::DATA_SEND_TIMEOUT);
    getUnsigned("GAME", "uAfterWinWaitTicks",                   GameSettings::WIN_TIME);
    getUnsigned("GAME", "uRoundStartWaitTicks",                 GameSettings::FREEZE_TIME);
    getUnsigned("GAME", "uNoCollisionStartTicks",               GameSettings::GRACE_TIME);
    getUnsigned("GAME", "uTickRate",                            GameSettings::TICK_RATE);

    getUnsigned("GUI",  "uPlayerSize",                          GameSettings::PLAYER_SIZE);
    getUnsigned("GUI",  "uFreezeFontSize",                      GameSettings::FREEZE_FONT_SIZE);
    getUnsigned("GUI",  "uWinFontSize",                         GameSettings::WIN_FONT_SIZE);
    getUnsigned("GUI",  "uNameFontSize",                        GameSettings::NAME_FONT_SIZE);
}

void loadNetworkINI(MainWindow *window)
{
    //Tworzy plik .ini, jeżeli wcześniej go nie było
    std::ofstream networkINI("network.ini", std::ofstream::app);
    networkINI.close();

    //Wczytywanie opcji przy pomocy INIH
    //https://github.com/benhoyt/inih
    INIReader networkReader("network.ini");

    auto getUnsigned = [&networkReader, window](const char *section, const char *name, unsigned &read)
    {
        int val = networkReader.GetInteger(section, name, read);
        if (val < 0)
        {
            QMessageBox::critical(window, "Error",
                                 (name + (": it cannot be less than 0! Returning to the default value " + std::to_string(read))).c_str());
        }
        else read = static_cast<unsigned>(val);
    };

    getUnsigned("NET", "uTimeout",                              NetworkSettings::TIMEOUT);
    getUnsigned("NET", "uPingInterval",                         NetworkSettings::PING_INTERVAL);
    unsigned temp;
    getUnsigned("NET", "uPortStart",                            temp);
    NetworkSettings::UDP_PORT_START = static_cast<unsigned short>(temp);
    getUnsigned("NET", "uPoolTime",                             NetworkSettings::POOL_TIME);

}

void voidvoid(int)
{

}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, &voidvoid); //Zepsuty pipe będzie kończyć połączenie, a nie program
    //writeINIDefaults();
    QApplication a(argc, argv);
    MainWindow window;

    //program
    window.show();
    loadGameINI(&window);
    loadNetworkINI(&window);

    return a.exec();
}
