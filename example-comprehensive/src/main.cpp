#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLWindowSettings settings;
    settings.setSize(1920, 1080);
    settings.windowMode = OF_WINDOW;
    settings.title = "ofxBeatLink - Comprehensive Monitor";
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
