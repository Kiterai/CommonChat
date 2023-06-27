#include <uvw.hpp>

class Communicate
{
private:
    std::shared_ptr<uvw::loop> defaultLoop;
public:
    Communicate();
    ~Communicate();

    void run();
};
