#include "Communicate.hpp"

Communicate::Communicate() : defaultLoop(uvw::loop::get_default())
{
}

Communicate::~Communicate()
{
}

void Communicate::run() {
    defaultLoop->run();
}
