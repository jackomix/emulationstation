#include <iostream>
#include <algorithm>
#include <cmath>
#include <string>

// Mock of Renderer/Math/Font
struct Renderer {
    static float getScreenWidth() { return 640.0f; }
    static float getScreenHeight() { return 480.0f; }
};

struct Math {
    static float min(float a, float b) { return std::min(a, b); }
    static float max(float a, float b) { return std::max(a, b); }
};

// Based on the user's instructions
const float FONT_SIZE_LARGE = 41.0f;
const float FONT_SIZE_SMALL = 16.0f; 

#define TITLE_VERT_PADDING (Renderer::getScreenHeight()*0.0637f)
#define TITLE_WITHSUB_VERT_PADDING (Renderer::getScreenHeight()*0.05f)
#define SUBTITLE_VERT_PADDING (Renderer::getScreenHeight()*0.019f)
#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.90f)

// Mocking the macro behavior
float getTitleHeight(bool hasSubtitle, float subtitleSizeY) {
    return FONT_SIZE_LARGE + 
           (hasSubtitle ? TITLE_WITHSUB_VERT_PADDING : TITLE_VERT_PADDING) + 
           (hasSubtitle ? subtitleSizeY + SUBTITLE_VERT_PADDING : 0);
}

int main() {
    // 1. Initial size of MenuComponent (from centerWindow)
    float menuSizeX = WINDOW_WIDTH;
    float menuSizeY = Renderer::getScreenHeight() * 0.901f;

    // 2. setTitleImage is called when mSubtitle is nullptr!
    float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
    
    // Evaluate TITLE_HEIGHT when mSubtitle is nullptr
    float oldTitleHeight = getTitleHeight(false, 0);
    float iw = oldTitleHeight / width;

    float col0_perc = 1.0f - iw;
    float col1_perc = iw;

    // 3. updateTab() -> setSubTitle() is called with 8 lines of text
    // 8 lines * 16px * 1.1 line spacing
    float subtitleSizeY = 8.0f * FONT_SIZE_SMALL * 1.1f;
    
    // Evaluate new TITLE_HEIGHT with the 8-line subtitle
    float newTitleHeight = getTitleHeight(true, subtitleSizeY);
    
    float titleHeight = FONT_SIZE_LARGE + TITLE_WITHSUB_VERT_PADDING;
    float row0_perc = titleHeight / newTitleHeight;
    float row1_perc = 1.0f - row0_perc;

    // Now compute actual pixel bounds on the mHeaderGrid
    // Note: mHeaderGrid spans the entire width of mMenu and its height is exactly newTitleHeight
    float gridW = menuSizeX;
    float gridH = newTitleHeight;

    // Title text is in Col 0, Row 0
    float titleX = 0;
    float titleY = 0;
    float titleW = gridW * col0_perc;
    float titleH = gridH * row0_perc;

    // Subtitle text is in Col 0, Row 1
    float subtitleX = 0;
    float subtitleY = titleH;
    float subtitleW = gridW * col0_perc;
    float subtitleH = gridH * row1_perc;

    // Title image (Box art) is in Col 1, spans Row 0 and Row 1
    float boxartX = titleW;
    float boxartY = 0;
    float boxartW = gridW * col1_perc;
    float boxartH = gridH; // Spans both rows

    std::cout << "--- EmulationStation Layout Math Proof ---" << std::endl;
    std::cout << "WINDOW_WIDTH: " << menuSizeX << std::endl;
    std::cout << "Initial TITLE_HEIGHT (no subtitle): " << oldTitleHeight << std::endl;
    std::cout << "New TITLE_HEIGHT (8-line subtitle): " << newTitleHeight << std::endl;
    
    std::cout << "\nHeader Grid Percentages:" << std::endl;
    std::cout << "Col 0 (Title col): " << (col0_perc * 100.0f) << "%" << std::endl;
    std::cout << "Col 1 (Boxart col): " << (col1_perc * 100.0f) << "%" << std::endl;
    
    std::cout << "\nExact Pixel Bounds:" << std::endl;
    std::cout << "Title Bound:    X=" << titleX << ", Y=" << titleY << ", W=" << titleW << ", H=" << titleH << std::endl;
    std::cout << "Subtitle Bound: X=" << subtitleX << ", Y=" << subtitleY << ", W=" << subtitleW << ", H=" << subtitleH << std::endl;
    std::cout << "Box Art Bound:  X=" << boxartX << ", Y=" << boxartY << ", W=" << boxartW << ", H=" << boxartH << std::endl;

    return 0;
}
