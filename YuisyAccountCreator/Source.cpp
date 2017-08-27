#include <iostream>
#include <locale>
#include <string>
#include <vector>
#include <limits>
#include <regex>
#include "curl/curl.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"

#define MAX_QUANTITY 100
#define MIN_NAME_LENGTH 3
#define MAX_NAME_LENGTH 12
#define MAX_BASENAME_LENGTH (MAX_NAME_LENGTH - 3)
#define MIN_PASSWORD_LENGTH 6
#define MAX_PASSWORD_LENGTH 16

using namespace std;

size_t WriteFunction(char *ptr, size_t size, size_t nmemb, string *userdata);
vector<string> RequestDomainsList();
int AskQuantity();
string AskName(bool);
string AskPassword(bool);

class Account {
public:
  void SetEmail(string);
  void RequestTemporaryEmailAddress();
protected:
  string name;
  string email;
};

class SingleAccount : public Account {
public:
  SingleAccount(const string Name) { name = Name; }
};

class MultiAccount : public Account {
public:
  MultiAccount(const int Id) : id(Id) {}
  void SetName(string);
private:
  const int id;
};

int main()
{
  locale::global(locale("spanish"));

  cout << "# Yuisy Account Creator" << endl;
  cout << "# Hecho por Mars.-" << endl;
  cout << "# Reportar problemas al siguiente correo: everythingispermitted@outlook.com" << endl << endl;

  curl_global_init(CURL_GLOBAL_ALL);

  const vector<string> domains_list = RequestDomainsList();
  bool exit = true;

  do {
    const unsigned quantity = AskQuantity();
    string password;

    if (quantity == 1) {
      const string name = AskName(false);
      password = AskPassword(false);
      SingleAccount account(name);
      account.SetEmail(domains_list[(rand() % domains_list.size())]);
      account.RequestTemporaryEmailAddress();
    } else {
      const string base_name = AskName(true);
      password = AskPassword(true);
      vector<MultiAccount *> accounts;

      for (unsigned i = 0; i < quantity; ++i) {
        const int id = (i + 1);
        MultiAccount *account = new MultiAccount(id);
        account->SetName(base_name);
        account->SetEmail(domains_list[(rand() % domains_list.size())]);
        account->RequestTemporaryEmailAddress();
        accounts.push_back(account);
      }
      // ...
      for (vector<MultiAccount *>::iterator it = accounts.begin(); it != accounts.end(); ++it) {
        delete *it;
      }
    }
  } while (!exit);

  curl_global_cleanup();
  system("PAUSE");
  return 0;
}

size_t WriteFunction(char *ptr, size_t size, size_t nmemb, string *userdata)
{
  size_t real_size = (size * nmemb);
  userdata->append(ptr, real_size);
  return real_size;
}

vector<string> RequestDomainsList()
{
  cout << "> Obteniendo lista de dominios para los emails temporales..." << endl;

  CURL *curl = curl_easy_init();
  vector<string> domains_list;

  if (curl) {
    CURLcode res;
    string json;
    curl_easy_setopt(curl, CURLOPT_URL, "http://api.temp-mail.ru/request/domains/format/json/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      cout << "> No se pudo obtener la lista" << endl;
      exit(EXIT_FAILURE);
    }

    rapidjson::Document document;
    document.Parse(json);
    const rapidjson::Value &a = document;

    for (rapidjson::SizeType i = 0; i < a.Size(); ++i)
      domains_list.push_back(a[i].GetString());

    curl_easy_cleanup(curl);
  }

  return domains_list;
}

int AskQuantity()
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa la cantidad de cuentas que quieres crear";
      cout << (" (Debe ser entre 1 y " + to_string(MAX_QUANTITY) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "> Cantidad inválida" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([1-9]([0-9]{1,2})?)$")) || (stoi(answer) > MAX_QUANTITY));

  return stoi(answer);
}

string AskName(const bool multiple)
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa un " << (multiple ? "nombre base para las cuentas" : "nombre para la cuenta");
      cout << (" (Debe contener sólo letras y una longitud entre "
        + to_string(MIN_NAME_LENGTH) + " y " + to_string(multiple ? MAX_BASENAME_LENGTH : MAX_NAME_LENGTH) + ")") << endl;
      first_attempt = false;
    } else {
      cout << "> " << (multiple ? "Nombre base" : "Nombre") << " inválido" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-zA-Z]{"
    + to_string(MIN_NAME_LENGTH) + "," + to_string(multiple ? MAX_BASENAME_LENGTH : MAX_NAME_LENGTH) + "})$")));

  return answer;
}

string AskPassword(const bool multiple)
{
  bool first_attempt = true;
  string answer;

  do {
    if (first_attempt) {
      cout << "< Ingresa una contrasenya para " << (multiple ? "las cuentas" : "la cuenta");
      cout << (" (Debe contener sólo letras y numeros, y una longitud entre "
        + to_string(MIN_PASSWORD_LENGTH) + " y " + to_string(MAX_PASSWORD_LENGTH) + ")") << endl;
      first_attempt = false;
    }
    else {
      cout << "Contrasenya inválida" << endl;
    }

    cin >> answer;
  } while (!regex_match(answer, regex("^([a-zA-Z0-9]{"
    + to_string(MIN_PASSWORD_LENGTH) + "," + to_string(MAX_PASSWORD_LENGTH) + "})$")));

  return answer;
}

inline void Account::SetEmail(const string domain)
{
  email = (name + domain);
}

void Account::RequestTemporaryEmailAddress()
{
}

void MultiAccount::SetName(const string base_name)
{
  if (id < 10) {
    name = (base_name + "00" + to_string(id));
  } else if (id < 100) {
    name = (base_name + "0" + to_string(id));
  } else {
    name = (base_name + to_string(id));
  }
}
