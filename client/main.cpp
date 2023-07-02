#include <iostream>
#include <thread>
#include "Gui.hpp"
#include "communicate/Communicate.hpp"

int main() {
    std::thread commThread{[](){
        Communicate comm;
        comm.run();
    }};
    Gui gui;
    gui.mainloop();

    commThread.join();
    return 0;
}