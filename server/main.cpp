#include <uvw.hpp>

int main() {
    auto defaultLoop = uvw::loop::get_default();
    
    defaultLoop->run();
    return 0;
}