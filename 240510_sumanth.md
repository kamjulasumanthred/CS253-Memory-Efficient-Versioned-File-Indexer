# CS253 Assignment 1: Memory-Efficient Versioned File Indexer

**Name:** Kamjula Sumanth Reddy  
**Roll Number:** 240510  
**Course:** CS253 – Software Development and DevOps  
**Department:** Computer Science & Engineering, IIT Kanpur  
**Instructor:** Prof. Pritam Choudhury  

---

## 1. Project Description
This project implements a high-performance, memory-efficient, and version-controlled word-level text indexer in standard C++. Large text files (such as logs) are processed incrementally in chunks using a fixed-size buffer. 

The program supports multiple versions of documents loaded under distinct names in the same execution session and allows querying:
1. **Word Count Query:** Frequency of a word in a specific version.
2. **Difference Query:** The frequency difference of a word between two versions.
3. **Top-K Query:** The top-K most frequent words in a version, sorted in descending order.

---

## 2. Architectural Design & OOP Principles

The codebase adheres strictly to object-oriented design principles and implements the following requirements:

### User-Defined Classes
1. **`Tokenizer`**: Extracts completed alphanumeric words from text chunks. It handles edge cases where words are split across buffer boundaries.
2. **`BufferedFileReader`**: Opens a file stream and reads the file incrementally in chunks using a fixed-size buffer.
3. **`VersionedIndexer`**: Maintains a mapping from versions to word frequencies and offers utility functions for top-K, word-count, and diff operations.
4. **`Query` (Abstract Base Class)**: Defines the interface for polymorphic query execution.
   - `WordQuery` (Derived class)
   - `DiffQuery` (Derived class)
   - `TopKQuery` (Derived class)

### C++ Features Demonstrated
- **Inheritance & Runtime Polymorphism**: Achieved using the abstract base class `Query` and dynamic dispatch via the virtual function `execute()`.
- **Function Overloading**: The `VersionedIndexer` class implements overloaded methods:
  - `void addWord(const string& version, const string& word)`
  - `void addWord(const string& version, const string& word, int count)`
- **User-Defined Templates**: A generic `QueryResult<T>` template class is implemented to wrap query outcomes. 
  - An **Explicit Template Specialization** `QueryResult<vector<pair<string, int>>>` is implemented to specifically handle and format top-K frequency output lists.
- **Exception Handling**: Standard `try-catch` blocks catch runtime errors such as missing command-line arguments, division errors, invalid types, incorrect buffer sizes (must be $256 \text{ KB} \le \text{buffer} \le 1024 \text{ KB}$), and nonexistent files.
- **Memory Boundedness**: The file is streamed chunk-by-chunk and processed immediately. The memory usage is completely independent of the file size and is proportional only to the number of unique words in the text.

---

## 3. Boundary Token Handling

When reading files using a fixed-size buffer, a word can be split across two buffer chunks. For example, if a buffer ends with `er` and the next buffer starts with `ror` (representing the word `error`), a naive reader would process them as two distinct words.

**Our Algorithm:**
- The `Tokenizer::tokenizeChunk` function accepts a chunk, a `leftover` string, and an `isEOF` flag.
- The `leftover` from the previous chunk is prepended to the new chunk before tokenization.
- If the last character of the combined chunk is alphanumeric, the final token is deemed potentially incomplete. It is popped off the completed words list and saved in `leftover` for the next chunk.
- If it is the end of the file (`isEOF = true`), the `leftover` token is processed as a complete word.

---

## 4. Compilation & Execution

### Compilation
Compile the program using `g++` with C++17 support:
```bash
g++ -std=c++17 -O3 240510_sumanth.cpp -o analyzer
```

### Execution Command Examples

#### 1. Word Query (Single Version)
```bash
./analyzer --file test_logs.txt --version v1 --buffer 512 --query word --word error
```

#### 2. Top-K Query (Single Version)
```bash
./analyzer --file test_logs.txt --version v1 --buffer 512 --query top --top 10
```

#### 3. Difference Query (Two Versions)
```bash
./analyzer --file1 test_logs.txt --version1 v1 --file2 verbose_logs.txt --version2 v2 --buffer 512 --query diff --word error
```

---

## 5. Output Format
For every query run, the program displays:
1. Version name(s)
2. Query execution results (word frequency, difference, or sorted top-K list)
3. Allocated buffer size in KB
4. Total execution time in seconds (including indexing and query execution)
