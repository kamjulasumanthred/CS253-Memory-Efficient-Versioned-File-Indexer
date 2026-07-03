#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <stdexcept>
#include <string>

using namespace std;

// ============================================================
// Template Class for Formatting and Displaying Query Results
// ============================================================
template <typename T>
class QueryResult {
private:
    T value;
public:
    QueryResult(const T& val) : value(val) {}
    
    void print() const {
        cout << value << endl;
    }
};

// Explicit Template Specialization for Top-K Results
template <>
class QueryResult<vector<pair<string, int>>> {
private:
    vector<pair<string, int>> value;
public:
    QueryResult(const vector<pair<string, int>>& val) : value(val) {}
    
    void print() const {
        for (const auto& p : value) {
            cout << p.first << " " << p.second << endl;
        }
    }
};

// ============================================================
// Tokenizer Class
// ============================================================
class Tokenizer {
public:
    // Extracts completed tokens from the chunk and updates the leftover.
    static vector<string> tokenizeChunk(const string& chunk, string& leftover, bool isEOF) {
        string combined = leftover + chunk;
        leftover.clear();
        
        vector<string> completed;
        string currentWord;
        
        for (char c : combined) {
            if (isalnum(static_cast<unsigned char>(c))) {
                currentWord += tolower(static_cast<unsigned char>(c));
            } else {
                if (!currentWord.empty()) {
                    completed.push_back(currentWord);
                    currentWord.clear();
                }
            }
        }
        
        if (!currentWord.empty()) {
            if (isEOF) {
                completed.push_back(currentWord);
            } else {
                leftover = currentWord;
            }
        }
        
        return completed;
    }
};

// ============================================================
// Versioned Indexer Class
// ============================================================
class VersionedIndexer {
private:
    // Map from version name to a map of words and their frequencies
    unordered_map<string, unordered_map<string, int>> versions;

public:
    // Function Overloading 1: Increments word frequency by 1
    void addWord(const string& version, const string& word) {
        versions[version][word]++;
    }

    // Function Overloading 2: Increments word frequency by a specific count
    void addWord(const string& version, const string& word, int count) {
        versions[version][word] += count;
    }

    int wordCount(const string& version, const string& word) const {
        auto it_v = versions.find(version);
        if (it_v == versions.end()) return 0;
        
        // Ensure word query is case-insensitive
        string lowerWord = word;
        for (char& c : lowerWord) c = tolower(static_cast<unsigned char>(c));
        
        auto it_w = it_v->second.find(lowerWord);
        if (it_w == it_v->second.end()) return 0;
        
        return it_w->second;
    }

    int diff(const string& v1, const string& v2, const string& word) const {
        return wordCount(v2, word) - wordCount(v1, word);
    }

    vector<pair<string, int>> topK(const string& version, size_t k) const {
        auto it = versions.find(version);
        if (it == versions.end()) return {};

        vector<pair<string, int>> vec(it->second.begin(), it->second.end());

        // Sort in descending order of frequency, alphabetical order as secondary
        sort(vec.begin(), vec.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
            if (a.second != b.second) {
                return a.second > b.second;
            }
            return a.first < b.first;
        });

        if (vec.size() > k) {
            vec.resize(k);
        }
        return vec;
    }
};

// ============================================================
// Buffered File Reader Class
// ============================================================
class BufferedFileReader {
private:
    string filepath;
    size_t bufferSize;

public:
    BufferedFileReader(const string& path, size_t size) : filepath(path), bufferSize(size) {}

    void parseAndIndex(const string& version, VersionedIndexer& indexer) {
        ifstream file(filepath, ios::binary);
        if (!file) {
            throw runtime_error("Cannot open file: " + filepath);
        }

        vector<char> buffer(bufferSize);
        string leftover;

        while (true) {
            file.read(buffer.data(), bufferSize);
            streamsize bytesRead = file.gcount();
            if (bytesRead <= 0) {
                break;
            }

            bool isEOF = file.eof() || (file.peek() == EOF);
            string chunk(buffer.data(), bytesRead);
            vector<string> words = Tokenizer::tokenizeChunk(chunk, leftover, isEOF);
            for (const auto& word : words) {
                indexer.addWord(version, word);
            }
        }

        if (!leftover.empty()) {
            indexer.addWord(version, leftover);
        }
    }
};

// ============================================================
// Abstract Query Class (Inheritance & Polymorphism)
// ============================================================
class Query {
public:
    virtual ~Query() {}
    virtual void execute(const VersionedIndexer& indexer) = 0;
};

// Subclass: Word Count Query
class WordQuery : public Query {
private:
    string version;
    string word;

public:
    WordQuery(const string& v, const string& w) : version(v), word(w) {}

    void execute(const VersionedIndexer& indexer) override {
        int count = indexer.wordCount(version, word);
        QueryResult<int> res(count);
        
        cout << "Version: " << version << endl;
        cout << "Word Count (" << word << "): ";
        res.print();
    }
};

// Subclass: Diff Query
class DiffQuery : public Query {
private:
    string v1;
    string v2;
    string word;

public:
    DiffQuery(const string& a, const string& b, const string& w) : v1(a), v2(b), word(w) {}

    void execute(const VersionedIndexer& indexer) override {
        int difference = indexer.diff(v1, v2, word);
        QueryResult<int> res(difference);
        
        cout << "Versions: " << v1 << " vs " << v2 << endl;
        cout << "Difference for " << word << ": ";
        res.print();
    }
};

// Subclass: Top-K Query
class TopKQuery : public Query {
private:
    string version;
    int k;

public:
    TopKQuery(const string& v, int kvalue) : version(v), k(kvalue) {}

    void execute(const VersionedIndexer& indexer) override {
        auto result = indexer.topK(version, k);
        QueryResult<vector<pair<string, int>>> res(result);
        
        cout << "Version: " << version << endl;
        cout << "Top " << k << " words:" << endl;
        res.print();
    }
};

// ============================================================
// Argument parsing structures
// ============================================================
struct Args {
    string file;
    string file1;
    string file2;
    string version;
    string version1;
    string version2;
    int bufferKB = 512;
    string query;
    string word;
    int topK = -1;
};

Args parseArgs(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--file" && i + 1 < argc) {
            args.file = argv[++i];
        } else if (arg == "--file1" && i + 1 < argc) {
            args.file1 = argv[++i];
        } else if (arg == "--file2" && i + 1 < argc) {
            args.file2 = argv[++i];
        } else if (arg == "--version" && i + 1 < argc) {
            args.version = argv[++i];
        } else if (arg == "--version1" && i + 1 < argc) {
            args.version1 = argv[++i];
        } else if (arg == "--version2" && i + 1 < argc) {
            args.version2 = argv[++i];
        } else if (arg == "--buffer" && i + 1 < argc) {
            try {
                args.bufferKB = stoi(argv[++i]);
            } catch (...) {
                throw runtime_error("Invalid buffer size specified. Must be an integer.");
            }
        } else if (arg == "--query" && i + 1 < argc) {
            args.query = argv[++i];
        } else if (arg == "--word" && i + 1 < argc) {
            args.word = argv[++i];
        } else if (arg == "--top" && i + 1 < argc) {
            try {
                args.topK = stoi(argv[++i]);
            } catch (...) {
                throw runtime_error("Invalid Top-K count specified. Must be an integer.");
            }
        }
    }
    return args;
}

// ============================================================
// Main Execution
// ============================================================
int main(int argc, char* argv[]) {
    try {
        auto start = chrono::high_resolution_clock::now();

        Args args = parseArgs(argc, argv);

        // Validation: Buffer Size Limits
        if (args.bufferKB < 256 || args.bufferKB > 1024) {
            throw runtime_error("Buffer size must be between 256 KB and 1024 KB (inclusive).");
        }

        VersionedIndexer indexer;

        // Perform loading and indexing based on query type
        if (args.query == "word" || args.query == "top") {
            if (args.file.empty()) {
                throw runtime_error("Missing parameter: --file is required for single-version queries.");
            }
            if (args.version.empty()) {
                throw runtime_error("Missing parameter: --version is required for single-version queries.");
            }
            
            BufferedFileReader reader(args.file, args.bufferKB * 1024);
            reader.parseAndIndex(args.version, indexer);
        } else if (args.query == "diff") {
            if (args.file1.empty() || args.file2.empty()) {
                throw runtime_error("Missing parameter: --file1 and --file2 are required for diff queries.");
            }
            if (args.version1.empty() || args.version2.empty()) {
                throw runtime_error("Missing parameter: --version1 and --version2 are required for diff queries.");
            }
            
            BufferedFileReader reader1(args.file1, args.bufferKB * 1024);
            BufferedFileReader reader2(args.file2, args.bufferKB * 1024);
            
            reader1.parseAndIndex(args.version1, indexer);
            reader2.parseAndIndex(args.version2, indexer);
        } else if (args.query.empty()) {
            throw runtime_error("Missing parameter: --query is required.");
        } else {
            throw runtime_error("Unknown query type: '" + args.query + "'. Use 'word', 'diff', or 'top'.");
        }

        // Initialize and execute the query polymorphically
        Query* q = nullptr;
        if (args.query == "word") {
            if (args.word.empty()) {
                throw runtime_error("Missing parameter: --word is required for word count query.");
            }
            q = new WordQuery(args.version, args.word);
        } else if (args.query == "diff") {
            if (args.word.empty()) {
                throw runtime_error("Missing parameter: --word is required for diff query.");
            }
            q = new DiffQuery(args.version1, args.version2, args.word);
        } else if (args.query == "top") {
            if (args.topK <= 0) {
                throw runtime_error("Missing or invalid parameter: --top must be a positive integer.");
            }
            q = new TopKQuery(args.version, args.topK);
        }

        if (q) {
            q->execute(indexer);
            delete q;
        }

        auto end = chrono::high_resolution_clock::now();
        double execTime = chrono::duration<double>(end - start).count();

        cout << "Allocated Buffer Size: " << args.bufferKB << " KB" << endl;
        cout << "Execution Time: " << execTime << " seconds" << endl;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
