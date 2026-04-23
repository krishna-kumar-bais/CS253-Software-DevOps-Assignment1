#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <stdexcept>
#include <memory>

using namespace std;

/* =====================================================
   Template: Timer (TEMPLATE REQUIREMENT)
   ===================================================== */
template <typename Clock>
class Timer {
    typename Clock::time_point startTime;
public:
    void start() { startTime = Clock::now(); }
    double stop() const {
        return chrono::duration<double>(Clock::now() - startTime).count();
    }
};

/* =====================================================
   FileStreamReader
   ===================================================== */
class FileStreamReader {
    ifstream file;
    size_t bufferSize;

public:
    FileStreamReader(const string& path, size_t bufSize)
        : bufferSize(bufSize) {
        file.open(path, ios::binary);
        if (!file)
            throw runtime_error("Cannot open file: " + path);
    }

    bool readNext(char* buffer, size_t& bytesRead) {
        file.read(buffer, bufferSize);
        bytesRead = file.gcount();
        return bytesRead > 0;
    }
};

/* =====================================================
   Tokenizer (handles split tokens)
   ===================================================== */
class Tokenizer {
    string carry;

    static bool isWordChar(char c) {
        return isalnum(static_cast<unsigned char>(c));
    }

public:
    vector<string> tokenize(const char* data, size_t len) {
        vector<string> tokens;
        string current = carry;

        for (size_t i = 0; i < len; i++) {
            char c = tolower(static_cast<unsigned char>(data[i]));
            if (isWordChar(c)) current.push_back(c);
            else {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
        }
        carry = current;
        return tokens;
    }

    void flush(vector<string>& tokens) {
        if (!carry.empty()) {
            tokens.push_back(carry);
            carry.clear();
        }
    }
};

/* =====================================================
   VersionedIndex
   ===================================================== */
class VersionedIndex {
    unordered_map<string, size_t> freq;

public:
    void addTokens(const vector<string>& tokens) {
        for (const auto& t : tokens) freq[t]++;
    }

    /* FUNCTION OVERLOADING */
    size_t count(const string& word) const {
        auto it = freq.find(word);
        return (it == freq.end()) ? 0 : it->second;
    }

    size_t count(const string& word, bool /*dummy*/) const {
        return count(word);   // overloaded variant
    }

    vector<pair<string, size_t>> topK(size_t k) const {
        vector<pair<string, size_t>> v(freq.begin(), freq.end());
        sort(v.begin(), v.end(),
             [](auto& a, auto& b) { return a.second > b.second; });
        if (v.size() > k) v.resize(k);
        return v;
    }
};

/* =====================================================
   QueryEngine
   ===================================================== */
class QueryEngine {
    unordered_map<string, VersionedIndex> versions;

public:
    void addVersion(const string& name, VersionedIndex&& idx) {
        versions.emplace(name, std::move(idx));
    }

    const VersionedIndex& get(const string& v) const {
        auto it = versions.find(v);
        if (it == versions.end())
            throw runtime_error("Version not found: " + v);
        return it->second;
    }
};

/* =====================================================
   Abstract Base Class: Query (INHERITANCE)
   ===================================================== */
class Query {
public:
    virtual void execute(const QueryEngine&) const = 0;
    virtual string description() const = 0;
    virtual ~Query() = default;
};

/* =====================================================
   WordQuery
   ===================================================== */
class WordQuery : public Query {
    string version, word;
public:
    WordQuery(string v, string w) : version(v), word(w) {}
    void execute(const QueryEngine& engine) const override {
        cout << "Version: " << version << "\n";
        cout << "Count: " << engine.get(version).count(word) << "\n";
    }
    string description() const override { return "Word Count Query"; }
};

/* =====================================================
   DiffQuery (TWO FILES)
   ===================================================== */
class DiffQuery : public Query {
    string word, v1, v2;
public:
    DiffQuery(string w, string a, string b)
        : word(w), v1(a), v2(b) {}
    void execute(const QueryEngine& engine) const override {
        long diff =
            static_cast<long>(engine.get(v2).count(word)) -
            static_cast<long>(engine.get(v1).count(word));
        cout << "Difference (" << v2 << " - " << v1 << "): " << diff << "\n";
    }
    string description() const override { return "Difference Query"; }
};

/* =====================================================
   TopKQuery
   ===================================================== */
class TopKQuery : public Query {
    string version;
    size_t k;
public:
    TopKQuery(string v, size_t k) : version(v), k(k) {}
    void execute(const QueryEngine& engine) const override {
        cout << "Top-" << k << " words in version " << version << ":\n";
        for (auto& p : engine.get(version).topK(k))
            cout << p.first << " " << p.second << "\n";
    }
    string description() const override { return "Top-K Query"; }
};

/* =====================================================
   Build index helper
   ===================================================== */
VersionedIndex buildIndex(const string& file, size_t bufferSize) {
    FileStreamReader reader(file, bufferSize);
    Tokenizer tokenizer;
    VersionedIndex index;

    vector<char> buffer(bufferSize);
    size_t bytesRead;

    while (reader.readNext(buffer.data(), bytesRead)) {
        index.addTokens(tokenizer.tokenize(buffer.data(), bytesRead));
    }

    vector<string> tail;
    tokenizer.flush(tail);
    index.addTokens(tail);

    return index;
}

string toLower(string s) {
    for (char &c : s) {
        c = tolower(static_cast<unsigned char>(c));
    }
    return s;
}

/* =====================================================
   MAIN
   ===================================================== */
int main(int argc, char* argv[]) {
    try {
        string file, file1, file2;
        string version, version1, version2;
        string queryType, word;
        size_t bufferKB = 0, topK = 0;

        for (int i = 1; i < argc; i++) {
            string a = argv[i];
            if (a == "--file") file = argv[++i];
            else if (a == "--file1") file1 = argv[++i];
            else if (a == "--file2") file2 = argv[++i];
            else if (a == "--version") version = argv[++i];
            else if (a == "--version1") version1 = argv[++i];
            else if (a == "--version2") version2 = argv[++i];
            else if (a == "--buffer") bufferKB = stoul(argv[++i]);
            else if (a == "--query") queryType = argv[++i];
            else if (a == "--word") word = argv[++i];
            else if (a == "--top") topK = stoul(argv[++i]);
        }

        if (!word.empty()) {
            word = toLower(word);
        }
        if (bufferKB < 256 || bufferKB > 1024)
            throw runtime_error("Buffer size must be 256–1024 KB");

        size_t bufferSize = bufferKB * 1024;
        QueryEngine engine;
        unique_ptr<Query> query;

        Timer<chrono::high_resolution_clock> timer;
        timer.start();

        if (queryType == "word") {
            engine.addVersion(version, buildIndex(file, bufferSize));
            query = make_unique<WordQuery>(version, word);
        }
        else if (queryType == "top") {
            engine.addVersion(version, buildIndex(file, bufferSize));
            query = make_unique<TopKQuery>(version, topK);
        }
        else if (queryType == "diff") {
            engine.addVersion(version1, buildIndex(file1, bufferSize));
            engine.addVersion(version2, buildIndex(file2, bufferSize));
            query = make_unique<DiffQuery>(word, version1, version2);
        }
        else {
            throw runtime_error("Invalid query type");
        }

        query->execute(engine);

        double timeTaken = timer.stop();

        cout << "Buffer Size (KB): " << bufferKB << "\n";
        cout << "Execution Time (s): " << timeTaken << "\n";
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}


// g++ -O2 reference_checker.cpp -o analyzer

// ./analyzer \
  --file test_logs.txt \
  --version v1 \
  --buffer 512 \
  --query word \
  --word error

//   ./analyzer \
  --file test_logs.txt \
  --version v1 \
  --buffer 512 \
  --query top \
  --top 10

// ./analyzer \
  --file1 test_logs.txt \
  --version1 v1 \
  --file2 verbose_logs.txt \
  --version2 v2 \
  --buffer 512 \
  --query diff \
  --word error