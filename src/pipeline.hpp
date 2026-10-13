#ifndef FLD_PIPELINE_HPP
#define FLD_PIPELINE_HPP

#include <optional>
#include <string>
#include <vector>

#include "layout.hpp"
#include "stem.hpp"

// The order of the constructs in the output.
enum class SortOrder { None, Reads, Length };

extern const std::string SORT_NONE;
extern const std::string SORT_READS;
extern const std::string SORT_LENGTH;

// Parses one of the three sort names.
SortOrder parse_sort_order(const std::string& text);

// Returns the three sort names as "none, reads, or length".
std::string sort_order_names();

struct PipelineOptions {
    std::vector<std::string> inputs;
    std::string output_dir;
    bool overwrite = false;
    Layout layout;
    size_t construct_length = 0;
    StemLimits stem;
    bool m2 = false;
    bool predict = true;
    SortOrder sort = SortOrder::None;
    std::optional<size_t> seed;
};

const std::string LIBRARY_CSV = "library.csv";
const std::string LIBRARY_FASTA = "library.fasta";

// Designs the library and writes library.csv and library.fasta into the output directory.
void run_pipeline(const PipelineOptions& options);

#endif
