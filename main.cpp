#include <iostream>
#include <list>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>

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

class Aliases {
  private:
    map<string, string> cmd_pt;

  public:

    ReturnFlag add_alias(string old_name, string new_name) {
      if (old_name == new_name) return {"O alias nao pode ser igual ao comando original", 0};

      if (cmd_pt.count(old_name) > 0) {
        string old_alias = cmd_pt[old_name];
        cmd_pt[old_name] = new_name;
        return {"Alias do comando \"" + old_name + "\" alterado de \"" + old_alias + "\" para \"" + new_name + "\"", 1};
      }

      else {
        cmd_pt.insert({old_name, new_name});
        return {"Novo alias adicionado: de \"" + old_name + "\" para \"" + new_name + "\"", 1};
      }
    }

    string cmd_translation(string cmd) {
      if (cmd_pt.count(cmd) > 0) {
        return cmd_translation(cmd_pt[cmd]); // ! eh possivel fazer loop infinito na funcao recursiva. rever.
      }
      return cmd;
    }
};

// Funcao que recebe uma string, e retorna um vetor de strings, divididas pelos espacos em branco.
vector<string> string_split(string cmd) {
  vector<string> temp;
  stringstream stream(cmd);
  string word;

  while (stream >> word) {
    temp.push_back(word);
  }

  vector<string> ret;
  bool aspas_flag = false;
  string sentence_str = "";

  for (int i = 0; i < temp.size(); i++) {

    string temp_str = temp[i];
    int str_size = temp_str.size();

    if (aspas_flag == false) {
      // palavra com aspas no comeco e no final:
      if (temp_str[0] == '"' && temp_str[str_size-1] == '"') {
        temp_str.erase(str_size-1, 1);
        temp_str.erase(0, 1);
        ret.push_back(temp_str);
      }

      // palavra que abre sentenca
      else if (temp_str[0] == '"') {
        temp_str.erase(0, 1);
        aspas_flag = true;
        sentence_str = temp_str;
      }

      // palavra sem aspas, fora de aspas
      else {
        ret.push_back(temp_str);
      }
    }
    
    else { // aspas_flag == true
      // palavra que fecha sentenca
      if (temp_str[str_size-1] == '"') {
        temp_str.erase(str_size-1, 1);
        sentence_str += " " + temp_str;
        ret.push_back(sentence_str);

        aspas_flag = false;
        sentence_str = "";
      }

      // palavra no meio de uma sentenca
      else {
        sentence_str += " " + temp_str;
      }
    }

  }
  return ret;
}

History hist;
Aliases alias;

ReturnFlag function_switch(string user_input) {
  vector<string> command_vec = string_split(user_input);
  if (command_vec.size() == 0) return {"Comando nao identificado.", 0};
  command_vec[0] = alias.cmd_translation(command_vec[0]);

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
      return function_switch(cmd); // ! possibilidade de loop infinito de comandos no historico
    }
  }

  else if (command_vec[0] == "alias") {
    if (command_vec.size() != 3) return {"O comando recebe dois parametros", 0}; 
    return alias.add_alias(command_vec[1], command_vec[2]);
  }

  else if (command_vec[0] == "exit") {
    return {"", -1};
  }

  return {"Comando nao identificado", 0};
}

int main () {
  bool exit = false;

  // inicializacao do shell

  ifstream file("./aliases.txt");
  
  string s;
  bool flag = false;
  if (!file.fail()) {
    while (file.peek() != EOF) {
      getline(file, s);
      if (!flag) {
        auto cmd_response = function_switch(s);
        if (cmd_response.msg != ""&& cmd_response.cod == 1)
          cout << cmd_response.msg << endl;
      }
    }
  }
  file.close();


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