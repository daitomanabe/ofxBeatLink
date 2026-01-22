#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLWindowSettings settings;
    settings.setSize(1200, 600);
    settings.windowMode = OF_WINDOW;
    ofCreateWindow(settings);
    return ofRunApp(std::make_shared<ofApp>());
}
