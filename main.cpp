#include <iostream>
#include <list>
using namespace std;

class History {
  private:
    int cmd_count;      // quantidade de comandos atualmente no historico
    list<string> hist;  // estrutura de dados que guarda os comandos

  public:
    History() {
      cmd_count = 0;
      hist = list<string>();
    }

    // Metodo que printa o historico em uma lista ordenada, com os comandos mais recentes primeiro.
    void print_hist () {
      if (cmd_count == 0) {
        cout << "O historico esta vazio" << endl;
        return;
      }
      int j = 1;
      for (auto i = hist.begin(); i != hist.end(); ++i){
        cout << j << " - " << *i << endl;
        j++;
      }
      return;
    }

    // Coloca um comando no historico, com um maximo de 10 comandos armazenados simultaneamente.
    void create_elem (string command) {
      hist.push_back(command);
      if (cmd_count == 10) 
        hist.pop_front();
      
      else cmd_count++;
      return;
    }

    // Recupera string do comando da posicao especificada
    string get_command (int position) {
      if (position < 0 || position > cmd_count) return "Este comando nao existe";
      position--; // agora a range eh 0 - 9, ao inves de 1 - 10

      auto it = hist.begin();
      for (int i = 0; i < position; i++){
        ++it;
      }
      return *it;
    }
};

// void function_switch(string user_input) {

// }

int main () {
  bool exit = false;
  History hist;
  while (!exit) {
    cout << "lucash> ";
    string user_input;
    getline(cin, user_input);

    if (user_input == "historico") {
      hist.print_hist();
    }

    // ! gambiarra para testes
    if (user_input == "historico2") {
      cout << hist.get_command(2) << endl;
    }

    else if (user_input == "exit") exit = true;
    
    hist.create_elem(user_input);
  }
  cout << "Encerrando lucash..." << endl;
  return 0;
}