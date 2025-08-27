#include "wordcount.hpp"

void print_search_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " <index_file> <search_word>" << std::endl;
  std::cout << "  index_file: path to saved inverted index file" << std::endl;
  std::cout << "  search_word: word to search for in the index" << std::endl;
}

int main(int argc, char* argv[])
{
  if (argc != 3) {
    print_search_usage(argv[0]);
    return 1;
  }
  
  std::string index_file = argv[1];
  std::string search_word = argv[2];
  
  wordcount wc;
  if (!wc.load_inverted_index(index_file)) {
    std::cerr << "Error: Failed to load inverted index from: " << index_file << std::endl;
    return 1;
  }
  
  wc.print_search_results(search_word);
  return 0;
}