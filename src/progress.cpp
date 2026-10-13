#include "progress.hpp"

#include <iostream>

// A stream buffer that accepts and drops everything written to it.
class NullBuffer : public std::streambuf {
protected:
    int overflow(int c) override { return c; }
};

static NullBuffer null_buffer;
static std::ostream null_stream(&null_buffer);
static std::ostream* progress_stream = &std::cout;

std::ostream& progress() {
    return *progress_stream;
}

void silence_progress() {
    progress_stream = &null_stream;
}
