#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

// -----------------------------------------------------------------------------
// BinScribe CLI v0.1
// Minimal, single-file, cross-platform, zero dependencies.
// -----------------------------------------------------------------------------

static const std::string VERSION = "0.1";
static const std::string APPNAME = "BinScribe CLI";

// Convert a single byte (unsigned char) to an 8-bit "01010101" string
std::string byteToBinary(unsigned char c) {
    std::string out;
    out.reserve(8);
    for (int i = 7; i >= 0; --i) {
        out.push_back((c & (1 << i)) ? '1' : '0');
    }
    return out;
}

// Convert full text into space-separated 8-bit binary chunks
std::string textToBinary(const std::string& input) {
    std::ostringstream oss;
    for (size_t i = 0; i < input.size(); ++i) {
        if (i > 0) {
            oss << " ";
        }
        unsigned char c = static_cast<unsigned char>(input[i]);
        oss << byteToBinary(c);
    }
    return oss.str();
}

// Helper: convert "01000001" -> one byte (as char)
// Returns pair<success?,charValue>
std::pair<bool, char> binary8ToChar(const std::string& bits) {
    if (bits.size() != 8) {
        return {false, 0};
    }
    int value = 0;
    for (char b : bits) {
        if (b != '0' && b != '1') {
            return {false, 0};
        }
        value = (value << 1) | (b == '1' ? 1 : 0);
    }
    return {true, static_cast<char>(value)};
}

// Convert space/newline separated 8-bit binary chunks into text
std::string binaryToText(const std::string& input, bool& ok) {
    ok = true;
    std::istringstream iss(input);
    std::ostringstream out;
    std::string chunk;

    while (iss >> chunk) {
        auto [success, ch] = binary8ToChar(chunk);
        if (!success) {
            ok = false;
            break;
        }
        out << ch;
    }
    return out.str();
}

// Read whole file into string (binary-safe-ish for text use)
bool readFile(const std::string& path, std::string& outData) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    outData = buffer.str();
    return true;
}

// Write whole string to file
bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    return true;
}

void printAbout() {
    std::cout
        << APPNAME << " v" << VERSION << "\n"
        << "Lightweight cross-platform CLI utility that converts text <-> binary (0s and 1s).\n"
        << "No external dependencies. Single-file build.\n"
        << "© 2025 Dalibor Klobučarić\n"
        << "License: MIT\n";
}

void printUsage() {
    std::cout
        << APPNAME << " v" << VERSION << "\n\n"
        << "Usage:\n"
        << "  binscribe-cli --about\n"
        << "  binscribe-cli --encode <input.txt> <output.bin>\n"
        << "  binscribe-cli --decode <input.bin> <output.txt>\n\n"
        << "Description:\n"
        << "  --about    Show version and credits\n"
        << "  --encode   Read plain text and write binary (space-separated 8-bit chunks)\n"
        << "  --decode   Read binary 0/1 chunks and write plain text\n";
}

int main(int argc, char* argv[]) {
    // No args -> show usage
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string cmd = argv[1];

    // binscribe-cli --about
    if (cmd == "--about") {
        printAbout();
        return 0;
    }

    // binscribe-cli --encode input.txt output.bin
    if (cmd == "--encode") {
        if (argc < 4) {
            std::cerr << "[ERROR] Missing arguments.\n\n";
            printUsage();
            return 1;
        }
        std::string inPath = argv[2];
        std::string outPath = argv[3];

        std::string plain;
        if (!readFile(inPath, plain)) {
            std::cerr << "[ERROR] Cannot read input file: " << inPath << "\n";
            return 1;
        }

        std::string binData = textToBinary(plain);

        if (!writeFile(outPath, binData)) {
            std::cerr << "[ERROR] Cannot write output file: " << outPath << "\n";
            return 1;
        }

        std::cout << "[OK] Encoded " << inPath << " -> " << outPath << "\n";
        return 0;
    }

    // binscribe-cli --decode input.bin output.txt
    if (cmd == "--decode") {
        if (argc < 4) {
            std::cerr << "[ERROR] Missing arguments.\n\n";
            printUsage();
            return 1;
        }
        std::string inPath = argv[2];
        std::string outPath = argv[3];

        std::string binData;
        if (!readFile(inPath, binData)) {
            std::cerr << "[ERROR] Cannot read input file: " << inPath << "\n";
            return 1;
        }

        bool ok = false;
        std::string plain = binaryToText(binData, ok);
        if (!ok) {
            std::cerr << "[ERROR] Input is not valid 8-bit binary chunks.\n";
            return 1;
        }

        if (!writeFile(outPath, plain)) {
            std::cerr << "[ERROR] Cannot write output file: " << outPath << "\n";
            return 1;
        }

        std::cout << "[OK] Decoded " << inPath << " -> " << outPath << "\n";
        return 0;
    }

    // Unknown command
    std::cerr << "[ERROR] Unknown command: " << cmd << "\n\n";
    printUsage();
    return 1;
}
