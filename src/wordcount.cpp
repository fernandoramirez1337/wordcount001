#include "wordcount.hpp"

wordcount::wordcount(unsigned int num_threads, BlockSize block_sz) 
  : block_size(static_cast<size_t>(block_sz)), thread_count(num_threads)
{
  if (thread_count != 1 && thread_count != 2 && thread_count != 8) {
    thread_count = 1;
  }
  
  thread_local_maps.resize(thread_count);
  thread_local_indexes.resize(thread_count);
}

wordcount::~wordcount()
{
}

void wordcount::clear_word_counts()
{
  final_word_counts.clear();
  inverted_index.clear();
  for (auto& local_map : thread_local_maps) {
    local_map.clear();
  }
  for (auto& local_index : thread_local_indexes) {
    local_index.clear();
  }
}

void wordcount::load_txt(const std::string& filename)
{
  clear_word_counts();
  
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open file " << filename << std::endl;
    return;
  }

  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::string content;
  content.reserve(file_size + 1);
  content.resize(file_size);
  file.read(&content[0], file_size);
  file.close();

  std::vector<std::string> blocks = split_into_blocks(content);
  
  std::vector<std::future<void>> futures;
  size_t blocks_per_thread = (blocks.size() + thread_count - 1) / thread_count;
  
  for (size_t t = 0; t < thread_count; ++t) {
    size_t start_block = t * blocks_per_thread;
    size_t end_block = std::min(start_block + blocks_per_thread, blocks.size());
    
    if (start_block < end_block) {
      futures.emplace_back(std::async(std::launch::async, [this, &blocks, start_block, end_block, t]() {
        LocalWordMap& local_map = thread_local_maps[t];
        LocalInvertedIndex& local_index = thread_local_indexes[t];
        local_map.clear();
        local_index.clear();
        
        for (size_t i = start_block; i < end_block; ++i) {
          count_words_in_text_local(blocks[i], local_map);
          build_inverted_index_local(blocks[i], local_index, i);
        }
      }));
    }
  }
  
  for (auto& future : futures) {
    future.wait();
  }
  
  merge_local_maps();
  merge_local_indexes();
}

std::vector<std::string> wordcount::split_into_blocks(const std::string& content)
{
  std::vector<std::string> blocks;
  
  if (content.size() <= block_size) {
    blocks.push_back(content);
    return blocks;
  }
  
  size_t start = 0;
  while (start < content.size()) {
    size_t end = std::min(start + block_size, content.size());
    
    if (end < content.size()) {
      while (end > start && !std::isspace(content[end])) {
        --end;
      }
      if (end == start) {
        end = std::min(start + block_size, content.size());
      }
    }
    
    blocks.push_back(content.substr(start, end - start));
    start = end;
    
    while (start < content.size() && std::isspace(content[start])) {
      ++start;
    }
  }
  
  return blocks;
}

void wordcount::count_words_in_text_local(const std::string& text, LocalWordMap& local_map)
{
  const char* ptr = text.c_str();
  const char* end = ptr + text.size();
  std::string word;
  word.reserve(32);
  
  while (ptr < end) {
    while (ptr < end && !std::isalpha(*ptr)) ++ptr;
    
    if (ptr >= end) break;
    
    const char* word_start = ptr;
    while (ptr < end && std::isalpha(*ptr)) ++ptr;
    
    word.clear();
    for (const char* c = word_start; c < ptr; ++c) {
      word += std::tolower(*c);
    }
    
    if (!word.empty()) {
      local_map[word]++;
    }
  }
}

void wordcount::build_inverted_index_local(const std::string& text, LocalInvertedIndex& local_index, size_t block_id)
{
  const char* ptr = text.c_str();
  const char* end = ptr + text.size();
  std::string word;
  word.reserve(32);
  
  while (ptr < end) {
    while (ptr < end && !std::isalpha(*ptr)) ++ptr;
    
    if (ptr >= end) break;
    
    const char* word_start = ptr;
    while (ptr < end && std::isalpha(*ptr)) ++ptr;
    
    word.clear();
    for (const char* c = word_start; c < ptr; ++c) {
      word += std::tolower(*c);
    }
    
    if (!word.empty()) {
      local_index[word].insert(block_id);
    }
  }
}

void wordcount::merge_local_maps()
{
  final_word_counts.clear();
  
  size_t estimated_unique_words = 0;
  for (const auto& local_map : thread_local_maps) {
    estimated_unique_words += local_map.size();
  }
  final_word_counts.reserve(estimated_unique_words);
  
  for (const auto& local_map : thread_local_maps) {
    for (const auto& pair : local_map) {
      final_word_counts[pair.first] += pair.second;
    }
  }
}

void wordcount::merge_local_indexes()
{
  inverted_index.clear();
  
  for (const auto& local_index : thread_local_indexes) {
    for (const auto& word_blocks : local_index) {
      const std::string& word = word_blocks.first;
      const std::unordered_set<size_t>& blocks = word_blocks.second;
      
      auto& global_blocks = inverted_index[word];
      global_blocks.insert(global_blocks.end(), blocks.begin(), blocks.end());
    }
  }
  
  for (auto& word_blocks : inverted_index) {
    std::sort(word_blocks.second.begin(), word_blocks.second.end());
  }
}

void wordcount::print_word_counts() const
{
  std::cout << "\n=== WORD COUNT RESULTS ===" << std::endl;
  std::cout << "Unique words: " << final_word_counts.size() << std::endl;
  std::cout << "Total words: " << get_total_word_count() << std::endl;
  std::cout << "=========================" << std::endl;
  
  std::vector<std::pair<std::string, int>> word_pairs;
  for (const auto& pair : final_word_counts) {
    word_pairs.push_back(pair);
  }
  
  std::sort(word_pairs.begin(), word_pairs.end(), 
        [](const auto& a, const auto& b) { return a.second > b.second; });
  
  std::cout << "\nTop 10 most frequent words:" << std::endl;
  for (size_t i = 0; i < std::min(size_t(10), word_pairs.size()); ++i) {
    std::cout << i+1 << ". " << word_pairs[i].first << ": " << word_pairs[i].second << std::endl;
  }
}

size_t wordcount::get_unique_word_count() const
{
  return final_word_counts.size();
}

size_t wordcount::get_total_word_count() const
{
  size_t total = 0;
  for (const auto& pair : final_word_counts) {
    total += pair.second;
  }
  return total;
}

std::vector<size_t> wordcount::search_word(const std::string& word) const
{
  std::string lower_word = word;
  std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
  
  auto it = inverted_index.find(lower_word);
  if (it != inverted_index.end()) {
    return it->second;
  }
  return std::vector<size_t>();
}

void wordcount::print_inverted_index() const
{
  std::cout << "\n=== INVERTED INDEX ===" << std::endl;
  std::cout << "Total indexed words: " << inverted_index.size() << std::endl;
  std::cout << "======================" << std::endl;
  
  std::vector<std::pair<std::string, std::vector<size_t>>> index_pairs;
  for (const auto& pair : inverted_index) {
    index_pairs.push_back(pair);
  }
  
  std::sort(index_pairs.begin(), index_pairs.end());
  
  std::cout << "\nFirst 20 words in index:" << std::endl;
  for (size_t i = 0; i < std::min(size_t(20), index_pairs.size()); ++i) {
    std::cout << index_pairs[i].first << ": blocks [";
    const auto& blocks = index_pairs[i].second;
    for (size_t j = 0; j < std::min(size_t(10), blocks.size()); ++j) {
      std::cout << blocks[j];
      if (j < std::min(size_t(10), blocks.size()) - 1) std::cout << ", ";
    }
    if (blocks.size() > 10) std::cout << "...";
    std::cout << "]" << std::endl;
  }
}

void wordcount::print_search_results(const std::string& word) const
{
  std::vector<size_t> blocks = search_word(word);
  
  std::cout << "\n=== SEARCH RESULTS ===" << std::endl;
  std::cout << "Word: \"" << word << "\"" << std::endl;
  
  if (blocks.empty()) {
    std::cout << "Not found in any blocks" << std::endl;
  } else {
    std::cout << "Found in " << blocks.size() << " blocks: [";
    for (size_t i = 0; i < blocks.size(); ++i) {
      std::cout << blocks[i];
      if (i < blocks.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
  }
  std::cout << "======================" << std::endl;
}

bool wordcount::save_inverted_index(const std::string& index_file) const
{
  std::ofstream file(index_file, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot create index file " << index_file << std::endl;
    return false;
  }
  
  size_t num_words = inverted_index.size();
  file.write(reinterpret_cast<const char*>(&num_words), sizeof(num_words));
  
  for (const auto& word_entry : inverted_index) {
    const std::string& word = word_entry.first;
    const std::vector<size_t>& blocks = word_entry.second;
    
    size_t word_length = word.length();
    file.write(reinterpret_cast<const char*>(&word_length), sizeof(word_length));
    file.write(word.c_str(), word_length);
    
    size_t num_blocks = blocks.size();
    file.write(reinterpret_cast<const char*>(&num_blocks), sizeof(num_blocks));
    file.write(reinterpret_cast<const char*>(blocks.data()), num_blocks * sizeof(size_t));
  }
  
  file.close();
  return true;
}

bool wordcount::load_inverted_index(const std::string& index_file)
{
  std::ifstream file(index_file, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open index file " << index_file << std::endl;
    return false;
  }
  
  inverted_index.clear();
  
  size_t num_words;
  if (!file.read(reinterpret_cast<char*>(&num_words), sizeof(num_words))) {
    std::cerr << "Error: Invalid index file format" << std::endl;
    return false;
  }
  
  for (size_t i = 0; i < num_words; ++i) {
    size_t word_length;
    if (!file.read(reinterpret_cast<char*>(&word_length), sizeof(word_length))) {
      std::cerr << "Error: Invalid index file format" << std::endl;
      return false;
    }
    
    std::string word(word_length, '\0');
    if (!file.read(&word[0], word_length)) {
      std::cerr << "Error: Invalid index file format" << std::endl;
      return false;
    }
    
    size_t num_blocks;
    if (!file.read(reinterpret_cast<char*>(&num_blocks), sizeof(num_blocks))) {
      std::cerr << "Error: Invalid index file format" << std::endl;
      return false;
    }
    
    std::vector<size_t> blocks(num_blocks);
    if (!file.read(reinterpret_cast<char*>(blocks.data()), num_blocks * sizeof(size_t))) {
      std::cerr << "Error: Invalid index file format" << std::endl;
      return false;
    }
    
    inverted_index[word] = std::move(blocks);
  }
  
  file.close();
  return true;
}

bool wordcount::has_inverted_index() const
{
  return !inverted_index.empty();
}