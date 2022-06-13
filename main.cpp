#include <iostream>
#include <list>
using namespace std;

class History {
  public:
    list<string> hist;
    History() {
      hist = list<string>(10, "");
    }

    void print_hist () {
      int j = 1;
      for (auto i = hist.begin(); i != hist.end(); ++i){
        cout << j << " " << *i << endl;
        j++;
      }
    }
};


int main () {
  bool exit = false;
  History hist;
  while (!exit) {
    cout << "lucash> ";
    string user_input;
    cin >> user_input;

    if (user_input == "historico") {
      hist.print_hist();
    }

    else if (user_input == "exit") exit = true;
    // todo salvahistorico
  }
  cout << "Encerrando lucash..." << endl;
  return 0;
}