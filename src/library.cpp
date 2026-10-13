#include "library.hpp"

#include <stdexcept>

#include "csv.hpp"
#include "fasta.hpp"
#include "files.hpp"

static const std::string INDEX_COLUMN = "index";
static const std::string NAME_COLUMN = "name";
static const std::string SUBLIBRARY_COLUMN = "sublibrary";
static const std::string SUBLIBRARY_FIELD = "sublibrary";
static const std::string BEGIN_FIELD = "begin";
static const std::string END_FIELD = "end";

std::string full_sequence(const Construct& construct) {
    std::string seq;
    for (const std::string& part : construct.parts) {
        seq += part;
    }
    return seq;
}

size_t design_length(const Layout& layout, const Construct& construct) {
    return construct.parts[design_slot(layout)].size();
}

size_t design_begin(const Layout& layout, const Construct& construct) {
    size_t offset = 0;
    for (size_t i = 0; i < design_slot(layout); i++) {
        offset += construct.parts[i].size();
    }
    return offset;
}

size_t design_end(const Layout& layout, const Construct& construct) {
    return design_begin(layout, construct) + design_length(layout, construct);
}

size_t padding_length(const Layout& layout, const Construct& construct, size_t construct_length) {
    size_t occupied = fixed_length(layout) + design_length(layout, construct);
    if (occupied > construct_length) {
        throw std::runtime_error(
            "Construct '" + construct.name + "' needs " + std::to_string(occupied) +
            " bases without padding, but the construct length is " +
            std::to_string(construct_length));
    }
    return construct_length - occupied;
}

static std::string header_field(const std::string& key, const std::string& value) {
    return " " + key + "=" + value;
}

std::string fasta_name(const Layout& layout, const Construct& construct) {
    return construct.name +
           header_field(SUBLIBRARY_FIELD, construct.sublibrary) +
           header_field(BEGIN_FIELD, std::to_string(design_begin(layout, construct))) +
           header_field(END_FIELD, std::to_string(design_end(layout, construct)));
}

std::vector<std::string> csv_header(const Library& library) {
    std::vector<std::string> header = {INDEX_COLUMN, NAME_COLUMN, SUBLIBRARY_COLUMN};
    for (const std::string& name : slot_names(library.layout)) {
        header.push_back(name);
    }
    for (const std::string& name : library.read_columns) {
        header.push_back(name);
    }
    return header;
}

std::vector<std::string> csv_row(const Construct& construct) {
    std::vector<std::string> row = {
        std::to_string(construct.index), construct.name, construct.sublibrary
    };
    for (const std::string& part : construct.parts) {
        row.push_back(part);
    }
    for (double reads : construct.reads) {
        row.push_back(std::to_string(reads));
    }
    return row;
}

void write_library_csv(const Library& library, const std::string& path) {
    std::ofstream file = open_output(path);
    file << csv_line(csv_header(library)) << "\n";
    for (const Construct& construct : library.constructs) {
        file << csv_line(csv_row(construct)) << "\n";
    }
}

void write_library_fasta(const Library& library, const std::string& path) {
    std::ofstream file = open_output(path);
    for (const Construct& construct : library.constructs) {
        write_fasta_entry(file, FastaEntry{fasta_name(library.layout, construct), full_sequence(construct)});
    }
}
