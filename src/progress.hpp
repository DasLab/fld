#ifndef FLD_PROGRESS_HPP
#define FLD_PROGRESS_HPP

#include <ostream>

// The stream that progress messages go to. Standard output unless silenced.
std::ostream& progress();

// Discards all further progress messages.
void silence_progress();

#endif
