#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <future>

enum class BlockSize {
  MB_16 = 16 * 1024 * 1024,
  MB_32 = 32 * 1024 * 1024,
  MB_64 = 64 * 1024 * 1024
};


using LocalWordMap = std::unordered_map<std::string, int>;
using InvertedIndex = std::unordered_map<std::string, std::vector<size_t>>;
using LocalInvertedIndex = std::unordered_map<std::string, std::unordered_set<size_t>>;

class wordcount
{
private:
  std::unordered_map<std::string, int> final_word_counts;
  InvertedIndex inverted_index;
  size_t block_size;
  unsigned int thread_count;
  
  std::vector<LocalWordMap> thread_local_maps;
  std::vector<LocalInvertedIndex> thread_local_indexes;
  
  void count_words_in_text_local(const std::string& text, LocalWordMap& local_map);
  void build_inverted_index_local(const std::string& text, LocalInvertedIndex& local_index, size_t block_id);
  std::vector<std::string> split_into_blocks(const std::string& content);
  void merge_local_maps();
  void merge_local_indexes();

public:
  wordcount(unsigned int num_threads = 1, BlockSize block_sz = BlockSize::MB_16);
  ~wordcount();
  
  void load_txt(const std::string& filename);
  void print_word_counts() const;
  size_t get_unique_word_count() const;
  size_t get_total_word_count() const;
  void clear_word_counts();
  
  std::vector<size_t> search_word(const std::string& word) const;
  void print_inverted_index() const;
  void print_search_results(const std::string& word) const;
  
  bool save_inverted_index(const std::string& index_file) const;
  bool load_inverted_index(const std::string& index_file);
  bool has_inverted_index() const;
};