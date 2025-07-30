#include "logger.hpp"
#include "service.hpp"

int main(int argc, char **argv)
{
    bool serviceMode = false;
    logger::init(serviceMode ? log_mode::service : log_mode::console);

    service service;
    service.start();
    return 0;
}