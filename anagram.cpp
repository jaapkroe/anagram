#include <stdio.h>
#include <string>
#include <getopt.h>
#include <algorithm> 
#include <fstream>
#include <iostream>
#include <ctime>
#include <map>
#include <set>
#include <list>
#include <queue>

using namespace std;

/* Hash based approach */
#define HASHTYPE unsigned long
#define HASHSEED 5381
HASHTYPE hash_word(string w, int lcase) { // anagrams generates identical hash-keys
  if(!lcase) transform(w.begin(), w.end(), w.begin(), ::tolower); // lower-case ..
  sort(w.begin(), w.end());                                       // .. and sort
  HASHTYPE h = HASHSEED;
  for(char& c : w) h = ((h << 5) + h) + c;   // djb2 hash with left-shift operator <<
  return h;
}

void load_dict(string dictfile, map<HASHTYPE,set<string>>& dict, int lcase, int verbose) {
  ifstream f(dictfile.c_str(), ifstream::in);
  string w;
  int nwords=0;
  if(!f) { printf("ERROR opening file %s\n",dictfile.c_str()); exit(1); };
  while(getline(f,w)) {
    HASHTYPE h = hash_word(w, lcase);
    dict[h].insert(w);
    if(verbose>1) printf("read word [ %6d ] %12s , hash = %ld\n",nwords,w.c_str(),h);
    nwords++;
  }
  if(verbose) printf("read %d words from %s.\n",nwords,dictfile.c_str());
}

/* Trie based approach */
struct trie {
  void insert(string& str, unsigned int n, int verbose) { // insert word
    if(str.length() == n) { isword=1; return; } // mark as word
    for(auto& child: children) {            // search list of children
      if(child.token == str[n]) {           // if character already in trie ..
        child.insert(str, n+1, verbose);    // .. add here
        return;                             // and exit loop
      }
    }
    trie child;             // otherwise, create a new child and continue there
    child.token = str[n];   // take the nth char of the string
    child.depth = n+1;
    child.insert(str, n+1, verbose); // insert the next level
    children.push_back(child);
  };
  void remove(string& str, unsigned int n, int verbose) { // remove a word
    for(auto& child: children) {            // search list of children
      if(child.token == str[n]) {           // if character already in trie ..
        child.remove(str, n+1, verbose);
      }
    }
  };
  void init() { // dfs for init
    visited = false;
    for(auto& child: children) child.init();
  };
  void anagram(string letters, string path, trie& root, int d, int minlen, int nword, int maxword, int verbose) { 
    // recursive depth-first-search 
    if(verbose>1) fprintf(stderr, "path=%-20s    remaining letters=%-20s   depth=%d (%d)\n",path.c_str(),letters.c_str(),d,depth);
    if(isword && depth>=minlen && !visited) {     // word is found
      if(letters.empty()) { // .. all letters used
        printf("%s\n",path.c_str());
        return;
      } else if(!maxword || nword+1<maxword) { // or not .., in which case go back to root and add another word
        path += ' ';      // add a space between words..
        root.anagram(letters, path, root, d+1, minlen, nword+1, maxword, verbose);
        //root.anagram(letters, path, root, 0, minlen, nword+1, maxword, verbose);
        path.pop_back();  // and remove it when we come back out of recursion
      }
    }
    for(auto& child: children) { // search all children of this node
      int i=0;
      for(auto l: letters) {
        if(l == child.token) {   // which have a token that is in the set of letters
          path += l;             // add to path, remove from letters
          letters.erase(letters.begin()+i);
          child.anagram(letters, path, root, d+1, minlen, nword, maxword, verbose); // enter recursion
          path.pop_back();       // remove from path, add to letters when we come out of recursion
          letters += l;
          break;                 // exit the for loop, no need search the same string twice
        }
        i++;
      }
    }
      //if(path.length() == depth && isword && depth>= minlen) {
      if(isword && depth>= minlen && d==depth) {
     // if(isword) {
        visited = true;
    //if(verbose) fprintf(stderr, "visited : %c at level %d / %d\n",token,depth,d);
//         this actually speeds up a lot but
//            first words are now unique and non-recurrent but "twelve plus one", will still give
//            "eleven plus two" and
//            "eleven two plus" .. etc.
      }
  };
  list<trie> children;  // list of children
  //map<char, trie> children; // map of characters to trie children nodes
  char token;           // current character
  int isword=0;         // node = word ?
  int depth=0;          // current depth (lenght of word)
  bool visited=false;
};

void load_triedict(string dictfile, trie &root, int lcase, int verbose) {
  ifstream f(dictfile.c_str(), ifstream::in);
  string w;
  if(!f) { printf("ERROR opening file %s\n",dictfile.c_str()); exit(1); };
  int nwords = 0;
  while(getline(f,w)) {
    if(!lcase) transform(w.begin(), w.end(), w.begin(), ::tolower);
    root.insert(w,0,verbose);
    if(verbose==2) fprintf(stderr,"read word [ %6d ] %12s\n",nwords,w.c_str());
    nwords++;
  }
  if(verbose) printf("read %d words from %s.\n",nwords,dictfile.c_str());
}

/**** MAIN ****/
int main(int argc, char* argv[]) {
  int verbose=0, method=0;
  unsigned int minlen=1, maxword=0;
  bool lcase=false;
  string dictfile="/usr/share/dict/british-english";
  clock_t start = clock();
  char c, usage[512];

  sprintf(usage,"anagram <options> \"string\"\n\noptions are :\n\
 -c            - case sensitive           (false)\n\
 -d [filename] - dictionary file          (/usr/share/dict/british-english)\n\
 -m [int]      - method                   (0=trie-based (default), 1=hash-based)\n\
 -n [int]      - minimum word length      (1,          only for trie-based)\n\
 -w [int]      - maximum number of words  (0=infinite, only for trie-based)\n\
 -v            - verbose\n\
 -h            - help %s\n",dictfile.c_str());
  while((c = getopt(argc, argv, "cd:m:n:w:vh")) != -1) {
    switch(c) {
      case 'c': lcase=!lcase; break;
      case 'd': dictfile=optarg; break;
      case 'm': method=atoi(optarg); break;
      case 'n': minlen=atoi(optarg); break;
      case 'w': maxword=atoi(optarg); break;
      case 'v': verbose++; break;
      case 'h': fprintf(stderr,"%s",usage); return 0;
      default : fprintf(stderr,"%s",usage); return 2;
    }
  }
  bool lmultifile = (argc-optind > 1 ? true : false);
  if(argc-optind<1) {fprintf(stderr,"ERROR: no input string specified.\n"); return 1;};

  if(method==0) { // trie based
      if(verbose) fprintf(stderr,"search anagrams : minimum length=%d, max words=%d, case sensitive=%d\n",minlen,maxword,lcase);
      trie root;                                            // initialize trie
      load_triedict(dictfile, root, lcase, verbose);        // and read dictionary
      while(optind<argc) {
        string word = argv[optind]; optind++;                                     // read word (or sentence if enclosed in parentheses)
        word.erase(remove_if(word.begin(), word.end(), ::isspace), word.end());   // remove spaces
        if(!lcase) transform(word.begin(), word.end(), word.begin(), ::tolower);  // lower-case
        if(word.length() < minlen) minlen=word.length();
        if(lmultifile) printf("===== %s =====\n",word.c_str());                   // multiple strings to process?
        //root.init();
        root.anagram(word, "", root, 0, minlen, 0, maxword, verbose);           // compute anagrams
      }
  } else if (method==1) { // hash based
      map<HASHTYPE, set<string> > dict;                     // allocate hash map
      load_dict(dictfile, dict, lcase, verbose);            // and read dictionary
      while(optind<argc) {
        string word = argv[optind]; optind++;                                     // read word (or sentence if enclosed in parentheses)
        word.erase(remove_if(word.begin(), word.end(), ::isspace), word.end());   // remove spaces (lower-case is done in hash function)
        HASHTYPE h = hash_word(word, lcase);                                      // hash 
        if(lmultifile) printf("===== %s =====\n",word.c_str());
        for(const auto& a: dict[h]) printf("%s\n",a.c_str());                     // print all strings with identical hash value
      }
  }

  if(verbose) fprintf(stderr,"CPU time: %.2f sec\n",double(clock()-start)/CLOCKS_PER_SEC);
}
