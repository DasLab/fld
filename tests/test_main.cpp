#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.hpp"
#include "progress.hpp"

int main(int argc, char** argv) {
    silence_progress();
    return doctest::Context(argc, argv).run();
}
