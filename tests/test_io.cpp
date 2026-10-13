#include "doctest.hpp"
#include "helpers.hpp"
#include "csv.hpp"
#include "fasta.hpp"

TEST_CASE("csv fields round trip through quoting") {
    std::vector<std::string> fields = {"plain", "with,comma", "with \"quote\"", "", "a\nb"};
    std::vector<std::string> parsed = parse_csv_line(csv_line(fields));
    CHECK(parsed == fields);
}

TEST_CASE("csv_field leaves plain values alone") {
    CHECK(csv_field("ACGT") == "ACGT");
    CHECK(csv_field("a,b") == "\"a,b\"");
}

TEST_CASE("read_fasta joins lines and strips carriage returns") {
    TempDir dir;
    std::string path = dir.file("in.fasta");
    {
        std::ofstream file(path);
        file << ">one\r\nACGT\r\nACGT\r\n\n>two\nGGGG\n";
    }
    std::vector<FastaEntry> entries = read_fasta(path);
    REQUIRE(entries.size() == 2);
    CHECK(entries[0].name == "one");
    CHECK(entries[0].sequence == "ACGTACGT");
    CHECK(entries[1].name == "two");
    CHECK(entries[1].sequence == "GGGG");
}

TEST_CASE("read_fasta rejects empty entries and headerless data") {
    TempDir dir;
    std::string path = dir.file("in.fasta");
    write_fasta_file(path, {{"one", "ACGT"}, {"two", ""}});
    CHECK_THROWS(read_fasta(path));
    {
        std::ofstream file(path);
        file << "ACGT\n";
    }
    CHECK_THROWS(read_fasta(path));
    CHECK_THROWS(read_fasta(dir.file("missing.fasta")));
}
