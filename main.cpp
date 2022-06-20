#include <iostream>
#include <list>
#include <sstream>
#include <vector>

using namespace std;

typedef struct {
  string msg;
  int cod; // -1: exit, 0: deu ruim mas nao faca nada, 1: deu bom
} ReturnFlag;

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
      if (position < 0 || position > cmd_count) return "";
      position--; // agora a range eh 0 - 9, ao inves de 1 - 10

      auto it = hist.begin();
      for (int i = 0; i < position; i++){
        ++it;
      }
      return *it;
    }
};

// Funcao que recebe uma string, e retorna um vetor de strings, divididas pelos espacos em branco.
vector<string> string_split(string cmd) {
  vector<string> ret;
  stringstream stream(cmd);
  string word;

  while (stream >> word) {
    ret.push_back(word);
  }
  return ret;
}

History hist;

ReturnFlag function_switch(string user_input) {
  vector<string> command_vec = string_split(user_input);
  if (command_vec.size() == 0) return {"Comando nao identificado.", 0};

  if (command_vec[0] == "historico") {
    if (command_vec.size() > 2) return {"O comando historico recebe um ou nenhum parametros.", 0};
    else if (command_vec.size() == 1) {
      hist.print_hist();
      return {"", 1};
    }
    else if (command_vec.size() == 2) {
      int position = stoi(command_vec[1]);
      string cmd = hist.get_command(position);
      if (cmd == "") return {"Esta posicao no historico (ainda) nao existe", 0};
      return function_switch(cmd); // ! possibilidade de loop infinito de comandos
    }
  }

  if (command_vec[0] == "exit") {
    return {"", -1};
  }

  return {"Comando nao identificado", 0};
}

int main () {
  bool exit = false;
  while (!exit) {
    cout << "lucash> ";
    string user_input;
    getline(cin, user_input);

    auto cmd_response = function_switch(user_input);
    if (cmd_response.msg != "") cout << cmd_response.msg << endl;
    if (cmd_response.cod == -1) exit = true;
    
    hist.create_elem(user_input);
  }
  cout << "Encerrando lucash..." << endl;
  return 0;
}