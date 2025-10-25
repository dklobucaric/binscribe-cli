#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

// -----------------------------------------------------------------------------
// BinScribe CLI v0.2
// Minimal, single-file, cross-platform, zero dependencies.
// Now with interactive mode.
// -----------------------------------------------------------------------------

static const std::string VERSION  = "0.2";
static const std::string APPNAME  = "BinScribe CLI";
static const std::string COPYRIGHT =
    "© 2025 Dalibor Klobučarić\nLicense: MIT\n";

// ---------- Conversion helpers ------------------------------------------------

// Convert one byte (unsigned char) to 8-bit "01010101"
std::string byteToBinary(unsigned char c) {
    std::string out;
    out.reserve(8);
    for (int i = 7; i >= 0; --i) {
        out.push_back((c & (1 << i)) ? '1' : '0');
    }
    return out;
}

// Convert full text to space-separated 8-bit binary tokens
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

// Convert "01000001" -> char
// returns {success, char}
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

// Convert binary tokens (space/newline separated 8-bit groups) to text
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

// ---------- File helpers ------------------------------------------------------

bool readFile(const std::string& path, std::string& outData) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    outData = buffer.str();
    return true;
}

bool writeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    return true;
}

// ---------- Presentation / UX helpers ----------------------------------------

void printAbout() {
    std::cout
        << APPNAME << " v" << VERSION << "\n"
        << "Lightweight cross-platform CLI utility that converts text <-> binary (0s and 1s).\n"
        << "No external dependencies. Single-file build.\n"
        << COPYRIGHT;
}

void printUsage() {
    std::cout
        << APPNAME << " v" << VERSION << "\n\n"
        << "Usage:\n"
        << "  binscribe-cli --about\n"
        << "  binscribe-cli --encode <input.txt> <output.bin>\n"
        << "  binscribe-cli --decode <input.bin> <output.txt>\n"
        << "  binscribe-cli            (interactive mode)\n\n"
        << "Description:\n"
        << "  --about    Show version and credits\n"
        << "  --encode   Read plain text and write binary (space-separated 8-bit chunks)\n"
        << "  --decode   Read 0/1 chunks and write plain text\n"
        << "  no args    Start interactive menu\n";
}

// We ask user for a line like "path/to/file" safely
std::string askPath(const std::string& prompt) {
    std::string p;
    std::cout << prompt;
    std::getline(std::cin, p);
    return p;
}

// ---------- Core actions ------------------------------------------------------

bool doEncodeFile(const std::string& inPath, const std::string& outPath) {
    std::string plain;
    if (!readFile(inPath, plain)) {
        std::cerr << "[ERROR] Cannot read input file: " << inPath << "\n";
        return false;
    }

    std::string binData = textToBinary(plain);

    if (!writeFile(outPath, binData)) {
        std::cerr << "[ERROR] Cannot write output file: " << outPath << "\n";
        return false;
    }

    std::cout << "[OK] Encoded " << inPath << " -> " << outPath << "\n";
    return true;
}

bool doDecodeFile(const std::string& inPath, const std::string& outPath) {
    std::string binData;
    if (!readFile(inPath, binData)) {
        std::cerr << "[ERROR] Cannot read input file: " << inPath << "\n";
        return false;
    }

    bool ok = false;
    std::string plain = binaryToText(binData, ok);
    if (!ok) {
        std::cerr << "[ERROR] Input is not valid 8-bit binary chunks.\n";
        return false;
    }

    if (!writeFile(outPath, plain)) {
        std::cerr << "[ERROR] Cannot write output file: " << outPath << "\n";
        return false;
    }

    std::cout << "[OK] Decoded " << inPath << " -> " << outPath << "\n";
    return true;
}

// ---------- Interactive mode --------------------------------------------------

void runInteractive() {
    while (true) {
        std::cout << "\n"
            << "=====================================\n"
            << APPNAME << " v" << VERSION << "\n"
            << "1) Encode file (text -> binary)\n"
            << "2) Decode file (binary -> text)\n"
            << "3) About\n"
            << "0) Exit\n"
            << "-------------------------------------\n"
            << "Choice: ";

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            // input stream closed or error
            std::cout << "\n[INFO] Input closed. Exiting.\n";
            return;
        }

        if (choice == "0") {
            std::cout << "Goodbye.\n";
            return;
        }
        else if (choice == "1") {
            std::string inPath  = askPath("Input text file : ");
            std::string outPath = askPath("Output binary file: ");
            doEncodeFile(inPath, outPath);
        }
        else if (choice == "2") {
            std::string inPath  = askPath("Input binary file : ");
            std::string outPath = askPath("Output text file   : ");
            doDecodeFile(inPath, outPath);
        }
        else if (choice == "3") {
            printAbout();
        }
        else {
            std::cout << "[WARN] Invalid choice.\n";
        }
    }
}

// ---------- main() -----------------------------------------------------------

int main(int argc, char* argv[]) {
    // no args -> interactive mode
    if (argc < 2) {
        runInteractive();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "--about") {
        printAbout();
        return 0;
    }

    if (cmd == "--encode") {
        if (argc < 4) {
            std::cerr << "[ERROR] Missing arguments.\n\n";
            printUsage();
            return 1;
        }
        std::string inPath = argv[2];
        std::string outPath = argv[3];
        return doEncodeFile(inPath, outPath) ? 0 : 1;
    }

    if (cmd == "--decode") {
        if (argc < 4) {
            std::cerr << "[ERROR] Missing arguments.\n\n";
            printUsage();
            return 1;
        }
        std::string inPath = argv[2];
        std::string outPath = argv[3];
        return doDecodeFile(inPath, outPath) ? 0 : 1;
    }

    // unknown arg
    std::cerr << "[ERROR] Unknown command: " << cmd << "\n\n";
    printUsage();
    return 1;
}
