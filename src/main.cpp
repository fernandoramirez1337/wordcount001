#include "wordcount.hpp"

void print_usage(const char* program_name) {
  std::cout << "Usage:" << std::endl;
  std::cout << "  Build index: " << program_name << " build <filename> <threads> <block_size_mb> <index_file>" << std::endl;
  std::cout << "  Search:      " << program_name << " search <index_file> <search_word>" << std::endl;
  std::cout << "  Legacy:      " << program_name << " <filename> <threads> <block_size_mb> [search_word]" << std::endl;
  std::cout << std::endl;
  std::cout << "Parameters:" << std::endl;
  std::cout << "  threads: 1, 2, or 8" << std::endl;
  std::cout << "  block_size_mb: 16, 32, or 64" << std::endl;
}

BlockSize parse_block_size(int block_mb) {
  if (block_mb == 16) return BlockSize::MB_16;
  else if (block_mb == 32) return BlockSize::MB_32;
  else if (block_mb == 64) return BlockSize::MB_64;
  else {
    std::cerr << "Error: block size must be 16, 32, or 64 MB" << std::endl;
    exit(1);
  }
}

bool validate_threads(int threads) {
  if (threads != 1 && threads != 2 && threads != 8) {
    std::cerr << "Error: threads must be 1, 2, or 8" << std::endl;
    return false;
  }
  return true;
}

int build_mode(int argc, char* argv[]) {
  std::string filename = argv[2];
  int threads = std::atoi(argv[3]);
  int block_mb = std::atoi(argv[4]);
  std::string index_file = argv[5];
  
  if (!validate_threads(threads)) return 1;
  BlockSize block_size = parse_block_size(block_mb);
  
  std::cout << "=== BUILDING INVERTED INDEX ===" << std::endl;
  std::cout << "File: " << filename << std::endl;
  std::cout << "Threads: " << threads << std::endl;
  std::cout << "Block size: " << block_mb << " MB" << std::endl;
  std::cout << "Index file: " << index_file << std::endl;
  std::cout << "===============================" << std::endl;
  
  wordcount wc(threads, block_size);
  wc.load_txt(filename);
  wc.print_word_counts();
  
  if (wc.save_inverted_index(index_file)) {
    std::cout << "\nInverted index saved to: " << index_file << std::endl;
    return 0;
  } else {
    std::cerr << "Failed to save inverted index" << std::endl;
    return 1;
  }
}

int search_mode(int argc, char* argv[]) {
  std::string index_file = argv[2];
  std::string search_word = argv[3];
  
  std::cout << "=== SEARCHING INVERTED INDEX ===" << std::endl;
  std::cout << "Index file: " << index_file << std::endl;
  std::cout << "Search word: " << search_word << std::endl;
  std::cout << "================================" << std::endl;
  
  wordcount wc;
  if (!wc.load_inverted_index(index_file)) {
    std::cerr << "Failed to load inverted index from: " << index_file << std::endl;
    return 1;
  }
  
  wc.print_search_results(search_word);
  return 0;
}

int legacy_mode(int argc, char* argv[]) {
  std::string filename = argv[1];
  int threads = std::atoi(argv[2]);
  int block_mb = std::atoi(argv[3]);
  std::string search_word = (argc == 5) ? argv[4] : "";
  
  if (!validate_threads(threads)) return 1;
  BlockSize block_size = parse_block_size(block_mb);
  
  std::cout << "=== MULTITHREADED WORD COUNT ===" << std::endl;
  std::cout << "File: " << filename << std::endl;
  std::cout << "Threads: " << threads << std::endl;
  std::cout << "Block size: " << block_mb << " MB" << std::endl;
  std::cout << "================================" << std::endl;
  
  wordcount wc(threads, block_size);
  wc.load_txt(filename);
  wc.print_word_counts();
  
  if (!search_word.empty()) {
    wc.print_search_results(search_word);
  } else {
    wc.print_inverted_index();
  }
  
  return 0;
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  
  std::string mode = argv[1];
  
  if (mode == "build" && argc == 6) {
    return build_mode(argc, argv);
  } else if (mode == "search" && argc == 4) {
    return search_mode(argc, argv);
  } else if (argc >= 4 && argc <= 5 && mode != "build" && mode != "search") {
    return legacy_mode(argc, argv);
  } else {
    print_usage(argv[0]);
    return 1;
  }
}