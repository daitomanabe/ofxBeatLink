#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLWindowSettings settings;
    settings.setSize(800, 500);
    settings.windowMode = OF_WINDOW;
    ofCreateWindow(settings);
    return ofRunApp(std::make_shared<ofApp>());
}
