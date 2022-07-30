#include <iostream>
#include <list>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <numeric>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

typedef struct {
  string msg;
  int cod; // -1: exit, 0: deu ruim mas nao faca nada, 1: deu bom
} ReturnFlag;

vector<string> generic_split(string str, string delimiter);

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
      if (generic_split(command, " ")[0] == "historico") return;
      
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

      if (cmd_pt.count(new_name) > 0) {
        string old_alias = cmd_pt[new_name];
        cmd_pt[new_name] = old_name;
        return {"O alias de \"" + old_name + "\" alterado de \"" + old_alias + "\" para \"" + new_name + "\"", 1};
      }

      else {
        cmd_pt.insert({new_name, old_name});
        // return {"Novo alias adicionado: de \"" + old_name + "\" para \"" + new_name + "\"", 1};
        return {"", 1};
      }
    }

    string cmd_translation(string cmd) {
      if (cmd_pt.count(cmd) > 0) {
        return cmd_pt[cmd];
      }
      return cmd;
    }
};

class Shell {
  private:
    History hist;
    Aliases alias;
    string curr_user;
    string curr_path;
    vector<string> paths;

    int stdin_backup;
    int stdout_backup;

    bool debug = false;

    ReturnFlag exec_cmd(string path, const char ** argv);
    ReturnFlag function_switch(vector<string> command_vec, bool child);
    ReturnFlag pipe_parse(string user_input);
    vector<string> string_split(string cmd);
    int redirect(vector<string> cmd);
    void redirect();
    bool cmd_exists(string str);

    void update_current_user();
    void update_current_path();
    ReturnFlag get_cmd_paths();

    bool alias_init();    

  public:
    Shell() {
      update_current_user();
      update_current_path();
      alias_init();
      
      auto response = get_cmd_paths();
      if (response.msg != "") cout << response.msg << endl;
    }

    void run() {
      bool exit = false;

      while (!exit) {
        cout <<  "lucash-" + curr_user + "-" + curr_path + "> ";

        string user_input;
        getline(cin, user_input);

        int stat;
        waitpid(-1, &stat, WNOHANG); // ? checar se eh wnohang mesmo
        auto cmd_response = pipe_parse(user_input);

        redirect();

        if (cmd_response.msg != "") cout << cmd_response.msg << endl;
        if (cmd_response.cod == -1) exit = true;
        
        hist.create_elem(user_input);
      }
      cout << "Encerrando lucash..." << endl;
      return;
    }
};

int main () {
  Shell lucash;
  lucash.run();
  return 0;
}

ReturnFlag Shell::get_cmd_paths() {
  ifstream file("/home/" + curr_user + "/.BRbshrc_profile");

  string s;
  if (!file.fail() && file.peek() != EOF) {
    getline(file, s);
    s.erase(0, 5);
    s.erase(s.size()-1, 1);

    paths = generic_split(s, ";");
  }
  else {
    file.close();
    return {"Arquivo de configuracao inexistente ou mal formatado", 0};
  }
    
  file.close();
  return {"", 1};
}

void Shell::update_current_path() {
  char curr_path_cstr[256];
  getcwd(curr_path_cstr, 256);
  curr_path = string(curr_path_cstr);
}

void Shell::update_current_user() {
  char curr_name[256];
  auto name = popen("whoami", "r");
  fgets(curr_name, 256, name);
  curr_user = string(curr_name);
  curr_user.pop_back();
}

bool Shell::alias_init() {
  ifstream file("/home/" + curr_user + "/.BRbshrc");

  string s;
  bool flag = false;
  if (!file.fail()) {
    while (file.peek() != EOF) {
      getline(file, s);
      if (!flag) {
        auto cmd_response = pipe_parse(s);
      }
    }
  }
  else {
    file.close();
    return false;
  }
  file.close();
  return true;
}

void Shell::redirect() {
  dup2(stdin_backup, STDIN_FILENO);
  dup2(stdout_backup, STDOUT_FILENO);
}

int Shell::redirect(vector<string> cmd) {
  for (unsigned int i = 0; i < cmd.size()-1; i++) { // evita seg fault
    if (cmd[i] == "<") { // input
      stdin_backup = dup(STDIN_FILENO);
      freopen(cmd[i+1].c_str(), "r", stdin);
      return i;
    }
    else if (cmd[i] == ">") { // output w
      stdout_backup = dup(STDOUT_FILENO);
      freopen(cmd[i+1].c_str(), "w", stdout);
      return i;
    }
    else if (cmd[i] == ">>") { // output a
      stdout_backup = dup(STDOUT_FILENO);
      freopen(cmd[i+1].c_str(), "a", stdout);
      return i;
    }
  }
  return -1;
}

ReturnFlag Shell::pipe_parse(string user_input) {
  int last = 0;
  bool background = user_input[user_input.size() - 1] == '&';
  if (background) user_input.pop_back();
  vector<vector<string>> commands;
  vector<string> command_vec = string_split(user_input);
  for (unsigned int i = 0; i < command_vec.size(); i++) {
    if (command_vec[i] == "|") {
      vector<string> temp;
      for (unsigned int j = last; j < i; j++) 
        temp.push_back(command_vec[j]);
      
      commands.push_back(temp);
      last = i+1;
    }
  }

  vector<string> temp;
  for (unsigned int j = last; j < command_vec.size(); j++) 
    temp.push_back(command_vec[j]);
  commands.push_back(temp);

  
  pid_t pid;
  bool child = false;
  if (background) {
    pid = fork();
    if (pid == -1)
      return {"Nao foi possivel realizar o fork", 0};

    else if (pid == 0) {
      background = false;
      child = true;
    }
  }

  if (background) return {"", 1};
  if (commands.size() == 1) 
    return function_switch(commands[0], false);

  vector<int[2]> pipes(commands.size()-1);
  for (unsigned int i = 0; i < commands.size()-1; i++) {
    pipe(pipes[i]);
  }

  vector<pid_t> pids;

  for (unsigned int i = 0; i < commands.size(); i++) {
    pid = fork();
    if (pid == -1) return {"Nao foi possivel realizar o fork", 0};

    else if (pid == 0) {
      if (i != 0) {
        dup2(pipes[i-1][0], STDIN_FILENO);
      }
      if (i != commands.size()-1) { 
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      for (unsigned int j = 0; j < pipes.size(); j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      auto ret = function_switch(commands[i], true);
      if (ret.msg != "") cout << ret.msg << endl;
      if (ret.cod == -1) return {"", -1};
      exit(0);
    }

    else {
      pids.push_back(pid);
    }
  }
  for (unsigned int i = 0; i < pipes.size(); i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  for (unsigned int i = 0; i < pids.size(); i++) {
    int stat;
    waitpid(pids[i], &stat, 0);
  }

  if (child) exit(0);

  return {"", 1};
}

ReturnFlag Shell::function_switch(vector<string> command_vec, bool child) {
  if (command_vec.size() == 0) return {"", 0};
  command_vec[0] = alias.cmd_translation(command_vec[0]);

  int a = redirect(command_vec);
  if (a != -1) { // ? talvez nao seja generico. analisar.
    command_vec.pop_back();
    command_vec.pop_back();
  }

  if (command_vec[0] == "historico") {
    if (command_vec.size() > 2) return {"historico: O comando recebe um ou nenhum parametros.", 0};
    else if (command_vec.size() == 1) {
      hist.print_hist();
      return {"", 1};
    }
    else if (command_vec.size() == 2) {
      int position = stoi(command_vec[1]);
      string cmd = hist.get_command(position);
      if (cmd == "") return {"historico: Esta posicao no historico (ainda) nao existe", 0};
      return pipe_parse(cmd);
    }
  }

  else if (command_vec[0] == "alias") {
    if (command_vec.size() != 3) return {"alias: O comando recebe dois parametros", 0}; 
    return alias.add_alias(command_vec[1], command_vec[2]);
  }

  else if (command_vec[0] == "cd") {
    if (command_vec.size() > 2) 
      return {"cd: Numero invalido de argumentos.", 0};

    else if (command_vec.size() == 1) 
      chdir("/");

    else  // (command_vec.size() == 2)
      chdir(command_vec[1].c_str());
    
    update_current_path();
    return {"", 1};
    
  }

  else if (command_vec[0] == "exit") {
    return {"", -1};
  }

  else if (command_vec[0] == "ver") {
    cout << endl << "Versao: 0.5.2" << endl << "Atualizado em 29/06/2022" << endl << "Autor: Lucas ABM" << endl << endl;
    return {"", 1};
  }

  // else

  string command = command_vec[0]; 
  
  bool found = false;
  unsigned int i;

  const char * argv[command_vec.size() + 1];

  for (i = 0; i < paths.size() && !found; i++) 
    if (cmd_exists(paths[i] + "/" + command))
      found = true;

  if (!found) return {"Nao achei o comando", 0};
  i--;

  argv[0] = (paths[i] + "/" + command).c_str();
  for (unsigned int j = 1; j < command_vec.size(); j++) 
    argv[j] = command_vec[j].c_str();

  argv[command_vec.size()] = NULL;

  return exec_cmd(paths[i] + "/" + command, argv);
}

ReturnFlag Shell::exec_cmd(string path, const char ** argv) {
  int status;
  pid_t pid = fork();
  if (pid == -1) return {"Nao foi possivel realizar o fork", 0};
  else if (pid == 0) {
    execv(path.c_str(), (char **) argv);
    exit(0);
  }
  else {
    waitpid(pid, &status, 0); // ? talvez tenha algo de util nesse status
  }
  return {"", 1};
}

vector<string> generic_split(string str, string delimiter) {
  size_t pos = 0;
  string token;
  vector<string> ret_vec;

  pos = str.find(delimiter);
  while (pos != string::npos) {
    ret_vec.push_back(str.substr(0, pos));
    str.erase(0, pos + delimiter.length());
    pos = str.find(delimiter);
  }
  ret_vec.push_back(str);

  for (unsigned int i = 0; i < ret_vec.size(); i++) 
    if (ret_vec[i] == "" || ret_vec[i] == " ") ret_vec.erase(ret_vec.begin() + i);

  return ret_vec;
}

bool Shell::cmd_exists(string str) {
  auto f = ifstream(str);
  return f.good();
}

vector<string> Shell::string_split(string cmd) {
  vector<string> temp;
  stringstream stream(cmd);
  string word;

  while (stream >> word) {
    temp.push_back(word);
  }

  vector<string> ret;
  bool aspas_flag = false;
  string sentence_str = "";

  for (unsigned int i = 0; i < temp.size(); i++) {

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