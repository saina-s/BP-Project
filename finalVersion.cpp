//#include <SDL2.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <regex>
#include <map>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <shobjidl.h> 
#include <cmath>
#include <cstdlib>
namespace fs = std::filesystem;
using namespace std;


//TTF_Font* font = TTF_OpenFont(R"(C:\Windows\Fonts\consola.ttf)", 12);

void SetWindowIcon(SDL_Window* window) {
    SDL_Surface* icon = IMG_Load("icon.png");
    if (!icon) {
        printf("Failed to load icon: %s\n", IMG_GetError());
        return;
    }
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
}

struct button {
    SDL_Rect Rect = { 0,0,70,26 };
    SDL_Color normal;
    SDL_Color temp = normal;
    SDL_Color hover;
    //text related
    SDL_Color textColor;
    string name;
    fs::path path;

    void drawButton(SDL_Renderer* renderer) {

        SDL_SetRenderDrawColor(renderer, temp.r, temp.g, temp.b, temp.a);
        SDL_RenderFillRect(renderer, &Rect);
        SDL_Surface* TextSurface = TTF_RenderText_Solid(TTF_OpenFont(R"(C:\Windows\Fonts\consola.ttf)", 12), name.c_str(), textColor);
        SDL_Rect textRect = { Rect.x + (Rect.w - TextSurface->w) / 2, Rect.y + (Rect.h - TextSurface->h) / 2, TextSurface->w,TextSurface->h };
        SDL_Texture* TextTexture = SDL_CreateTextureFromSurface(renderer, TextSurface);
        SDL_RenderCopy(renderer, TextTexture, NULL, &textRect);
        SDL_FreeSurface(TextSurface);
    }
    void SetColor(SDL_Color color) { temp = color; }
    bool IsWithinBounds(int x, int y) const {
        // Too far left
        if (x < Rect.x) return false;
        // Too far right
        if (x > Rect.x + Rect.w) return false;
        // Too high
        if (y < Rect.y) return false;
        // Too low
        if (y > Rect.y + Rect.h) return false;
        // Within bounds
        return true;
    }
    int HandleEvent(const SDL_Event& E) {
        if (E.type == SDL_MOUSEMOTION) {
            HandleMouseMotion(E.motion);
            return 0;
        }
        else if (E.type == SDL_MOUSEBUTTONDOWN) {
            return HandleMouseButton(E.button);
        }
    }

    void HandleMouseMotion(
        const SDL_MouseMotionEvent& E) {
        if (IsWithinBounds(E.x, E.y)) {
            SetColor(hover);
        }
        else {
            SetColor(normal);
        }
    }

    int HandleMouseButton(const SDL_MouseButtonEvent& E) {
        if (IsWithinBounds(E.x, E.y)) {
            const Uint8 Button{ E.button };
            if (Button == SDL_BUTTON_LEFT) {
                return 1;
            }
            else if (Button == SDL_BUTTON_RIGHT) {
                return -1;
            }
        }

    }

};

// Screen dimensions
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 800;
// to be checked
const int MAX_UNDO_STATES = 100;
const int MAX_REDO_STATES = 100;


bool fileSubButton = false;
bool editSubButton = false;
bool dark = false;
bool quit = false;
SDL_Renderer* renderer;
TTF_Font* font;
SDL_Texture* bk_light;
SDL_Texture* bk_dark;
SDL_Rect bk_rect = { 200, 0, 800, 600 };
//SDL_Rect bk_rect = {0, 0, 1000, 600};

struct CPP {
    string title = "Untitled";
    vector<string> lines = { "" };
    vector<pair<vector<string>, vector<int>>> UndoStack;
    vector<pair<vector<string>, vector<int>>> RedoStack;
    fs::path myfilepath = "";
    bool saved = false;
};

void ensureLastLineVisible(int currentLine, int& scrollOffset, int SCREEN_HEIGHT, int LINE_HEIGHT, int totalLines);
void write(CPP& file);
void SaveCurrentState(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset);
void Undo(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset);
void Redo(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset);
int goToLine(int totalLines);
void Copy(CPP& file, pair<int, int> selectStart, pair <int, int> selectEnd);
void Delete(CPP& file, pair<int, int> selectStart, pair <int, int> selectEnd, int& currentLine, int& cursorPos);
void Paste(CPP& file, int& currentLine, int& cursorPos);
void getName(string& name);
string openFileDialog();
string openFolderDialog();
void saveProject(CPP &file);
void getName(string& name);

string ConvertWideStringToString(const std::wstring& wideStr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string result(bufferSize, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);
    result.pop_back();  // Remove extra null character
    return result;
}

// Debug related
vector <string> Errors = {};
void checkParentheses(const CPP& file);
void checkMultiLineComments(const CPP& file);
void checkSemicolons(const CPP& file);


// Compile and run related function
void compileAndRun(const std::string& myfilepath) {
    // file path without the extension
    std::string pathWithoutExt = myfilepath.substr(0, myfilepath.find_last_of('.'));

    // Compile the file using g++
    std::string compileCommand = "g++ \"" + myfilepath + "\" -o \"" + pathWithoutExt + "\"";
    std::cout << "Compiling: " << compileCommand << std::endl;
    int compileResult = system(compileCommand.c_str());

    if (compileResult != 0) {
        std::cerr << "Compilation failed!" << std::endl;
        return;
    }

    // Run the executable in a new command prompt window
    std::string runCommand = "start cmd /k \"" + pathWithoutExt + " & pause\"";
    std::cout << "Running: " << runCommand << std::endl;
    system(runCommand.c_str());
}


// Tree related
vector<button> folderbuttons;
vector<button> filebuttons;
const SDL_Rect startree = { 5,104,150, 26 };
void showTree(fs::path path, vector<button>& file);
fs::path path = ".";
string currentfile;
string currentfolder;
string showpath(fs::path str);
void importFile(fs::path p, CPP& f);

int main(int argc, char* argv[]) {

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow("SDL Text Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    SetWindowIcon(window);
    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Load font
    font = TTF_OpenFont(R"(C:\Windows\Fonts\consola.ttf)", 24); // Replace with the path to your .ttf font
    //TTF_Font* font = TTF_OpenFont(R"(..\cmake-build-debug\cour.ttf)", 20); // Replace with the path to your .ttf font
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }


    bk_light = IMG_LoadTexture(renderer, "bk_light.png");
    bk_dark = IMG_LoadTexture(renderer, "bk_dark.png");

    CPP file;

    write(file);


    // Clean up
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();


    return 0;

}

void ensureLastLineVisible(int currentLine, int& scrollOffset, int SCREEN_HEIGHT, int LINE_HEIGHT, int totalLines) {
    int cursorY = currentLine * LINE_HEIGHT - scrollOffset;
    if (cursorY < 0) {
        // Scroll up
        scrollOffset = currentLine * LINE_HEIGHT;
    }
    else if (cursorY + LINE_HEIGHT > SCREEN_HEIGHT) {
        // Scroll down
        //cout << "THIS" << endl;
        scrollOffset = (currentLine + 1) * LINE_HEIGHT - SCREEN_HEIGHT;
    }

    // Ensure last line is always visible
    int contentHeight = totalLines * LINE_HEIGHT;
    if (contentHeight > SCREEN_HEIGHT) {
        scrollOffset = min(scrollOffset, contentHeight - SCREEN_HEIGHT);
    }
    else {
        scrollOffset = 0; // No scrolling needed if content fits
    }
}

void write(CPP& file) {
    //bool quit = false;
    //initializing needed buttons
    SDL_Color normal1;
    SDL_Color hover1;
    SDL_Color textColor1;

    normal1 = { 200, 200,200,255 };
    hover1 = { 180,180,180,255 };
    textColor1 = { 0,0,0,255 };

    //main buttons
    //file
    button fileb;
    fileb.name = "File";
    fileb.Rect = { 0,0,110,26 };
    fileb.normal = normal1;
    fileb.hover = hover1;
    fileb.temp = fileb.normal;
    fileb.textColor = textColor1;

    //Edit
    button editb;
    editb.name = "Edit";
    editb.Rect = { 111,0,110,26 };
    editb.normal = normal1;
    editb.hover = hover1;
    editb.temp = editb.normal;
    editb.textColor = textColor1;


    //Debug & compile
    button debugb;
    debugb.name = "Debug & compile";
    debugb.Rect = { 222,0,130,26 };
    debugb.normal = normal1;
    debugb.hover = hover1;
    debugb.temp = debugb.normal;
    debugb.textColor = textColor1;

    // Run 
    button runb;
    runb.name = "RUN";
    runb.Rect = { 353, 0, 50, 26 };
    runb.normal = { 207,35, 41, 255 };
    runb.hover = { 179, 29, 34, 255 };
    runb.temp = runb.normal;
    runb.textColor = textColor1;

    //choose a file
    button open;
    open.name = "Choose a file";
    open.Rect = { 404,0,110,26 };
    open.normal = normal1;
    open.hover = hover1;
    open.textColor = textColor1;

    // Mode
    button modeb;
    modeb.Rect = { 515, 0 ,60, 26 };
    modeb.name = "MODE";
    modeb.normal = normal1;
    modeb.hover = hover1;
    modeb.temp = modeb.normal;
    modeb.textColor = textColor1;


    // New Project
    button newp;
    newp.normal = normal1;
    newp.hover = hover1;
    newp.textColor = textColor1;
    newp.name = "New Project";
    newp.Rect = { 0,26,110,26 };

    // Save Project
    button savep;
    savep.normal = normal1;
    savep.hover = hover1;
    savep.textColor = textColor1;
    savep.name = "Save Project";
    savep.Rect = { 0,52,110,26 };

    // Exit
    button exitb;
    exitb.normal = normal1;
    exitb.hover = hover1;
    exitb.textColor = textColor1;
    exitb.name = "Exit";
    exitb.Rect = { 0,78,110,26 };

    // Undo
    button undob;
    undob.normal = normal1;
    undob.hover = hover1;
    undob.textColor = textColor1;
    undob.name = "Undo";
    undob.Rect = { 111,26,110,26 };

    // Redo
    button redob;
    redob.normal = normal1;
    redob.hover = hover1;
    redob.textColor = textColor1;
    redob.name = "Redo";
    redob.Rect = { 111,52,110,26 };


    //showTree(path, filebuttons);

    // current file
    button currentfilebutton;
    currentfilebutton.normal = { 161,37,84, 255 };
    currentfilebutton.hover = { 161,37,84, 255 };
    currentfilebutton.temp = { 161,37,84, 255 };
    currentfilebutton.textColor = { 255,255,255,255 };
    //currentfilebutton.name = "Project: " + file.title;
    currentfilebutton.Rect = { 5,34,185,26 };

    // Current folder
    button currentfolderbutton;
    currentfolderbutton.normal = { 161,37,84, 255 };
    currentfolderbutton.hover = { 161,37,84, 255 };
    currentfolderbutton.temp = { 161,37,84, 255 };
    currentfolderbutton.textColor = { 255,255,255,255 };
    //currentfolderbutton.name = showpath(path) == "" ? "?" : showpath(path);
    currentfolderbutton.Rect = { 5,70,185,26 };

    // Tree Graph
    SDL_Rect treeg;
    treeg.x = 0;
    treeg.y = 27;
    treeg.w = 195;
    treeg.h = SCREEN_WIDTH;


    int currentLine = 0;
    int cursorPos = 0;
    int scrollOffset = 0;
    const int LINE_HEIGHT = TTF_FontHeight(font);
    SaveCurrentState(file, currentLine, cursorPos, scrollOffset);

    // Timer for cursor blinking
    Uint32 lastCursorToggle = SDL_GetTicks();
    bool cursorVisible = true;
    const Uint32 CURSOR_BLINK_INTERVAL = 500; // 500 ms for blinking

    bool select = false;
    pair <int, int> selectStart;
    pair <int, int> selectEnd;
    //line, curpos
    char signsOpen[3] = { '(', '[', '{' };
    char signsClose[3] = { ')', ']', '}' };

    while (!quit) {
        // Handle cursor blinking
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > lastCursorToggle + CURSOR_BLINK_INTERVAL) {
            cursorVisible = !cursorVisible;
            lastCursorToggle = currentTime;
        }

        SDL_Event e;
        // Event loop


        while (SDL_PollEvent(&e) != 0) {
            showTree(path, filebuttons);


            currentfilebutton.name = "Project: " + file.title;
            currentfolderbutton.name = showpath(path) == "" ? "?" : showpath(path);



            if (debugb.HandleEvent(e) == 1) {
                if (file.saved == false) {
                    saveProject(file);
                }
                else {
                    checkSemicolons(file);
                    checkParentheses(file);
                    checkMultiLineComments(file);
                    SDL_Rect ErrorRect = { 195,400,805,400 };
                    SDL_SetRenderDrawColor(renderer, 201, 54, 172, 255);
                    SDL_RenderFillRect(renderer, &ErrorRect);
                    SDL_RenderPresent(renderer);
                    SDL_Color textColor2 = { 255, 255, 255, 255 };
                    TTF_Font* font2 = TTF_OpenFont(R"(C:\Windows\Fonts\consola.ttf)", 18);
                    for (int i = 0; i < Errors.size(); i++) {
                        SDL_Surface* textSurface2 = TTF_RenderText_Blended(font2, Errors[i].c_str(), textColor2);
                        SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
                        int textWidth = textSurface2->w;
                        int textHeight = textSurface2->h;
                        SDL_Rect renderQuad = { 200, 400 + 20 * i, textWidth, textHeight };
                        SDL_FreeSurface(textSurface2);
                        SDL_RenderCopy(renderer, textTexture2, nullptr, &renderQuad);
                        SDL_DestroyTexture(textTexture2);
                    }
                    SDL_RenderPresent(renderer);
                    SDL_Event e2;
                    bool openErrorBox = true;
                    while (openErrorBox) {
                        while (SDL_PollEvent(&e2) != 0) {
                            if (e2.type == SDL_QUIT) {
                                quit = true;
                                openErrorBox = false;
                            }
                            else if (e2.type == SDL_KEYDOWN) {
                                if (e2.key.keysym.sym == SDLK_RETURN) {
                                    openErrorBox = false;
                                }
                            }
                        }
                    }
                }
                Errors.clear();

            }

            if(runb.HandleEvent(e)==1){
                if (!file.saved) {
                    saveProject(file);
                }
                else {
                    compileAndRun(file.myfilepath.string());
                }
                
            }

            if (modeb.HandleEvent(e) == 1) {
                if (dark) {
                    dark = false;
                }
                else dark = true;
            }

            if (fileb.HandleEvent(e) == 1) {
                fileSubButton = fileSubButton ? false : true;
            }

            if (fileSubButton == true) {
                
                if (exitb.HandleEvent(e)==1) {
                    quit = true;
                    break;
                }
                if (newp.HandleEvent(e)==1) {
                    file.lines = {""};
                    file.myfilepath = "";
                    file.RedoStack.clear();
                    file.UndoStack.clear();
                    file.saved = false;
                    currentfile = "?";
                }
                if (savep.HandleEvent(e) == 1) {
                    saveProject(file);
                    currentfolder = file.myfilepath.parent_path().string();
                }
            }
            if (editb.HandleEvent(e) == 1) {
                editSubButton = editSubButton ? false : true;
            }
            if (editSubButton == true) {
                int unb = undob.HandleEvent(e);
                int reb = redob.HandleEvent(e);
                if (unb == 1) { Undo(file, currentLine, cursorPos, scrollOffset); }
                else if (reb == 1) { Redo(file, currentLine, cursorPos, scrollOffset); }

            }
            if (open.HandleEvent(e) == 1) {
                path = openFileDialog();
                importFile(path, file);
                path = path.parent_path();
                filebuttons.clear();
                folderbuttons.clear();
                currentfile = path.parent_path().string();
                currentfolder = fs::path(currentfile).parent_path().string();
                showTree(path, filebuttons);
            }

            int i1 = 0, j1 = 0;
            if (!filebuttons.empty()) {
                for (auto& i : filebuttons) {
                    i.Rect = SDL_Rect{ 5, 104 + i1 * 26, 170, 26 };
                    i.hover = SDL_Color{ 100, 100, 100, 255 };
                    i1++;
                    int event = i.HandleEvent(e);
                    if (event == 1) {
                        importFile(i.path, file);
                        //currentfile = i.path.filename().string();
                        //file.saved = true;
                        file.RedoStack.clear();
                        file.UndoStack.clear();
                    }

                }
            }

            if (!folderbuttons.empty()) {
                for (auto& j : folderbuttons) {
                    j.hover = SDL_Color{ 100, 100, 100, 255 };
                    j1++;
                    j.Rect = SDL_Rect{ 5, 104 + (j1 + i1) * 26, 170, 26 };
                    int event = j.HandleEvent(e);
                    if (event == 1) {

                        i1 = 0;
                        j1 = 0;
                        currentfolder = showpath(j.path);
                        string temp = j.path.string();
                        path = fs::path(temp);
                        showTree(path, filebuttons);
                        filebuttons.clear();
                        folderbuttons.clear();

                    }
                }
            }




            if (e.type == SDL_QUIT) {
                quit = true;
                //return;
            }
            else if (e.type == SDL_MOUSEWHEEL) {
                // Handle scroll
                if (e.wheel.y > 0) { // Scroll up
                    scrollOffset = max(0, scrollOffset - LINE_HEIGHT);
                }
                else if (e.wheel.y < 0) { // Scroll down
                    scrollOffset += LINE_HEIGHT;
                }
            }
            else if (e.type == SDL_KEYDOWN) {

                if (e.key.keysym.sym == SDLK_s) {
                    // Check if Ctrl is held down
                    if (SDL_GetModState() & KMOD_CTRL) {
                        //std::cout << "Ctrl + S was pressed!" << std::endl;
                        //SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                        //cout << file.UndoStack.size() << endl;
                        saveProject(file);
                        //cout << "SAVE" << endl;
                        // to be filled
                    }
                }
                else if (e.key.keysym.sym == SDLK_h) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        //cout << "last curpose of this line: " << file.lines[currentLine].length() << endl;
                        //cout << "last curpose of last line: " << file.lines.back().length() << endl;
                    }
                }
                else if (e.key.keysym.sym == SDLK_z) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        Undo(file, currentLine, cursorPos, scrollOffset);

                    }
                }
                else if (e.key.keysym.sym == SDLK_y) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        Redo(file, currentLine, cursorPos, scrollOffset);

                    }
                }
                else if (e.key.keysym.sym == SDLK_g) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        int lineOfGoTo = goToLine(file.lines.size());
                        scrollOffset = (lineOfGoTo - 1) * LINE_HEIGHT;
                        currentLine = lineOfGoTo - 1;
                        cursorPos = 0;
                    }
                }
                else if (e.key.keysym.sym == SDLK_c) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        Copy(file, selectStart, selectEnd);
                    }
                }
                else if (e.key.keysym.sym == SDLK_v) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        Paste(file, currentLine, cursorPos);
                        ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                        SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                        //ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                    }
                }
                else if (e.key.keysym.sym == SDLK_x) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        Copy(file, selectStart, selectEnd);
                        Delete(file, selectStart, selectEnd, currentLine, cursorPos);
                        SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                        //ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                    }
                }
                else if (e.key.keysym.sym == SDLK_a) {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        if (e.key.keysym.mod & KMOD_SHIFT) {
                            if (!select) {
                                selectStart = { currentLine, cursorPos };
                                select = true;
                                cout << "selecting ..." << endl;
                            }
                            else {
                                select = false;
                                selectEnd = { currentLine, cursorPos };
                                if (selectStart.first > selectEnd.first || (selectStart.first == selectEnd.first && selectStart.second > selectEnd.second)) {
                                    swap(selectStart, selectEnd);
                                }
                                cout << "from line " << selectStart.first << " at " << selectStart.second << " till line " << selectEnd.first << " at " << selectEnd.second << endl;
                            }
                        }
                        else {
                            // to be checked
                            select = false;
                            selectStart = { 0, 0 };
                            selectEnd = { file.lines.size() - 1, file.lines.back().length() };
                            cout << "selected the whole text" << endl;
                        }
                    }
                }
                else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    // Ensure cursorPos is within the valid range
                    if (cursorPos > 0 && cursorPos <= file.lines[currentLine].size()) {
                        // Remove character before cursor
                        file.lines[currentLine].erase(cursorPos - 1, 1);
                        cursorPos--;
                    }
                    else if (currentLine > 0) {
                        // Merge with previous line
                        cursorPos = file.lines[currentLine - 1].size();
                        file.lines[currentLine - 1] += file.lines[currentLine].substr(0, file.lines[currentLine].size());
                        file.lines.erase(file.lines.begin() + currentLine);
                        currentLine--;
                    }

                    // Ensure there's always at least one line
                    if (file.lines.empty()) {
                        file.lines.push_back("");
                        currentLine = 0;
                        cursorPos = 0;
                    }
                    SaveCurrentState(file, currentLine, cursorPos, scrollOffset);

                }
                else if (e.key.keysym.sym == SDLK_RETURN) {
                    //cout << "pressed enter" << endl;
                    if (cursorPos <= file.lines[currentLine].size()) {
                        std::string remainder = file.lines[currentLine].substr(cursorPos);
                        file.lines[currentLine] = file.lines[currentLine].substr(0, cursorPos);
                        file.lines.insert(file.lines.begin() + currentLine + 1, remainder);
                        currentLine++;
                        cursorPos = 0;
                        ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                        SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                    }
                }
                else if (e.key.keysym.sym == SDLK_TAB) {
                    // Add spaces for tab
                    file.lines[currentLine].insert(cursorPos, "    ");
                    cursorPos += 4;
                    SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                }
                else if (e.key.keysym.sym == SDLK_LEFT) {
                    // Move cursor left
                    if (cursorPos > 0) {
                        cursorPos--;
                    }
                    else if (currentLine > 0) {
                        currentLine--;
                        cursorPos = file.lines[currentLine].size();
                    }
                }
                else if (e.key.keysym.sym == SDLK_RIGHT) {
                    // Move cursor right
                    if (cursorPos < file.lines[currentLine].size()) {
                        cursorPos++;
                    }
                    else if (currentLine < file.lines.size() - 1) {
                        currentLine++;
                        cursorPos = 0;
                    }
                }
                else if (e.key.keysym.sym == SDLK_UP) {
                    // Move cursor up
                    if (currentLine > 0) {
                        currentLine--;
                        cursorPos = min(cursorPos, (int)file.lines[currentLine].size());
                        ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                    }
                }
                else if (e.key.keysym.sym == SDLK_DOWN) {
                    if (currentLine < file.lines.size() - 1) {
                        currentLine++;
                        cursorPos = min(cursorPos, (int)file.lines[currentLine].size());
                        ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                    }
                }
            }
            else if (e.type == SDL_TEXTINPUT) {
                if (e.text.text) {
                    bool skip = false;
                    for (int i = 0; i < 3; i++) {
                        if (string(e.text.text)[0] == signsClose[i] && cursorPos && file.lines[currentLine][cursorPos - 1] == signsOpen[i]) {
                            //cout << "this" << endl;
                            cursorPos++;
                            //cursorPos += strlen(e.text.text);
                            skip = true;
                        }
                    }
                    if (!skip) {
                        file.lines[currentLine].insert(cursorPos, e.text.text);
                        cursorPos += strlen(e.text.text);
                        for (int i = 0; i < 3; i++) {
                            if (string(e.text.text)[0] == signsOpen[i]) {
                                //cout << signsClose[i];
                                file.lines[currentLine].insert(cursorPos, 1, signsClose[i]);
                            }
                        }
                    }
                    ensureLastLineVisible(currentLine, scrollOffset, SCREEN_HEIGHT, LINE_HEIGHT, file.lines.size());
                    SaveCurrentState(file, currentLine, cursorPos, scrollOffset);
                }
            }
        }

        // Clear screen
        //dark ? SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) : SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        //SDL_RenderClear(renderer);

        // Load background        
        dark ? SDL_RenderCopy(renderer, bk_dark, nullptr, nullptr) : SDL_RenderCopy(renderer, bk_light, nullptr, nullptr);


        // Render text
        std::regex commentRegex("[//]{2}(.*)");
        std::regex keywordRegex("\\b(class|if|else|while|switch|case|default)\\b")
            ;
        std::regex dataTypeRegex("\\b(int|float|double|bool|string|char|long long)\\b");

        //function name
        std::regex funcNameRegex("\\b(func)\\b");
        //variable name
        std::regex varNameRegex("\\b(var)\\b");

        std::regex charStringRegex("((\\').{1}(\\'))|((\").*(\"))");
        //doesn't let you use patterns inside ""

        std::regex numberRegex("\\b\\d+\\b");
        std::regex preprocessorRegex("#include");
        std::regex operatorRegex("[=+-/&|\\*]");
        std::regex parenthesesRegex("[(){}\\[\\]]");

        vector<SDL_Color> lightColors;

        lightColors.push_back({ 128, 128, 128, 255 });
        lightColors.push_back({ 0, 51, 102, 255 });
        lightColors.push_back({ 0, 128, 128, 255 });
        lightColors.push_back({ 255, 140, 0, 255 });
        lightColors.push_back({ 139, 0, 0, 255 });
        lightColors.push_back({ 0, 100, 0, 255 });
        lightColors.push_back({ 128, 0, 128, 255 });
        lightColors.push_back({ 0, 136, 184, 255 });
        lightColors.push_back({ 128, 0, 0, 255 });
        lightColors.push_back({ 184, 134, 11, 255 });
        lightColors.push_back({ 255, 255, 255, 255 });

        vector<SDL_Color> darkColors;

        darkColors.push_back({ 92, 99, 112, 255 });
        darkColors.push_back({ 198, 120, 221, 255 });
        darkColors.push_back({ 224, 108, 117, 255 });
        darkColors.push_back({ 97, 175, 254, 255 });
        darkColors.push_back({ 229, 192, 123, 255 });
        darkColors.push_back({ 152, 195, 121, 255 });
        darkColors.push_back({ 209, 154, 102, 255 });
        darkColors.push_back({ 86, 182, 194, 255 });
        darkColors.push_back({ 213, 94, 0, 255 });
        darkColors.push_back({ 171, 178, 191, 255 });
        darkColors.push_back({ 0, 0, 0, 255 });


        for (size_t i = 0; i < file.lines.size(); ++i) {
            int y = i * LINE_HEIGHT - scrollOffset + 27;
            if (y + LINE_HEIGHT > 0 && y < SCREEN_HEIGHT) {
                int offsetX = 200; // Starting X position
                const std::string& line = file.lines[i];

                std::sregex_iterator commentIt(line.begin(), line.end(), commentRegex);
                std::sregex_iterator keywordIt(line.begin(), line.end(), keywordRegex);
                std::sregex_iterator dataTypeIt(line.begin(), line.end(), dataTypeRegex);
                std::sregex_iterator funcNameIt(line.begin(), line.end(), funcNameRegex);
                std::sregex_iterator varNameIt(line.begin(), line.end(), varNameRegex);
                std::sregex_iterator charStringIt(line.begin(), line.end(), charStringRegex);
                std::sregex_iterator numberIt(line.begin(), line.end(), numberRegex);
                std::sregex_iterator preprocessorIt(line.begin(), line.end(), preprocessorRegex);
                std::sregex_iterator operatorIt(line.begin(), line.end(), operatorRegex);
                std::sregex_iterator parenthesesIt(line.begin(), line.end(), parenthesesRegex);
                std::sregex_iterator end;


                size_t startPos = 0;
                bool commented = false;
                while (!commented && (commentIt != end || keywordIt != end || dataTypeIt != end ||
                    funcNameIt != end || varNameIt != end || charStringIt != end || numberIt != end ||
                    preprocessorIt != end || operatorIt != end || parenthesesIt != end)) {
                    std::smatch match;
                    SDL_Color color;


                    if (commentIt != end &&
                        (keywordIt == end || commentIt->position() <= keywordIt->position()) &&
                        (dataTypeIt == end || commentIt->position() <= dataTypeIt->position()) &&
                        (funcNameIt == end || commentIt->position() <= funcNameIt->position()) &&
                        (varNameIt == end || commentIt->position() <= varNameIt->position()) &&
                        (charStringIt == end || commentIt->position() <= charStringIt->position()) &&
                        (numberIt == end || commentIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || commentIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || commentIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || commentIt->position() <= parenthesesIt->position())) {
                        match = *commentIt;
                        //color = (dark == false)? lightColors[0] : darkColors[0];
                        color = dark ? darkColors[0] : lightColors[0];
                        ++commentIt;
                        commented = true;
                    }
                    else if (keywordIt != end &&
                        (dataTypeIt == end || keywordIt->position() <= dataTypeIt->position()) &&
                        (funcNameIt == end || keywordIt->position() <= funcNameIt->position()) &&
                        (varNameIt == end || keywordIt->position() <= varNameIt->position()) &&
                        (charStringIt == end || keywordIt->position() <= charStringIt->position()) &&
                        (numberIt == end || keywordIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || keywordIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || keywordIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || keywordIt->position() <= parenthesesIt->position())) {

                        match = *keywordIt;
                        color = dark ? darkColors[1] : lightColors[1];
                        ++keywordIt;
                    }
                    else if (dataTypeIt != end &&
                        (funcNameIt == end || dataTypeIt->position() <= funcNameIt->position()) &&
                        (varNameIt == end || dataTypeIt->position() <= varNameIt->position()) &&
                        (charStringIt == end || dataTypeIt->position() <= charStringIt->position()) &&
                        (numberIt == end || dataTypeIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || dataTypeIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || dataTypeIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || dataTypeIt->position() <= parenthesesIt->position())) {


                        match = *dataTypeIt;
                        color = dark ? darkColors[2] : lightColors[2];
                        ++dataTypeIt;
                    }
                    else if (funcNameIt != end &&
                        (varNameIt == end || funcNameIt->position() <= varNameIt->position()) &&
                        (charStringIt == end || funcNameIt->position() <= charStringIt->position()) &&
                        (numberIt == end || funcNameIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || funcNameIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || funcNameIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || funcNameIt->position() <= parenthesesIt->position())) {

                        match = *funcNameIt;
                        color = dark ? darkColors[3] : lightColors[3];
                        ++funcNameIt;
                    }
                    else if (varNameIt != end &&
                        (charStringIt == end || varNameIt->position() <= charStringIt->position()) &&
                        (numberIt == end || varNameIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || varNameIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || varNameIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || varNameIt->position() <= parenthesesIt->position())) {
                        match = *varNameIt;
                        color = dark ? darkColors[4] : lightColors[4];
                        ++varNameIt;
                    }
                    else if (charStringIt != end &&
                        (numberIt == end || charStringIt->position() <= numberIt->position()) &&
                        (preprocessorIt == end || charStringIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || charStringIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || charStringIt->position() <= parenthesesIt->position())) {
                        match = *charStringIt;
                        color = dark ? darkColors[5] : lightColors[5];
                        ++charStringIt;
                    }
                    else if (numberIt != end &&
                        (preprocessorIt == end || numberIt->position() <= preprocessorIt->position()) &&
                        (operatorIt == end || numberIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || numberIt->position() <= parenthesesIt->position())) {
                        match = *numberIt;
                        color = dark ? darkColors[6] : lightColors[6];
                        ++numberIt;
                    }
                    else if (preprocessorIt != end &&
                        (operatorIt == end || preprocessorIt->position() <= operatorIt->position()) &&
                        (parenthesesIt == end || preprocessorIt->position() <= parenthesesIt->position())) {
                        match = *preprocessorIt;
                        color = dark ? darkColors[7] : lightColors[7];
                        ++preprocessorIt;
                    }
                    else if (operatorIt != end &&
                        (parenthesesIt == end || operatorIt->position() <= parenthesesIt->position())) {
                        match = *operatorIt;
                        color = dark ? darkColors[8] : lightColors[8];
                        ++operatorIt;
                    }
                    else {
                        match = *parenthesesIt;
                        color = dark ? darkColors[9] : lightColors[9];

                        ++parenthesesIt;
                    }

                    // Render text before the keyword
                    if (match.position() > startPos) {
                        std::string beforeKeyword = line.substr(startPos, match.position() - startPos);
                        SDL_Color remainColor = dark ? lightColors[10] : darkColors[10];
                        SDL_Surface* surface = TTF_RenderText_Solid(font, beforeKeyword.c_str(), remainColor);
                        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                        int textWidth, textHeight;
                        TTF_SizeText(font, beforeKeyword.c_str(), &textWidth, &textHeight);
                        SDL_Rect renderQuad = { offsetX, y, textWidth, textHeight };
                        SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);

                        offsetX += textWidth;
                        SDL_FreeSurface(surface);
                        SDL_DestroyTexture(texture);
                    }

                    // Render the keyword in respective color
                    SDL_Surface* surface = TTF_RenderText_Solid(font, match.str().c_str(), color);
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                    int textWidth, textHeight;
                    TTF_SizeText(font, match.str().c_str(), &textWidth, &textHeight);
                    SDL_Rect renderQuad = { offsetX, y, textWidth, textHeight };
                    SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);

                    offsetX += textWidth;
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);

                    startPos = match.position() + match.length();
                }

                // Render remaining text
                if (startPos < line.size()) {
                    std::string rest = line.substr(startPos);
                    //SDL_Color black = {0, 0, 0, 255};
                    //SDL_Surface* surface = TTF_RenderText_Solid(font, rest.c_str(), black);
                    SDL_Color remainColor = dark ? lightColors[10] : darkColors[10];
                    SDL_Surface* surface = TTF_RenderText_Solid(font, rest.c_str(), remainColor);
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

                    int textWidth, textHeight;
                    TTF_SizeText(font, rest.c_str(), &textWidth, &textHeight);
                    SDL_Rect renderQuad = { offsetX, y, textWidth, textHeight };
                    SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);

                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                }

                // Render cursor
                if (i == currentLine && cursorVisible) {
                    int cursorX = 200;
                    if (cursorPos > 0) {
                        TTF_SizeText(font, file.lines[i].substr(0, cursorPos).c_str(), &cursorX, nullptr);
                        cursorX += 200;
                    }
                    dark ? SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) : SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDrawLine(renderer, cursorX, y, cursorX, y + LINE_HEIGHT);
                }
            }
        }



        // Update screen
        SDL_Rect head = { 0,0,1000,26 };
        dark ? SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) : SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &head);
        // tree graph
        if (dark == true) {
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255);
        }
        SDL_RenderFillRect(renderer, &treeg);

        if (!folderbuttons.empty()) {
            for (auto &x : folderbuttons) {
                x.normal = dark ? SDL_Color{ 50, 50, 50, 255 } : SDL_Color{ 155, 155, 155, 255 };
                x.temp = x.normal;
                x.textColor = dark ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 0, 0, 0, 255 };
                x.hover = SDL_Color{ 100, 100, 100, 255 };
                x.drawButton(renderer);
            }
        }
        if (!filebuttons.empty()) {
            for (auto &x : filebuttons) {
                x.normal = dark ? SDL_Color{ 50, 50, 50, 255 } : SDL_Color{ 155, 155, 155, 255 };
                x.temp = x.normal;
                x.textColor = dark ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 0, 0, 0, 255 };
                x.hover = SDL_Color{ 100, 100, 100, 255 };
                x.drawButton(renderer);
            }
        }
        currentfilebutton.drawButton(renderer);
        currentfolderbutton.drawButton(renderer);

        // Sub buttons
        if (fileSubButton == true) {
            newp.drawButton(renderer);
            savep.drawButton(renderer);
            exitb.drawButton(renderer);
        }
        if (editSubButton == true) {
            redob.drawButton(renderer);
            undob.drawButton(renderer);
        }
        // buttons
        fileb.drawButton(renderer);
        editb.drawButton(renderer);
        debugb.drawButton(renderer);
        runb.drawButton(renderer);
        modeb.drawButton(renderer);
        open.drawButton(renderer);


        SDL_RenderPresent(renderer);
    }
}

void SaveCurrentState(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset) {
    if (file.UndoStack.size() >= MAX_UNDO_STATES) {
        file.UndoStack.erase(file.UndoStack.begin());
    }
    file.UndoStack.push_back({ file.lines,{currentLine, cursorPos, scrollOffset} });
    file.RedoStack.clear();
}

void Undo(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset) {
    if (file.UndoStack.size() > 1) {
        if (file.RedoStack.size() >= MAX_UNDO_STATES) {
            file.RedoStack.erase(file.RedoStack.begin());
        }
        file.RedoStack.push_back(file.UndoStack.back());
        file.UndoStack.pop_back();
        file.lines = file.UndoStack.back().first;
        currentLine = file.UndoStack.back().second[0];
        cursorPos = file.UndoStack.back().second[1];
        scrollOffset = file.UndoStack.back().second[2];
    }
}

void Redo(CPP& file, int& currentLine, int& cursorPos, int& scrollOffset) {
    if (file.RedoStack.size() > 0) {
        file.lines = file.RedoStack.back().first;
        currentLine = file.RedoStack.back().second[0];
        cursorPos = file.RedoStack.back().second[1];
        scrollOffset = file.RedoStack.back().second[2];
        if (file.UndoStack.size() >= MAX_UNDO_STATES) {
            file.UndoStack.erase(file.UndoStack.begin());
        }
        file.UndoStack.push_back(file.RedoStack.back());
        file.RedoStack.pop_back();
    }
}

int goToLine(int totalLines) {
    int line = 0;
    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                return 1;
            }
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    if (line > totalLines) {
                        return totalLines;
                    }
                    return line;
                }
                if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
                    line *= 10;
                    line += e.key.keysym.sym - SDLK_0;
                }
                if (e.key.keysym.sym >= SDLK_KP_1 && e.key.keysym.sym <= SDLK_KP_0) {
                    line *= 10;
                    line += (e.key.keysym.sym - SDLK_KP_0 + 10) % 10;
                }
            }
        }
    }
}

void getName(string& name) {
    name = "";
    SDL_Event e;

    while (true) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                return;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    return;
                }
            }
            else if (e.type == SDL_TEXTINPUT) {
                if (e.text.text) {
                    name += e.text.text;
                }
            }
        }
    }
}

void Copy(CPP& file, pair<int, int> selectStart, pair <int, int> selectEnd) {
    // cout << selectStart.first << " " << selectStart.second << endl;
    // cout << selectEnd.first << " " << selectEnd.second << endl;

    string text = "";

    if (selectStart.first == selectEnd.first) {
        text = file.lines[selectStart.first].substr(selectStart.second, selectEnd.second - selectStart.second);
        SDL_SetClipboardText(text.c_str());
        return;
    }

    // start line
    text = file.lines[selectStart.first].substr(selectStart.second);
    // middle lines
    for (int i = selectStart.first + 1; i < selectEnd.first; i++) {
        text += '\n';
        text += file.lines[i];
    }
    //last line
    text += '\n';
    text += file.lines[selectEnd.first].substr(0, selectEnd.second);

    SDL_SetClipboardText(text.c_str());
}

void Delete(CPP& file, pair<int, int> selectStart, pair <int, int> selectEnd, int& currentLine, int& cursorPos) {
    file.lines[selectStart.first] = file.lines[selectStart.first].substr(0, selectStart.second) + file.lines[selectEnd.first].substr(selectEnd.second, file.lines[selectEnd.first].length() - selectEnd.second);
    file.lines.erase(file.lines.begin() + selectStart.first + 1, file.lines.begin() + selectEnd.first + 1);
    currentLine = selectStart.first;
    cursorPos = selectStart.second;
}

void Paste(CPP& file, int& currentLine, int& cursorPos) {
    string get;
    if (SDL_HasClipboardText()) {
        char* clipboardText = SDL_GetClipboardText();
        get = string(clipboardText);
        SDL_free(clipboardText);
    }
    for (int i = 0; i < get.size(); i++) {
        if (get[i] == '\n') {
            if (cursorPos <= file.lines[currentLine].size()) {
                std::string remainder = file.lines[currentLine].substr(cursorPos);
                file.lines[currentLine] = file.lines[currentLine].substr(0, cursorPos);
                file.lines.insert(file.lines.begin() + currentLine + 1, remainder);
                currentLine++;
                cursorPos = 0;
            }
        }
        else {
            file.lines[currentLine].insert(cursorPos, 1, get[i]);
            std::string str(1, get[i]);
            const char* cstr = str.c_str();
            cursorPos += strlen(cstr);
        }
    }
}

void showTree(fs::path path, vector<button>& file) {
    file.clear();
    folderbuttons.clear();
    fs::path absolutePath = filesystem::absolute(path);
    if (fs::exists(path) && fs::is_directory(path)) {
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_directory(entry.path())) {
                    string str = "[Folder] " + entry.path().filename().string();
                    button treebutton;
                    treebutton.name = str;
                    //treebutton.textColor = dark ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 0, 0, 0, 255 };
                    //treebutton.hover =  SDL_Color{ 100, 100, 100, 255 };
                    treebutton.path = fs::absolute(entry.path());
                    folderbuttons.push_back(treebutton);



                }
                else {
                    string str = "[File] " + entry.path().filename().string();
                    button treebutton;
                    treebutton.name = str;
                    //treebutton.textColor = dark ? SDL_Color{ 255, 255, 255, 255 } : SDL_Color{ 0, 0, 0, 255 };
                    //treebutton.hover =  SDL_Color{ 100, 100, 100, 255 };
                    treebutton.path = fs::absolute(entry.path());
                    file.push_back(treebutton);
                }
            }
        }
        catch (const fs::filesystem_error& e) {
            cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }
}

string showpath(fs::path str1) {
    fs::path str = filesystem::absolute(str1);
    string out = str.filename().string();
    return out;
}

void importFile(fs::path p, CPP& f) {
    ifstream file(p);
    if (!file) {
        std::cerr << "Error: Could not open file " << p << std::endl;
    }
    else {
        f.title = p.filename().string();
        f.lines.clear();
        currentfile = p.filename().string();
        currentfolder = p.parent_path().string();
        f.saved = true;
        f.myfilepath = p;
        string line;
        while (getline(file, line)) {
            f.lines.push_back(line);
        }
        file.close();

    }
}


string openFileDialog() {
    string filePath = "";
    IFileDialog* pFileDialog = nullptr;

    // Initialize COM (required for Windows API dialogs)
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
        // Create a File Open Dialog
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
            // Show the dialog
            if (SUCCEEDED(pFileDialog->Show(NULL))) {
                IShellItem* pItem;
                if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        filePath = ConvertWideStringToString(pszFilePath);  // Convert wide string to std::string
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }
        CoUninitialize();
    }
    return filePath;  // Returns the selected file's path
}

std::string openFolderDialog() {
    std::string folderPath = "";
    IFileDialog* pFileDialog = nullptr;

    // Initialize COM (required for Windows API dialogs)
    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
        // Create a File Open Dialog
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog)))) {
            DWORD dwOptions;
            pFileDialog->GetOptions(&dwOptions);
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS);  // Set to folder selection mode

            // Show the dialog
            if (SUCCEEDED(pFileDialog->Show(NULL))) {
                IShellItem* pItem;
                if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                        folderPath = ConvertWideStringToString(pszFilePath);  // Convert wide string to std::string
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }
        CoUninitialize();
    }
    return folderPath;
}

void saveProject(CPP& file) {
    if (file.myfilepath.empty()) {
        file.myfilepath = openFolderDialog();
        getName(file.title);
        file.myfilepath += "\\" + file.title + ".cpp";
        currentfolder = file.myfilepath.parent_path().string();

    }
    file.saved = true;
    ofstream savedfile(file.myfilepath);
    if (savedfile) {
        for (string line : file.lines) {
            savedfile << line << "\n";
        }
    }

    savedfile.close();
}

void checkParentheses(const CPP& filename) {
    ifstream file(filename.myfilepath);
    if (!file) {
        cerr << "Error: Could not open file " << filename.title << endl;
        return;
    }

    string line;
    int lineNumber = 0;
    pair <int, int> par[3];
    // <open, close>
    for (int i = 0; i < 3; i++) {
        par[i].first = 0;
        par[i].second = 0;
    }
    char signsOpen[3] = { '(', '[', '{' };
    char signsClose[3] = { ')', ']', '}' };
    string names[3] = { "parentheses", "brackets", "braces" };
    while (getline(file, line)) {
        lineNumber++;
        // Ignore empty lines and comments
        if (line.empty() || line.find("//") == 0 || line.find("#") == 0)
            continue;
        for (auto c : line) {
            for (int i = 0; i < 3; i++) {
                if (c == signsOpen[i]) {
                    par[i].first++;
                }
                if (c == signsClose[i]) {
                    par[i].second++;
                    if (par[i].second > par[i].first) {
                        Errors.push_back("Warning: Closed " + names[i] + " before opening them at line " + to_string(lineNumber));
                    }
                }
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        if (par[i].first > par[i].second) {
            Errors.push_back("Warning: Unclosed " + names[i]);
        }
    }
    file.close();
}


void checkMultiLineComments(const CPP& filename) {
    ifstream file(filename.myfilepath);
    if (!file) {
        cerr << "Error: Could not open file " << filename.title << endl;
        return;
    }
    string line;
    int lineNumber = 0;
    bool comment = false;
    while (getline(file, line)) {
        lineNumber++;
        if (!line.length())
            continue;
        for (int i = 0; i < line.length() - 1; i++) {
            if (line[i] == '/' && line[i + 1] == '*') {
                comment = true;
                i++;
            }
            if (line[i] == '*' && line[i + 1] == '/') {
                if (comment == true)
                    comment = false;
                else
                    Errors.push_back("Warning: Closed a multi-line comment before opening it at line " + to_string(lineNumber));
                i++;
            }
        }
    }
    if (comment) {
        Errors.push_back("Warning: Unclosed multi-line comment");
    }
    file.close();
}

void checkSemicolons(const CPP& filename) {
    ifstream file(filename.myfilepath);
    if (!file) {
        cerr << "Error: Could not open file " << filename.title << endl;
        return;
    }

    string line;
    int lineNumber = 0;
    regex statementRegex(R"(^\s*[^#\s].*[^;]\s*$)"); // Lines that are not comments, preprocessor directives, or empty

    while (getline(file, line)) {
        lineNumber++;

        // Ignore empty lines and comments
        if (line.empty() || line.find("//") == 0 || line.find("#") == 0) continue;

        // Check if the line should have a semicolon
        if (regex_match(line, statementRegex)) {
            Errors.push_back("Warning: Possible missing semicolon at line " + to_string(lineNumber));
        }
    }

    file.close();
}
