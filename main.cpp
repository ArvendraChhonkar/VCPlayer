#include "App.h"
#include "FileDialog.h"



//entry Point
int main(){

    App app;
    if(!app.init()) return 1;
    app.run();
    return 0;
}