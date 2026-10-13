#ifndef FLD_PREDICT_HPP
#define FLD_PREDICT_HPP

#include <string>
#include <vector>

bool rn_coverage_on_path();

// Predicts the read count of each sequence with rn-coverage. Sequences are predicted in groups
// of equal length. Empty sequences get zero reads. Intermediate files go into work_dir under
// names that start with the label.
std::vector<double> predict_reads(
    const std::vector<std::string>& sequences,
    const std::string& work_dir,
    const std::string& label
);

#endif
